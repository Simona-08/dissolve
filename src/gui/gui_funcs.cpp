/*
	*** Dissolve GUI - Core Functions
	*** src/gui/gui_funcs.cpp
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

#include "main/dissolve.h"
#include "version.h"
#include "gui/gui.h"
#include "gui/configurationtab.h"
#include "gui/forcefieldtab.h"
#include "gui/layertab.h"
#include "gui/moduletab.h"
#include "gui/speciestab.h"
#include "gui/workspacetab.h"
#include "base/lineparser.h"
#include <QCloseEvent>
#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>
#include <QLCDNumber>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QFileInfo>

// Constructor
DissolveWindow::DissolveWindow(Dissolve& dissolve) : QMainWindow(NULL), dissolve_(dissolve), threadController_(this, dissolve)
{
	// Initialise resources
	Q_INIT_RESOURCE(main);

	// Register custom font(s)
	QFontDatabase::addApplicationFont(":/fonts/fonts/Cousine-Regular.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/Cousine-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/Cousine-Italic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/Cousine-BoldItalic.ttf");

	// Set up user interface
	ui_.setupUi(this);

	// Set fonts
	ui_.MessagesEdit->setFont(QFont("Cousine", 10));

	// Connect signals to thread controller
	connect(this, SIGNAL(iterate(int)), &threadController_, SLOT(iterate(int)));
	connect(this, SIGNAL(stopIterating()), &threadController_, SLOT(stopIterating()));

	// Connect signals from our main tab widget
	connect(ui_.MainTabs, SIGNAL(dataModified()), this, SLOT(setModified()));
	connect(ui_.MainTabs, SIGNAL(dataModified()), this, SLOT(fullUpdate()));

	refreshing_ = false;
	modified_ = false;
	localSimulation_ = true;
	dissolveState_ = EditingState;

	// Create statusbar widgets
	localSimulationIndicator_ = new QLabel;
	localSimulationIndicator_->setPixmap(QPixmap(":/general/icons/general_local.svg"));
	localSimulationIndicator_->setMaximumSize(QSize(20,20));
	localSimulationIndicator_->setScaledContents(true);
	restartFileIndicator_ = new QLabel;
	restartFileIndicator_->setPixmap(QPixmap(":/general/icons/general_restartfile.svg"));
	restartFileIndicator_->setMaximumSize(QSize(20,20));
	restartFileIndicator_->setScaledContents(true);
	heartbeatFileIndicator_ = new QLabel;
	heartbeatFileIndicator_->setPixmap(QPixmap(":/general/icons/general_heartbeat.svg"));
	heartbeatFileIndicator_->setMaximumSize(QSize(20,20));
	heartbeatFileIndicator_->setScaledContents(true);
	etaLabel_ = new QLabel("ETA: --:--:--");
	statusBar()->addPermanentWidget(etaLabel_);
	statusBar()->addPermanentWidget(heartbeatFileIndicator_);
	statusBar()->addPermanentWidget(restartFileIndicator_);
	statusBar()->addPermanentWidget(localSimulationIndicator_);

	// Set up main tabs
	ui_.MainTabs->addCoreTabs(this);
	ui_.MainTabs->updateAllTabs();

	updateWindowTitle();
	updateControlsFrame();

	// Initialise the available system templates
	initialiseSystemTemplates();

	// Show the Start stack page (we call this mostly to ensure correct availability of other controls)
	showMainStackPage(DissolveWindow::StartStackPage);
}

// Destructor
DissolveWindow::~DissolveWindow()
{
}

// Catch window close event
void DissolveWindow::closeEvent(QCloseEvent* event)
{
	// Mark the window as refreshing, so we don't try to update any more widgets
	refreshing_ = true;

	if (!checkSaveCurrentInput())
	{
		event->ignore();
		return;
	}

	// Save the state before we go...
	saveState();

	// If Dissolve is running, stop the thread now
	if (dissolveState_ == RunningState)
	{
		// Send the signal to stop
		emit(stopIterating());

		// Wait for the thread to stop
		while (dissolveState_ == RunningState) QApplication::processEvents();
	}

	// Clear tabs before we try to close down the application, otherwise we'll get in to trouble with object deletion
	ui_.MainTabs->clearTabs();

	event->accept();
}

void DissolveWindow::resizeEvent(QResizeEvent* event)
{
}

/*
 * Dissolve Integration
 */

// Flag that data has been modified via the GUI
void DissolveWindow::setModified()
{
	modified_ = true;

	updateWindowTitle();
}

// Return reference to Dissolve
Dissolve& DissolveWindow::dissolve()
{
	return dissolve_;
}

// Return const reference to Dissolve
const Dissolve& DissolveWindow::constDissolve() const
{
	return dissolve_;
}

// Link output handler in to the Messenger
void DissolveWindow::addOutputHandler()
{
	Messenger::setOutputHandler(&outputHandler_);
	connect(&outputHandler_, SIGNAL(printText(const QString&)), this, SLOT(appendMessage(const QString&)));
	connect(&outputHandler_, SIGNAL(setColour(const QColor&)), ui_.MessagesEdit, SLOT(setTextColor(const QColor&)));
}

/*
 * File
 */

// Open specified input file from the CLI
bool DissolveWindow::openLocalFile(const char* inputFile, const char* restartFile, bool ignoreRestartFile, bool ignoreLayoutFile)
{
	// Clear any existing tabs etc.
	ui_.MainTabs->clearTabs();

	// Clear Dissolve itself
	dissolve_.clear();

	// Set the current dir to the location of the new file
	QFileInfo inputFileInfo(inputFile);

	// Load the input file
	Messenger::banner("Parse Input File");
	if (inputFileInfo.exists())
	{
		QDir::setCurrent(inputFileInfo.absoluteDir().absolutePath());
		if (!dissolve_.loadInput(qPrintable(inputFileInfo.fileName()))) QMessageBox::warning(this, "Input file contained errors.", "The input file failed to load correctly.\nCheck the simulation carefully, and see the messages for more details.", QMessageBox::Ok, QMessageBox::Ok);
	}
	else return Messenger::error("Input file does not exist.\n");

	// Load restart file if it exists
	Messenger::banner("Parse Restart File");
	if (!ignoreRestartFile)
	{
		CharString actualRestartFile = restartFile;
		if (actualRestartFile.isEmpty()) actualRestartFile.sprintf("%s.restart", dissolve_.inputFilename());
		
		if (DissolveSys::fileExists(actualRestartFile))
		{
			Messenger::print("\nRestart file '%s' exists and will be loaded.\n", actualRestartFile.get());
			if (!dissolve_.loadRestart(actualRestartFile)) QMessageBox::warning(this, "Restart file contained errors.", "The restart file failed to load correctly.\nSee the messages for more details.", QMessageBox::Ok, QMessageBox::Ok);

			// Reset the restart filename to be the standard one
			dissolve_.setRestartFilename(CharString("%s.restart", dissolve_.inputFilename()));
		}
		else Messenger::print("\nRestart file '%s' does not exist.\n", actualRestartFile.get());
	}
	else Messenger::print("\nRestart file (if it exists) will be ignored.\n");

	refreshing_ = true;

	ui_.MainTabs->reconcileTabs(this);

	refreshing_ = false;

	// Does a window state exist for this input file?
	stateFilename_.sprintf("%s.state", dissolve_.inputFilename());

	// Try to load in the window state file
	if (DissolveSys::fileExists(stateFilename_) && (!ignoreLayoutFile)) loadState();

	localSimulation_ = true;

	// Check the beat file
	CharString beatFile("%s.beat", dissolve_.inputFilename());
	if (DissolveSys::fileExists(beatFile))
	{
		// TODO
// 		if (
	}

	dissolveState_ = EditingState;

	// Fully update GUI
	fullUpdate();

	// Make sure we are now on the Simulation stack page
	showMainStackPage(DissolveWindow::SimulationStackPage);

	return true;
}

/*
 * System Templates
 */

// Initialise system templates from the main resource
void DissolveWindow::initialiseSystemTemplates()
{
	// Probe our main resource object for the templates, and create local data from them
	QDirIterator templateIterator(":/data/systemtemplates/");
	while (templateIterator.hasNext())
	{
		QDir dir = templateIterator.next();

		// Open the associated xml file
		SystemTemplate* sysTemp = systemTemplates_.add();
		if (!sysTemp->read(dir))
		{
			Messenger::error("Error reading the template info file '%s'.\n", qPrintable(dir.filePath("info.xml")));
			systemTemplates_.remove(sysTemp);
			continue;
		}
	}
}

/*
 * Update Functions
 */

// Update window title
void DissolveWindow::updateWindowTitle()
{
	// Window Title
	QString title = QString("Dissolve v%1 - %2%3").arg(DISSOLVEVERSION, dissolve_.hasInputFilename() ? dissolve_.inputFilename() : "<untitled>", modified_ ? "(*)" : "");
	setWindowTitle(title);

	// Update save menu item
	ui_.FileSaveAction->setEnabled(modified_);
}

// Update controls frame
void DissolveWindow::updateControlsFrame()
{
	// Update ControlFrame to reflect Dissolve's current state
	ui_.ControlRunButton->setEnabled(dissolveState_ == DissolveWindow::EditingState);
	ui_.ControlStepButton->setEnabled(dissolveState_ == DissolveWindow::EditingState);
	ui_.ControlPauseButton->setEnabled(dissolveState_ == DissolveWindow::RunningState);
	ui_.ControlReloadButton->setEnabled(dissolveState_ == DissolveWindow::MonitoringState);

	// Set current iteration number
	ui_.ControlIterationLabel->setText(CharString("%06i", dissolve_.iteration()).get());

	// Set relevant file locations
	if (localSimulation_)
	{
		localSimulationIndicator_->setPixmap(QPixmap(":/general/icons/general_local.svg"));
		restartFileIndicator_->setEnabled(dissolve_.hasRestartFilename());
		restartFileIndicator_->setToolTip(dissolve_.hasRestartFilename() ? CharString("Current restart file is '%s'", dissolve_.restartFilename()).get() : "No restart file available");
		heartbeatFileIndicator_->setEnabled(false);
		heartbeatFileIndicator_->setToolTip("Heartbeat file not monitored.");
	}
	else
	{
		localSimulationIndicator_->setPixmap(QPixmap(":/menu/icons/menu_connect.svg"));
		// TODO!
	}
}

// Update menus
void DissolveWindow::updateMenus()
{
	MainTab* activeTab = ui_.MainTabs->currentTab();
	if (!activeTab) return;

	// Species Menu
	ui_.SpeciesRenameAction->setEnabled(activeTab->type() == MainTab::SpeciesTabType);
	ui_.SpeciesDeleteAction->setEnabled(activeTab->type() == MainTab::SpeciesTabType);
	ui_.SpeciesAddForcefieldTermsAction->setEnabled(activeTab->type() == MainTab::SpeciesTabType);

	// Configuration Menu
	ui_.ConfigurationRenameAction->setEnabled(activeTab->type() == MainTab::ConfigurationTabType);
	ui_.ConfigurationDeleteAction->setEnabled(activeTab->type() == MainTab::ConfigurationTabType);
	ui_.ConfigurationExportToMenu->setEnabled(activeTab->type() == MainTab::ConfigurationTabType);

	// Layer Menu
	ui_.LayerRenameAction->setEnabled(activeTab->type() == MainTab::LayerTabType);
	ui_.LayerDeleteAction->setEnabled(activeTab->type() == MainTab::LayerTabType);
}

// Perform full update of the GUI, including tab reconciliation
void DissolveWindow::fullUpdate()
{
	refreshing_ = true;

	ui_.MainTabs->reconcileTabs(this);
	ui_.MainTabs->updateAllTabs();
	updateWindowTitle();
	updateControlsFrame();
	updateMenus();

	refreshing_ = false;
}

// Update while running
void DissolveWindow::updateWhileRunning(int iterationsRemaining)
{
	refreshing_ = true;

	// Set current iteration number
	ui_.ControlIterationLabel->setText(CharString("%06i", dissolve_.iteration()).get());

	// Set ETA text if we can
	if (iterationsRemaining == -1) etaLabel_->setText("ETA: --:--:--");
	else etaLabel_->setText(QString("ETA: %1").arg(Timer::etaString(iterationsRemaining*dissolve_.iterationTime())));

	// Enable data access in Renderables, and update all tabs.
	Renderable::setSourceDataAccessEnabled(true);
	ui_.MainTabs->updateAllTabs();
	repaint();
	Renderable::setSourceDataAccessEnabled(false);

	refreshing_ = false;
}

/*
 * Stack
 */

// Set currently-visible main stack page
void DissolveWindow::showMainStackPage(DissolveWindow::MainStackPage page)
{
	ui_.MainStack->setCurrentIndex(page);

	// Enable / disable main menu items as appropriate
	ui_.SimulationMenu->setEnabled(page == DissolveWindow::SimulationStackPage);
	ui_.SpeciesMenu->setEnabled(page == DissolveWindow::SimulationStackPage);
	ui_.ConfigurationMenu->setEnabled(page == DissolveWindow::SimulationStackPage);
	ui_.LayerMenu->setEnabled(page == DissolveWindow::SimulationStackPage);
	ui_.WorkspaceMenu->setEnabled(page == DissolveWindow::SimulationStackPage);
}
