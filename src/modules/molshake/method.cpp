/*
	*** MolShake Module - Method
	*** src/modules/molshake/method.cpp
	Copyright T. Youngs 2012-2018

	This file is part of dUQ.

	dUQ is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	dUQ is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with dUQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "modules/molshake/molshake.h"
#include "main/duq.h"
#include "classes/box.h"
#include "classes/cell.h"
#include "classes/changestore.h"
#include "classes/configuration.h"
#include "classes/energykernel.h"
#include "base/processpool.h"
#include "base/timer.h"
#include "templates/genericlisthelper.h"

// Perform set up tasks for module
bool MolShakeModule::setUp(ProcessPool& procPool)
{
	return true;
}

// Execute pre-processing stage
bool MolShakeModule::preProcess(DUQ& duq, ProcessPool& procPool)
{
	return false;
}

// Execute Method
bool MolShakeModule::process(DUQ& duq, ProcessPool& procPool)
{
	/*
	 * Perform a Molecule shake
	 * 
	 * This is a parallel routine, with processes operating as one pool.
	 */

	// Check for zero Configuration targets
	if (targetConfigurations_.nItems() == 0)
	{
		Messenger::warn("No Configuration targets for Module.\n");
		return true;
	}

	// Loop over target Configurations
	for (RefListItem<Configuration,bool>* ri = targetConfigurations_.first(); ri != NULL; ri = ri->next)
	{
		// Grab Configuration pointer
		Configuration* cfg = ri->item;

		// Set up process pool - must do this to ensure we are using all available processes
		procPool.assignProcessesToGroups(cfg->processPool());

		// Get reference to relevant module data
		GenericList& moduleData = configurationLocal_ ? cfg->moduleData() : duq.processingModuleData();

		// Retrieve control parameters from Configuration
		double cutoffDistance = keywords_.asDouble("CutoffDistance");
		if (cutoffDistance < 0.0) cutoffDistance = duq.pairPotentialRange();
		double rotationStepSize = GenericListHelper<double>::retrieve(cfg->moduleData(), "RotationStepSize", uniqueName(), keywords_.asDouble("RotationStepSize"));
		const double rotationStepSizeMax = keywords_.asDouble("RotationStepSizeMax");
		const double rotationStepSizeMin = keywords_.asDouble("RotationStepSizeMin");

		const int nShakesPerMolecule = keywords_.asInt("ShakesPerMolecule");
		const double targetAcceptanceRate = keywords_.asDouble("TargetAcceptanceRate");
		double translationStepSize = GenericListHelper<double>::retrieve(cfg->moduleData(), "TranslationStepSize", uniqueName(), keywords_.asDouble("TranslationStepSize"));
		const double translationStepSizeMax = keywords_.asDouble("TranslationStepSizeMax");
		const double translationStepSizeMin = keywords_.asDouble("TranslationStepSizeMin");
		const double termScale = 1.0;
		const double rRT = 1.0/(.008314472*cfg->temperature());

		// Print argument/parameter summary
		Messenger::print("MolShake: Cutoff distance is %f\n", cutoffDistance);
		Messenger::print("MolShake: Performing %i shake(s) per Molecule\n", nShakesPerMolecule);
		Messenger::print("MolShake: Step size for translation adjustments is %f Angstroms (allowed range is %f <= delta <= %f).\n", translationStepSize, translationStepSizeMin, translationStepSizeMax);
		Messenger::print("MolShake: Step size for rotation adjustments is %f degrees (allowed range is %f <= delta <= %f).\n", rotationStepSize, rotationStepSizeMin, rotationStepSizeMax);

		// Create a local ChangeStore and EnergyKernel
		ChangeStore changeStore(procPool);
		EnergyKernel kernel(procPool, cfg, duq.potentialMap(), cutoffDistance);

		// Initialise the random number buffer
		procPool.initialiseRandomBuffer(ProcessPool::Pool);

		int shake, nRotationAttempts = 0, nTranslationAttempts = 0, nRotationsAccepted = 0, nTranslationsAccepted = 0, nGeneralAttempts = 0;
		bool accept;
		double currentEnergy, newEnergy, delta, totalDelta = 0.0;
		Vec3<double> rDelta;
		Matrix3 transform;
		const Box* box = cfg->box();

		/*
		 * In order to be able to adjust translation and rotational steps independently, we will perform 80% of moves including both a translation
		 * a rotation, 10% using only translations, and 10% using only rotations.
		 */

		// Set initial random offset for our counter determining whether to perform R+T, R, or T.
		int count = procPool.random()*10;
		bool rotate, translate;

		Timer timer;
		procPool.resetAccumulatedTime();
		for (int m = 0; m < cfg->nMolecules(); ++m)
		{
			/*
			 * Calculation Begins
			 */

			// Get Molecule pointer
			Molecule* mol = cfg->molecule(m);

			// Set current atom targets in ChangeStore (entire cell contents)
			changeStore.add(mol);

			// Calculate reference energy for Molecule, including intramolecular terms
			currentEnergy = kernel.energy(mol, ProcessPool::OverPoolProcesses);

			// Loop over number of shakes per atom
			for (shake=0; shake<nShakesPerMolecule; ++shake)
			{
				// Determine what move(s) will we attempt
				if (count == 0)
				{
					rotate = true;
					translate = false;
				}
				else if (count == 1)
				{
					rotate = false;
					translate = true;
				}
				else
				{
					rotate = true;
					translate = true;
				}

				// Create a random translation vector and apply it to the Molecule's centre
				if (translate)
				{
					rDelta.set(procPool.randomPlusMinusOne()*translationStepSize, procPool.randomPlusMinusOne()*translationStepSize, procPool.randomPlusMinusOne()*translationStepSize);
					mol->translate(rDelta);
				}

				// Create a random rotation matrix and apply it to the Molecule
				if (rotate)
				{
					transform.createRotationXY(procPool.randomPlusMinusOne()*rotationStepSize, procPool.randomPlusMinusOne()*rotationStepSize);
					mol->transform(box, transform);
				}

				// Calculate new energy
				newEnergy = kernel.energy(mol, ProcessPool::OverPoolProcesses);
				
				// Trial the transformed atom position
				delta = newEnergy - currentEnergy;
// 				printf("delta = %f\n", delta);
				accept = delta < 0 ? true : (procPool.random() < exp(-delta*rRT));

				if (accept)
				{
// 					Messenger::print("Accepts move with delta %f\n", delta);
					// Accept new (current) position of target Atom
					changeStore.updateAll();
					currentEnergy = newEnergy;
					totalDelta += delta;
					if (rotate) ++nRotationsAccepted;
					if (translate) ++nTranslationsAccepted;
				}
				else changeStore.revertAll();

				// Increase attempt counters
				if (rotate) ++nRotationAttempts;
				if (translate) ++nTranslationAttempts;
				++nGeneralAttempts;

				// Increase and fold move type counter
				++count;
				if (count > 9) count = 0;
			}

			// Store modifications to Atom positions ready for broadcast
			changeStore.storeAndReset();

			/*
			* Calculation End
			*/

			// Distribute coordinate changes to all processes
			changeStore.distributeAndApply(cfg);
			changeStore.reset();
		}
		timer.stop();

		Messenger::print("MolShake: Total energy delta was %10.4e kJ/mol.\n", totalDelta);

		// Calculate and print acceptance rates
		double transRate = double(nTranslationsAccepted)/nTranslationAttempts;
		double rotRate = double(nRotationsAccepted)/nRotationAttempts;
		Messenger::print("MolShake: Total number of attempted moves was %i (%s work, %s comms)\n", nGeneralAttempts, timer.totalTimeString(), procPool.accumulatedTimeString());

		Messenger::print("MolShake: Overall translation acceptance rate was %4.2f% (%i of %i attempted moves)\n", 100.0*transRate, nTranslationsAccepted, nTranslationAttempts);
		Messenger::print("MolShake: Overall rotation acceptance rate was %4.2f% (%i of %i attempted moves)\n", 100.0*rotRate, nRotationsAccepted, nRotationAttempts);

		// Update and set translation step size
		translationStepSize *= (nTranslationsAccepted == 0) ? 0.8 : transRate/targetAcceptanceRate;
		if (translationStepSize < translationStepSizeMin) translationStepSize = translationStepSizeMin;
		else if (translationStepSize > translationStepSizeMax) translationStepSize = translationStepSizeMax;

		Messenger::print("MolShake: Updated step size for translations is %f Angstroms.\n", translationStepSize); 
		GenericListHelper<double>::realise(cfg->moduleData(), "TranslationStepSize", uniqueName(), GenericItem::InRestartFileFlag) = translationStepSize;

		// Update and set rotation step size
		rotationStepSize *= (nRotationsAccepted == 0) ? 0.8 : rotRate/targetAcceptanceRate;
		if (rotationStepSize < rotationStepSizeMin) rotationStepSize = rotationStepSizeMin;
		else if (rotationStepSize > rotationStepSizeMax) rotationStepSize = rotationStepSizeMax;

		Messenger::print("MolShake: Updated step size for rotations is %f Angstroms.\n", rotationStepSize); 
		GenericListHelper<double>::realise(cfg->moduleData(), "RotationStepSize", uniqueName(), GenericItem::InRestartFileFlag) = rotationStepSize;

		// Increment configuration changeCount_
		if ((nRotationsAccepted > 0) || (nTranslationsAccepted > 0)) cfg->incrementCoordinateIndex();

		// Update total energy
		cfg->registerEnergyChange(totalDelta);
		cfg->accumulateEnergyChange();
	}

	return true;
}

// Execute post-processing stage
bool MolShakeModule::postProcess(DUQ& duq, ProcessPool& procPool)
{
	return true;
}
