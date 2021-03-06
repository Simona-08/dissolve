/*
	*** OPLSAA (2005) Alcohols Forcefield
	*** src/data/ff/oplsaa2005/alcohols.h
	Copyright T. Youngs 2019-2020

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

#ifndef DISSOLVE_FORCEFIELD_OPLSAA_ALCOHOLS_H
#define DISSOLVE_FORCEFIELD_OPLSAA_ALCOHOLS_H

#include "data/ff/oplsaa2005/alkanes.h"

// Forward Declarations
/* none */

// OPLS-AA/2005 Alcohols Forcefield
class Forcefield_OPLSAA2005_Alcohols : public Forcefield_OPLSAA2005_Alkanes
{
	public:
	// Constructor / Destructor
	Forcefield_OPLSAA2005_Alcohols();
	~Forcefield_OPLSAA2005_Alcohols();


	/*
	 * Definition
	 */
	public:
	// Return name of Forcefield
	const char* name() const;
	// Return description for Forcefield
	const char* description() const;
};

#endif
