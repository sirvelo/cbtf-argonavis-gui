/*!
   \file MainWindow.cpp
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2016 Argo Navis Technologies, LLC

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by the
   Free Software Foundation; either version 2.1 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "managers/PerformanceDataManager.h"
#include "managers/ApplicationOverrideCursorManager.h"
#include "widgets/DerivedMetricInformationDialog.h"
#include "SourceView/SourceView.h"

#include "common/config.h"   // auto-generated config header

#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QMetaMethod>
#include <QMessageBox>
#include <QThread>
#include <QTimer>


namespace ArgoNavis { namespace GUI {


const QString OSS_QUICK_START_GUIDE_FILEPATH = QString("%1/share/doc/packages/OpenSpeedShop/doc/users_guide/OpenSpeedShop_Quick_Start_Guide.pdf").arg(OSS_CBTF_ROOT);
const QString OSS_REFERENCE_GUIDE_FILEPATH = QString("%1/share/doc/packages/OpenSpeedShop/doc/users_guide/OpenSpeedShop_Reference_Guide.pdf").arg(OSS_CBTF_ROOT);

/**
 * \brief MainWindow::MainWindow
 * @param parent - specify parent of the MainWindow instance
 *
 * Constructs a widget which is a child of parent.  If parent is 0, the new widget becomes a window.  If parent is another widget,
 * this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    ui->setupUi( this );

#if defined(HAS_CONCURRENT_PROCESSING_VIEW_DEBUG)
    qDebug() << "MainWindow::MainWindow: thread=" << QString::number((long long)QThread::currentThread(), 16);
#endif

    setStyleSheet(
                "QSplitter::handle:vertical   { height: 4px; image: url(:/images/vsplitter-handle); background-color: rgba(200, 200, 200, 80); }"
                "QSplitter::handle:horizontal { width:  4px; image: url(:/images/hsplitter-handle); background-color: rgba(200, 200, 200, 80); }"
                );

    ui->scrollArea_MetricPlotView->setBackgroundRole( QPalette::Base );

    // create show derived metrics dialog
    m_derivedMetricDialog = new DerivedMetricInformationDialog( this );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->actionLoad_OSS_Experiment, &QAction::triggered, this, &MainWindow::loadOpenSsExperiment );
    connect( ui->actionExit, &QAction::triggered, this, &MainWindow::shutdownApplication );
    connect( ui->actionView_Open_SpeedShop_Quick_Start_Guide, &QAction::triggered, this, &MainWindow::handleViewQuickStartGuide );
    connect( ui->actionView_Open_SpeedShop_Reference_Guide, &QAction::triggered, this, &MainWindow::handleViewReferenceGuide );
    connect( ui->actionShow_Derived_Metrics, &QAction::triggered, m_derivedMetricDialog, &DerivedMetricInformationDialog::show );
    connect( ui->actionAbout, &QAction::triggered, this, &MainWindow::handleAbout );
#else
    connect( ui->actionLoad_OSS_Experiment, SIGNAL(triggered(bool)), this, SLOT(loadOpenSsExperiment()) );
    connect( ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(shutdownApplication()) );
    connect( ui->actionView_Open_SpeedShop_Quick_Start_Guide, SIGNAL(triggered(bool)), this, SLOT(handleViewQuickStartGuide()) );
    connect( ui->actionView_Open_SpeedShop_Reference_Guide, SIGNAL(triggered(bool)), this, SLOT(handleViewReferenceGuide()) );
    connect( ui->actionShow_Derived_Metrics, SIGNAL(triggered(bool)), m_derivedMetricDialog, SLOT(show()) );
    connect( ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(handleAbout()) );
#endif

    // connect performance data manager signals to experiment panel slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::signalShowWarningMessage, this, &MainWindow::handleShowWarningDialog );
        connect( dataMgr, &PerformanceDataManager::loadComplete, this, &MainWindow::handleLoadComplete );
        connect( dataMgr, &PerformanceDataManager::addExperiment, ui->widget_ExperimentPanel, &ExperimentPanel::handleAddExperiment );
        connect( ui->widget_ExperimentPanel, &ExperimentPanel::signalSelectedClustersChanged, dataMgr, &PerformanceDataManager::signalSelectedClustersChanged );
        connect( dataMgr, &PerformanceDataManager::metricViewRangeChanged, ui->widget_MetricTableView, &PerformanceDataMetricView::handleRangeChanged );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalClearSourceView, ui->widget_SourceCodeViewer, &SourceView::handleClearSourceView );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalDisplaySourceFileLineNumber, ui->widget_SourceCodeViewer, &SourceView::handleDisplaySourceFileLineNumber );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalAddPathSubstitution, ui->widget_SourceCodeViewer, &SourceView::handleAddPathSubstitution );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalRequestMetricView, dataMgr, &PerformanceDataManager::handleRequestMetricView );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalRequestDerivedMetricView, dataMgr, &PerformanceDataManager::handleRequestDerivedMetricView );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalRequestLoadBalanceView, dataMgr, &PerformanceDataManager::handleRequestLoadBalanceView );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalRequestCalltreeView, dataMgr, &PerformanceDataManager::handleRequestMetricView );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalRequestTraceView, dataMgr, &PerformanceDataManager::handleRequestTraceView );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalRequestCompareView, dataMgr, &PerformanceDataManager::handleRequestCompareView );
        connect( dataMgr, &PerformanceDataManager::addCluster, this, &MainWindow::handleAdjustPlotViewScrollArea );
        connect( dataMgr, &PerformanceDataManager::removeCluster, this, &MainWindow::handleRemoveCluster );
        connect( dataMgr, &PerformanceDataManager::addMetricView, ui->widget_SourceCodeViewer, &SourceView::addMetricView );
        connect( dataMgr, &PerformanceDataManager::addMetricViewData, ui->widget_SourceCodeViewer, &SourceView::addMetricViewData );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalMetricViewChanged, ui->widget_SourceCodeViewer, &SourceView::handleMetricViewChanged );
        connect( dataMgr, &PerformanceDataManager::signalSetDefaultMetricView, ui->widget_MetricViewManager, &MetricViewManager::handleSwitchView );
        connect( dataMgr, &PerformanceDataManager::signalSetDefaultMetricView, this, &MainWindow::handleSetDefaultMetricView );
        connect( dataMgr, &PerformanceDataManager::addDevice, ui->widget_MetricTableView, &PerformanceDataMetricView::signalAddDevice );
        connect( dataMgr, &PerformanceDataManager::signalRequestMetricTableViewUpdate, ui->widget_MetricTableView, &PerformanceDataMetricView::handleRequestViewUpdate );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalTraceItemSelected,
                 ui->widget_MetricViewManager, &MetricViewManager::signalTraceItemSelected );
        connect( ui->widget_MetricTableView, &PerformanceDataMetricView::signalMetricViewChanged,
                 ui->widget_MetricViewManager, &MetricViewManager::handleMetricViewChanged );
#else
        connect( dataMgr, SIGNAL(signalShowWarningMessage(QString,QString)), this, SLOT(handleShowWarningDialog(QString,QString)) );
        connect( dataMgr, SIGNAL(loadComplete()), this, SLOT(handleLoadComplete()) );
        connect( dataMgr, SIGNAL(addExperiment(QString,QString,QVector<QString>,QVector<bool>,QVector<QString>)),
                 ui->widget_ExperimentPanel, SLOT(handleAddExperiment(QString,QString,QVector<QString>,QVector<bool>,QVector<QString>)) );
        connect( ui->widget_ExperimentPanel, SIGNAL(signalSelectedClustersChanged(QString,QSet<QString>)),
                 dataMgr, SIGNAL(signalSelectedClustersChanged(QString,QSet<QString>)) );
        connect( dataMgr, SIGNAL(metricViewRangeChanged(QString,QString,QString,QString,double,double)),
                 ui->widget_MetricTableView, SLOT(handleRangeChanged(QString,QString,QString,QString,double,double)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalClearSourceView()), ui->widget_SourceCodeViewer, SLOT(handleClearSourceView()) );
        connect( ui->widget_MetricTableView, SIGNAL(signalDisplaySourceFileLineNumber(QString,int)), ui->widget_SourceCodeViewer, SLOT(handleDisplaySourceFileLineNumber(QString,int)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalAddPathSubstitution(int,QString,QString)), ui->widget_SourceCodeViewer, SLOT(handleAddPathSubstitution(int,QString,QString)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalRequestMetricView(QString,QString,QString)),
                 dataMgr, SLOT(handleRequestMetricView(QString,QString,QString)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalRequestDerivedMetricView(QString,QString,QString)),
                 dataMgr, SLOT(handleRequestDerivedMetricView(QString,QString,QString)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalRequestLoadBalanceView(QString,QString,QString)),
                 dataMgr, SLOT(handleRequestLoadBalanceView(QString,QString,QString)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalRequestCalltreeView(QString,QString,QString)),
                 dataMgr, SLOT(handleRequestMetricView(QString,QString,QString)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalRequestTraceView(QString,QString,QString)),
                 dataMgr, SLOT(handleRequestTraceView(QString,QString,QString)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalRequestCompareView(QString,QString,QString,QString)), dataMgr, SLOT(handleRequestCompareView(QString,QString,QString,QString)) );
        connect( dataMgr, SIGNAL(addCluster(QString,QString,double,double,bool,double,double)), this, SLOT(handleAdjustPlotViewScrollArea(QString,QString)) );
        connect( dataMgr, SIGNAL(removeCluster(QString,QString)), this, SLOT(handleRemoveCluster(QString,QString)) );
        connect( dataMgr, SIGNAL(addMetricView(QString,QString,QString,QString,QStringList)),
                 ui->widget_SourceCodeViewer, SIGNAL(addMetricView(QString,QString,QString,QString,QStringList)) );
        connect( dataMgr, SIGNAL(addMetricViewData(QString,QString,QString,QString,QVariantList,QStringList)),
                 ui->widget_SourceCodeViewer, SIGNAL(addMetricViewData(QString,QString,QString,QString,QVariantList,QStringList)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalMetricViewChanged(QString)),
                 ui->widget_SourceCodeViewer, SLOT(handleMetricViewChanged(QString)) );
        connect( dataMgr, SIGNAL(signalSetDefaultMetricView(MetricViewTypes,bool,bool,bool,bool,bool)),
                 ui->widget_MetricViewManager, SLOT(handleSwitchView(MetricViewTypes)) );
        connect( dataMgr, SIGNAL(signalSetDefaultMetricView(MetricViewTypes,bool,bool,bool,bool,bool)),
                 this, SLOT(handleSetDefaultMetricView(MetricViewTypes,bool,bool,bool,bool,bool)) );
        connect( dataMgr, SIGNAL(addDevice(quint32,quint32,NameValueList,NameValueList)), ui->widget_MetricTableView, SIGNAL(signalAddDevice(quint32,quint32,NameValueList,NameValueList)) );
        connect( dataMgr, SIGNAL(signalRequestMetricTableViewUpdate(bool)), ui->widget_MetricTableView, SLOT(handleRequestViewUpdate(bool)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalTraceItemSelected(QString,double,double,int)),
                 ui->widget_MetricViewManager, SIGNAL(signalTraceItemSelected(QString,double,double,int)) );
        connect( ui->widget_MetricTableView, SIGNAL(signalMetricViewChanged(QString)),
                 ui->widget_MetricViewManager, SLOT(handleMetricViewChanged(QString)) );
#endif
    }

    // Verify that Open|SpeedShop Quick Start Guide exists; otherwise disable the Help menu item for viewing it
    QFileInfo fileInfo1( OSS_QUICK_START_GUIDE_FILEPATH );
    if ( ! fileInfo1.exists() ) {
        ui->actionView_Open_SpeedShop_Reference_Guide->setEnabled( false );
    }

    // Verify that Open|SpeedShop Reference Guide exists; otherwise disable the Help menu item for viewing it
    QFileInfo fileInfo2( OSS_REFERENCE_GUIDE_FILEPATH );
    if ( ! fileInfo2.exists() ) {
        ui->actionView_Open_SpeedShop_Quick_Start_Guide->setEnabled( false );
    }
}

/**
 * \brief MainWindow::~MainWindow
 *
 * Destroys the MainWindow instance.
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::setExperimentDatabase
 * @param filename - the experiment database filename (.openss)
 *
 * This method is called when the user had specified an experiment database filename on the command-line
 * using the '-f' command-line option.  Once this method sets the filename, when the show event occurs the
 * filename will be used to call the method to load the experimeent database.
 */
void MainWindow::setExperimentDatabase(const QString &filename)
{
    QFileInfo fileInfo( filename );

    if ( fileInfo.exists() && Experiment::isAccessible( filename.toUtf8().data() ) )
        m_filename = filename;
    else
        m_filename = QString();
}

/**
 * @brief MainWindow::showEvent
 * @param event - the show event
 *
 * This method overrides the QMainWindow::showEvent implementation to load an experiment
 * database if one was specified on the command-line when the application was run.
 */
void MainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED( event )

    if ( ! m_filename.isEmpty() ) {
        loadExperimentDatabase( m_filename );

        m_filename.clear();
    }
}

/**
 * @brief MainWindow::loadOpenSsExperiment
 *
 * Action handler for loading Open|SpeedShop experiments.  Present open file dialog to user so
 * the user can browse the local file system to select the desired Open|SpeedShop experiment
 * (files with .openss extension).  The specified filename is passed to another loadOpenSsExperiment
 * method to actually load the database into the application.
 */
void MainWindow::loadOpenSsExperiment()
{
    QString filePath;

    if ( m_lastFilePath.isEmpty() )
        // upon initial application launch use the application root directory
        filePath = QApplication::applicationDirPath();
    else {
        // then use the directory path in the last filename that was opened
        filePath = m_lastFilePath;
    }

    filePath = QFileDialog::getOpenFileName( this, tr("Open File"), filePath, "*.openss" );

    if ( filePath.isEmpty() )
        return;

    loadExperimentDatabase( filePath );
}

/**
 * @brief MainWindow::loadOpenSsExperiment
 * @param filepath - the experiment database to load
 *
 * The specified filename is passed to the performance data manager in order to parse the experiment
 * database into local data structures for viewing by the performance data view.  Add the experiment
 * loaded to the unload Open|SpeedShop experiment menu.
 */
void MainWindow::loadExperimentDatabase(const QString& filepath)
{
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( ! dataMgr )
        return;

    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->startWaitingOperation( QStringLiteral("load-experiment" ) );
    }

    // enable the metric mode in the Metric Table View
    ui->widget_MetricTableView->setAvailableMetricModes( PerformanceDataMetricView::METRIC_MODE );

    QByteArray normalizedSignature = QMetaObject::normalizedSignature( "asyncLoadCudaViews(QString)" );
    int methodIndex = dataMgr->metaObject()->indexOfMethod( normalizedSignature );
    if ( -1 != methodIndex ) {
        QMetaMethod method = dataMgr->metaObject()->method( methodIndex );
        method.invoke( dataMgr, Qt::QueuedConnection, Q_ARG( QString, filepath ) );
    }

    addUnloadOpenSsExperimentMenuItem( filepath );

    QFileInfo fileInfo( filepath );
    m_lastFilePath = fileInfo.absolutePath();

#if defined(HAS_OSSCUDA2XML)
    dataMgr->xmlDump( filepath );
#endif
}

/**
 * @brief MainWindow::addUnloadMenuItem
 * @param filePath - filepath of experiment to be added to unload menu
 *
 * Added experiment loaded to the unload menu.
 */
void MainWindow::addUnloadOpenSsExperimentMenuItem(const QString& filePath)
{
    QFileInfo fileInfo( filePath );
    QString expName( fileInfo.fileName() );
    expName.replace( QString(".openss"), QString("") );

    // Add menu item to allow unloading the loaded experiment.
    // NOTE: Unload menu takes ownership of the returned QAction.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    ui->menuUnload_OSS_Experiment->addAction( expName, this, &MainWindow::unloadOpenSsExperiment );
#else
    ui->menuUnload_OSS_Experiment->addAction( expName, this, SLOT(unloadOpenSsExperiment()) );
#endif
    ui->menuUnload_OSS_Experiment->setEnabled( true );
    ui->actionLoad_OSS_Experiment->setDisabled( true );
}

/**
 * @brief MainWindow::unloadOpenSsExperiment
 *
 * Action handler for unloading Open|SpeedShop experiments.  Present confirmation dialog to user so
 * the user can acknowledge unload action.  If the confirmation is acknowledged, then the performance data manager is
 * invoked to remove related data from local data structures and viewing in the
 * performance data view.  Remove the experiment from the unload Open|SpeedShop experiment menu.
 */
void MainWindow::unloadOpenSsExperiment()
{
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( ! action )
        return;

    if ( QMessageBox::Yes == QMessageBox::question( this, tr("Unload Experiment"), tr("Are you sure that you want to unload this experiment?"), QMessageBox::Yes | QMessageBox::No ) ) {

        ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
        if ( cursorManager ) {
            cursorManager->startWaitingOperation( QStringLiteral("unload-experiment" ) );
        }

        QString expName( action->text() );

        ui->widget_MetricViewManager->unloadExperimentDataFromView( expName );

        ui->widget_MetricTableView->deleteAllModelsViews();

        ui->widget_ExperimentPanel->handleRemoveExperiment( expName );

        ui->widget_SourceCodeViewer->reset();

        ui->menuUnload_OSS_Experiment->removeAction( action );
        ui->menuUnload_OSS_Experiment->setDisabled( true );
        ui->actionLoad_OSS_Experiment->setEnabled( true );

        m_plotsMap.clear();

        if ( cursorManager ) {
            cursorManager->finishWaitingOperation( QStringLiteral("unload-experiment" ) );
        }
    }
}

/**
 * @brief MainWindow::handleLoadComplete
 *
 * Handles special processing after completion of task to load experiment database.
 *
 * Reset override cursor.
 */
void MainWindow::handleLoadComplete()
{
    ApplicationOverrideCursorManager* cursorManager = ApplicationOverrideCursorManager::instance();
    if ( cursorManager ) {
        cursorManager->finishWaitingOperation( QStringLiteral("load-experiment" ) );
    }
}

/**
 * @brief MainWindow::handleAdjustPlotViewScrollArea
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @param clusterName - the cluster group name
 *
 * This method is called as plots are added to the plot view and re-adjusts the fixed height of the
 * plot view widget so that the widget grows in height appropriately and causes the scroll area to
 * activate the vertical scroll bar as needed to be able to see all the plots.
 */
void MainWindow::handleAdjustPlotViewScrollArea(const QString& clusteringCriteriaName, const QString& clusterName)
{
    const QString key( clusteringCriteriaName + QStringLiteral("-") + clusterName );

    m_plotsMap.insert( key );

    if ( clusteringCriteriaName != clusterName ) {
        ui->widget_MetricViewManager->setFixedHeight( m_plotsMap.size() * 150 );
    }
}

/**
 * @brief MainWindow::handleRemoveCluster
 * @param clusteringCriteriaName - the clustering criteria name associated with the cluster group
 * @param clusterName - the cluster group name
 *
 * This method is called as plots are removed from the plot view and reduces the fixed height appropriately
 * in accordance with the remaining number of plots in the plot view.
 */
void MainWindow::handleRemoveCluster(const QString &clusteringCriteriaName, const QString &clusterName)
{
    const QString key( clusteringCriteriaName + QStringLiteral("-") + clusterName );

    m_plotsMap.remove( key );

    if ( clusteringCriteriaName != clusterName ) {
        const int height( m_plotsMap.size() * 150 );
        if ( height > 0 ) {
            ui->widget_MetricViewManager->setFixedHeight( height );
        }
        else {
            ui->widget_MetricViewManager->setFixedHeight( QWIDGETSIZE_MAX );
        }
    }
}

/**
 * @brief MainWindow::handleSetDefaultMetricView
 * @param view - the default view
 * @param hasDerivedMetrics - specified whether to activate derived metric views
 * @param hasCompareViews - specifies whether to activate compare views
 * @param hasLoadBalanceViews - specifies whether to activate load balance views
 * @param hasTraceViews - specifies whether to activate trace views
 * @param hasCallTreeViews - specifies whether to activate calltree views
 *
 * The choice of default view is used to determine which modes the user can choose from in the Metric Table View.
 */
void MainWindow::handleSetDefaultMetricView(const MetricViewTypes &view, bool hasDerivedMetrics, bool hasCompareViews, bool hasLoadBalanceViews, bool hasTraceViews, bool hasCallTreeViews)
{
    // define views always present
    PerformanceDataMetricView::ModeTypes modes( PerformanceDataMetricView::METRIC_MODE );

    if ( hasDerivedMetrics ) {
        // activate derived metric views
        modes |= PerformanceDataMetricView::DERIVED_METRIC_MODE;
    }

    if ( hasCompareViews ) {
        // activate compare views
        modes |= PerformanceDataMetricView::COMPARE_MODE;
        modes |= PerformanceDataMetricView::COMPARE_BY_RANK_MODE;
        modes |= PerformanceDataMetricView::COMPARE_BY_HOST_MODE;
        modes |= PerformanceDataMetricView::COMPARE_BY_PROCESS_MODE;
    }

    if ( hasLoadBalanceViews ) {
        // activate load balance views
        modes |= PerformanceDataMetricView::LOAD_BALANCE_MODE;
    }

    // may only activate either trace or details views or neither
    if ( hasTraceViews ) {
        // activate trace views
        modes |= PerformanceDataMetricView::TRACE_MODE;
    }
    else if ( TIMELINE_VIEW == view ) {
        // activate details views
        modes |= PerformanceDataMetricView::DETAILS_MODE;
    }

    if ( hasCallTreeViews || CALLTREE_VIEW == view ) {
        // activate calltree view
        modes |= PerformanceDataMetricView::CALLTREE_MODE;
    }

    ui->widget_MetricTableView->setAvailableMetricModes( modes );
}

/**
 * @brief MainWindow::shutdownApplication
 *
 * Action handler for terminating the application.
 */
void MainWindow::shutdownApplication()
{
    qApp->quit();
}

/**
 * @brief MainWindow::handleViewQuickStartGuide
 *
 * Uses Qt QDesktopServices::openUrl to open the application associated with the file type
 * for the Open|SpeedShop Quick Start Guide document.  The file type is expected to be PDF.
 */
void MainWindow::handleViewQuickStartGuide()
{
    QDesktopServices::openUrl( QUrl("file://" + OSS_QUICK_START_GUIDE_FILEPATH ) );
}

/**
 * @brief MainWindow::handleViewReferenceGuide
 *
 * Uses Qt QDesktopServices::openUrl to open the application associated with the file type
 * for the Open|SpeedShop Reference Guide document.  The file type is expected to be PDF.
 */
void MainWindow::handleViewReferenceGuide()
{
    QDesktopServices::openUrl( QUrl("file://" + OSS_REFERENCE_GUIDE_FILEPATH ) );
}

/**
 * @brief MainWindow::handleShowWarningDialog
 * @param title - title of the warning dialog
 * @param message - the message to be displayed
 *
 * This method will display a warning QMessageDialog with the indicated dialog title and message.  A timer is setup
 * to automatically close the dialog after 10 seconds if the user hadn't.
 */
void MainWindow::handleShowWarningDialog(const QString& title, const QString& message)
{
    QMessageBox* dialog = new QMessageBox( Q_NULLPTR );
    dialog->setWindowTitle( title );
    dialog->setIcon( QMessageBox::Information );
    dialog->setText( QString("<font size=\"8\">%1</font>").arg( title ) );
    dialog->setInformativeText( message );
    dialog->addButton( QMessageBox::Ok );
    dialog->setAttribute( Qt::WA_DeleteOnClose, true );
    dialog->setContentsMargins( 20, 10, 40, 10 );

    QTimer* timer = new QTimer;
    timer->setSingleShot( true );
    connect( timer, SIGNAL(timeout()), timer, SLOT(deleteLater()) );
    connect( timer, SIGNAL(timeout()), dialog, SLOT(close()) );

    dialog->show();
    timer->start( 10000 );
}

/**
 * @brief MainWindow::handleAbout
 *
 * Opens a dialog that displays information about the Open|SpeedShop GUI.
 */
void MainWindow::handleAbout()
{
    QMessageBox msgBox( QMessageBox::Information,
                        tr("About Open|SpeedShop GUI"),
                        tr("<html>"
                           "    <style type=\"text/css\">"
                           "        .text1 { font-size: 24pt; font-family: Verdana; color: #efefef; }"
                           "        .text2 { font-size: 16pt; font-family: Arial; color: #efefef; }"
                           "        .text3 { font-size: 12pt; font-family: Fixed; color: #efefef; }"
                           "    </style>"
                           "    <div class=\"text1\"><p align=\"center\">The <font color=\"#40a0ae\">Open</font><font color=\"#404040\">|</font><font color=\"#0030d8\">SpeedShop</font> GUI</p></div>"
                           "    <div class=\"text2\"><p align=\"center\">Version: %1.%2.%3</p></div>"
                           "    <div class=\"text2\"><p align=\"center\">Developed by Gregory L Schultz</p></div>"
                           "    <div class=\"text3\"><p align=\"center\">See <a href=\"http://www.openspeedshop.org\">http://www.openspeedshop.org</a> for more information on Open|SpeedShop</p></div>"
                           "</html>").arg(APP_MAJOR_VERSION).arg(APP_MINOR_VERSION).arg(APP_SUBMINOR_VERSION) );

    QPalette palette;
    palette.setBrush( QPalette::Background, QBrush( qRgb(100,100,100) ) );
    msgBox.setPalette( palette );
    msgBox.setIcon( QMessageBox::NoIcon );

    msgBox.exec();
}

} // GUI
} // ArgoNavis
