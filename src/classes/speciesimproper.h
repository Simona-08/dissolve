/*
	*** SpeciesImproper Definition
	*** src/classes/speciesimproper.h
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

#ifndef DISSOLVE_SPECIESIMPROPER_H
#define DISSOLVE_SPECIESIMPROPER_H

#include "classes/speciesintra.h"
#include "base/enumoptions.h"
#include "templates/dynamicarrayobject.h"

// Forward Declarations
class SpeciesAtom;
class Species;
class ProcessPool;

// SpeciesImproper Definition
class SpeciesImproper : public SpeciesIntra, public DynamicArrayObject<SpeciesImproper>
{
	public:
	// Constructor
	SpeciesImproper();
	// Destructor
	~SpeciesImproper();


	/*
	 * DynamicArrayObject Virtuals
	 */
	public:
	// Clear object, ready for re-use
	void clear();


	/*
	 * Atom Information
	 */
	private:
	// First SpeciesAtom in interaction
	SpeciesAtom* i_;
	// Second SpeciesAtom in interaction
	SpeciesAtom* j_;
	// Third SpeciesAtom in interaction
	SpeciesAtom* k_;
	// Fourth SpeciesAtom in interaction
	SpeciesAtom* l_;

	public:
	// Set Atoms involved in Improper
	void setAtoms(SpeciesAtom* i, SpeciesAtom* j, SpeciesAtom* k, SpeciesAtom* l);
	// Return first SpeciesAtom
	SpeciesAtom* i() const;
	// Return second SpeciesAtom
	SpeciesAtom* j() const;
	// Return third SpeciesAtom
	SpeciesAtom* k() const;
	// Return fourth SpeciesAtom
	SpeciesAtom* l() const;
	// Return whether the improper uses the specified SpeciesAtom
	bool uses(SpeciesAtom* spAtom) const;
	// Return index (in parent Species) of first SpeciesAtom
	int indexI() const;
	// Return index (in parent Species) of second SpeciesAtom
	int indexJ() const;
	// Return index (in parent Species) of third SpeciesAtom
	int indexK() const;
	// Return index (in parent Species) of fourth SpeciesAtom
	int indexL() const;
	// Return index (in parent Species) of nth SpeciesAtom in interaction
	int index(int n) const;
	// Return whether SpeciesAtoms match those specified
	bool matches(SpeciesAtom* i, SpeciesAtom* j, SpeciesAtom* k, SpeciesAtom* l) const;


	/*
	 * Interaction Parameters
	 */
	public:
	// Improper functional forms
	enum ImproperFunction
	{
		NoForm,
		CosineForm
	};
	// Return enum options for ImproperFunction
	static EnumOptions<ImproperFunction> improperFunctions(); 

	public:
	// Set up any necessary parameters
	void setUp();
	// Return fundamental frequency for the interaction
	double fundamentalFrequency(double reducedMass) const;
	// Return type of this interaction
	SpeciesIntra::InteractionType type() const;
	// Return energy for specified angle
	double energy(double angleInDegrees) const;
	// Return force multiplier for specified angle
	double force(double angleInDegrees) const;


	/*
	 * Parallel Comms
	 */
	public:
	// Broadcast data from Master to all Slaves
	bool broadcast(ProcessPool& procPool, const List<SpeciesAtom>& atoms);
};

#endif
