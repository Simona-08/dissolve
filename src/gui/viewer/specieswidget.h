/*
	*** Species Widget
	*** src/gui/viewer/specieswidget.h
	Copyright T. Youngs 2019

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

#ifndef DISSOLVE_SPECIESWIDGET_H
#define DISSOLVE_SPECIESWIDGET_H

#include "gui/viewer/ui_specieswidget.h"

// Forward Declarations
/* none */

// Species Widget
class SpeciesWidget : public QWidget
{
	// All Qt declarations must include this macro
	Q_OBJECT

	public:
	// Constructor / Destructor
	SpeciesWidget(QWidget* parent = 0);
	~SpeciesWidget();

	private:
	// Main form declaration
	Ui::SpeciesWidget ui_;

	public:
	// Return contained SpeciesViewer
	SpeciesViewer* speciesViewer();


	/*
	 * Signals / Slots
	 */
	public slots:
	// Toolbar
	void on_ViewToolButton_clicked(bool checked);
	void on_DrawToolButton_clicked(bool checked);

	// Update info label
	void updateInfoLabel();
};

#endif