/*
	*** Dissolve Main Structure
	*** src/main/dissolve.h
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

#ifndef DISSOLVE_DISSOLVE_H
#define DISSOLVE_DISSOLVE_H

#include "data/elements.h"
#include "module/module.h"
#include "module/layer.h"
#include "classes/configuration.h"
#include "classes/coredata.h"
#include "classes/pairpotential.h"
#include "classes/potentialmap.h"

// Forward Declarations
class Atom;
class Box;
class Cell;
class Isotopologue;
class Molecule;
class ChangeStore;

// Dissolve Main Class
class Dissolve
{
	public:
	// Constructor
	Dissolve(CoreData& coreData);
	// Destructor
	~Dissolve();


	/*
	 * Core
	 */
	private:
	// Reference to CoreData 
	CoreData& coreData_;

	public:
	// Return reference to CoreData
	CoreData& coreData();
	// Return const reference to CoreData
	const CoreData& constCoreData() const;
	// Clear all data
	void clear();
	// Register GenericItems
	void registerGenericItems();


	/*
	 * Atom Types
	 * (Exposes List<AtomType> in coreData_)
	 */
	public:
	// Add AtomType with specified Element
	AtomType* addAtomType(Element* el);
	// Return number of AtomTypes in list
	int nAtomTypes() const;
	// Return AtomTypes list
	List<AtomType>& atomTypes();
	// Return nth AtomType in list
	AtomType* atomType(int n);
	// Search for AtomType by name
	AtomType* findAtomType(const char* name) const;
	// Clear all AtomTypes
	void clearAtomTypes();


	/*
	 * Master Terms
	 * (Exposes lists in coreData_)
	 */
	public:
	// Return list of master Bond parameters
	const List<MasterIntra>& masterBonds() const;
	// Return list of master Angle parameters
	const List<MasterIntra>& masterAngles() const;
	// Return list of master Torsion parameters
	const List<MasterIntra>& masterTorsions() const;
	// Check and print MasterTerm usage
	void checkMasterTermUsage() const;


	/*
	 * Species Definitions
	 * (Exposes List<Species> in coreData_)
	 */
	public:
	// Add a new Species to the list
	Species* addSpecies();
	// Remove the specified Species from the list
	void removeSpecies(Species* sp);
	// Return number of defined Species
	int nSpecies() const;
	// Return Species list
	List<Species>& species();
	// Return nth Species in the list
	Species* species(int n);
	// Search for Species by name
	Species* findSpecies(const char* name) const;
	// Copy AtomType, creating a new one if necessary
	void copyAtomType(const SpeciesAtom* sourceAtom, SpeciesAtom* destAtom);
	// Copy intramolecular interaction parameters, adding MasterIntra if necessary
	void copySpeciesIntra(const SpeciesIntra* sourceIntra, SpeciesIntra* destIntra);
	// Copy Species
	Species* copySpecies(const Species* species);


	/*
	 * Pair Potentials
	 */
	private:
	// Maximum distance for tabulated PairPotentials
	double pairPotentialRange_;
	// Maximum squared distance for tabulated PairPotentials
	double pairPotentialRangeSquared_;
	// Delta to use in tabulation
	double pairPotentialDelta_;
	// Whether Coulomb terms are included in generated PairPotentials
	bool pairPotentialsIncludeCoulomb_;
	// Simulation PairPotentials
	List<PairPotential> pairPotentials_;
	// Version of AtomTypes at which PairPotential were last generated
	int pairPotentialAtomTypeVersion_;
	// Map for PairPotentials
	PotentialMap potentialMap_;

	public:
	// Set maximum distance for tabulated PairPotentials
	void setPairPotentialRange(double range);
	// Return maximum distance for tabulated PairPotentials
	double pairPotentialRange() const;
	// Return maximum squared distance for tabulated PairPotentials
	double pairPotentialRangeSquared() const;
	// Set delta to use in tabulations
	void setPairPotentialDelta(double delta);
	// Return delta to use in tabulations
	double pairPotentialDelta() const;
	// Set whether Coulomb term should be included in generated PairPotentials
	void setPairPotentialsIncludeCoulomb(bool b);
	// Return whether Coulomb term should be included in generated PairPotentials
	bool pairPotentialsIncludeCoulomb();
	// Return index of specified PairPotential
	int indexOf(PairPotential* pp);
	// Return number of defined PairPotentials
	int nPairPotentials() const;
	// Add new pair potential to list
	PairPotential* addPairPotential(AtomType* at1, AtomType* at2);
	// Return PairPotentials list
	const List<PairPotential>& pairPotentials() const;
	// Return nth PairPotential in list
	PairPotential* pairPotential(int n);
	// Return whether specified PairPotential is defined
	PairPotential* pairPotential(AtomType* at1, AtomType* at2) const;
	// Return whether specified PairPotential is defined
	PairPotential* pairPotential(const char* at1, const char* at2) const;
	// Return map for PairPotentials
	const PotentialMap& potentialMap();
	// Clear and regenerate all PairPotentials, replacing those currently defined
	void regeneratePairPotentials();
	// Generate all necessary PairPotentials, adding missing terms where necessary
	bool generatePairPotentials(AtomType* onlyInvolving = NULL);


	/*
	 * Configurations
	 * (Exposes List<Configuration> in coreData_)
	 */
	public:
	// Add new Configuration
	Configuration* addConfiguration();
	// Own the specified Configuration
	bool ownConfiguration(Configuration* cfg);
	// Remove specified Configuration
	void removeConfiguration(Configuration* cfg);
	// Return number of defined Configurations
	int nConfigurations() const;
	// Return Configuration list
	List<Configuration>& configurations();
	// Return Configuration list (const)
	const List<Configuration>& constConfigurations() const;
	// Find configuration by name
	Configuration* findConfiguration(const char* name) const;
	// Find configuration by 'nice' name
	Configuration* findConfigurationByNiceName(const char* name) const;


	/*
	 * Modules
	 */
	private:
	// List of all instances of all used Modules
	RefList<Module> moduleInstances_;
	// List of master Module instances
	List<Module> masterModules_;

	private:
	// Register master Module
	bool registerMasterModule(Module* masterInstance);

	public:
	// Register master instances for all Modules
	bool registerMasterModules();
	// Return master Module instances
	const List<Module>& masterModules() const;
	// Search for master Module of the named type
	Module* findMasterModule(const char* moduleType) const;
	// Create a Module instance for the named Module type
	Module* createModuleInstance(const char* moduleType);
	// Create a Module instance for the named Module type, and add it to the specified layer
	Module* createModuleInstance(const char* moduleType, ModuleLayer* destinationLayer, bool configurationLocal = false);
	// Search for any instance of any Module with the specified unique name
	Module* findModuleInstance(const char* uniqueName);
	// Search for any instance of any Module with the specified Module type
	RefList<Module> findModuleInstances(const char* moduleType);
	// Generate unique Module name with base name provided
	const char* uniqueModuleName(const char* name, Module* excludeThis = NULL);
	// Delete specified Module instance
	bool deleteModuleInstance(Module* instance);


	/*
	 * Layers
	 */
	private:
	// List of defined processing layers
	List<ModuleLayer> processingLayers_;
	// Data associated with processing Modules
	GenericList processingModuleData_;

	public:
	// Add new processing layer
	ModuleLayer* addProcessingLayer();
	// Remove specified processing layer
	void removeProcessingLayer(ModuleLayer* layer);
	// Find named processing layer
	ModuleLayer* findProcessingLayer(const char* name) const;
	// Own the specified processing layer
	bool ownProcessingLayer(ModuleLayer* layer);
	// Return number of defined processing layers
	int nProcessingLayers() const;
	// Generate unique processing layer name, with base name provided
	const char* uniqueProcessingLayerName(const char* baseName) const;
	// Return list of processing layers
	List<ModuleLayer>& processingLayers();
	// Return data associated with main processing Modules
	GenericList& processingModuleData();
	// Create and add a named Module to the named layer (creating it if necessary), with optional Configuration target
	Module* createModuleInLayer(const char* moduleType, const char* layerName, Configuration* cfg = NULL);


	/*
	 * Simulation
	 */
	private:
	// Random seed
	int seed_;
	// Frequency at which to write restart file
	int restartFileFrequency_;
	// Current simulation step
	int iteration_;
	// Number of iterations performed
	int nIterationsPerformed_;
	// Main loop timer
	Timer iterationTimer_;
	// Accumulated timing information for main loop iterations
	SampledDouble iterationTime_;

	public:
	// Set number of test points to use when calculating Box normalisation arrays
	void setNBoxNormalisationPoints(int nPoints);
	// Return number of test points to use when calculating Box normalisation arrays
	int nBoxNormalisationPoints() const;
	// Set random seed
	void setSeed(int seed);
	// Return random seed
	int seed() const;
	// Set frequency with which to write various iteration data	
	void setRestartFileFrequency(int n);
	// Return frequency with which to write restart file
	int restartFileFrequency() const;
	// Prepare for main simulation
	bool prepare();
	// Iterate main simulation
	bool iterate(int nIterations = -1);
	// Reset current simulation step
	void resetIterationCounter();
	// Return current simulation step
	int iteration() const;
	// Return per-iteration time in seconds
	double iterationTime() const;
	// Print timing information
	void printTiming();


	/*
	 * I/O
	 */
	private:  
	// Filename of current input file
	CharString inputFilename_;
	// Filename of current restart file
	CharString restartFilename_;
	// Accumulated timing information for saving restart file
	SampledDouble saveRestartTimes_;
	// Check if heartbeat file needs to be written or not
	bool writeHeartBeat_;

	private:
	// Load input file through supplied parser
	bool loadInput(LineParser& parser);
	
	public:
	// Load input file
	bool loadInput(const char* filename);
	// Load input from supplied string
	bool loadInputFromString(const char* inputString);
	// Save input file
	bool saveInput(const char* filename);
	// Load restart file
	bool loadRestart(const char* filename);
	// Load restart file as reference point
	bool loadRestartAsReference(const char* filename, const char* dataSuffix);
	// Save restart file
	bool saveRestart(const char* filename);
	// Save heartbeat file
	bool saveHeartBeat(const char* filename, double estimatedNSecs);
	// Set bool for heartbeat file to be written
	void setWriteHeartBeat(bool b);
	// write heartbeat file
	bool writeHeartBeat() const;
	// Return whether an input filename has been set
	bool hasInputFilename() const;
	// Set current input filenamea
	void setInputFilename(const char* filename);
	// Return current input filename
	const char* inputFilename() const;
	// Set restart filename
	void setRestartFilename(const char* filename);
	// Return restart filename
	const char* restartFilename() const;
	// Return whether a restart filename has been set
	bool hasRestartFilename() const;


	/*
	 * Object Management
	 */
	public:
	// Remove all references to the specified Configuration
	void removeReferencesTo(Configuration* cfg);
	// Remove all references to the specified Module
	void removeReferencesTo(Module* module);
	// Remove all references to the specified Species
	void removeReferencesTo(Species* sp);
	// Remove all references to the specified SpeciesSite
	void removeReferencesTo(SpeciesSite* site);


	/*
	 * Parallel Comms
	 */
	public:
	// Parallel Strategy
	enum ParallelStrategy { SequentialConfigStrategy, EvenStrategy, nParallelStrategies };
	// Convert string to ParallelStrategy
	static ParallelStrategy parallelStrategy(const char* s);

	private:
	// Parallel strategy for Configuration work
	ParallelStrategy parallelStrategy_;
	// Default process group population (per Configuration)
	int parallelGroupPopulation_;

	public:
	// Set parallel strategy for Configuration work
	void setParallelStrategy(ParallelStrategy ps);
	// Return parallel strategy for Configuration work
	ParallelStrategy parallelStrategy() const;
	// Set default process group population (per Configuration)
	void setParallelGroupPopulation(int groupPopulation);
	// Return default process group population (per Configuration)
	int parallelGroupPopulation() const;
	// Return world process pool
	ProcessPool& worldPool();
	// Set up local MPI pools
	bool setUpMPIPools();
};

#endif
