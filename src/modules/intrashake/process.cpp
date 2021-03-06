/*
	*** IntraShake Module - Processing
	*** src/modules/intrashake/process.cpp
	Copyright T. Youngs 2012-2020

	This file is part of Dissolve.

	Dissolve is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Dissolve is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Dissolve.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "modules/intrashake/intrashake.h"
#include "main/dissolve.h"
#include "classes/changestore.h"
#include "classes/energykernel.h"
#include "classes/box.h"
#include "classes/regionaldistributor.h"
#include "classes/species.h"
#include "classes/speciesangle.h"
#include "classes/speciesbond.h"
#include "classes/speciestorsion.h"
#include "base/sysfunc.h"
#include "genericitems/listhelper.h"

// Run main processing
bool IntraShakeModule::process(Dissolve& dissolve, ProcessPool& procPool)
{
	/*
	 * Perform intramolecular shakes, modifying individual bound terms on molecules.
	 * 
	 * This is a parallel routine, with processes operating as one pool
	 */

	// Check for zero Configuration targets
	if (targetConfigurations_.nItems() == 0) return Messenger::error("No configuration targets set for module '%s'.\n", uniqueName());

	// Loop over target Configurations
	for (RefListItem<Configuration>* ri = targetConfigurations_.first(); ri != NULL; ri = ri->next())
	{
		// Grab Configuration pointer
		Configuration* cfg = ri->item();

		// Set up process pool - must do this to ensure we are using all available processes
		procPool.assignProcessesToGroups(cfg->processPool());

		// Retrieve control parameters from Configuration
		bool adjustAngles = keywords_.asBool("AdjustAngles");
		bool adjustBonds = keywords_.asBool("AdjustBonds");
		bool adjustTorsions = keywords_.asBool("AdjustTorsions");
		double& angleStepSize = keywords_.retrieve<double>("AngleStepSize");
		const double angleStepSizeMax = keywords_.asDouble("AngleStepSizeMax");
		const double angleStepSizeMin = keywords_.asDouble("AngleStepSizeMin");
		double& bondStepSize = keywords_.retrieve<double>("BondStepSize");
		const double bondStepSizeMax = keywords_.asDouble("BondStepSizeMax");
		const double bondStepSizeMin = keywords_.asDouble("BondStepSizeMin");
		double cutoffDistance = keywords_.asDouble("CutoffDistance");
		if (cutoffDistance < 0.0) cutoffDistance = dissolve.pairPotentialRange();
		const int nShakesPerTerm = keywords_.asInt("ShakesPerTerm");
		const double targetAcceptanceRate = keywords_.asDouble("TargetAcceptanceRate");
		const bool termEnergyOnly = keywords_.asBool("TermEnergyOnly");
		double& torsionStepSize = keywords_.retrieve<double>("TorsionStepSize");
		const double torsionStepSizeMax = keywords_.asDouble("TorsionStepSizeMax");
		const double torsionStepSizeMin = keywords_.asDouble("TorsionStepSizeMin");
		const double rRT = 1.0/(.008314472*cfg->temperature());

		// Print argument/parameter summary
		Messenger::print("IntraShake: Cutoff distance is %f\n", cutoffDistance);
		Messenger::print("IntraShake: Performing %i shake(s) per term\n", nShakesPerTerm);
		if (adjustBonds) Messenger::print("IntraShake: Distance step size for bond adjustments is %f Angstroms (allowed range is %f <= delta <= %f).\n", bondStepSize, bondStepSizeMin, bondStepSizeMax);
		if (adjustAngles) Messenger::print("IntraShake: Angle step size for angle adjustments is %f degrees (allowed range is %f <= delta <= %f).\n", angleStepSize, angleStepSizeMin, angleStepSizeMax);
		if (adjustTorsions) Messenger::print("IntraShake: Rotation step size for torsion adjustments is %f degrees (allowed range is %f <= delta <= %f).\n", torsionStepSize, torsionStepSizeMin, torsionStepSizeMax);
		Messenger::print("IntraShake: Target acceptance rate is %f.\n", targetAcceptanceRate);
		if (termEnergyOnly) Messenger::print("IntraShake: Only term energy will be considered (interatomic contributions with the system will be excluded).\n");
		Messenger::print("\n");

		ProcessPool::DivisionStrategy strategy = procPool.bestStrategy();

		// Create a Molecule distributor
		DynamicArray<Molecule>& moleculeArray = cfg->molecules();
		RegionalDistributor distributor(moleculeArray, cfg->cells(), procPool, strategy);

		// Create a local ChangeStore and EnergyKernel
		ChangeStore changeStore(procPool);
		EnergyKernel kernel(procPool, cfg, dissolve.potentialMap(), cutoffDistance);

		// Initialise the random number buffer
		procPool.initialiseRandomBuffer(ProcessPool::subDivisionStrategy(strategy));

		// Ensure that the Species used in the present Configuration have attached atom lists
		ListIterator<SpeciesInfo> speciesInfoIterator(cfg->usedSpecies());
		while (SpeciesInfo* spInfo = speciesInfoIterator.iterate())
		{
			Species* sp = spInfo->species();
			if (!sp->attachedAtomListsGenerated())
			{
				Messenger::print("Performing one-time generation of attached atom lists for intramolecular terms in Species '%s'...\n", sp->name());
				if (sp->nAtoms() > 500) Messenger::warn("'%s' is a large molecule - this might take a while! Consider using a different evolution module.\n", sp->name());
				sp->generateAttachedAtomLists();
			}
		}

		int shake, nBondAttempts = 0, nAngleAttempts = 0, nTorsionAttempts = 0, nBondAccepted = 0, nAngleAccepted = 0, nTorsionAccepted = 0;
		int molId, terminus;
		bool accept;
		double ppEnergy, newPPEnergy, intraEnergy, newIntraEnergy, delta, totalDelta = 0.0;
		Vec3<double> vji, vjk, v;
		Matrix3 transform;
		const Box* box = cfg->box();
		const Atom* i, *j, *k, *l;

		Timer timer;
		procPool.resetAccumulatedTime();
		while (distributor.cycle())
		{
			// Get next set of Molecule targets from the distributor
			Array<int> targetMolecules = distributor.assignedMolecules();

			// Switch parallel strategy if necessary
			if (distributor.currentStrategy() != strategy)
			{
				// Set the new strategy
				strategy = distributor.currentStrategy();

				// Re-initialise the random buffer
				procPool.initialiseRandomBuffer(ProcessPool::subDivisionStrategy(strategy));
			}

			// Loop over target Molecule
			for (int n = 0; n<targetMolecules.nItems(); ++n)
			{
				/*
				* Calculation Begins
				*/

				// Get Molecule index and pointer
				molId = targetMolecules[n];
				Molecule* mol = cfg->molecule(molId);
				const int indexOffset = mol->atom(0)->arrayIndex();

				// Set current atom targets in ChangeStore (whole molecule)
				changeStore.add(mol);

				// Calculate reference pairpotential energy for Molecule
				ppEnergy = termEnergyOnly ? 0.0 : kernel.energy(mol, ProcessPool::subDivisionStrategy(strategy), true);

				// Loop over defined bonds
				DynamicArrayConstIterator<SpeciesBond> bondIterator(mol->species()->constBonds());
				if (adjustBonds) while (const SpeciesBond* b = bondIterator.iterate())
				{
					// Get Atom pointers
					i = mol->atom(b->indexI());
					j = mol->atom(b->indexJ());
					
					// Store current energy of this intramolecular term, or the whole Molecule if it is present in a cycle
					intraEnergy = b->inCycle() ? kernel.intramolecularEnergy(mol) : kernel.energy(b, i, j);

					// Select random terminus
					terminus = procPool.random() > 0.5 ? 1 : 0;

					// Loop over number of shakes per term
					for (shake=0; shake<nShakesPerTerm; ++shake)
					{
						// Get translation vector, normalise, and apply random delta
						vji = box->minimumVector(i, j);
						vji.normalise();
						vji *= procPool.randomPlusMinusOne() * bondStepSize;

						// Adjust the Atoms attached to the selected terminus
						mol->translate(vji, b->nAttached(terminus), b->attached(terminus));

						// Update Cell positions of the adjusted Atoms
						cfg->updateCellLocation(b->nAttached(terminus), b->attached(terminus), indexOffset);

						// Calculate new energy
						newPPEnergy = termEnergyOnly ? 0.0 : kernel.energy(mol, ProcessPool::subDivisionStrategy(strategy), true);
						newIntraEnergy = b->inCycle() ? kernel.intramolecularEnergy(mol) : kernel.energy(b, i, j);
						
						// Trial the transformed Molecule
						delta = (newPPEnergy + newIntraEnergy) - (ppEnergy + intraEnergy);
						accept = delta < 0 ? true : (procPool.random() < exp(-delta*rRT));

						// Accept new (current) positions of the Molecule's Atoms?
						if (accept)
						{
							changeStore.updateAll();
							ppEnergy = newPPEnergy;
							intraEnergy = newIntraEnergy;
							distributor.increase(totalDelta, delta);
							distributor.increment(nBondAccepted);
						}
						else changeStore.revertAll();

						distributor.increment(nBondAttempts);
					}
				}

				// Loop over defined angles
				DynamicArrayConstIterator<SpeciesAngle> angleIterator(mol->species()->constAngles());
				if (adjustAngles) while (const SpeciesAngle* a = angleIterator.iterate())
				{
					// Get Atom pointers
					i = mol->atom(a->indexI());
					j = mol->atom(a->indexJ());
					k = mol->atom(a->indexK());

					// Store current energy of this intramolecular term
					intraEnergy = a->inCycle() ? kernel.intramolecularEnergy(mol) : kernel.energy(a, i, j, k);

					// Select random terminus
					terminus = procPool.random() > 0.5 ? 1 : 0;

					// Loop over number of shakes per term
					for (shake=0; shake<nShakesPerTerm; ++shake)
					{
						// Get bond vectors and calculate cross product to get rotation axis
						vji = box->minimumVector(j, i);
						vjk = box->minimumVector(j, k);
						v = vji * vjk;

						// Create suitable transformation matrix
						transform.createRotationAxis(v.x, v.y, v.z, procPool.randomPlusMinusOne()*angleStepSize, true);

						// Adjust the Atoms attached to the selected terminus
						mol->transform(box, transform, a->j()->r(), a->nAttached(terminus), a->attached(terminus));

						// Update Cell positions of the adjusted Atoms
						cfg->updateCellLocation(a->nAttached(terminus), a->attached(terminus), indexOffset);

						// Calculate new energy
						newPPEnergy = termEnergyOnly ? 0.0 : kernel.energy(mol, ProcessPool::subDivisionStrategy(strategy), true);
						newIntraEnergy = a->inCycle() ? kernel.intramolecularEnergy(mol) : kernel.energy(a, i, j, k);
						
						// Trial the transformed Molecule
						delta = (newPPEnergy + newIntraEnergy) - (ppEnergy + intraEnergy);
						accept = delta < 0 ? true : (procPool.random() < exp(-delta*rRT));

						// Accept new (current) positions of the Molecule's Atoms?
						if (accept)
						{
							changeStore.updateAll();
							ppEnergy = newPPEnergy;
							intraEnergy = newIntraEnergy;
							distributor.increase(totalDelta, delta);
							distributor.increment(nAngleAccepted);
						}
						else changeStore.revertAll();

						distributor.increment(nAngleAttempts);
					}
				}

				// Loop over defined torsions
				DynamicArrayConstIterator<SpeciesTorsion> torsionIterator(mol->species()->constTorsions());
				if (adjustTorsions) while (const SpeciesTorsion* t = torsionIterator.iterate())
				{
					// Get Atom pointers
					i = mol->atom(t->indexI());
					j = mol->atom(t->indexJ());
					k = mol->atom(t->indexK());
					l = mol->atom(t->indexL());

					// Store current energy of this intramolecular term
					intraEnergy = t->inCycle() ? kernel.intramolecularEnergy(mol) : kernel.energy(t, i, j, k, l);

					// Select random terminus
					terminus = procPool.random() > 0.5 ? 1 : 0;

					// Loop over number of shakes per term
					for (shake=0; shake<nShakesPerTerm; ++shake)
					{
						// Get bond vectors j-k to get rotation axis
						vjk = box->minimumVector(j, k);

						// Create suitable transformation matrix
						transform.createRotationAxis(vjk.x, vjk.y, vjk.z, procPool.randomPlusMinusOne()*torsionStepSize, true);

						// Adjust the Atoms attached to the selected terminus
						mol->transform(box, transform, terminus == 0 ? j->r() : k->r(), t->nAttached(terminus), t->attached(terminus));

						// Update Cell positions of the adjusted Atoms
						cfg->updateCellLocation(t->nAttached(terminus), t->attached(terminus), indexOffset);

						// Calculate new energy
						newPPEnergy = termEnergyOnly ? 0.0 : kernel.energy(mol, ProcessPool::subDivisionStrategy(strategy), true);
						newIntraEnergy = t->inCycle() ? kernel.intramolecularEnergy(mol) : kernel.energy(t, i, j, k, l);
						
						// Trial the transformed Molecule
						delta = (newPPEnergy + newIntraEnergy) - (ppEnergy + intraEnergy);
						accept = delta < 0 ? true : (procPool.random() < exp(-delta*rRT));

						// Accept new (current) positions of the Molecule's Atoms?
						if (accept)
						{
							changeStore.updateAll();
							ppEnergy = newPPEnergy;
							intraEnergy = newIntraEnergy;
							distributor.increase(totalDelta, delta);
							distributor.increment(nTorsionAccepted);
						}
						else changeStore.revertAll();

						distributor.increment(nTorsionAttempts);
					}
				}

				// Store modifications to Atom positions ready for broadcast
				changeStore.storeAndReset();

				/*
				* Calculation End
				*/
			}
			
			// Now all target Molecules have been processes, broadcast the changes made
			changeStore.distributeAndApply(cfg);
			changeStore.reset();
		}
		timer.stop();

		// Collect statistics across all processe
		if (!procPool.allSum(&totalDelta, 1, strategy)) return false;
		if (!procPool.allSum(&nBondAttempts, 1, strategy)) return false;
		if (!procPool.allSum(&nBondAccepted, 1, strategy)) return false;
		if (!procPool.allSum(&nAngleAttempts, 1, strategy)) return false;
		if (!procPool.allSum(&nAngleAccepted, 1, strategy)) return false;
		if (!procPool.allSum(&nTorsionAttempts, 1, strategy)) return false;
		if (!procPool.allSum(&nTorsionAccepted, 1, strategy)) return false;

		Messenger::print("IntraShake: Total energy delta was %10.4e kJ/mol.\n", totalDelta);
		Messenger::print("IntraShake: Total number of attempted moves was %i (%s work, %s comms).\n", nBondAttempts+nAngleAttempts+nTorsionAttempts, timer.totalTimeString(), procPool.accumulatedTimeString());

		// Calculate and report acceptance rates and adjust step sizes - if no moves were accepted, just decrease the current stepSize by a constant factor
		if (adjustBonds && (nBondAttempts > 0))
		{
			double bondRate = double(nBondAccepted) / nBondAttempts;
			Messenger::print("IntraShake: Overall bond shake acceptance rate was %4.2f% (%i of %i attempted moves).\n", 100.0*bondRate, nBondAccepted, nBondAttempts);

			bondStepSize *= (nBondAccepted == 0) ? 0.8 : bondRate/targetAcceptanceRate;
			if (bondStepSize < bondStepSizeMin) bondStepSize = bondStepSizeMin;
			else if (bondStepSize > bondStepSizeMax) bondStepSize = bondStepSizeMax;

			Messenger::print("IntraShake: Updated distance step size for bond adjustments is %f Angstroms.\n", bondStepSize); 
		}

		if (adjustAngles && (nAngleAttempts > 0))
		{
			double angleRate = double(nAngleAccepted) / nAngleAttempts;
			Messenger::print("IntraShake: Overall angle shake acceptance rate was %4.2f% (%i of %i attempted moves).\n", 100.0*angleRate, nAngleAccepted, nAngleAttempts);

			angleStepSize *= (nAngleAccepted == 0) ? 0.8 : angleRate/targetAcceptanceRate;
			if (angleStepSize < angleStepSizeMin) angleStepSize = angleStepSizeMin;
			else if (angleStepSize > angleStepSizeMax) angleStepSize = angleStepSizeMax;

			Messenger::print("IntraShake: Updated rotation step size for angle adjustments is %f degrees.\n", angleStepSize); 
		}

		if (adjustTorsions && (nTorsionAttempts > 0))
		{
			double torsionRate = double(nTorsionAccepted) / nTorsionAttempts;
			Messenger::print("IntraShake: Overall torsion shake acceptance rate was %4.2f% (%i of %i attempted moves).\n", 100.0*torsionRate, nTorsionAccepted, nTorsionAttempts);

			torsionStepSize *= (nTorsionAccepted == 0) ? 0.8 : torsionRate/targetAcceptanceRate;
			if (torsionStepSize < torsionStepSizeMin) torsionStepSize = torsionStepSizeMin;
			else if (torsionStepSize > torsionStepSizeMax) torsionStepSize = torsionStepSizeMax;

			Messenger::print("IntraShake: Updated rotation step size for torsion adjustments is %f degrees.\n", torsionStepSize); 
		}

		// Increase contents version in Configuration
		if ((nBondAccepted > 0) || (nAngleAccepted > 0) || (nTorsionAccepted > 0)) cfg->incrementContentsVersion();
	}

	return true;
}

