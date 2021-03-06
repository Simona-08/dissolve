/*
	*** Procedure Node - Process3D
	*** src/procedure/nodes/process3d.h
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

#ifndef DISSOLVE_PROCEDURENODE_PROCESS3D_H
#define DISSOLVE_PROCEDURENODE_PROCESS3D_H

#include "procedure/nodes/node.h"
#include "procedure/nodes/nodereference.h"
#include "io/export/data3d.h"
#include "base/charstring.h"
#include "templates/reflist.h"

// Forward Declarations
class Collect3DProcedureNode;
class Data3D;
class LineParser;
class NodeScopeStack;

// Procedure Node - Process3D
class Process3DProcedureNode : public ProcedureNode
{
	public:
	// Constructor
	Process3DProcedureNode(const Collect3DProcedureNode* target = NULL);
	// Destructor
	~Process3DProcedureNode();


	/*
	 * Identity
	 */
	public:
	// Return whether specified context is relevant for this node type
	bool isContextRelevant(ProcedureNode::NodeContext context);


	/*
	 * Data
	 */
	private:
	// Collect3D node that we are processing
	const Collect3DProcedureNode* collectNode_;
	// Pointer to processed data (stored in processing data list)
	Data3D* processedData_;
	// Export file and format
	Data3DExportFileFormat exportFileAndFormat_;

	public:
	// Return processed data
	const Data3D& processedData() const;
	// Return value label
	const char* valueLabel() const;
	// Return x axis label
	const char* xAxisLabel() const;
	// Return y axis label
	const char* yAxisLabel() const;
	// Return z axis label
	const char* zAxisLabel() const;


	/*
	 * Branches
	 */
	private:
	// Branch for normalisation of data (if defined)
	SequenceProcedureNode* normalisationBranch_;

	public:
	// Add and return normalisation sequence branch
	SequenceProcedureNode* addNormalisationBranch();
	// Return whether this node has a branch
	bool hasBranch() const;
	// Return SequenceNode for the branch (if it exists)
	SequenceProcedureNode* branch();


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
