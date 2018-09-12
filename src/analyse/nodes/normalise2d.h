/*
	*** Analysis Node - Normalise2D
	*** src/analyse/nodes/normalise2d.h
	Copyright T. Youngs 2012-2018

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

#ifndef DISSOLVE_ANALYSISNORMALISE2D_H
#define DISSOLVE_ANALYSISNORMALISE2D_H

#include "analyse/nodes/node.h"
#include "math/data2d.h"
#include "base/charstring.h"
#include "templates/reflist.h"

// Forward Declarations
class AnalysisCollect2DNode;
class AnalysisSelectNode;
class Data2D;
class LineParser;
class NodeContextStack;

// Analysis Node - Normalise2D
class AnalysisNormalise2DNode : public AnalysisNode
{
	public:
	// Constructor
	AnalysisNormalise2DNode(AnalysisCollect2DNode* target = NULL);
	// Destructor
	~AnalysisNormalise2DNode();


	/*
	 * Node Keywords
	 */
	public:
	// Node Keywords
	enum Normalise2DNodeKeyword { EndNormalise2DKeyword, FactorKeyword, LabelValueKeyword, LabelXKeyword, LabelYKeyword, NSitesKeyword, NumberDensityKeyword, SaveKeyword, nNormalise2DNodeKeywords };
	// Convert string to control keyword
	static Normalise2DNodeKeyword normalise2DNodeKeyword(const char* s);
	// Convert control keyword to string
	static const char* normalise2DNodeKeyword(Normalise2DNodeKeyword nk);


	/*
	 * Data
	 */
	private:
	// Collect2D node which we are normalising
	AnalysisCollect2DNode* collectNode_;
	// Reference to sites against which we will normalise by population
	RefList<AnalysisSelectNode,double> sitePopulationNormalisers_;
	// Reference to sites against which we will normalise by number density
	RefList<AnalysisSelectNode,double> numberDensityNormalisers_;
	// Whether to normalise by supplied factor
	bool normaliseByFactor_;
	// Normalisation factor to apply (if requested)
	double normalisationFactor_;
	// Whether to save data after normalisation
	bool saveNormalisedData_;
	// Value label
	CharString valueLabel_;
	// Axis labels
	CharString xAxisLabel_, yAxisLabel_;

	public:
	// Add site population normaliser
	void addSitePopulationNormaliser(AnalysisSelectNode* selectNode);
	// Add number density normaliser
	void addNumberDensityNormaliser(AnalysisSelectNode* selectNode);
	// Set whether to normalise by factor
	void setNormaliseByFactor(bool on);
	// Set normalisation factor
	void setNormalisationFactor(double factor);
	// Set whether to save normalised data
	void setSaveNormalisedData(bool on);
	// Set value label
	void setValueLabel(const char* label);
	// Return value label
	const char* valueLabel() const;
	// Set x axis label
	void setXAxisLabel(const char* label);
	// Return x axis label
	const char* xAxisLabel() const;
	// Set y axis label
	void setYAxisLabel(const char* label);
	// Return y axis label
	const char* yAxisLabel() const;


	/*
	 * Execute
	 */
	public:
	// Prepare any necessary data, ready for execution
	bool prepare(Configuration* cfg, const char* prefix, GenericList& targetList);
	// Execute node, targetting the supplied Configuration
	AnalysisNode::NodeExecutionResult execute(ProcessPool& procPool, Configuration* cfg, const char* prefix, GenericList& targetList);
	// Finalise any necessary data after execution
	bool finalise(ProcessPool& procPool, Configuration* cfg, const char* prefix, GenericList& targetList);


	/*
	 * Read / Write
	 */
	public:
	// Read structure from specified LineParser
	bool read(LineParser& parser, NodeContextStack& contextStack);
	// Write structure to specified LineParser
	bool write(LineParser& parser, const char* prefix);
};

#endif