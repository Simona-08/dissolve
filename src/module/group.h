/*
	*** Module Group
	*** src/module/group.h
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

#ifndef DISSOLVE_MODULEGROUP_H
#define DISSOLVE_MODULEGROUP_H

#include "base/charstring.h"
#include "templates/listitem.h"
#include "templates/reflist.h"

// Forward Declarations
class Module;

// ModuleGroup
class ModuleGroup : public ListItem<ModuleGroup>
{
	public:
	// Constructor
	ModuleGroup(const char* name = NULL);
	// Destructor
	~ModuleGroup();


	/*
	 * Module Group
	 */
	private:
	// Name of the group
	CharString name_;
	// RefList of Modules in this group
	RefList<Module> modules_;

	public:
	// Return name of group
	const char* name();
	// Return whether name of group is as specified
	bool isName(const char* queryName);
	// Associate Module to group
	void add(Module* module);
	// Remove Module from group
	void remove(Module* module);
	// Return whether the specified Module is in the group
	bool contains(Module* module) const;
	// Return number of Modules in the group
	int nModules() const;
	// Return reflist of Modules
	const RefList<Module>& modules() const;
};

#endif

