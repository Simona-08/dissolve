/*
	*** Generic Item Container - Array<int>
	*** src/genericitems/arrayint.h
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

#ifndef DISSOLVE_GENERICITEMCONTAINER_ARRAYINT_H
#define DISSOLVE_GENERICITEMCONTAINER_ARRAYINT_H

#include "genericitems/container.h"

// GenericItemContainer< Array<int> >
template <> class GenericItemContainer< Array<int> > : public GenericItem
{
	public:
	// Constructor
	GenericItemContainer< Array<int> >(const char* name, int flags = 0) : GenericItem(name, flags)
	{
	}


	/*
	 * Data
	 */
	private:
	// Data item
	Array<int> data_;

	public:
	// Return data item
	Array<int>& data()
	{
		return data_;
	}


	/*
	 * Item Class
	 */
	protected:
	// Create a new GenericItem containing same class as current type
	GenericItem* createItem(const char* className, const char* name, int flags = 0)
	{
		if (DissolveSys::sameString(className, itemClassName())) return new GenericItemContainer< Array<int> >(name, flags);
		return NULL;
	}

	public:
	// Return class name contained in item
	const char* itemClassName()
	{
		return "Array<int>";
	}


	/*
	 * I/O
	 */
	public:
	// Write data through specified parser
	bool write(LineParser& parser)
	{
		parser.writeLineF("%i\n", data_.nItems());
		for (int n=0; n<data_.nItems(); ++n)
		{
			if (!parser.writeLineF("%i\n", data_.constAt(n))) return false;
		}
		return true;
	}
	// Read data through specified parser
	bool read(LineParser& parser, const CoreData& coreData)
	{
		if (parser.getArgsDelim(LineParser::Defaults) != LineParser::Success) return false;
		int nItems = parser.argi(0);
		data_.createEmpty(nItems);
		for (int n=0; n<nItems; ++n)
		{
			if (parser.getArgsDelim(LineParser::Defaults) != LineParser::Success) return false;
			data_.add(parser.argi(0));
		}
		return true;
	}


	/*
	 * Parallel Comms
	 */
	public:
	// Broadcast item contents
	bool broadcast(ProcessPool& procPool, const int root, const CoreData& coreData)
	{
		return procPool.broadcast(data_, root);
	}
	// Return equality between items
	bool equality(ProcessPool& procPool)
	{
		return procPool.equality(data_);
	}
};

#endif
