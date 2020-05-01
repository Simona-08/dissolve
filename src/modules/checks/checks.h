/*
	*** Checks Module
	*** src/modules/checks/checks.h
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

#ifndef DISSOLVE_MODULE_CHECKS_H
#define DISSOLVE_MODULE_CHECKS_H

#include "base/geometry.h"
#include "module/module.h"

// Forward Declarations
/* none */

// Checks Module
class ChecksModule : public Module
{
	/*
	 * Module for making simple checkss
	 */

      public:
	ChecksModule();
	~ChecksModule();

	/*
	 * Instances
	 */
      public:
	// Create instance of this module
	Module *createInstance() const;

	/*
	 * Definition
	 */
      public:
	// Return type of module
	const char *type() const;
	// Return category for module
	const char *category() const;
	// Return brief description of module
	const char *brief() const;
	// Return the number of Configuration targets this Module requires
	int nRequiredTargets() const;

	/*
	 * Initialisation
	 */
      protected:
	// Perform any necessary initialisation for the Module
	void initialise();
	/*
	 * Processing
	 */
      private:
	// Run main processing
	bool process(Dissolve &dissolve, ProcessPool &procPool);

	/*
	 * Checks
	 */
      private:
	// Distances between atoms
	List<Geometry> distances_;
	// Angles between atoms
	List<Geometry> angles_;
};

#endif
