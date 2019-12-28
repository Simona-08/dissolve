/*
	*** Calculate Average Molecule Module - Initialisation
	*** src/modules/calculate_avgmol/init.cpp
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

#include "modules/calculate_avgmol/avgmol.h"
#include "keywords/types.h"

// Perform any necessary initialisation for the Module
void CalculateAvgMolModule::initialise()
{
	// Set up keywords
	// TODO Must only accept oriented sites (updated in plot3d branch... fix later)
	keywords_.add("Target", new SpeciesSiteKeyword(NULL), "Site", "Target site about which to calculate average species geometry");

	targetSpecies_ = NULL;
}