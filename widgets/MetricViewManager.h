/*!
   \file MetricViewManager.h
   \author Gregory Schultz <gregory.schultz@embarqmail.com>

   \section LICENSE
   This file is part of the Open|SpeedShop Graphical User Interface
   Copyright (C) 2010-2017 Argo Navis Technologies, LLC

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

#ifndef METRICVIEWMANAGER_H
#define METRICVIEWMANAGER_H

#include <QStackedWidget>
#include <QMetaType>

namespace Ui {
class MetricViewManager;
}


namespace ArgoNavis { namespace GUI {

enum MetricViewTypes {
    TIMELINE_VIEW = 0,
    GRAPH_VIEW,
    CALLTREE_VIEW,
    MAX_VIEW_TYPES
};

class MetricViewManager : public QStackedWidget
{
    Q_OBJECT

public:

    explicit MetricViewManager(QWidget *parent = 0);
    ~MetricViewManager();

    void unloadExperimentDataFromView(const QString& experimentName);

public slots:

    void handleSwitchView(const MetricViewTypes viewType);
    void handleMetricViewChanged(const QString &metricView);

signals:

    void signalTraceItemSelected(const QString& definingLocation, double timeBegin, double timeEnd, int rank);

private:

    Ui::MetricViewManager *ui;

    MetricViewTypes m_defaultView;

};


} // GUI
} // ArgoNavis

Q_DECLARE_METATYPE( ArgoNavis::GUI::MetricViewTypes )

#endif // METRICVIEWMANAGER_H
