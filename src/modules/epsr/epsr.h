/*
	*** EPSR Module
	*** src/modules/epsr/epsr.h
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

#ifndef DISSOLVE_EPSRMODULE_H
#define DISSOLVE_EPSRMODULE_H

#include "module/module.h"
#include "module/group.h"
#include "classes/xydatastore.h"
#include "math/xydata.h"
#include "templates/array3d.h"

// Forward Declarations
class AtomType;
class PartialSet;

// EPSR Module
class EPSRModule : public Module
{
	/*
	 * Generates interatomic potentials using methodology mimicking EPSR as closely as possible.
	 */

	public:
	// Constructor
	EPSRModule();
	// Destructor
	~EPSRModule();


	/*
	 * Instances
	 */
	protected:
	// List of all instances of this Module type
	static List<Module> instances_;

	public:
	// Return list of all created instances of this Module
	List<Module>& instances();
	// Create instance of this module
	Module* createInstance();


	/*
	 * Definition
	 */
	public:
	// Return name of module
	const char* name();
	// Return brief description of module
	const char* brief();
	// Return instance type for module
	InstanceType instanceType();
	// Return the maximum number of Configurations the Module can target (or -1 for any number)
	int nTargetableConfigurations();
	// Modules upon which this Module depends to have run first
	const char* dependentModules();
	// Set up supplied dependent module (only if it has been auto-added)
	bool setUpDependentModule(Module* depMod);


	/*
	 * Options
	 */
	public:
	// Expansion Function Type Enum
	enum ExpansionFunctionType
	{
		PoissonExpansionFunction,		/* Fit difference functions using Poisson (power exponential) functions */
		GaussianExpansionFunction,		/* Fit difference functiuns using Gaussian functions */
		nExpansionFunctionTypes
	};
	// Convert text string to ExpansionFunctionType
	static ExpansionFunctionType expansionFunctionType(const char* s);
	// Convert ExpansionFunctionType to text string
	static const char* expansionFunctionType(ExpansionFunctionType eft);

	protected:
	// Set up options for Module
	void setUpKeywords();
	// Parse complex keyword line, returning true (1) on success, false (0) for recognised but failed, and -1 for not recognised
	int parseComplexKeyword(ModuleKeywordBase* keyword, LineParser& parser, Dissolve* dissolve, GenericList& targetList, const char* prefix);


	/*
	 * Processing
	 */
	private:
	// Run main processing
	bool process(Dissolve& dissolve, ProcessPool& procPool);

	public:
	// Run set-up stage
	bool setUp(Dissolve& dissolve, ProcessPool& procPool);
	// Whether the Module has a processing stage
	bool hasProcessing();


	/*
	 * Functions
	 */
	private:
	// Test datasets (if any)
	XYDataStore testData_;
	// List of target Modules to be targetted in refinement process
	RefList<Module,bool> targets_;
	// Target groups
	List<ModuleGroup> targetGroups_;
	// Simulated data added as reference data
	Array<XYData> simulatedReferenceData_;

	private:
	// Add Module target to specified group
	bool addTarget(const char* moduleTarget, const char* group);

	public:
	// Return list of target Modules / data for fitting process
	const RefList<Module,bool>& targets() const;
	// Return list of target groups defined
	const List<ModuleGroup>& targetGroups() const;
	// Create / retrieve arrays for storage of empirical potential coefficients
	Array2D< Array<double> >& potentialCoefficients(Dissolve& dissolve, const int nAtomTypes, const int ncoeffp = -1);
	// Generate empirical potentials from current coefficients
	bool generateEmpiricalPotentials(Dissolve& dissolve, EPSRModule::ExpansionFunctionType functionType, int ncoeffp, double rminpt, double rmaxpt, double sigma1, double sigma2);
	// Calculate absolute energy of empirical potentials
	double absEnergyEP(Dissolve& dissolve);
	// Truncate the supplied data
	void truncate(XYData& data, double rMin, double rMax);


	/*
	 * EPSR File I/O
	 */
	public:
	// PCof File Keywords
	enum EPSRPCofKeyword { 
		AddPotTypePCofKeyword,		/* addpottype - Additional potential type: Gaussian or modmorse. [Gaussian] */
		ExpecFPCofKeyword,		/* expecf - Additional potential type: Gaussian or modmorse. [Gaussian] */
		GaussianPCofKeyword,		/* gaussian - Select T for Gaussian representation of EP. Otherwise Poisson. [F] */
		NCoeffPPCofKeyword,		/* ncoeffp - Number of coefficients used to define the EP. */
		NPItSSPCofKeyword,		/* npitss - Number of steps for refining the potential. */
		PAcceptPCofKeyword,		/* paccept - Acceptance factor for potential refinement. [0.0005] */
		PDMaxPCofKeyword,		/* pdmax - Maximum distance of Empirical Potential. */
		PDStepPCofKeyword,		/* pdstep - Spacing between coefficients in r. */
		PowerPCofKeyword,		/* power - Repulsive power in Lennard-Jones potential. [12] */
		PSigma2PCofKeyword,		/* psigma2 - Width for empirical potential functions. [0.01] */
		QuitPCofKeyword,		/* q - Signals the end of the pcof keyword section */
		RBroadPCofKeyword,		/* rbroad - Controls potential decay. [0.0] */
		RChargePCofKeyword,		/* rcharge - Calculates energy due to molecular polarisation. [0.0] */
		RefPotFacPCofKeyword,		/* refpotfac - Factor to apply to reference potential. [1.0] */
		RepPotTypePCofKeyword,		/* reppottype - Repulsive potential type: exponential or harmonic. [exponential] */
		RMaxPtPCofKeyword,		/* rmaxpt - Radius at which potential truncation goes to 0.0. */
		RMinFacPCofKeyword,		/* rminfac - Factor to set the minimum separation between pairs. [0.0] */
		RMinPtPCofKeyword,		/* rminpt - Radius at which potential truncation begins. */
		ROverlapPCofKeyword,		/* roverlap - Minimum allowed intermolecular separation between two atoms. */
		nEPSRPCofKeywords
	};
	// Convert text string to EPSRPCofKeyword
	static EPSRPCofKeyword epsrPCofKeyword(const char* s);
	// Convert EPSRPCofKeyword to text string
	static const char* epsrPCofKeyword(EPSRPCofKeyword pcofkwd);

	public:
	// Read data from supplied pcof file
	bool readPCof(Dissolve& dissolve, ProcessPool& procPool, const char* filename);


	/*
	 * GUI Widget
	 */
	public:
	// Return a new widget controlling this Module
	ModuleWidget* createWidget(QWidget* parent, Dissolve& dissolve);
};

#endif