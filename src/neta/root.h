/*
	*** NETA Root Node
	*** src/neta/root.h
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

#ifndef DISSOLVE_NETA_ROOT_H
#define DISSOLVE_NETA_ROOT_H

#include "neta/node.h"
#include "classes/speciesbond.h"
#include "templates/pointerarray.h"

// Forward Declarations
class Element;
class ForcefieldAtomType;
class NETADefinition;

// NETA Root Node
class NETARootNode : public NETANode
{
	public:
	// Constructor / Destructor
	NETARootNode(NETADefinition* parent);
	~NETARootNode();


	/*
	 * Modifiers
	 */
	private:
	// Number of bonds value
	int nBondsValue_;
	// Numbe of bonds value comparison operator
	NETANode::ComparisonOperator nBondsValueOperator_;
	// Number of hydrogens value
	int nHydrogensValue_;
	// Numbe of hydrogens value comparison operator
	NETANode::ComparisonOperator nHydrogensValueOperator_;

	public:
	// Available modifiers
	enum NETARootModifier
	{
		NBondsModifier,			/* 'nbonds' - Specifies number of bonds (default = -1) */
		NHydrogensModifier,		/* 'nh' - Specifies number of hydrogens (default = -1) */
		nRootModifiers
	};
	// Return enum options for NETARootModifiers
	static EnumOptions<NETARootNode::NETARootModifier> modifiers();
	// Return whether the specified modifier is valid for this node
	bool isValidModifier(const char* s) const;
	// Set value and comparator for specified modifier
	bool setModifier(const char* modifier, ComparisonOperator op, int value);


	/*
	 * Scoring
	 */
	public:
	// Evaluate the node and return its score
	int score(const SpeciesAtom* i, RefList<const SpeciesAtom>& matchPath) const;
};

#endif
