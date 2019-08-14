/*
	*** Procedure Node - Parameters
	*** src/procedure/nodes/parameters.cpp
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

#include "procedure/nodes/parameters.h"
#include "procedure/nodescopestack.h"
#include "expression/variable.h"
#include "base/lineparser.h"
#include "base/sysfunc.h"

// Constructor
ParametersProcedureNode::ParametersProcedureNode() : ProcedureNode(ProcedureNode::ParametersNode)
{
}

// Destructor
ParametersProcedureNode::~ParametersProcedureNode()
{
}

/*
 * Identity
 */

// Return whether specified context is relevant for this node type
bool ParametersProcedureNode::isContextRelevant(ProcedureNode::NodeContext context)
{
	return (context != ProcedureNode::NoContext);
}

/*
 * Node Keywords
 */

// Set up keywords for node
void ParametersProcedureNode::setUpKeywords()
{
}

// Return enum option info for ParametersNodeKeyword
EnumOptions<ParametersProcedureNode::ParametersNodeKeyword> ParametersProcedureNode::parametersNodeKeywords()
{
	static EnumOptionsList ParametersNodeTypeKeywords = EnumOptionsList() <<
		EnumOption(ParametersProcedureNode::DoubleKeyword,		"Double",	2) <<
		EnumOption(ParametersProcedureNode::IntegerKeyword,		"Integer",	2) <<
		EnumOption(ParametersProcedureNode::EndParametersKeyword,	"EndParameters");

	static EnumOptions<ParametersProcedureNode::ParametersNodeKeyword> options("ParametersNodeKeyword", ParametersNodeTypeKeywords);

	return options;
}

/* 
 * Execute
 */

// Prepare any necessary data, ready for execution
bool ParametersProcedureNode::prepare(Configuration* cfg, const char* prefix, GenericList& targetList)
{
	return true;
}

// Execute node, targetting the supplied Configuration
ProcedureNode::NodeExecutionResult ParametersProcedureNode::execute(ProcessPool& procPool, Configuration* cfg, const char* prefix, GenericList& targetList)
{
	return ProcedureNode::Success;
}

/*
 * Read / Write
 */

// Read structure from specified LineParser
bool ParametersProcedureNode::read(LineParser& parser, const CoreData& coreData, NodeScopeStack& scopeStack)
{
	ExpressionVariable* parameter;

	// Read until we encounter the EndParameters keyword, or we fail for some reason
	while (!parser.eofOrBlank())
	{
		// Read and parse the next line
		if (parser.getArgsDelim() != LineParser::Success) return false;

		// Do we recognise this keyword and, if so, do we have the appropriate number of arguments?
		if (!parametersNodeKeywords().isValid(parser.argc(0))) return parametersNodeKeywords().errorAndPrintValid(parser.argc(0));
		ParametersNodeKeyword nk = parametersNodeKeywords().enumeration(parser.argc(0));
		if (!parametersNodeKeywords().validNArgs(nk, parser.nArgs()-1)) return false;

		// All OK, so process it
		switch (nk)
		{
			case (ParametersProcedureNode::DoubleKeyword):
				parameter = scopeStack.addParameter(parser.argc(1), parser.argd(2));
				if (!parameter) return false;

				// Add the parameter to our own RefList so we can write out its info
				parameterReferences_.append(parameter);
				break;
			case (ParametersProcedureNode::IntegerKeyword):
				parameter = scopeStack.addParameter(parser.argc(1), parser.argi(2));
				if (!parameter) return false;

				// Add the parameter to our own RefList so we can write out its info
				parameterReferences_.append(parameter);
				break;
			case (ParametersProcedureNode::EndParametersKeyword):
				return true;
				break;
			default:
				return Messenger::error("Epic Developer Fail - Don't know how to deal with the Parameters node keyword '%s'.\n", parser.argc(0));
		}
	}

	return true;
}

// Write structure to specified LineParser
bool ParametersProcedureNode::write(LineParser& parser, const char* prefix)
{
	// Block Start
	if (!parser.writeLineF("%s%s\n", ProcedureNode::nodeTypes().keyword(type_))) return false;

	// Parameters
	RefListIterator<ExpressionVariable> parameterIterator(parameterReferences_);
	while (ExpressionVariable* parameter = parameterIterator.iterate())
	{
		if (parameter->value().isInteger())
		{
			if (!parser.writeLineF("%s  %s  '%s'  %i\n", prefix, parametersNodeKeywords().keyword(ParametersProcedureNode::IntegerKeyword), parameter->name(), parameter->value().asInteger())) return false;
		}
		else
		{
			if (!parser.writeLineF("%s  %s  '%s'  %12.6e\n", prefix, parametersNodeKeywords().keyword(ParametersProcedureNode::DoubleKeyword), parameter->name(), parameter->value().asDouble())) return false;
		}
	}

	// Block End
	if (!parser.writeLineF("%s%s\n", parametersNodeKeywords().keyword(ParametersProcedureNode::EndParametersKeyword))) return false;

	return true;
}
