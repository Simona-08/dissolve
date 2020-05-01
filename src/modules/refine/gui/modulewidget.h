/*
	*** Module Widget
	*** src/modules/refine/gui/modulewidget.h
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

#ifndef DISSOLVE_MODULEWIDGET_REFINE_H
#define DISSOLVE_MODULEWIDGET_REFINE_H

#include "gui/modulewidget.h"
#include "modules/refine/gui/ui_modulewidget.h"

// Forward Declarations
class Dissolve;
class Module;
class PartialSet;
class RefineModule;
class DataViewer;

// Module Widget
class RefineModuleWidget : public ModuleWidget
{
	// All Qt declarations derived from QObject must include this macro
	Q_OBJECT

      public:
	RefineModuleWidget(QWidget *parent, RefineModule *module, Dissolve &dissolve);
	~RefineModuleWidget();

      private:
	// Associated Module
	RefineModule *module_;
	// DataViewers contained within this widget
	DataViewer *dataGraph_, *partialSQGraph_, *partialGRGraph_, *deltaPhiRGraph_, *phiMagGraph_, *errorsGraph_;
	// Reference to Dissolve
	Dissolve &dissolve_;

	/*
	 * UI
	 */
      private:
	// Main form declaration
	Ui::RefineModuleWidget ui_;

      public:
	// Update controls within widget
	void updateControls(int flags = ModuleWidget::DefaultUpdateFlag);

	/*
	 * State I/O
	 */
      public:
	// Read widget state through specified LineParser
	bool readState(LineParser &parser);
	// Write widget state through specified LineParser
	bool writeState(LineParser &parser) const;

	/*
	 * Widgets / Functions
	 */
      private:
	// Set data targets in graphs
	void setGraphDataTargets(RefineModule *module);
};

#endif
