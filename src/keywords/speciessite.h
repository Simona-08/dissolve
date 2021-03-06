/*
	*** Keyword - SpeciesSite
	*** src/keywords/speciessite.h
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

#ifndef DISSOLVE_KEYWORD_SPECIESSITE_H
#define DISSOLVE_KEYWORD_SPECIESSITE_H

#include "keywords/data.h"
#include "templates/list.h"

// Forward Declarations
class SpeciesSite;

// Keyword with Site Data
class SpeciesSiteKeyword : public KeywordData<SpeciesSite*>
{
	public:
	// Constructor
	SpeciesSiteKeyword(SpeciesSite* site = NULL, bool axesRequired = false);
	// Destructor
	~SpeciesSiteKeyword();


	/*
	 * Specification
	 */
	private:
	// Whether sites in the list must have a defined orientation
	bool axesRequired_;

	public:
	// Return whether axes are required for the site
	bool axesRequired() const;


	/*
	 * Arguments
	 */
	public:
	// Return minimum number of arguments accepted
	int minArguments() const;
	// Return maximum number of arguments accepted
	int maxArguments() const;
	// Parse arguments from supplied LineParser, starting at given argument offset
	bool read(LineParser& parser, int startArg, const CoreData& coreData);
	// Write keyword data to specified LineParser
	bool write(LineParser& parser, const char* keywordName, const char* prefix);


	/*
	 * Object Management
	 */
	protected:
	// Prune any references to the supplied Species in the contained data
	void removeReferencesTo(Species* sp);
	// Prune any references to the supplied SpeciesSite in the contained data
	void removeReferencesTo(SpeciesSite* spSite);
};

#endif

