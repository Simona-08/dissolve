/*
	*** Procedure Node - Select
	*** src/procedure/nodes/select.h
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

#ifndef DISSOLVE_PROCEDURENODE_SELECT_H
#define DISSOLVE_PROCEDURENODE_SELECT_H

#include "procedure/nodes/node.h"
#include "math/range.h"
#include "templates/array.h"
#include "templates/list.h"
#include "templates/reflist.h"

// Forward Declarations
class DynamicSiteProcedureNode;
class SequenceProcedureNode;
class Element;
class Molecule;
class SiteStack;
class Species;
class SpeciesSite;

// Select Node
class SelectProcedureNode : public ProcedureNode
{
	public:
	// Constructors
	SelectProcedureNode(SpeciesSite* site = NULL, bool axesRequired = false);
	// Destructor
	~SelectProcedureNode();


	/*
	 * Identity
	 */
	public:
	// Return whether specified context is relevant for this node type
	bool isContextRelevant(ProcedureNode::NodeContext context);


	/*
	 * Selection Targets
	 */
	private:
	// Whether sites must have a defined orientation
	bool axesRequired_;
	// List of sites within Species to select
	RefList<SpeciesSite> speciesSites_;
	// List of DynamicSites to select, if any
	RefList<DynamicSiteProcedureNode> dynamicSites_;


	/*
	 * Selection Control
	 */
	private:
	// List of other sites (nodes) which will exclude one of our sites if it has the same Molecule parent
	RefList<SelectProcedureNode> sameMoleculeExclusions_;
	// List of Molecules currently excluded from selection
	RefList<const Molecule> excludedMolecules_;
	// List of other sites (nodes) which will exclude one of our sites if it is the same site
	RefList<SelectProcedureNode> sameSiteExclusions_;
	// List of Sites currently excluded from selection
	RefList<const Site> excludedSites_;
	// Molecule (from site) in which the site must exist (retrieved from keyword data)
	SelectProcedureNode* sameMolecule_;
	// Site to use for distance check
	SelectProcedureNode* distanceReferenceSite_;
	// Range of distance to allow from distance reference site (if limiting)
	Range inclusiveDistanceRange_;

	public:
	// Return list of Molecules currently excluded from selection
	const RefList<const Molecule>& excludedMolecules() const;
	// List of Sites currently excluded from selection
	const RefList<const Site>& excludedSites() const;
	// Return Molecule (from site) in which the site must exist
	const Molecule* sameMoleculeMolecule();


	/*
	 * Selected Sites
	 */
	private:
	// Array containing pointers to our selected sites
	Array<const Site*> sites_;
	// Current Site index
	int currentSiteIndex_;
	// Number of selections made by the node
	int nSelections_;
	// Cumulative number of sites ever selected
	int nCumulativeSites_;

	public:
	// Return the number of available sites in the current stack, if any
	int nSitesInStack() const;
	// Return the average number of sites selected
	double nAverageSites() const;
	// Return the cumulative number of sites ever selected
	int nCumulativeSites() const;
	// Return current site
	const Site* currentSite() const;


	/*
	 * Branch
	 */
	private:
	// Branch for ForEach (if defined)
	SequenceProcedureNode* forEachBranch_;

	public:
	// Return whether this node has a branch
	bool hasBranch() const;
	// Return SequenceNode for the branch (if it exists)
	SequenceProcedureNode* branch();
	// Add and return ForEach sequence
	SequenceProcedureNode* addForEachBranch(ProcedureNode::NodeContext context);


	/*
	 * Execute
	 */
	public:
	// Prepare any necessary data, ready for execution
	bool prepare(Configuration* cfg, const char* prefix, GenericList& targetList);
	// Execute node, targetting the supplied Configuration
	ProcedureNode::NodeExecutionResult execute(ProcessPool& procPool, Configuration* cfg, const char* prefix, GenericList& targetList);
	// Finalise any necessary data after execution
	bool finalise(ProcessPool& procPool, Configuration* cfg, const char* prefix, GenericList& targetList);
};

#endif
