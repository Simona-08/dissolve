/*
	*** Configuration - Contents
	*** src/classes/configuration_contents.cpp
	Copyright T. Youngs 2012-2019

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

#include "classes/configuration.h"
#include "classes/atomtype.h"
#include "classes/box.h"
#include "classes/cell.h"
#include "classes/grain.h"
#include "classes/species.h"
#include "base/processpool.h"
#include "modules/import/import.h"

// Clear contents of Configuration, leaving other definitions intact
void Configuration::empty()
{
	molecules_.clear();
	grains_.clear();
	atoms_.clear();
	usedAtomTypes_.clear();
	if (box_ != NULL) delete box_;
	box_ = new CubicBox(1.0);
	cells_.clear();

	// Clear used species
	usedSpecies_.clear();

	++contentsVersion_;
}

// Initialise content arrays
void Configuration::initialiseArrays(int nMolecules, int nGrains)
{
	// Clear current contents
	empty();

	molecules_.initialise(nMolecules);
	grains_.initialise(nGrains);
}

// Return specified used type
AtomType* Configuration::usedAtomType(int index)
{
	return usedAtomTypes_.atomType(index);
}

// Return specified used type data
AtomTypeData* Configuration::usedAtomTypeData(int index)
{
	return usedAtomTypes_[index];
}

// Return first AtomTypeData for this Configuration
AtomTypeData* Configuration::usedAtomTypes()
{
	return usedAtomTypes_.first();
}

// Return AtomTypeList for this Configuration
const AtomTypeList& Configuration::usedAtomTypesList() const
{
	return usedAtomTypes_;
}

// Return number of atom types used in this Configuration
int Configuration::nUsedAtomTypes() const
{
	return usedAtomTypes_.nItems();
}

// Add Species to list of those used by the Configuration, setting/adding the population specified
SpeciesInfo* Configuration::addUsedSpecies(Species* sp, int population)
{
	// Check if we have an existing info for this Species
	SpeciesInfo* spInfo = usedSpeciesInfo(sp);
	if (!spInfo)
	{
		spInfo = usedSpecies_.add();
		spInfo->setSpecies(sp);
	}

	// Increase the population
	spInfo->addPopulation(population);

	return spInfo;
}

// Return SpeciesInfo for specified Species
SpeciesInfo* Configuration::usedSpeciesInfo(Species* sp)
{
	for (SpeciesInfo* spInfo = usedSpecies_.first(); spInfo != NULL; spInfo = spInfo->next()) if (spInfo->species() == sp) return spInfo;

	return NULL;
}

// Return list of SpeciesInfo for the Configuration
List<SpeciesInfo>& Configuration::usedSpecies()
{
	return usedSpecies_;
}

// Return if the specified Species is present in the usedSpecies list
bool Configuration::hasUsedSpecies(Species* sp)
{
	for (SpeciesInfo* spInfo = usedSpecies_.first(); spInfo != NULL; spInfo = spInfo->next()) if (spInfo->species() == sp) return true;

	return false;
}

// Return the atomic density of the Configuration
double Configuration::atomicDensity() const
{
	return nAtoms() / box_->volume();
}

// Return version of current contents
int Configuration::contentsVersion() const
{
	return contentsVersion_;
}

// Increment version of current contents
void Configuration::incrementContentsVersion()
{
	++contentsVersion_;
}

// Add Molecule to Configuration based on the supplied Species
Molecule* Configuration::addMolecule(Species* sp)
{
	// Create the new Molecule object and set its Species pointer
	Molecule* newMolecule = molecules_.add();
	newMolecule->setSpecies(sp);

	// Update the relevant SpeciesInfo population
	addUsedSpecies(sp, 1);

	// Add Atoms from Species to the Molecule
	SpeciesAtom* spi = sp->firstAtom();
	for (int n=0; n<sp->nAtoms(); ++n, spi = spi->next()) addAtom(spi, newMolecule, spi->r());

	// Add Grains from Species into the Molecule
	SpeciesGrain* spg = sp->grains();
	for (int n = 0; n<sp->nGrains(); ++n, spg = spg->next())
	{
		// Create new Grain
		Grain* g = addGrain(newMolecule);

		// Add Atoms to the Grain
		for (int m=0; m<spg->nAtoms(); ++m)
		{
			g->addAtom(newMolecule->atom(spg->atom(m)->item()->index()));
		}
	}

	return newMolecule;
}

// Return number of Molecules in Configuration
int Configuration::nMolecules() const
{
	return molecules_.nItems();
}

// Return array of Molecules
DynamicArray<Molecule>& Configuration::molecules()
{
	return molecules_;
}

// Return nth Molecule
Molecule* Configuration::molecule(int n)
{
	return molecules_[n];
}

// Add new Grain to Configuration, with Molecule parent specified
Grain* Configuration::addGrain(Molecule* molecule)
{
	// Create the new Grain object
	Grain* newGrain = grains_.add();

	// Add it to the specified Molecule, which also sets the Molecule parent of the Grain
	molecule->addGrain(newGrain);

	return newGrain;
}

// Return number of grains
int Configuration::nGrains() const
{
	return grains_.nItems();
}

// Return grain array
DynamicArray<Grain>& Configuration::grains()
{
	return grains_;
}

// Return nth grain
Grain* Configuration::grain(int n)
{
#ifdef CHECKS
	if ((n < 0) || (n >= grains_.nItems()))
	{
		Messenger::print("OUT_OF_RANGE - Grain index %i passed to Configuration::grain() is out of range (nGrains = %i).\n", n, grains_.nItems());
		return NULL;
	}
#endif
	return grains_[n];
}

// Add new Atom to Configuration, with Molecule and Grain parents specified
Atom* Configuration::addAtom(const SpeciesAtom* sourceAtom, Molecule* molecule, Vec3<double> r)
{
	// Create new Atom object and set its source pointer
	Atom* newAtom = atoms_.add();
	newAtom->setSpeciesAtom(sourceAtom);

	// Register the Atom in the specified Molecule (this will also set the Molecule pointer in the Atom)
	molecule->addAtom(newAtom);

	// Set the position
	newAtom->setCoordinates(r);

	// Update our typeIndex (non-isotopic) and set local and master type indices
	AtomTypeData* atd = usedAtomTypes_.add(sourceAtom->atomType(), 1);
	newAtom->setLocalTypeIndex(atd->listIndex());
	newAtom->setMasterTypeIndex(sourceAtom->atomType()->index());

	return newAtom;
}

// Return number of Atoms in Configuration
int Configuration::nAtoms() const
{
	return atoms_.nItems();
}

// Return Atom array
DynamicArray<Atom>& Configuration::atoms()
{
	return atoms_;
}

// Return Atom array (const)
const DynamicArray<Atom>& Configuration::constAtoms() const
{
	return atoms_;
}

// Return nth atom
Atom* Configuration::atom(int n)
{
#ifdef CHECKS
	if ((n < 0) || (n >= atoms_.nItems()))
	{
		Messenger::print("OUT_OF_RANGE - Atom index %i passed to Configuration::atom() is out of range (nAtoms = %i).\n", n, atoms_.nItems());
		return NULL;
	}
#endif
	return atoms_[n];
}
