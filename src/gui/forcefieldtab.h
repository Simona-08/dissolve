/*
	*** Forcefield Tab
	*** src/gui/forcefieldtab.h
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

#ifndef DISSOLVE_FORCEFIELDTAB_H
#define DISSOLVE_FORCEFIELDTAB_H

#include "gui/ui_forcefieldtab.h"
#include "gui/maintab.h"

// Forward Declarations
class MasterIntra;
class AtomType;
class PairPotential;

// Forcefield Tab
class ForcefieldTab : public QWidget, public MainTab
{
	// All Qt declarations derived from QObject must include this macro
	Q_OBJECT

	public:
	// Constructor / Destructor
	ForcefieldTab(DissolveWindow* dissolveWindow, Dissolve& dissolve, MainTabsWidget* parent, const char* title);
	~ForcefieldTab();


	/*
	 * UI
	 */
	private:
	// Main form declaration
	Ui::ForcefieldTab ui_;


	/*
	 * MainTab Reimplementations
	 */
	public:
	// Return tab type
	MainTab::TabType type() const;
	// Return whether the tab can be closed
	bool canClose() const;


	/*
	 * Update
	 */
	private:
	// Row update function for BondsTable
	void updateBondsTableRow(int row, MasterIntra* masterBond, bool createItems);
	// Row update function for AnglesTable
	void updateAnglesTableRow(int row, MasterIntra* masterAngle, bool createItems);
	// Row update function for TorsionsTable
	void updateTorsionsTableRow(int row, MasterIntra* masterTorsion, bool createItems);
	// Row update function for ImpropersTable
	void updateImpropersTableRow(int row, MasterIntra* masterImproper, bool createItems);
	// Row update function for AtomTypesTable
	void updateAtomTypesTableRow(int row, AtomType* atomType, bool createItems);
	// Row update function for PairPotentialsTable
	void updatePairPotentialsTableRow(int row, PairPotential* pairPotential, bool createItems);

	protected:
	// Update controls in tab
	void updateControls();
	// Disable sensitive controls within tab
	void disableSensitiveControls();
	// Enable sensitive controls within tab
	void enableSensitiveControls();


	/*
	 * Signals / Slots
	 */
	private:
	// Signal that some AtomType parameter has been modified, so pair potentials should be regenerated
	void atomTypeDataModified();

	private slots:
	// Atom Types
	void on_AtomTypeAddButton_clicked(bool checked);
	void on_AtomTypeRemoveButton_clicked(bool checked);
	void on_AtomTypesTable_itemChanged(QTableWidgetItem* w);
	// Pair Potentials
	void on_PairPotentialRangeSpin_valueChanged(double value);
	void on_PairPotentialDeltaSpin_valueChanged(double value);
	void on_PairPotentialsIncludeCoulombRadio_clicked(bool checked);
	void on_PairPotentialsShortRangeOnlyRadio_clicked(bool checked);
	void on_ShortRangeTruncationCombo_currentIndexChanged(int index);
	void on_CoulombTruncationCombo_currentIndexChanged(int index);
	void on_RegenerateAllPairPotentialsButton_clicked(bool checked);
	void on_UpdatePairPotentialsButton_clicked(bool checked);
	void on_AutoUpdatePairPotentialsCheck_clicked(bool checked);
	void on_PairPotentialsTable_currentItemChanged(QTableWidgetItem* currentItem, QTableWidgetItem* previousItem);
	void on_PairPotentialsTable_itemChanged(QTableWidgetItem* w);
	// Master Terms
	void on_MasterTermAddBondButton_clicked(bool checked);
	void on_MasterTermRemoveBondButton_clicked(bool checked);
	void on_MasterBondsTable_itemChanged(QTableWidgetItem* w);
	void on_MasterTermAddAngleButton_clicked(bool checked);
	void on_MasterTermRemoveAngleButton_clicked(bool checked);
	void on_MasterAnglesTable_itemChanged(QTableWidgetItem* w);
	void on_MasterTermAddTorsionButton_clicked(bool checked);
	void on_MasterTermRemoveTorsionButton_clicked(bool checked);
	void on_MasterTorsionsTable_itemChanged(QTableWidgetItem* w);
	void on_MasterTermAddImproperButton_clicked(bool checked);
	void on_MasterTermRemoveImproperButton_clicked(bool checked);
	void on_MasterImpropersTable_itemChanged(QTableWidgetItem* w);


	/*
	 * State
	 */
	public:
	// Read widget state through specified LineParser
	bool readState(LineParser& parser, const CoreData& coreData);
	// Write widget state through specified LineParser
	bool writeState(LineParser& parser) const;
};

#endif
