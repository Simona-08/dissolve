/*
	*** Bragg Reflection
	*** src/classes/braggreflection.h
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

#ifndef DISSOLVE_BRAGGREFLECTION_H
#define DISSOLVE_BRAGGREFLECTION_H

#include "templates/array2d.h"
#include "templates/mpilistitem.h"
#include "base/genericitembase.h"

// Forward Declarations
/* none */

// BraggReflection Class
class BraggReflection : public MPIListItem<BraggReflection>,  public GenericItemBase
{
	/*
	 *  BraggReflection acts as a 'bin' for collecting contributions arising from a set of KVectors which occur at the same Q value.
	 */
	public:
	// Constructor
	BraggReflection();
	// Destructor
	~BraggReflection();
	// Copy constructor
	BraggReflection(const BraggReflection& source);
	// Operator=
	void operator=(const BraggReflection& source);


	/*
	 * Data
	 */
	private:
	// Q-position of reflection
	double q_;
	// Integer index (derived from q_)
	int index_;
	// Intensity contributions from atomtype pairs
	Array2D<double> intensities_;
	// Number of k-vectors which contributed to the intensity (for normalisation)
	int nKVectors_;

	public:
	// Initialise arrays
	void initialise(double q, int index, int nTypes);
	// Return Q value of reflection
	double q() const;
	// Set index 
	void setIndex(int index);
	// Return index
	int index() const;
	// Reset stored intensities
	void resetIntensities();
	// Add intensity between specified atomtypes from k-vector
	void addIntensity(int typeI, int typeJ, double intensity);
	// Scale intensities between all atom types by factor provided
	void scaleIntensities(double factor);
	// Scale intensity between all specific atom types by factor provided
	void scaleIntensity(int typeI, int typeJ, double factor);
	// Return intensity between specified atom types for this reflection
	double intensity(int typeI, int typeJ) const;
	// Increment number of contributing k-vectors
	void addKVectors(int count);
	// Return number of k-vectors contributing to this reflection
	int nKVectors() const;


	/*
	 * GenericItemBase Implementations
	 */
	public:
	// Return class name
	static const char* itemClassName();


	/*
	 * Parallel Comms
	 */
	public:
	// Broadcast data from root to all other processes
	bool broadcast(ProcessPool& procPool, const int root, const CoreData& coreData);
	// Check item equality
	bool equality(ProcessPool& procPool);
};

#endif