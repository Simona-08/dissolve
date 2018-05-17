/*
	*** Module Group
	*** src/module/group.cpp
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

#include "module/group.h"
#include "base/sysfunc.h"

// Constructor
ModuleGroup::ModuleGroup(const char* name) : ListItem<ModuleGroup>()
{
	name_ = name;
}

// Destructor
ModuleGroup::~ModuleGroup()
{
}

/*
 * Module Group
 */

// Return name of group
const char* ModuleGroup::name()
{
	return name_.get();
}

// Return whether name of group is as specified
bool ModuleGroup::isName(const char* queryName)
{
	return DUQSys::sameString(name_, queryName);
}

// Associate Module to group
void ModuleGroup::add(Module* module)
{
	modules_.addUnique(module);
}

// Return whether the specified Module is in the group
bool ModuleGroup::contains(Module* module) const
{
	return modules_.contains(module);
}

// Return total number of Modules in the group
int ModuleGroup::nModules() const
{
	return modules_.nItems();
}

// Return reflist of Modules
const RefList<Module,bool>& ModuleGroup::modules() const
{
	return modules_;
}
