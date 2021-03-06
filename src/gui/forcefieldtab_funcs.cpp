/*
	*** ForcefieldTab Functions
	*** src/gui/forcefieldtab_funcs.cpp
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

#include "gui/forcefieldtab.h"
#include "gui/gui.h"
#include "gui/widgets/elementselector.hui"
#include "gui/delegates/combolist.hui"
#include "gui/delegates/exponentialspin.hui"
#include "gui/helpers/listwidgetupdater.h"
#include "gui/helpers/tablewidgetupdater.h"
#include "main/dissolve.h"
#include "classes/atomtype.h"
#include "classes/speciesbond.h"
#include "classes/speciesangle.h"
#include "classes/speciesimproper.h"
#include "classes/speciestorsion.h"
#include <QListWidgetItem>

// Constructor / Destructor
ForcefieldTab::ForcefieldTab(DissolveWindow* dissolveWindow, Dissolve& dissolve, MainTabsWidget* parent, const char* title) : MainTab(dissolveWindow, dissolve, parent, title, this)
{
	ui_.setupUi(this);

	Locker refreshLocker(refreshLock_);

	/*
	 * Master Terms
	 */
	
	// Set item delegates for tables
	// -- Functional Forms
	ui_.MasterBondsTable->setItemDelegateForColumn(1, new ComboListDelegate(this, new ComboEnumOptionsItems<SpeciesBond::BondFunction>(SpeciesBond::bondFunctions())));
	ui_.MasterAnglesTable->setItemDelegateForColumn(1, new ComboListDelegate(this, new ComboEnumOptionsItems<SpeciesAngle::AngleFunction>(SpeciesAngle::angleFunctions())));
	ui_.MasterTorsionsTable->setItemDelegateForColumn(1, new ComboListDelegate(this, new ComboEnumOptionsItems<SpeciesTorsion::TorsionFunction>(SpeciesTorsion::torsionFunctions())));
	ui_.MasterImpropersTable->setItemDelegateForColumn(1, new ComboListDelegate(this, new ComboEnumOptionsItems<SpeciesImproper::ImproperFunction>(SpeciesImproper::improperFunctions())));

	// -- Parameters
	for (int n=2; n<6; ++n)
	{
		ui_.MasterBondsTable->setItemDelegateForColumn(n, new ExponentialSpinDelegate(this));
		ui_.MasterAnglesTable->setItemDelegateForColumn(n, new ExponentialSpinDelegate(this));
		ui_.MasterTorsionsTable->setItemDelegateForColumn(n, new ExponentialSpinDelegate(this));
		ui_.MasterImpropersTable->setItemDelegateForColumn(n, new ExponentialSpinDelegate(this));
	}

	// Ensure fonts for table headers are set correctly and the headers themselves are visible
	ui_.MasterBondsTable->horizontalHeader()->setFont(font());
	ui_.MasterBondsTable->horizontalHeader()->setVisible(true);
	ui_.MasterAnglesTable->horizontalHeader()->setFont(font());
	ui_.MasterAnglesTable->horizontalHeader()->setVisible(true);
	ui_.MasterTorsionsTable->horizontalHeader()->setFont(font());
	ui_.MasterTorsionsTable->horizontalHeader()->setVisible(true);
	ui_.MasterImpropersTable->horizontalHeader()->setFont(font());
	ui_.MasterImpropersTable->horizontalHeader()->setVisible(true);

	/*
	 * Atom Types
	 */

	// Set item delegates for tables
	// -- Short Range Functional Forms
	ui_.AtomTypesTable->setItemDelegateForColumn(3, new ComboListDelegate(this, new ComboEnumOptionsItems<Forcefield::ShortRangeType>(Forcefield::shortRangeTypes())));
	// -- Charge / Parameters
	for (int n=4; n<9; ++n) ui_.AtomTypesTable->setItemDelegateForColumn(n, new ExponentialSpinDelegate(this));

	// Ensure fonts for table headers are set correctly and the headers themselves are visible
	ui_.AtomTypesTable->horizontalHeader()->setFont(font());
	ui_.AtomTypesTable->horizontalHeader()->setVisible(true);

	/*
	 * Pair Potentials
	 */

	// Set up combo delegates
	for (int n=0; n<PairPotential::nCoulombTruncationSchemes; ++n) ui_.CoulombTruncationCombo->addItem(PairPotential::coulombTruncationScheme( (PairPotential::CoulombTruncationScheme) n));
	for (int n=0; n<PairPotential::nShortRangeTruncationSchemes; ++n) ui_.ShortRangeTruncationCombo->addItem(PairPotential::shortRangeTruncationScheme( (PairPotential::ShortRangeTruncationScheme) n));

	// Set sensible lower limits and steps for range and delta
	ui_.PairPotentialRangeSpin->setRange(true, 1.0);
	ui_.PairPotentialRangeSpin->setSingleStep(1.0);
	ui_.PairPotentialDeltaSpin->setRange(true, 0.001);
	ui_.PairPotentialDeltaSpin->setSingleStep(0.001);

	// Ensure fonts for table headers are set correctly and the headers themselves are visible
	ui_.PairPotentialsTable->horizontalHeader()->setFont(font());
	ui_.PairPotentialsTable->horizontalHeader()->setVisible(true);

	// -- Charges / Parameters delegates
// 	for (int n=3; n<9; ++n) ui_.PairPotentialsTable->setItemDelegateForColumn(n, new ExponentialSpinDelegate(this));

	DataViewer* viewer = ui_.PairPotentialsPlotWidget->dataViewer();
	viewer->view().axes().setTitle(0, "\\it{r}, \\sym{angstrom}");
	viewer->view().axes().setTitle(1, "U, kj/mol");
	viewer->view().axes().setRange(1, -100.0, 100.0);
}

ForcefieldTab::~ForcefieldTab()
{
}

/*
 * MainTab Reimplementations
 */

// Return tab type
MainTab::TabType ForcefieldTab::type() const
{
	return MainTab::ForcefieldTabType;
}

// Return whether the tab can be closed
bool ForcefieldTab::canClose() const
{
	return false;
}

/*
 * Update
 */

// Row update function for BondsTable
void ForcefieldTab::updateBondsTableRow(int row, MasterIntra* masterBond, bool createItems)
{
	QTableWidgetItem* item;

	// Name
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterBond));
		ui_.MasterBondsTable->setItem(row, 0, item);
	}
	else item = ui_.MasterBondsTable->item(row, 0);
	item->setText(masterBond->name());

	// Functional Form
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterBond));
		ui_.MasterBondsTable->setItem(row, 1, item);
	}
	else item = ui_.MasterBondsTable->item(row, 1);
	item->setText(SpeciesBond::bondFunctions().keywordFromInt(masterBond->form()));

	// Parameters
	for (int n=0; n<MAXINTRAPARAMS; ++n)
	{
		if (createItems)
		{
			item = new QTableWidgetItem;
			item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterBond));
			ui_.MasterBondsTable->setItem(row, n+2, item);
		}
		else item = ui_.MasterBondsTable->item(row, n+2);
		item->setText(QString::number(masterBond->parameter(n)));
	}
}

// Row update function for AnglesTable
void ForcefieldTab::updateAnglesTableRow(int row, MasterIntra* masterAngle, bool createItems)
{
	QTableWidgetItem* item;

	// Name
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterAngle));
		ui_.MasterAnglesTable->setItem(row, 0, item);
	}
	else item = ui_.MasterAnglesTable->item(row, 0);
	item->setText(masterAngle->name());

	// Functional Form
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterAngle));
		ui_.MasterAnglesTable->setItem(row, 1, item);
	}
	else item = ui_.MasterAnglesTable->item(row, 1);
	item->setText(SpeciesAngle::angleFunctions().keywordFromInt(masterAngle->form()));

	// Parameters
	for (int n=0; n<MAXINTRAPARAMS; ++n)
	{
		if (createItems)
		{
			item = new QTableWidgetItem;
			item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterAngle));
			ui_.MasterAnglesTable->setItem(row, n+2, item);
		}
		else item = ui_.MasterAnglesTable->item(row, n+2);
		item->setText(QString::number(masterAngle->parameter(n)));
	}
}

// Row update function for TorsionsTable
void ForcefieldTab::updateTorsionsTableRow(int row, MasterIntra* masterTorsion, bool createItems)
{
	QTableWidgetItem* item;

	// Name
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterTorsion));
		ui_.MasterTorsionsTable->setItem(row, 0, item);
	}
	else item = ui_.MasterTorsionsTable->item(row, 0);
	item->setText(masterTorsion->name());

	// Functional Form
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterTorsion));
		ui_.MasterTorsionsTable->setItem(row, 1, item);
	}
	else item = ui_.MasterTorsionsTable->item(row, 1);
	item->setText(SpeciesTorsion::torsionFunctions().keywordFromInt(masterTorsion->form()));

	// Parameters
	for (int n=0; n<MAXINTRAPARAMS; ++n)
	{
		if (createItems)
		{
			item = new QTableWidgetItem;
			item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterTorsion));
			ui_.MasterTorsionsTable->setItem(row, n+2, item);
		}
		else item = ui_.MasterTorsionsTable->item(row, n+2);
		item->setText(QString::number(masterTorsion->parameter(n)));
	}
}

// Row update function for ImpropersTable
void ForcefieldTab::updateImpropersTableRow(int row, MasterIntra* masterImproper, bool createItems)
{
	QTableWidgetItem* item;

	// Name
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterImproper));
		ui_.MasterImpropersTable->setItem(row, 0, item);
	}
	else item = ui_.MasterImpropersTable->item(row, 0);
	item->setText(masterImproper->name());

	// Functional Form
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterImproper));
		ui_.MasterImpropersTable->setItem(row, 1, item);
	}
	else item = ui_.MasterImpropersTable->item(row, 1);
	item->setText(SpeciesImproper::improperFunctions().keywordFromInt(masterImproper->form()));

	// Parameters
	for (int n=0; n<MAXINTRAPARAMS; ++n)
	{
		if (createItems)
		{
			item = new QTableWidgetItem;
			item->setData(Qt::UserRole, VariantPointer<MasterIntra>(masterImproper));
			ui_.MasterImpropersTable->setItem(row, n+2, item);
		}
		else item = ui_.MasterImpropersTable->item(row, n+2);
		item->setText(QString::number(masterImproper->parameter(n)));
	}
}

// Row update function for AtomTypesTable
void ForcefieldTab::updateAtomTypesTableRow(int row, AtomType* atomType, bool createItems)
{
	QTableWidgetItem* item;

	// Name
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<AtomType>(atomType));
		ui_.AtomTypesTable->setItem(row, 0, item);
	}
	else item = ui_.AtomTypesTable->item(row, 0);
	item->setText(atomType->name());

	// Target element
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<AtomType>(atomType));
		item->setFlags(Qt::NoItemFlags);
		ui_.AtomTypesTable->setItem(row, 1, item);
	}
	else item = ui_.AtomTypesTable->item(row, 1);
	item->setText(atomType->element()->symbol());

	// Charge
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<AtomType>(atomType));
		ui_.AtomTypesTable->setItem(row, 2, item);
	}
	else item = ui_.AtomTypesTable->item(row, 2);
	item->setText(QString::number(atomType->parameters().charge()));

	// Short-Range Form
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<AtomType>(atomType));
		ui_.AtomTypesTable->setItem(row, 3, item);
	}
	else item = ui_.AtomTypesTable->item(row, 3);
	item->setText(Forcefield::shortRangeTypes().keyword(atomType->shortRangeType()));

	// Parameters
	for (int n=0; n<MAXINTRAPARAMS; ++n)
	{
		if (createItems)
		{
			item = new QTableWidgetItem;
			item->setData(Qt::UserRole, VariantPointer<AtomType>(atomType));
			ui_.AtomTypesTable->setItem(row, n+4, item);
		}
		else item = ui_.AtomTypesTable->item(row, n+4);
		item->setText(QString::number(atomType->parameters().parameter(n)));
	}
}

// Row update function for PairPotentialsTable
void ForcefieldTab::updatePairPotentialsTableRow(int row, PairPotential* pairPotential, bool createItems)
{
	QTableWidgetItem* item;

	// Type I
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<PairPotential>(pairPotential));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui_.PairPotentialsTable->setItem(row, 0, item);
	}
	else item = ui_.PairPotentialsTable->item(row, 0);
	item->setText(pairPotential->atomTypeNameI());

	// Type J
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<PairPotential>(pairPotential));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui_.PairPotentialsTable->setItem(row, 1, item);
	}
	else item = ui_.PairPotentialsTable->item(row, 1);
	item->setText(pairPotential->atomTypeNameJ());

	// Short-Range Form
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<PairPotential>(pairPotential));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui_.PairPotentialsTable->setItem(row, 2, item);
	}
	else item = ui_.PairPotentialsTable->item(row, 2);
	item->setText(Forcefield::shortRangeTypes().keyword(pairPotential->shortRangeType()));

	// Charge I
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<PairPotential>(pairPotential));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui_.PairPotentialsTable->setItem(row, 3, item);
	}
	else item = ui_.PairPotentialsTable->item(row, 3);
	item->setText(QString::number(pairPotential->chargeI()));

	// Charge I
	if (createItems)
	{
		item = new QTableWidgetItem;
		item->setData(Qt::UserRole, VariantPointer<PairPotential>(pairPotential));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ui_.PairPotentialsTable->setItem(row, 4, item);
	}
	else item = ui_.PairPotentialsTable->item(row, 4);
	item->setText(QString::number(pairPotential->chargeJ()));

	// Parameters
	for (int n=0; n<MAXINTRAPARAMS; ++n)
	{
		if (createItems)
		{
			item = new QTableWidgetItem;
			item->setData(Qt::UserRole, VariantPointer<PairPotential>(pairPotential));
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			ui_.PairPotentialsTable->setItem(row, n+5, item);
		}
		else item = ui_.PairPotentialsTable->item(row, n+5);
		item->setText(QString::number(pairPotential->parameter(n)));
	}
}

// Update controls in tab
void ForcefieldTab::updateControls()
{
	Locker refreshLocker(refreshLock_);

	// Master Bonds Table
	TableWidgetUpdater<ForcefieldTab,MasterIntra> bondsUpdater(ui_.MasterBondsTable, dissolve_.coreData().masterBonds(), this, &ForcefieldTab::updateBondsTableRow);
	ui_.MasterBondsTable->resizeColumnsToContents();

	// Master Angles Table
	TableWidgetUpdater<ForcefieldTab,MasterIntra> anglesUpdater(ui_.MasterAnglesTable, dissolve_.coreData().masterAngles(), this, &ForcefieldTab::updateAnglesTableRow);
	ui_.MasterAnglesTable->resizeColumnsToContents();

	// Torsions Table
	TableWidgetUpdater<ForcefieldTab,MasterIntra> torsionsUpdater(ui_.MasterTorsionsTable, dissolve_.coreData().masterTorsions(), this, &ForcefieldTab::updateTorsionsTableRow);
	ui_.MasterTorsionsTable->resizeColumnsToContents();

	// Impropers Table
	TableWidgetUpdater<ForcefieldTab,MasterIntra> impropersUpdater(ui_.MasterImpropersTable, dissolve_.coreData().masterImpropers(), this, &ForcefieldTab::updateImpropersTableRow);
	ui_.MasterImpropersTable->resizeColumnsToContents();

	// AtomTypes Table
	TableWidgetUpdater<ForcefieldTab,AtomType> atomTypesUpdater(ui_.AtomTypesTable, dissolve_.atomTypes(), this, &ForcefieldTab::updateAtomTypesTableRow);
	ui_.AtomTypesTable->resizeColumnsToContents();

	// PairPotentials
	// -- Automatically regenerate pair potentials (quietly)
	if (ui_.AutoUpdatePairPotentialsCheck->isChecked())
	{
		Messenger::mute();
		dissolve_.generatePairPotentials();
		Messenger::unMute();
	}
	ui_.PairPotentialRangeSpin->setValue(dissolve_.pairPotentialRange());
	ui_.PairPotentialDeltaSpin->setValue(dissolve_.pairPotentialDelta());
	if (dissolve_.pairPotentialsIncludeCoulomb()) ui_.PairPotentialsIncludeCoulombRadio->setChecked(true);
	else ui_.PairPotentialsShortRangeOnlyRadio->setChecked(true);
	ui_.ShortRangeTruncationCombo->setCurrentIndex(PairPotential::shortRangeTruncationScheme());
	ui_.ShortRangeTruncationWidthSpin->setValue(PairPotential::shortRangeTruncationWidth());
	ui_.ShortRangeTruncationWidthSpin->setEnabled(PairPotential::shortRangeTruncationScheme() == PairPotential::CosineShortRangeTruncation);
	ui_.CoulombTruncationCombo->setCurrentIndex(PairPotential::coulombTruncationScheme());
	// -- Table
	TableWidgetUpdater<ForcefieldTab,PairPotential> ppUpdater(ui_.PairPotentialsTable, dissolve_.pairPotentials(), this, &ForcefieldTab::updatePairPotentialsTableRow);
	ui_.PairPotentialsTable->resizeColumnsToContents();
}

// Disable sensitive controls within tab
void ForcefieldTab::disableSensitiveControls()
{
	setEnabled(false);
}

// Enable sensitive controls within tab
void ForcefieldTab::enableSensitiveControls()
{
	setEnabled(true);
}

/*
 * Signals / Slots
 */

// Signal that some AtomType parameter has been modified, so pair potentials should be regenerated
void ForcefieldTab::atomTypeDataModified()
{
	dissolve_.coreData().bumpAtomTypesVersion();

	if (ui_.AutoUpdatePairPotentialsCheck->isChecked())
	{
		Messenger::mute();
		dissolve_.generatePairPotentials();
		Messenger::unMute();
	}

}

void ForcefieldTab::on_AtomTypeAddButton_clicked(bool checked)
{
	// First, need to get target element for the new AtomType
	bool ok;
	Element* element = ElementSelector::getElement(this, "Element Selection", "Choose the Element for the AtomType", NULL, &ok);
	if (!ok) return;

	AtomType* at = dissolve_.addAtomType(element);

	Locker refreshLocker(refreshLock_);

	TableWidgetUpdater<ForcefieldTab,AtomType> atomTypesUpdater(ui_.AtomTypesTable, dissolve_.atomTypes(), this, &ForcefieldTab::updateAtomTypesTableRow);
	ui_.AtomTypesTable->resizeColumnsToContents();

	dissolveWindow_->setModified();
}

void ForcefieldTab::on_AtomTypeRemoveButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_AtomTypesTable_itemChanged(QTableWidgetItem* w)
{
	if (refreshLock_.isLocked()) return;

	// Get target AtomType from the passed widget
	AtomType* atomType = w ? VariantPointer<AtomType>(w->data(Qt::UserRole)) : NULL;
	if (!atomType) return;

	// Column of passed item tells us the type of data we need to change
	switch (w->column())
	{
		// Name
		case (0):
			atomType->setName(qPrintable(w->text()));
			dissolveWindow_->setModified();
			break;
		// Charge
		case (2):
			atomType->parameters().setCharge(w->text().toDouble());
			atomTypeDataModified();
			dissolveWindow_->setModified();
			break;
		// Short-range form
		case (3):
			atomType->setShortRangeType(Forcefield::shortRangeTypes().enumeration(qPrintable(w->text())));
			atomTypeDataModified();
			break;
		// Parameters
		case (4):
		case (5):
		case (6):
		case (7):
			atomType->parameters().setParameter(w->column()-4, w->text().toDouble());
			atomTypeDataModified();
			dissolveWindow_->setModified();
			break;
		default:
			Messenger::error("Don't know what to do with data from column %i of AtomTypes table.\n", w->column());
			break;
	}
}

void ForcefieldTab::on_PairPotentialRangeSpin_valueChanged(double value)
{
	if (refreshLock_.isLocked()) return;

	dissolve_.setPairPotentialRange(value);

	if (ui_.AutoUpdatePairPotentialsCheck->isChecked())
	{
		dissolve_.regeneratePairPotentials();
		updateControls();
	}

	dissolveWindow_->setModified();
}

void ForcefieldTab::on_PairPotentialDeltaSpin_valueChanged(double value)
{
	if (refreshLock_.isLocked()) return;

	dissolve_.setPairPotentialDelta(value);

	if (ui_.AutoUpdatePairPotentialsCheck->isChecked())
	{
		dissolve_.regeneratePairPotentials();
		updateControls();
	}

	dissolveWindow_->setModified();
}

void ForcefieldTab::on_PairPotentialsIncludeCoulombRadio_clicked(bool checked)
{
	if (refreshLock_.isLocked()) return;

	dissolve_.setPairPotentialsIncludeCoulomb(checked);

	if (ui_.AutoUpdatePairPotentialsCheck->isChecked())
	{
		dissolve_.regeneratePairPotentials();
		updateControls();
	}

	// Need to update to show/hide the charges column in atoms tables
	dissolveWindow_->fullUpdate();
	
	dissolveWindow_->setModified();
}

void ForcefieldTab::on_PairPotentialsShortRangeOnlyRadio_clicked(bool checked)
{
	on_PairPotentialsIncludeCoulombRadio_clicked(false);
}

void ForcefieldTab::on_ShortRangeTruncationCombo_currentIndexChanged(int index)
{
	if (refreshLock_.isLocked()) return;

	PairPotential::setShortRangeTruncationScheme( (PairPotential::ShortRangeTruncationScheme) index );
	ui_.ShortRangeTruncationWidthSpin->setEnabled(PairPotential::shortRangeTruncationScheme() == PairPotential::CosineShortRangeTruncation);

	if (ui_.AutoUpdatePairPotentialsCheck->isChecked())
	{
		dissolve_.regeneratePairPotentials();
		updateControls();
	}

	dissolveWindow_->setModified();
}

void ForcefieldTab::on_CoulombTruncationCombo_currentIndexChanged(int index)
{
	if (refreshLock_.isLocked()) return;

	PairPotential::setCoulombTruncationScheme( (PairPotential::CoulombTruncationScheme) index );

	if (ui_.AutoUpdatePairPotentialsCheck->isChecked() && dissolve_.pairPotentialsIncludeCoulomb())
	{
		dissolve_.regeneratePairPotentials();
		updateControls();
	}

	dissolveWindow_->setModified();
}

void ForcefieldTab::on_RegenerateAllPairPotentialsButton_clicked(bool checked)
{
	dissolve_.regeneratePairPotentials();

	updateControls();
}

void ForcefieldTab::on_UpdatePairPotentialsButton_clicked(bool checked)
{
	dissolve_.generatePairPotentials();

	updateControls();
}

void ForcefieldTab::on_AutoUpdatePairPotentialsCheck_clicked(bool checked)
{
	if (checked) on_UpdatePairPotentialsButton_clicked(false);
}

void ForcefieldTab::on_PairPotentialsTable_currentItemChanged(QTableWidgetItem* currentItem, QTableWidgetItem* previousItem)
{
	// Clear all data in the graph
	DataViewer* graph = ui_.PairPotentialsPlotWidget->dataViewer();
	graph->clearRenderables();

	if (!currentItem) return;

	// Get the selected pair potential
	PairPotential* pp = VariantPointer<PairPotential>(currentItem->data(Qt::UserRole));
	if (pp)
	{
		Renderable* fullPotential = graph->createRenderable(Renderable::Data1DRenderable, pp->uFull().objectTag(), "Full");
		fullPotential->setColour(StockColours::BlackStockColour);

		Renderable* originalPotential = graph->createRenderable(Renderable::Data1DRenderable, pp->uOriginal().objectTag(), "Original");
		originalPotential->setColour(StockColours::RedStockColour);
		originalPotential->lineStyle().set(1.0, LineStipple::HalfDashStipple);

		Renderable* additionalPotential = graph->createRenderable(Renderable::Data1DRenderable, pp->uAdditional().objectTag(), "Additional");
		additionalPotential->setColour(StockColours::BlueStockColour);
		additionalPotential->lineStyle().set(1.0, LineStipple::DotStipple);

		Renderable* dUFull = graph->createRenderable(Renderable::Data1DRenderable, pp->dUFull().objectTag(), "Force");
		dUFull->setColour(StockColours::GreenStockColour);
	}

}

void ForcefieldTab::on_PairPotentialsTable_itemChanged(QTableWidgetItem* w)
{
	if (refreshLock_.isLocked()) return;

	// Get target PairPotential from the passed widget
	PairPotential* pairPotential = w ? VariantPointer<PairPotential>(w->data(Qt::UserRole)) : NULL;
	if (!pairPotential) return;

	// Column of passed item tells us the type of data we need to change
	switch (w->column())
	{
		// Functional form
		case (2):
// 			pairPotential->setShortRangeType(PairPotential::shortRangeType(qPrintable(w->text())));
			dissolveWindow_->setModified();
			break;
		// Charge I
		case (3):
			pairPotential->setChargeI(w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		// Charge J
		case (4):
			pairPotential->setChargeJ(w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		// Parameters
		case (5):
		case (6):
		case (7):
		case (8):
			pairPotential->setParameter(w->column()-5, w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		default:
			Messenger::error("Don't know what to do with data from column %i of PairPotentials table.\n", w->column());
			break;
	}
}

void ForcefieldTab::on_MasterTermAddBondButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterTermRemoveBondButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterBondsTable_itemChanged(QTableWidgetItem* w)
{
	if (refreshLock_.isLocked()) return;

	// Get target MasterIntra from the passed widget
	MasterIntra* masterIntra = w ? VariantPointer<MasterIntra>(w->data(Qt::UserRole)) : NULL;
	if (!masterIntra) return;

	// Column of passed item tells us the type of data we need to change
	switch (w->column())
	{
		// Name
		case (0):
			masterIntra->setName(qPrintable(w->text()));
			dissolveWindow_->setModified();
			break;
		// Functional Form
		case (1):
			masterIntra->setForm(SpeciesBond::bondFunctions().enumeration(qPrintable(w->text())));
			dissolveWindow_->setModified();
			break;
		// Parameters
		case (2):
		case (3):
		case (4):
		case (5):
			masterIntra->setParameter(w->column()-2, w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		default:
			Messenger::error("Don't know what to do with data from column %i of MasterIntra table.\n", w->column());
			break;
	}
}

void ForcefieldTab::on_MasterTermAddAngleButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterTermRemoveAngleButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterAnglesTable_itemChanged(QTableWidgetItem* w)
{
	if (refreshLock_.isLocked()) return;

	// Get target MasterIntra from the passed widget
	MasterIntra* masterIntra = w ? VariantPointer<MasterIntra>(w->data(Qt::UserRole)) : NULL;
	if (!masterIntra) return;

	// Column of passed item tells us the type of data we need to change
	switch (w->column())
	{
		// Name
		case (0):
			masterIntra->setName(qPrintable(w->text()));
			dissolveWindow_->setModified();
			break;
		// Functional Form
		case (1):
			masterIntra->setForm(SpeciesAngle::angleFunctions().enumeration(qPrintable(w->text())));
			dissolveWindow_->setModified();
			break;
		// Parameters
		case (2):
		case (3):
		case (4):
		case (5):
			masterIntra->setParameter(w->column()-2, w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		default:
			Messenger::error("Don't know what to do with data from column %i of MasterIntra table.\n", w->column());
			break;
	}
}

void ForcefieldTab::on_MasterTermAddTorsionButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterTermRemoveTorsionButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterTorsionsTable_itemChanged(QTableWidgetItem* w)
{
	if (refreshLock_.isLocked()) return;

	// Get target MasterIntra from the passed widgetmasterIntra->setForm(SpeciesBond::bondFunction(qPrintable(w->text())));
	MasterIntra* masterIntra = w ? VariantPointer<MasterIntra>(w->data(Qt::UserRole)) : NULL;
	if (!masterIntra) return;

	// Column of passed item tells us the type of data we need to change
	switch (w->column())
	{
		// Name
		case (0):
			masterIntra->setName(qPrintable(w->text()));
			dissolveWindow_->setModified();
			break;
		// Functional Form
		case (1):
			masterIntra->setForm(SpeciesTorsion::torsionFunctions().enumeration(qPrintable(w->text())));
			dissolveWindow_->setModified();
			break;
		// Parameters
		case (2):
		case (3):
		case (4):
		case (5):
			masterIntra->setParameter(w->column()-2, w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		default:
			Messenger::error("Don't know what to do with data from column %i of MasterIntra table.\n", w->column());
			break;
	}
}

void ForcefieldTab::on_MasterTermAddImproperButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterTermRemoveImproperButton_clicked(bool checked)
{
	printf("NOT IMPLEMENTED YET.\n");
}

void ForcefieldTab::on_MasterImpropersTable_itemChanged(QTableWidgetItem* w)
{
	if (refreshLock_.isLocked()) return;

	// Get target MasterIntra from the passed widgetmasterIntra->setForm(SpeciesBond::bondFunction(qPrintable(w->text())));
	MasterIntra* masterIntra = w ? VariantPointer<MasterIntra>(w->data(Qt::UserRole)) : NULL;
	if (!masterIntra) return;

	// Column of passed item tells us the type of data we need to change
	switch (w->column())
	{
		// Name
		case (0):
			masterIntra->setName(qPrintable(w->text()));
			dissolveWindow_->setModified();
			break;
		// Functional Form
		case (1):
			masterIntra->setForm(SpeciesImproper::improperFunctions().enumeration(qPrintable(w->text())));
			dissolveWindow_->setModified();
			break;
		// Parameters
		case (2):
		case (3):
		case (4):
		case (5):
			masterIntra->setParameter(w->column()-2, w->text().toDouble());
			dissolveWindow_->setModified();
			break;
		default:
			Messenger::error("Don't know what to do with data from column %i of MasterIntra table.\n", w->column());
			break;
	}
}

/*
 * State
 */

// Read widget state through specified LineParser
bool ForcefieldTab::readState(LineParser& parser, const CoreData& coreData)
{
	return true;
}

// Write widget state through specified LineParser
bool ForcefieldTab::writeState(LineParser& parser) const
{
	return true;
}
