#-------------------------------------------------
#
# Project created by QtCreator 2016-03-30T18:41:01
#
#-------------------------------------------------

!greaterThan(QT_VERSION, "4.6.2"): error("Minimum required version of Qt is 4.6.3")

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent printsupport

DEFINES += APP_MAJOR_VERSION=1
DEFINES += APP_MINOR_VERSION=0
DEFINES += APP_SUBMINOR_VERSION=0
DEFINES += APP_BUILD_VERSION=0000

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
else: QMAKE_CXXFLAGS += -std=c++11

TARGET = openss-gui
TEMPLATE = app

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
}
CONFIG(release, debug|release) {
    DESTDIR = build/release
    DEFINES += QT_NO_DEBUG_OUTPUT
}

BUILD = $$(BUILD)

# BUILD should be automatically set to x86 or x86_64 unless passed in by user.
isEmpty(BUILD) {
    BUILD = $$QMAKE_HOST.arch
}

contains(BUILD, x86_64) {
    BUILDLIB = lib64
}
else {
    BUILDLIB = lib
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

KRELL_ROOT = $$(KRELL_ROOT)
CBTF_ROOT = $$(CBTF_ROOT)
OSS_CBTF_ROOT = $$(OSS_CBTF_ROOT)
GRAPHVIZ_ROOT = $$(GRAPHVIZ_ROOT)
BOOST_ROOT = $$(BOOST_ROOT)
BOOST_LIB_DIR = $$(BOOST_LIB_DIR)

CONFIG_H = $$quote(common/config.h)

config.target = $$CONFIG_H
config.commands = $$system("sh ./generate-config.sh $$OSS_CBTF_ROOT $$CONFIG_H")

QMAKE_EXTRA_TARGETS += $$CONFIG_H

PRE_TARGETDEPS += $$CONFIG_H

target.path = $$(OSS_CBTF_ROOT)/bin
INSTALLS += target

LIBS += -L$$KRELL_ROOT/$$BUILDLIB
LIBS += -Wl,-rpath $$KRELL_ROOT/$$BUILDLIB
LIBS += -lxerces-c-3.1 -lxplat
LIBS += -lmrnet

INCLUDEPATH += $$CBTF_ROOT/include
LIBS += -L$$CBTF_ROOT/$$BUILDLIB
LIBS += -largonavis-base -largonavis-cuda

CBTF_MESSAGES_PATH = $$CBTF_ROOT
include(CBTF-Messages.pri)

LIBOPENSS_INC = $$OSS_CBTF_ROOT
OPENSS_PATH = $$OSS_CBTF_ROOT
include(OpenSS-CLI.pri)

INCLUDEPATH += $$CBTF_ROOT/include/collectors

DEFINES += HAS_METRIC_TYPES
#DEFINES += USE_DISCRETE_SAMPLES
DEFINES += USE_PERIODIC_SAMPLE_AVG
DEFINES += HAS_PARALLEL_PROCESS_METRIC_VIEW
#DEFINES += HAS_PARALLEL_PROCESS_METRIC_VIEW_DEBUG
DEFINES += HAS_DESTROY_SINGLETONS
#DEFINES += HAS_ITEM_CLICK_DEBUG
#DEFINES += HAS_PROCESS_METRIC_VIEW_DEBUG
DEFINES += HAS_STRIP_DOMAIN_NAME
#DEFINES += HAS_REAL_SAMPLE_COUNTER_NAME
# uncommenting this is experimental
# NOTE: Normally QWidgets cannot be moved to a thread but when used in this context invoking a method executed with QtConcurrent::run
#       seems to be working.  Need to investigate this approach further.
DEFINES += HAS_EXPERIMENTAL_CONCURRENT_PLOT_TO_IMAGE
DEFINES += HAS_CONCURRENT_PROCESSING_VIEW_DEBUG
#DEFINES += HAS_TIMER_THREAD_DESTROYED_CHECKING

message("BOOST_ROOT="$$BOOST_ROOT)
INCLUDEPATH += $$BOOST_ROOT/include
exists($$BOOST_LIB_DIR): LIBS += -L$$BOOST_LIB_DIR/lib
else: LIBS += -L$$BOOST_ROOT/lib
LIBS += -lboost_system -lboost_thread -lboost_program_options
message("LIBS="$$LIBS)
LIBS += -lboost_date_time -lboost_filesystem -lboost_unit_test_framework
LIBS += -lgomp

QCUSTOMPLOTDIR = $$PWD/QCustomPlot
INCLUDEPATH += $$QCUSTOMPLOTDIR

INCLUDEPATH += $$GRAPHVIZ_ROOT/include/graphviz
LIBS += -L$$GRAPHVIZ_ROOT/lib -lcdt -lgvc -lcgraph

INCLUDEPATH += $$CBTF_ROOT/include/QtGraph
CONFIG(debug, debug|release) {
LIBS += -L$$CBTF_ROOT/lib64/$$QT_VERSION -lQtGraphd
}
CONFIG(release, debug|release) {
LIBS += -L$$CBTF_ROOT/lib64/$$QT_VERSION -lQtGraph
}

message("LD_LIBRARY_PATH="$$LD_LIBRARY_PATH)

SOURCES += \
    QCustomPlot/qcustomplot.cpp \
    QCustomPlot/CustomPlot.cpp \
    main/main.cpp \
    main/MainWindow.cpp \
    graphitems/OSSDataTransferItem.cpp \
    graphitems/OSSKernelExecutionItem.cpp \
    graphitems/OSSEventItem.cpp \
    graphitems/OSSPeriodicSampleItem.cpp \
    graphitems/OSSEventsSummaryItem.cpp \
    widgets/TreeItem.cpp \
    widgets/TreeModel.cpp \
    widgets/ExperimentPanel.cpp \
    widgets/PerformanceDataPlotView.cpp \
    widgets/PerformanceDataMetricView.cpp \
    SourceView/SourceView.cpp \
    SourceView/SyntaxHighlighter.cpp \
    managers/PerformanceDataManager.cpp \
    managers/BackgroundGraphRendererBackend.cpp \
    managers/BackgroundGraphRenderer.cpp \
    managers/UserGraphRangeChangeManager.cpp \
    SourceView/ModifyPathSubstitutionsDialog.cpp \
    CBTF-ArgoNavis-Ext/DataTransferDetails.cpp \
    CBTF-ArgoNavis-Ext/KernelExecutionDetails.cpp \
    widgets/ViewSortFilterProxyModel.cpp \
    CBTF-ArgoNavis-Ext/ClusterNameBuilder.cpp \
    managers/CalltreeGraphManager.cpp \
    widgets/CalltreeGraphView.cpp \
    widgets/MetricViewManager.cpp \
    widgets/MetricViewDelegate.cpp

greaterThan(QT_MAJOR_VERSION, 4): {
# uncomment the following to produce XML dump of database
#DEFINES += HAS_OSSCUDA2XML
contains(DEFINES, HAS_OSSCUDA2XML): {
    SOURCES += \
    util/osscuda2xml.cxx \
}
}

HEADERS += \
    common/openss-gui-config.h \
    QCustomPlot/qcustomplot.h \
    QCustomPlot/CustomPlot.h \
    main/MainWindow.h \
    graphitems/OSSDataTransferItem.h \
    graphitems/OSSKernelExecutionItem.h \
    graphitems/OSSEventItem.h \
    graphitems/OSSPeriodicSampleItem.h \
    graphitems/OSSEventsSummaryItem.h \
    widgets/TreeItem.h \
    widgets/TreeModel.h \
    widgets/ExperimentPanel.h \
    widgets/PerformanceDataPlotView.h \
    widgets/PerformanceDataMetricView.h \
    SourceView/SourceView.h \
    SourceView/SyntaxHighlighter.h \
    managers/PerformanceDataManager.h \
    managers/BackgroundGraphRendererBackend.h \
    managers/BackgroundGraphRenderer.h \
    managers/UserGraphRangeChangeManager.h \
    SourceView/ModifyPathSubstitutionsDialog.h \
    CBTF-ArgoNavis-Ext/DataTransferDetails.h \
    CBTF-ArgoNavis-Ext/KernelExecutionDetails.h \
    widgets/ViewSortFilterProxyModel.h \
    CBTF-ArgoNavis-Ext/ClusterNameBuilder.h \
    managers/CalltreeGraphManager.h \
    widgets/CalltreeGraphView.h \
    widgets/MetricViewManager.h \
    widgets/MetricViewDelegate.h

FORMS += main/mainwindow.ui \
    widgets/PerformanceDataPlotView.ui \
    widgets/PerformanceDataMetricView.ui \
    SourceView/ModifyPathSubstitutionsDialog.ui \
    widgets/MetricViewManager.ui

RESOURCES += \
    openss-gui.qrc
