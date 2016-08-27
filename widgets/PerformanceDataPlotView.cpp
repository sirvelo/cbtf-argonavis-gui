/*!
   \file PerformanceDataPlotView.cpp
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

#include "PerformanceDataPlotView.h"
#include "ui_PerformanceDataPlotView.h"

#include "managers/PerformanceDataManager.h"
#include "common/openss-gui-config.h"

#include "graphitems/OSSDataTransferItem.h"
#include "graphitems/OSSKernelExecutionItem.h"
#include "graphitems/OSSPeriodicSampleItem.h"
#include "graphitems/OSSEventsSummaryItem.h"

#include <QtGlobal>
#include <qmath.h>
#include <QPen>
#include <QInputDialog>


namespace ArgoNavis { namespace GUI {


/**
 * @brief PerformanceDataPlotView::PerformanceDataPlotView
 * @param parent - specify parent of the PerformanceDataPlotView instance
 *
 * Constructs a widget which is a child of parent.  If parent is 0, the new widget becomes a window.  If parent is another widget,
 * this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
 */
PerformanceDataPlotView::PerformanceDataPlotView(QWidget *parent)
    : QWidget( parent )
    , ui( new Ui::PerformanceDataPlotView )
    , m_metricCount( 0 )
{
    qsrand( QDateTime::currentDateTime().toTime_t() );

    ui->setupUi( this );

    ui->graphView->plotLayout()->clear(); // remove the default axis rect

    ui->graphView->setNoAntialiasingOnDrag( true );

    ui->graphView->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom |
                                    QCP::iSelectAxes | QCP::iSelectPlottables | QCP::iSelectItems );

    // connect slot that ties some axis selections together (especially opposite axes):
    connect( ui->graphView, SIGNAL(selectionChangedByUser()), this, SLOT(handleSelectionChanged()) );

    // connect some interaction slots:
    connect( ui->graphView, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(handleAxisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)) );

    // connect slot when a graph is clicked:
    connect( ui->graphView, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(handleGraphClicked(QCPAbstractPlottable*)) );

    // connect slot when an item is clicked
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    connect( ui->graphView, &QCustomPlot::itemClick, this, &PerformanceDataPlotView::handleItemClick );
#else
    connect( ui->graphView, SIGNAL(itemClick(QCPAbstractItem*,QMouseEvent*)), this, SLOT(handleItemClick(QCPAbstractItem*,QMouseEvent*)) );
#endif

    // connect performance data manager signals to performance data view slots
    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( dataMgr, &PerformanceDataManager::addCluster, this, &PerformanceDataPlotView::handleAddCluster, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::setMetricDuration, this, &PerformanceDataPlotView::handleSetMetricDuration, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::addDataTransfer, this, &PerformanceDataPlotView::handleAddDataTransfer, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::addKernelExecution, this, &PerformanceDataPlotView::handleAddKernelExecution, Qt::QueuedConnection );
        connect( dataMgr, &PerformanceDataManager::addPeriodicSample, this, &PerformanceDataPlotView::handleAddPeriodicSample, Qt::QueuedConnection );
        connect( ui->graphView, &QCustomPlot::afterReplot, dataMgr, &PerformanceDataManager::replotCompleted );
        connect( dataMgr, &PerformanceDataManager::addCudaEventSnapshot, this, &PerformanceDataPlotView::handleCudaEventSnapshot, Qt::QueuedConnection );
        connect( this, &PerformanceDataPlotView::graphRangeChanged, dataMgr, &PerformanceDataManager::graphRangeChanged );
#else
        connect( dataMgr, SIGNAL(addCluster(QString,QString)), this, SLOT(handleAddCluster(QString,QString)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(setMetricDuration(QString,QString,double)), this, SLOT(handleSetMetricDuration(QString,QString,double)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(addDataTransfer(QString,QString,Base::Time,CUDA::DataTransfer)), this, SLOT(handleAddDataTransfer(QString,QString,Base::Time,CUDA::DataTransfer)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(addKernelExecution(QString,QString,Base::Time,CUDA::KernelExecution)), this, SLOT(handleAddKernelExecution(QString,QString,Base::Time,CUDA::KernelExecution)), Qt::QueuedConnection );
        connect( dataMgr, SIGNAL(addPeriodicSample(QString,QString,double,double,double)), this, SLOT(handleAddPeriodicSample(QString,QString,double,double,double)), Qt::QueuedConnection );
        connect( ui->graphView, SIGNAL(afterReplot()), dataMgr, SIGNAL(replotCompleted()) );
        connect( dataMgr, SIGNAL(addCudaEventSnapshot(const QString&,const QString&,double,double,const QImage&)),
                                 this, SLOT(handleCudaEventSnapshot(const QString&,const QString&,double,double,const QImage&)), Qt::QueuedConnection );
        connect( this, SIGNAL(graphRangeChanged(QString,double,double,QSize)), dataMgr, SIGNAL(graphRangeChanged(QString,double,double,QSize)) );
#endif
    }
}

/**
 * @brief PerformanceDataPlotView::~PerformanceDataPlotView
 *
 * Destroys the PerformanceDataPlotView instance.
 */
PerformanceDataPlotView::~PerformanceDataPlotView()
{
    delete ui;
}

/**
 * @brief PerformanceDataPlotView::unloadExperimentDataFromView
 * @param experimentName - the name of the experiment to remove from view
 *
 * Removes the given experiment data from view.
 */
void PerformanceDataPlotView::unloadExperimentDataFromView(const QString &experimentName)
{
    Q_UNUSED( experimentName );  // for now until view supports more than one experiment

    ui->graphView->clearGraphs();
    ui->graphView->clearItems();
    ui->graphView->clearPlottables();

    ui->graphView->plotLayout()->clear();

    ui->graphView->replot();

    QMutexLocker guard( &m_mutex );

    PerformanceDataManager* dataMgr = PerformanceDataManager::instance();
    if ( dataMgr ) {
        QMap< QString, MetricGroup* >::iterator iter( m_metricGroups.begin() );
        while ( iter != m_metricGroups.end() ) {
            MetricGroup* group = iter.value();
            dataMgr->unloadCudaViews( iter.key(), group->metricList );
            iter++;
        }
    }
    qDeleteAll( m_metricGroups );
    m_metricGroups.clear();

    m_metricCount = 0;
}

/**
 * @brief PerformanceDataPlotView::handleValueAxisRangeChange
 * @param requestedRange - the range of this axis due to change
 *
 * Handle changes to x axis ranges.  Make sure any range change requests made, mainly from user axis range drag and zoom actions,
 * are keep within the valid range of the metric group.  The tick locations and label vectors are computed also.
 */
void PerformanceDataPlotView::handleAxisRangeChange(const QCPRange &requestedRange)
{
    // get the sender axis
    QCPAxis* xAxis = qobject_cast< QCPAxis* >( sender() );
    QString clusterName;
    QSize size;
    double duration = getGraphInfoForMetricGroup( xAxis, clusterName, size );

    if ( 0 == size.width() || 0 == size.height() )
        return;

    // temporarily block signals for X Axis
    xAxis->blockSignals( true );

    //qDebug() << "rangeChanged: lower: " << requestedRange.lower << "upper: " << requestedRange.upper;
    // clamp requested range to 0 .. duration but minimum allowed range is 'minXspread'
    const double minXspread = 2.0;
    double upper = qMax( minXspread, qMin( duration, requestedRange.upper ) );
    double lower = qMin( upper - minXspread, qMax( 0.0, requestedRange.lower ) );

    //qDebug() << "requestedRange: lower: " << lower << "upper: " << upper;
    xAxis->setRange( lower, upper );

    emit graphRangeChanged( clusterName, lower, upper, size );

    // unblock signals for X Axis
    xAxis->blockSignals( false );

    QCPRange newRange = xAxis->range();
    //qDebug() << "newRange: lower: " << newRange.lower << "upper: " << newRange.upper;

    // Generate tick positions according to linear scaling:
    double mTickStep = newRange.size() / (double)( 10.0 + 1e-10 ); // mAutoTickCount ticks on average, the small addition is to prevent jitter on exact integers
    double magnitudeFactor = qPow( 10.0, qFloor( qLn(mTickStep) / qLn(10.0) ) ); // get magnitude factor e.g. 0.01, 1, 10, 1000 etc.
    double tickStepMantissa = mTickStep / magnitudeFactor;

    mTickStep = qCeil( tickStepMantissa ) * magnitudeFactor;
    mTickStep = qMax( 1.0, mTickStep );

    //qDebug() << "using tick step: " << mTickStep << "magnitude factor: " << magnitudeFactor << "tickStepMantissa: " << tickStepMantissa;
    xAxis->setSubTickCount( qMax( 1.0, (qreal) qCeil( tickStepMantissa ) ) - 1 );

    // Generate tick positions according to mTickStep:
    qint64 firstStep = qMax( 0.0, floor( newRange.lower / mTickStep ) ); // do not use qFloor here, or we'll lose 64 bit precision
    qint64 lastStep = qMin( duration, ceil( newRange.upper / mTickStep ) ); // do not use qCeil here, or we'll lose 64 bit precision

    int tickcount = qMax( 0LL, lastStep - firstStep + 1 );

    QVector< double > mTickVector;
    QVector< QString > mTickLabelVector;

    mTickVector.resize( tickcount );
    mTickLabelVector.resize( tickcount );

    for ( int i=0; i<tickcount; ++i ) {
        double tickValue = ( firstStep + i ) * mTickStep;
        mTickVector[i] = tickValue;
        double tickLabelValue = tickValue;
#if defined(USE_DISCRETE_SAMPLES)
        tickLabelValue *= 10.0;
#endif
        mTickLabelVector[i] = QString::number( tickLabelValue, 'f', 0 );
    }

    xAxis->setTickVector( mTickVector );
    xAxis->setTickVectorLabels( mTickLabelVector );
}

/**
 * @brief PerformanceDataPlotView::handleAxisRangeChangeForMetricGroup
 * @param requestedRange - the range of this axis due to change
 *
 * When the range of one axis of a metric group has changed, then all other related axes must have the same range.
 */
void PerformanceDataPlotView::handleAxisRangeChangeForMetricGroup(const QCPRange &requestedRange)
{
    // get the sender axis
    QCPAxis* senderAxis = qobject_cast< QCPAxis* >( sender() );

    // get metric group the axis is associated with
    QVariant metricGroupVar = senderAxis->property( "associatedMetricGroup" );

    if ( ! metricGroupVar.isValid() )
        return;

    QString metricGroupName = metricGroupVar.toString();

    // get list of axes in metric group
    QList< QCPAxis* > axes = getAxesForMetricGroup( QCPAxis::atBottom, metricGroupName );

    // remove the sending axis from list
    axes.removeAll( senderAxis );

    // set the range for the other axes in the metric group
    foreach( QCPAxis* axis, axes ) {
        axis->setRange( requestedRange );
    }
}

/**
 * @brief PerformanceDataPlotView::handleAxisLabelDoubleClick
 * @param axis - the axis that received the click
 * @param part - the part of the axis that was clicked
 *
 * Handler to process QCustomPlot::axisDoubleClick signal resulting when an axis is double clicked.
 */
void PerformanceDataPlotView::handleAxisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
    // Set an axis label by double clicking on it
    if ( part == QCPAxis::spAxisLabel ) { // only react when the actual axis label is clicked, not tick label or axis backbone
        bool ok;
        QString newLabel = QInputDialog::getText( this, "Performance Data View", "New axis label:", QLineEdit::Normal, axis->label(), &ok );
        if ( ok ) {
            axis->setLabel( newLabel );
            ui->graphView->replot();
        }
    }
}

/**
 * @brief PerformanceDataPlotView::handleSelectionChanged
 *
 * Process graph item or plottable selection changes.
 */
void PerformanceDataPlotView::handleSelectionChanged()
{
    //qDebug() << "plottable count=" <<  ui->graphView->plottableCount() << "item count=" << ui->graphView->itemCount();

    // synchronize selection of graphs with selection of corresponding legend items:
    for ( int i=0; i< ui->graphView->plottableCount(); ++i ) {
        QCPAbstractPlottable* graph =  ui->graphView->plottable(i);
        QCPPlottableLegendItem *item =  ui->graphView->legend->itemWithPlottable( graph );
        //qDebug() << "graph: " << i << " graph selected: " << graph->selected() << " legend item selected: " << item->selected();
        item->setSelected( item->selected() );
        graph->setSelected( graph->selected() );
    }
}

/**
 * @brief PerformanceDataPlotView::handleGraphClicked
 * @param plottable - the plottable that was clicked
 *
 * Handle specific logic when graph clicked.
 */
void PerformanceDataPlotView::handleGraphClicked(QCPAbstractPlottable *plottable)
{
    Q_UNUSED( plottable )
}

/**
 * @brief PerformanceDataPlotView::handleItemClick
 * @param item - the item that received the click
 * @param event - the mouse event that caused the click
 *
 * Handle the user clicking an item in the graph.  Present overlay popup with more detailed
 * information of the graph item represented by the item that was clicked.
 */
void PerformanceDataPlotView::handleItemClick(QCPAbstractItem *item, QMouseEvent *event)
{
    Q_UNUSED( event );
#ifdef HAS_ITEM_CLICK_DEBUG
    QString text;
    QTextStream output( &text );
#endif

    OSSDataTransferItem* dataXferItem = qobject_cast< OSSDataTransferItem* >( item );
    if ( dataXferItem ) {
#ifdef HAS_ITEM_CLICK_DEBUG
        output << "Data Transfer: " << *dataXferItem;
#endif
    }
    else {
        OSSKernelExecutionItem* kernelExecItem = qobject_cast< OSSKernelExecutionItem* >( item );
        if ( kernelExecItem ) {
#ifdef HAS_ITEM_CLICK_DEBUG
            output << "Kernel Execution: " << *kernelExecItem;
#endif
        }
        else {
            OSSPeriodicSampleItem* sampleItem = qobject_cast< OSSPeriodicSampleItem* >( item );
            if ( sampleItem ) {

            }
        }
    }

#ifdef HAS_ITEM_CLICK_DEBUG
    qDebug() << "PerformanceDataPlotView::handleItemClick: " << text;
#endif
}

/**
 * @brief PerformanceDataPlotView::handleCudaEventSnapshot
 * @param clusteringCriteriaName
 * @param clusteringName
 * @param lower
 * @param upper
 * @param image
 */
void PerformanceDataPlotView::handleCudaEventSnapshot(const QString& clusteringCriteriaName, const QString& clusteringName, double lower, double upper, const QImage &image)
{
    QCPAxisRect* axisRect( Q_NULLPTR );
    OSSEventsSummaryItem* eventSummaryItem( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
            QMap< QString, QCPAxisRect* >& axisRects = m_metricGroups[ clusteringCriteriaName ]->axisRects;
            if ( axisRects.contains( clusteringName ) )
                axisRect = axisRects[ clusteringName ];
            QMap< QString, OSSEventsSummaryItem* >& eventSummary = m_metricGroups[ clusteringCriteriaName ]->eventSummary;
            if ( eventSummary.contains( clusteringName ) ) {
                eventSummaryItem = eventSummary[ clusteringName ];
            }
        }
    }

    qDebug() << "PerformanceDataPlotView::handleCudaEventSnapshot CALLED: clusterName=" << clusteringName << "lower=" << lower << "upper=" << upper << "image size=" << image.size();

    if ( Q_NULLPTR == axisRect )
        return;

    bool newItem( false );
    if ( Q_NULLPTR == eventSummaryItem ) {
        eventSummaryItem = new OSSEventsSummaryItem( axisRect, ui->graphView );
        newItem = true;
    }

    if ( Q_NULLPTR == eventSummaryItem )
        return;

    eventSummaryItem->setData( lower, upper, image );

    if ( newItem ) {
        ui->graphView->addItem( eventSummaryItem );

        {
            QMutexLocker guard( &m_mutex );

            if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
                m_metricGroups[ clusteringCriteriaName ]->eventSummary.insert( clusteringName, eventSummaryItem );
            }
        }
    }
    //else {
        ui->graphView->replot( QCustomPlot::rpQueued );
    //}
}

/**
 * @brief PerformanceDataPlotView::getRange
 * @param values - vector of data for either x or y axis
 * @param sortHint - specifies if vector can be expected to be pre-sorted (increasing order)
 * @return - the QCPRange specifying the lowest and highest values in the vector
 *
 * Determine the range for the specified vector.  This range is the lowest and highest values in the vector.
 */
const QCPRange PerformanceDataPlotView::getRange(const QVector<double> &values, bool sortHint)
{
    double minValue;
    double maxValue;

    if ( sortHint ) {
        minValue = values.first();
        maxValue = values.last();
    }
    else {
        minValue = std::numeric_limits<double>::max();
        maxValue = std::numeric_limits<double>::min();

        foreach( double value, values ) {
            if ( value < minValue )
                minValue = value;
            else if ( value > maxValue )
                maxValue = value;
        }
    }

    return QCPRange( minValue, maxValue );
}

/**
 * @brief PerformanceDataPlotView::getDurationForMetricGroup
 * @param axis
 * @param clusterName
 * @return Duration for metric group
 */
double PerformanceDataPlotView::getGraphInfoForMetricGroup(const QCPAxis *axis, QString& clusterName, QSize& size)
{
    double durationResult( 0.0 );

    // get metric group the axis is associated with axis
    QVariant clusteringCriteriaNameVar = axis->property( "associatedMetricGroup" );

    if ( clusteringCriteriaNameVar.isValid() ) {
        QString clusteringCriteriaName = clusteringCriteriaNameVar.toString();

        QMutexLocker guard( &m_mutex );

        if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
            durationResult = m_metricGroups[ clusteringCriteriaName ]->duration;
            MetricGroup* group = m_metricGroups[ clusteringCriteriaName ];
            QMap< QString, QCPAxisRect* >::iterator iter( group->axisRects.begin() );
            while ( iter != group->axisRects.end() ) {
                if ( iter.value()->axis( QCPAxis::atBottom ) == axis ) {
                    clusterName = iter.key();
                    size = iter.value()->size();
                    break;
                }
                iter++;
            }
        }
    }

    return durationResult;
}

/**
 * @brief PerformanceDataPlotView::initPlotView
 * @param axisRect - initialize the x and y axis appropriate for metric graph
 *
 * Initialize desired properties for the axes of the metric graphs, including:
 * - no auto tick value and label computation
 * - font for tick labels
 * - set lower value of axis range to be zero and make the axis grid visible but no sub-grid
 * - allow dragging and zoom of axis range
 * - setup signal/slot connections to handle axis range changes for individual axis and for metric group
 */
void PerformanceDataPlotView::initPlotView(const QString &metricGroupName, QCPAxisRect *axisRect)
{
    if ( ! axisRect )
        return;

    // get x and y axes
    QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
    QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );

    // prepare x axis
    if ( xAxis ) {
        xAxis->setAutoTicks( false );
        xAxis->setAutoTickLabels( false );
        xAxis->setAutoTickStep( false );
        QFont font;
        font.setFamily( "arial" );
        font.setBold( true );
        font.setPixelSize( 12 );
        xAxis->setTickLabelFont( font );
        xAxis->setPadding( 20 ); // a bit more space to the bottom border
        xAxis->grid()->setVisible( true );

        axisRect->setRangeDragAxes( xAxis, NULL );
        axisRect->setRangeZoomAxes( xAxis, NULL );

        xAxis->grid()->setPen( QPen(QColor(140, 140, 140), 1, Qt::DotLine) );
        xAxis->grid()->setSubGridPen( QPen(QColor(80, 80, 80), 1, Qt::DotLine) );
        xAxis->grid()->setSubGridVisible( false );
        xAxis->setAutoSubTicks( false );

        // display no axis until metric activated
        xAxis->setVisible( false );

        // set X and Y axis lower range
        xAxis->setRangeLower( 0.0 );

        // set associated metric group property
        xAxis->setProperty( "associatedMetricGroup", metricGroupName );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        connect( xAxis, static_cast<void(QCPAxis::*)(const QCPRange &newRange)>(&QCPAxis::rangeChanged),
                 this, &PerformanceDataPlotView::handleAxisRangeChangeForMetricGroup );
        // connect slot to handle value axis range changes and regenerate the tick marks appropriately
        connect( xAxis, static_cast<void(QCPAxis::*)(const QCPRange &newRange)>(&QCPAxis::rangeChanged),
                 this, &PerformanceDataPlotView::handleAxisRangeChange );
#else
        connect( xAxis, SIGNAL(rangeChanged(QCPRange)), this,SLOT(handleAxisRangeChangeForMetricGroup(QCPRange)) );
        connect( xAxis, SIGNAL(rangeChanged(QCPRange)), this,SLOT(handleAxisRangeChange(QCPRange)) );
#endif
    }

    // prepare y axis
    if ( yAxis ) {
        QFont font;
        font.setFamily( "arial" );
        font.setBold( true );
        font.setPixelSize( 10 );
        yAxis->setLabelFont( font );
        yAxis->setAutoTicks( false );
        yAxis->setAutoTickLabels( false );
        yAxis->setAutoTickStep( false );
        yAxis->setPadding( 5 ); // a bit more space to the left border
        QPen gridPen;
        gridPen.setStyle( Qt::SolidLine );
        gridPen.setColor( QColor(0, 0, 0, 25) );
        yAxis->grid()->setPen( gridPen );
        yAxis->setTickPen( Qt::NoPen );

        // display no axis until metric activated
        yAxis->setVisible( false );

        // set X and Y axis lower range
        yAxis->setRangeLower( 0.0 );
    }
}

/**
 * @brief PerformanceDataPlotView::handleAddCluster
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterName - the cluster name
 *
 * Setup a graph representing a metric for a specific cluster.  If this is for a new clustering criteria, then
 * initialize the clustering critera which is a new grid layout to be added to the QCustomPlot layout.  A new axis rect
 * is created to hold the graph items and plottables to be added for this metric.  The axis rect is added to the
 * grid layout for the metric group.  A metric group has synchronized x axis ranges; thus, if one axis has been
 * dragged or zoomed, then the ranges for the other axes of the metric group will be modified to be identical.
 */
void PerformanceDataPlotView::handleAddCluster(const QString &clusteringCriteriaName, const QString &clusterName)
{
    // create axis rect for this metric
    QCPAxisRect *axisRect = new QCPAxisRect( ui->graphView );

    if ( ! axisRect )
        return;

    QMutexLocker guard( &m_mutex );

    MetricGroup* metricGroup( Q_NULLPTR );
    if ( ! m_metricGroups.contains( clusteringCriteriaName ) ) {
        metricGroup = new MetricGroup;
        QCPLayoutGrid* layout = new QCPLayoutGrid;
        ui->graphView->plotLayout()->addElement( m_metricCount++, 0, layout );
        // when there are two elements set the row spacing
        if ( ui->graphView->plotLayout()->elementCount() == 2 ) {
            ui->graphView->plotLayout()->setRowSpacing( 0 );
        }
        // make axis rects' left side line up:
        QCPMarginGroup* group = new QCPMarginGroup( ui->graphView );
        axisRect->setMarginGroup( QCP::msLeft | QCP::msRight, group );
        metricGroup->marginGroup = group;
        metricGroup->layout = layout;
        m_metricGroups[ clusteringCriteriaName ] = metricGroup;
    }
    else {
        metricGroup = m_metricGroups[ clusteringCriteriaName ];
    }

    if ( ! metricGroup && ! metricGroup->layout )
        return;

    int idx = metricGroup->axisRects.size();

    // set/update attributes of metric group
    metricGroup->layout->addElement( idx, 0, axisRect );  // add axis rect to layout
    metricGroup->axisRects[ clusterName ] = axisRect;      // save axis rect in map
    metricGroup->metricList << clusterName;                // add new metric name to metric list

    // initialize axis rect
    foreach ( QCPAxis *axis, axisRect->axes() ) {
      axis->setLayer("axes");
      axis->grid()->setLayer("grid");
    }

    // bring bottom and main axis rect closer together:
    axisRect->setAutoMargins( QCP::msLeft | QCP::msRight | QCP::msBottom );
    axisRect->setMargins( QMargins( 0, 0, 0, 0 ) );

    initPlotView( clusteringCriteriaName, axisRect );
}

/**
 * @brief PerformanceDataPlotView::setMetricDuration
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterName - the cluster name
 * @param duration - the duration of the experiment in the range [ 0 .. duration ]
 *
 * This method sets the upper value of the visible range of data in the graph view.  Also cause update of metric graph by calling QCustomPlot::replot method.
 */
void PerformanceDataPlotView::handleSetMetricDuration(const QString& clusteringCriteriaName, const QString& clusterName, double duration)
{
    QCPAxisRect* axisRect( Q_NULLPTR );

    {
        // handle references to metric group map inside this local block
        QMutexLocker guard( &m_mutex );

        if ( m_metricGroups.contains( clusteringCriteriaName ) &&
             m_metricGroups[ clusteringCriteriaName ]->axisRects.contains( clusterName ) ) {
            axisRect = m_metricGroups[ clusteringCriteriaName ]->axisRects[ clusterName ];
            if ( axisRect ) {
                m_metricGroups[ clusteringCriteriaName ]->duration = duration;
            }
        }
    }

    ui->graphView->replot();

    if ( axisRect ) {
        QCPAxis* xAxis = axisRect->axis( QCPAxis::atBottom );
        QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );

        yAxis->setLabel( clusterName );
        yAxis->setVisible( true );

        xAxis->setRangeUpper( duration );
        xAxis->setVisible( true );

        // emit signal to provide initial graph range and size
        emit graphRangeChanged( clusterName, 0, duration, axisRect->size());
    }
}

/**
 * @brief PerformanceDataPlotView::handleAddDataTransfer
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterName - the cluster name
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the data transfer event
 *
 * Find the axis rect associated with the specified metric group and metric name.  Create the data transfer graph item from the details using the associated axis rect.
 * Add graph item to QCustomPlot instance.
 */
void PerformanceDataPlotView::handleAddDataTransfer(const QString &clusteringCriteriaName, const QString &clusterName, const Base::Time &time_origin, const CUDA::DataTransfer &details)
{
    QCPAxisRect* axisRect( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
            QMap< QString, QCPAxisRect* >& axisRects = m_metricGroups[ clusteringCriteriaName ]->axisRects;
            if ( axisRects.contains( clusterName ) )
                axisRect = axisRects[ clusterName ];
        }
    }

    if ( Q_NULLPTR == axisRect )
        return;

    OSSDataTransferItem* dataXferItem = new OSSDataTransferItem( axisRect, ui->graphView );

    dataXferItem->setData( time_origin, details );

    ui->graphView->addItem( dataXferItem );
}

/**
 * @brief PerformanceDataPlotView::handleAddKernelExecution
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterName - the cluster name
 * @param time_origin - the time origin of the experiment
 * @param details - the details of the kernel execution event
 *
 * Find the axis rect associated with the specified metric group and metric name.  Create the kernel execution graph item from the details using the associated axis rect.
 * Add graph item to QCustomPlot instance.
 */
void PerformanceDataPlotView::handleAddKernelExecution(const QString &clusteringCriteriaName, const QString &clusterName, const Base::Time &time_origin, const CUDA::KernelExecution &details)
{
    QCPAxisRect* axisRect( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
            QMap< QString, QCPAxisRect* >& axisRects = m_metricGroups[ clusteringCriteriaName ]->axisRects;
            if ( axisRects.contains( clusterName ) )
                axisRect = axisRects[ clusterName ];
        }
    }

    if ( Q_NULLPTR == axisRect )
        return;

    OSSKernelExecutionItem* kernelExecItem = new OSSKernelExecutionItem( axisRect, ui->graphView );

    kernelExecItem->setData( time_origin, details );

    ui->graphView->addItem( kernelExecItem );
}

/**
 * @brief PerformanceDataPlotView::handleAddPeriodicSample
 * @param clusteringCriteriaName - the clustering criteria name
 * @param clusterName - the cluster name
 * @param time_begin - the begin time of the periodic sample (relative to time origin of the experiment)
 * @param time_end - the end time of the periodic sample (relative to time origin of the experiment)
 * @param count - the period sample counter value
 *
 * Find the axis rect associated with the specified metric group and sample counter index.  Create the periodic sample graph item from the details using the associated axis rect.
 * Add graph item to QCustomPlot instance.  Update y-axis upper range value if counter value is greater than the current y-axis upper range value.
 */
void PerformanceDataPlotView::handleAddPeriodicSample(const QString &clusteringCriteriaName, const QString& clusterName, const double &time_begin, const double &time_end, const double &count)
{
    QCPAxisRect* axisRect( Q_NULLPTR );

    {
        QMutexLocker guard( &m_mutex );

        if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
            QMap< QString, QCPAxisRect* >& axisRects = m_metricGroups[ clusteringCriteriaName ]->axisRects;
            if ( axisRects.contains( clusterName ) )
                axisRect = axisRects[ clusterName ];
        }
    }

    if ( Q_NULLPTR == axisRect )
        return;

    OSSPeriodicSampleItem* periodicSampleItem = new OSSPeriodicSampleItem( axisRect, ui->graphView );

    periodicSampleItem->setData( time_begin, time_end, count );

    ui->graphView->addItem( periodicSampleItem );

    QCPAxis* yAxis = axisRect->axis( QCPAxis::atLeft );

    if ( count > yAxis->range().upper )
        yAxis->setRangeUpper( count );
}

/**
 * @brief PerformanceDataPlotView::getAxisRectsForMetricGroup
 * @param clusteringCriteriaName - the clustering criteria name
 * @return - the list of axis rects for the metric group (if any or names a valid metric group)
 *
 * Determines the list of axis rects for the metric group, if any, or names a valid metric group.
 */
QList<QCPAxisRect *> PerformanceDataPlotView::getAxisRectsForMetricGroup(const QString &clusteringCriteriaName)
{
    QList<QCPAxisRect *> axisRects;

    QMutexLocker guard( &m_mutex );

    if ( m_metricGroups.contains( clusteringCriteriaName ) ) {
        MetricGroup* group = m_metricGroups[ clusteringCriteriaName ];

        foreach( const QString& metricName, group->metricList ) {
            if ( group->axisRects.contains( metricName ) )
                axisRects << group->axisRects.value( metricName );
        }
    }

    return axisRects;
}

/**
 * @brief PerformanceDataPlotView::sizeHint
 * @return - the recommended size for the PerformanceDataPlotView instance
 *
 * Overrides the QWidget::sizeHint method.  Returns the recommended size for the PerformanceDataPlotView instance.
 */
QSize PerformanceDataPlotView::sizeHint() const
{
    return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}

/**
 * @brief PerformanceDataPlotView::getAxisRectsForMetricGroup
 * @param axisType - the axis type desired
 * @param metricGroupName - the metric group name
 * @return - the list of matching axis instances
 *
 * Return all axes of the type specified found in the axis rects of the specified metric group.
 */
QList<QCPAxis *> PerformanceDataPlotView::getAxesForMetricGroup(const QCPAxis::AxisType axisType, const QString &metricGroupName)
{
    QList<QCPAxis *> axes;
    QList<QCPAxisRect *> axisRects;

    QMutexLocker guard( &m_mutex );

    if ( m_metricGroups.contains( metricGroupName ) ) {
        MetricGroup* group = m_metricGroups[ metricGroupName ];

        foreach( const QString& metricName, group->metricList ) {
            if ( group->axisRects.contains( metricName ) )
                axisRects << group->axisRects.value( metricName );
        }
    }

    foreach( const QCPAxisRect* axisRect, axisRects ) {
        QCPAxis* axis = axisRect->axis( axisType );
        if ( axis )
            axes << axis;
    }

    return axes;
}


} // GUI
} // ArgoNavis
