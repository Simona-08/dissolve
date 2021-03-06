/*
	*** Regional Distributor
	*** src/classes/regionaldistributor.cpp
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

#include "classes/regionaldistributor.h"
#include "classes/atom.h"
#include "classes/cell.h"
#include "classes/cellneighbour.h"
#include "classes/molecule.h"
#include "base/processpool.h"
#include "base/lineparser.h"
#include "templates/array3d.h"

// Debug Mode
const bool DND = false;

// Molecule Status Flag Keywords
const char* MoleculeStatusFlagKeywords[] = { "Waiting", "Distributed" };

// Return string for specified MoleculeStatusFlag
const char* RegionalDistributor::moleculeStatusFlag(RegionalDistributor::MoleculeStatusFlag flag)
{
	return MoleculeStatusFlagKeywords[flag];
}

// Cell Status Flag Keywords
const char* CellStatusFlagKeywords[] = { "Unused", "LockedForEditing", "ReadByOne", "ReadByMany" };

// Return string for specified CellStatusFlag
const char* RegionalDistributor::cellStatusFlag(RegionalDistributor::CellStatusFlag flag)
{
	return CellStatusFlagKeywords[flag];
}

// Constructor
RegionalDistributor::RegionalDistributor(const DynamicArray<Molecule>& moleculeArray, const CellArray& cellArray, ProcessPool& procPool, ProcessPool::DivisionStrategy strategy) :  processPool_(procPool), originalStrategy_(strategy), cellArray_(cellArray), moleculeArray_(moleculeArray)
{
	// Core
	currentStrategy_ = originalStrategy_;
	setProcessOrGroupLimits(currentStrategy_);
	nCycles_ = 0;

	// Cells
	lockedCells_ = new OrderedPointerList<Cell>[nProcessesOrGroups_];
	cellStatusFlags_.initialise(cellArray.nCells());
	cellStatusFlags_ = RegionalDistributor::UnusedFlag;
	cellLockOwners_.initialise(cellArray.nCells());
	cellLockOwners_ = -1;

	// Molecules
	assignedMolecules_ = new Array<int>[nProcessesOrGroups_];
	moleculeStatus_.initialise(moleculeArray_.nItems());
	moleculeStatus_ = RegionalDistributor::WaitingFlag;
	nMoleculesToDistribute_ = moleculeArray_.nItems();
	nMoleculesDistributed_ = 0;
}

// Destructor
RegionalDistributor::~RegionalDistributor()
{
	delete[] assignedMolecules_;
	delete[] lockedCells_;
}

/*
 * Core
 */

// Set process/group limits based on supplied strategy
bool RegionalDistributor::setProcessOrGroupLimits(ProcessPool::DivisionStrategy strategy)
{
	nProcessesOrGroups_ = processPool_.strategyNDivisions(strategy);
	processOrGroupIndex_ = processPool_.strategyProcessIndex(strategy);

	return false;
}

// Set up next distribution of Molecules amongst processes/groups, returning false if there are no more Molecules to distribute
bool RegionalDistributor::cycle()
{
	/*
	 * All processes should call here together.
	 * We start by selecting a starting Cell index for the first process/group. Then, we choose a Molecule (pertly) present in the Cell which needs to be
	 * calculated. If no such Molecule is available, we choose a new Cell (increase the Cell index) and try again.
	 * Once a Molecule has been located, we attempt to assign all Cells which the Molecule's atoms exist in, and all of the immediate adjacent neighbours,
	 * to that process/group. If any other process/group has already marked that Cell as 'Locked', or a Cell has been marked read-only by a process/group
	 * other than us, then we cannot modify the Molecule (since doing so would potentially lead to inconsistencies in the calculation of other Molecules).
	 * Once a Molecule has been successfully locked, we move on to the next process/group and repeat the procedure.
	 * When we return to the first process, we use the already-locked list of Cells as a source of potential Molecule candidates.
	 */

	// Initial check - if all target Molecules have been distributed, we can return the AllComplete flag
	if (nMoleculesDistributed_ == nMoleculesToDistribute_)
	{
		Messenger::printVerbose("All target Molecules distributed.\n");

		return false;
	}

	Molecule* molecule;
	Array<bool> allPossibleMoleculesAssigned(nProcessesOrGroups_);
	allPossibleMoleculesAssigned = false;
	int processOrGroup, nMoleculesAssigned, allPossibleMoleculesAssignedCount = 0;

	// Set Molecule completed flags and clear distribution arrays
	for (processOrGroup=0; processOrGroup<nProcessesOrGroups_; ++processOrGroup)
	{
		assignedMolecules_[processOrGroup].clear();
		lockedCells_[processOrGroup].clear();
	}

	// Reset the Cell status flags
	cellStatusFlags_ = RegionalDistributor::UnusedFlag;
	cellLockOwners_ = -1;

	// Set the process/group numbers for the original parallel strategy before we try to assign Molecules to groups
	// In this way we will always allow the parallel strategy to go 'back up' to the original one specified, if we can.
	setProcessOrGroupLimits(originalStrategy_);

	// Reset counter of number of Molecules assigned this iteration - we will use it to sanity check the loop below.
	nMoleculesAssigned = 0;

	// If there is only one process/group, put all Molecules in it. Otherwise loop over target groups/processes, assigning molecules sequentially to each
	if (nProcessesOrGroups_ == 1)
	{
		for (int n=0; n<nMoleculesToDistribute_; ++n)
		{
			if (moleculeStatus_[n] == RegionalDistributor::WaitingFlag)
			{
				assignedMolecules_[0].add(n);
				++nMoleculesDistributed_;
			}
		}
	}
	else while (allPossibleMoleculesAssignedCount < nProcessesOrGroups_)
	{
		for (processOrGroup=0; processOrGroup<nProcessesOrGroups_; ++processOrGroup)
		{
			if (DND) Messenger::print("\n ** Searching for suitable Molecule to assign to process/group %i...\n\n", processOrGroup);

			// If we have already assigned all possible Molecules for this process/group, continue the loop
			if (allPossibleMoleculesAssigned[processOrGroup]) continue;

			// Try to assign a Molecule to this process/group
			molecule = assignMolecule(processOrGroup);
			if (!molecule)
			{
				allPossibleMoleculesAssigned[processOrGroup] = true;
				++allPossibleMoleculesAssignedCount;

				if (DND) Messenger::print("Failed to find a suitable Molecule for process/group %i\n", processOrGroup);
			}
			else
			{
				// Valid Molecule found, so add it to our distribution array and mark it as such
				assignedMolecules_[processOrGroup].add(molecule->arrayIndex());
				moleculeStatus_[molecule->arrayIndex()] = RegionalDistributor::DistributedFlag;
				++nMoleculesDistributed_;
				++nMoleculesAssigned;

				if (DND) Messenger::print("Molecule %i assigned to process/group %i - nMoleculesDistributed is now %i. Process/group has %i locked Cells in total.\n", molecule->arrayIndex(), processOrGroup, nMoleculesDistributed_, lockedCells_[processOrGroup].nItems());
			}

			// Have all possible Molecules been assigned?
			if (allPossibleMoleculesAssignedCount == nProcessesOrGroups_) break;
		}

		/*
		 * We have assigned all possible Molecules, so let's sanity check exactly how we have divided them up.
		 * If only the first process/group has any Molecules assigned to it, we will revert to PoolStrategy and send the only
		 * populated Molecule list to all processes.
		 */
		if (nMoleculesAssigned == 1)
		{
			// Assign all remaining Molecules to group 0, and copy this group for distribution to all others.
			assignedMolecules_[0].clear();
			for (int n=0; n<nMoleculesToDistribute_; ++n)
			{
				if (moleculeStatus_[n] == RegionalDistributor::WaitingFlag)
				{
					assignedMolecules_[0].add(n);
					++nMoleculesDistributed_;
				}
			}
			for (processOrGroup=1; processOrGroup<nProcessesOrGroups_; ++processOrGroup) assignedMolecules_[processOrGroup] = assignedMolecules_[0];

			// Revert to PoolStrategy
			currentStrategy_ = ProcessPool::PoolStrategy;
			setProcessOrGroupLimits(currentStrategy_);

			Messenger::printVerbose("Distributor has reverted to PoolStrategy. Target Molecules will be the same for all processes.\n");

			break;
		}
	}

	++nCycles_;

	// Summarise
	for (processOrGroup=0; processOrGroup<nProcessesOrGroups_; ++processOrGroup)
	{
		Messenger::printVerbose("Distributor cycle %i : Process/Group %i has %i Molecules asigned to it over %i locked Cells.\n", nCycles_, processOrGroup, assignedMolecules_[processOrGroup].nItems(), lockedCells_[processOrGroup].nItems());
	}

// 	XXX Check for balanced load (molecules/atoms)
	return true;
}

// Return current parallel strategy, based on last cycle
ProcessPool::DivisionStrategy RegionalDistributor::currentStrategy()
{
	return currentStrategy_;
}

/*
 * Cells
 */

// Return whether the specified processOrGroup can lock the given Cell index
bool RegionalDistributor::canLockCellForEditing(int processOrGroup, int cellIndex)
{
	CellStatusFlag status = cellStatusFlags_.constAt(cellIndex);

	if (DND) Messenger::print("  0-- Checking ability to lock Cell index %i for process/group %i: current status = %s\n", cellIndex, processOrGroup, cellStatusFlag(cellStatusFlags_.constAt(cellIndex)));

	// If the Cell is flagged as unused, return true
	if (status == RegionalDistributor::UnusedFlag) return true;

	// If the Cell is flagged as 'LockedForEditing', and not by this processOrGroup, return false. If we have locked it, return true.
	if (status == RegionalDistributor::LockedForEditingFlag) return (cellLockOwners_.constAt(cellIndex) == processOrGroup);

	// If the Cell is flagged as 'ReadByOne', but not by this processOrGroup, return false (if we are the sole reader, we can lock it)
	if (status == RegionalDistributor::ReadByOneFlag) return (cellLockOwners_.constAt(cellIndex) == processOrGroup);

	// If the Cell is flagged as 'ReadByMany', there is no chance of locking it, so return false.
	if (status == RegionalDistributor::ReadByManyFlag) return false;

	// That's all four possibilities, so raise an error if we get here (we never should)
	Messenger::error("Failed to determine lock possibility correctly.\n");

	return false;
}

/*
 * Molecules
 */

// Assign Molecule to process/group if possible
bool RegionalDistributor::assignMolecule(const Molecule* mol, int processOrGroup)
{
	Cell* primaryCell, *readOnlyCell;
	int cellIndex;

	// Obvious check first - is the Molecule available for distribution / assignment?
	const int molId = mol->arrayIndex();

	if (DND) Messenger::print("  -- Checking Molecule %i for process/group %i: status = %s\n", molId, processOrGroup, moleculeStatusFlag(moleculeStatus_[molId]));

	if (moleculeStatus_[molId] != RegionalDistributor::WaitingFlag) return false;

	// Go through the Atoms of the Molecule, assembling a list of primary Cells in which its Atoms are found.
	Array<Cell*> primaryCells;
	for (int i=0; i<mol->nAtoms(); ++i)
	{
		// Get Cell pointer and index
		primaryCell = mol->atom(i)->cell();
		cellIndex = primaryCell->index();

		// Make sure we can lock this Cell for editing, unless we have locked it already...
		if ((cellLockOwners_[cellIndex] == processOrGroup) && (cellStatusFlags_[cellIndex] == RegionalDistributor::LockedForEditingFlag))
		{
			if (DND) Messenger::print(" -- Cell %i is already locked by us (%i)\n", cellIndex, cellLockOwners_[cellIndex]);
			continue;
		}
		if (!canLockCellForEditing(processOrGroup, cellIndex))
		{
			if (DND) Messenger::print(" -- Cell %i cannot be locked for editing - current owner (%i) and/or status (%s) forbit it\n", cellIndex, cellLockOwners_[cellIndex], cellStatusFlag(cellStatusFlags_[cellIndex]));
			return false;
		}

		if (DND) Messenger::print(" -- Cell %i can be locked - current owner (%i)\n", cellIndex, cellLockOwners_[cellIndex]);

		// Add to the primary Cells list
		primaryCells.add(primaryCell);
	}

	// We are able to lock all Cells that we need to edit, so now construct a list of those within the cutoff range of any primaryCell that we must be able to read (but not modify)
	OrderedPointerList<Cell> readOnlyCells;
	for (int c=0; c<primaryCells.nItems(); ++c)
	{
		// Loop over all cell neighbours for this primary Cell
		CellNeighbour* neighbours = primaryCells[c]->allCellNeighbours();
		for (int n=0; n<primaryCells[c]->nTotalCellNeighbours(); ++n)
		{
			readOnlyCell = neighbours[n].cell();
			cellIndex = readOnlyCell->index();

			// If we have locked this Cell already, continue
			if (cellStatusFlags_[cellIndex] == RegionalDistributor::LockedForEditingFlag)
			{
				if (cellLockOwners_[cellIndex] == processOrGroup) continue;
				else
				{
					if (DND) Messenger::print(" -- Can't add Cell %i as a read-only Cell, since process/group %i has locked it for editing.\n", cellIndex, cellLockOwners_[cellIndex]);
					return false;
				}
			}

			// All good - add to our list
			readOnlyCells.addExclusive(readOnlyCell);
		}
	}

	// If we reach this point, we can lock all the necessary Cells for editing, and mark all those necessary for reading.

	// Add primary and secondary lock Cells to our list, sanity checking along the way
	for (int c=0; c<primaryCells.nItems(); ++c)
	{
		cellIndex = primaryCells[c]->index();

		// Set lock index
		if ((cellLockOwners_[cellIndex] == processOrGroup) || (cellLockOwners_[cellIndex] == -1))
		{
			lockedCells_[processOrGroup].addExclusive(primaryCell);
			cellLockOwners_[cellIndex] = processOrGroup;
			cellStatusFlags_[cellIndex] = RegionalDistributor::LockedForEditingFlag;
		}
		else return Messenger::error("Tried to lock a (primary) Cell which is already locked by someone else.\n");
	}

	// For the read-only Cells, we just need to set relevant ownership in the cellLockOwners_ array
	OrderedPointerListIterator<Cell> readOnlyCellIterator(readOnlyCells);
	while ((readOnlyCell = readOnlyCellIterator.iterate()))
	{
		cellIndex = readOnlyCell->index();

		// Check status
		if (cellStatusFlags_[cellIndex] == RegionalDistributor::LockedForEditingFlag)
		{
			if (cellLockOwners_[cellIndex] == processOrGroup) continue;
			else return Messenger::error("Tried to mark a Cell for reading that is locked.\n");
		}
		else if (cellStatusFlags_[cellIndex] == RegionalDistributor::UnusedFlag)
		{
			// Not currently used, so mark it as being read by one process/group (us) and set its new status
			cellStatusFlags_[cellIndex] = RegionalDistributor::ReadByOneFlag;
			cellLockOwners_[cellIndex] = processOrGroup;
		}
		else if (cellStatusFlags_[cellIndex] == RegionalDistributor::ReadByOneFlag)
		{
			// If the Cell is currently being read, but not by us, change the status to read-by-many
			if (cellLockOwners_[cellIndex] != processOrGroup)
			{
				cellStatusFlags_[cellIndex] = RegionalDistributor::ReadByManyFlag;
				cellLockOwners_[cellIndex] = -1;
			}
		}
		else if (cellStatusFlags_[cellIndex] == RegionalDistributor::ReadByManyFlag)
		{
			// Already being read by more than one processs/group, so nothing more to do
		}
	}

	return true;
}

// Try to assign a Molecule from the specified Cell to the process/group
Molecule* RegionalDistributor::assignMolecule(Cell* cell, int processOrGroup)
{
	// TODO May be beneficial to do this by size order (nAtoms in molecules)?

	if (DND) Messenger::print("  Looking through molecules in Cell %i for process/group %i..\n", cell->index(), processOrGroup);

	// There will likely be multiple atoms from the same, so note each Molecule as we check it
	OrderedPointerList<Molecule> checkedMolecules;

	// Loop over Atoms in Cell
	Molecule* mol;
	Atom** atoms = cell->indexOrderedAtoms();
	for (int n=0; n<cell->nAtoms(); ++n)
	{
		// Get the Atom's Molecule pointer
		mol = atoms[n]->molecule();

		if (DND) Messenger::print("  <> Molecule index is %i (from Atom index %i) and this molecule %s already in our list..\n", mol->arrayIndex(), atoms[n]->arrayIndex(), checkedMolecules.contains(mol) ? "IS" : "IS NOT");

		// Have we already checked this Molecule?
		if (checkedMolecules.contains(mol)) continue;

		// Try to assign this Molecule to the present process/group
		if (assignMolecule(mol, processOrGroup)) return mol;

		// Not possible to assign the Molecule, so add it to our list of checked Molecules and move on
		checkedMolecules.addExclusive(mol);
	}

	return NULL;
}

// Try to find a Molecule target for the process/group
Molecule* RegionalDistributor::assignMolecule(int processOrGroup)
{
	/*
	 * For this process/group, look at its current list of locked Cells, and search for a suitable Molecule within those.
	 * If there are no suitable Molecules (or there are no Cells, as is the case at the beginning), pick a suitable Cell close to 
	 * those already in the list (or one at a suitable starting location for the current process/group).
	 */
	Molecule* molecule = NULL;

	OrderedPointerListIterator<Cell> lockedCellIterator(lockedCells_[processOrGroup]);
	while (Cell* cell = lockedCellIterator.iterate())
	{
		if (DND) Messenger::print("  Searching for suitable Molecule to assign to process/group %i from Cell index %i (already locked by this process/group)...\n", processOrGroup, cell->index());

		// Try to assign a Molecule from those (partially) present in this Cell
		molecule = assignMolecule(cell, processOrGroup);

		// If we have found a suitable Molecule for assignment, return it now
		if (molecule) return molecule;
	}

	if (DND) Messenger::print(" No Molecules available in locked cells for this process/group.\n");

	/*
	 * If we did *not* find a suitable Molecule in the current list of locked Cells for this process/group, we need to get a new Cell.
	 * If there are Cells in the locked list, find one that is ReadByOne and assigned to this process, and search from there.
	 * If we fail, or there are zero Cells in the locked list, then try to add a Cell at a suitable 'distance' along the Cell Array for this process/group.
	 */
	int cellIndex;
	if (lockedCells_[processOrGroup].nItems() > 0)
	{
		// Loop over all Cells, searching for one which this process/group alone has marked read-only
		for (cellIndex=0; cellIndex<cellArray_.nCells(); ++cellIndex)
		{
			if (DND) Messenger::print(" Searching for suitable Molecule to assign to process/group %i from Cell index %i (lock owner = %i, status == %s)...\n", processOrGroup, cellIndex, cellLockOwners_[cellIndex], cellStatusFlag(cellStatusFlags_[cellIndex]));

			if (cellStatusFlags_[cellIndex] != RegionalDistributor::ReadByOneFlag) continue;
			if (cellLockOwners_[cellIndex] != processOrGroup) continue;

			// Found a Cell we have marked as read-only - does a suitable Molecule exist therein
			molecule = assignMolecule(cellArray_.cell(cellIndex), processOrGroup);
			if (molecule) return molecule;
		}
	}

	// No suitable Molecule yet, so start searching over all Cells from a suitable point along the array.
	for (int n=0; n<cellArray_.nCells(); ++n)
	{
		// Determine Cell index
		cellIndex = n;

		if (DND) Messenger::print("  -- Checking Cell %i for process/group %i: status = %i\n", cellIndex, processOrGroup, cellStatusFlags_[cellIndex]);

		if (cellStatusFlags_[cellIndex] != RegionalDistributor::UnusedFlag) continue;

		// Found an unused cell - does a suitable Molecule exist therein?
		molecule = assignMolecule(cellArray_.cell(cellIndex), processOrGroup);
		if (molecule) return molecule;
	}

	return NULL;
}

// Return next set of Molecule IDs assigned to this process
Array<int> RegionalDistributor::assignedMolecules()
{
	return assignedMolecules_[processOrGroupIndex_];
}

/*
 * Helper Functions
 */

// Return whether to collect statistics (counts, energies etc. - ensures correct summation) 
bool RegionalDistributor::collectStatistics()
{
	return ((currentStrategy_ == ProcessPool::PoolStrategy) || (processPool_.isMaster()));
}

// Increment integer counter, but only if the process should (depend on the current strategy)
void RegionalDistributor::increment(int& counter)
{
	if (collectStatistics()) ++counter;
}

// Add to double variable, but only if the process should (depend on the current strategy)
void RegionalDistributor::increase(double& var, double value)
{
	if (collectStatistics()) var += value;
}
