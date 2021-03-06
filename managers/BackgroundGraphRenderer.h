/*!
   \file BackgroundGraphRenderer.h
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

#ifndef BACKGROUNDGRAPHRENDERER_H
#define BACKGROUNDGRAPHRENDERER_H

#include <QObject>
#include <QMap>
#include <QPair>
#include <QThread>
#include <QImage>

#include "common/openss-gui-config.h"

#include <ArgoNavis/CUDA/PerformanceData.hpp>

#include "UserGraphRangeChangeManager.h"

class CustomPlot;


namespace ArgoNavis { namespace GUI {


class BackgroundGraphRendererBackend;


class BackgroundGraphRenderer : public QObject
{
    Q_OBJECT

public:

    explicit BackgroundGraphRenderer(QObject *parent = 0);
    virtual ~BackgroundGraphRenderer();

    void setPerformanceData(const QString& clusteringCriteriaName, const QVector< QString >& clusterNames, const CUDA::PerformanceData& data);
    void unloadCudaViews(const QString &clusteringCriteriaName, const QStringList &clusterNames);

signals:

    void signalProcessCudaEventView();
    void signalCudaEventSnapshot(const QString& clusteringCriteriaName, const QString& clusteringName, double lower, double upper, const QImage& image);
    void createPlotForClustering(const QString& clusteringCriteriaName, const QString& clusteringName);

public slots:

    void handleGraphRangeChanged(const QString &clusteringCriteriaName, const QString& clusterName, double lower, double upper, const QSize& size);

private slots:

    void processDataTransferEvent(const QString& clusteringName,
                                  const Base::Time &time_origin,
                                  const CUDA::DataTransfer &details);
    void processKernelExecutionEvent(const QString& clusteringName,
                                     const Base::Time &time_origin,
                                     const CUDA::KernelExecution &details);
    void handleProcessCudaEventViewDone();
    void handleCreatePlotForClustering(const QString& clusteringCriteriaName, const QString& clusteringName);
    void processCudaEventSnapshot();
    void handleGraphRangeChangedTimeout(const QString& clusteringCriteriaName, const QString& clusterName, double lower, double upper, const QSize& size);

private:

    void processCudaEventSnapshot(CustomPlot* plot);

private:

    QMap< QString, CustomPlot* > m_plot;

    QThread m_thread;

    QMap< QString, BackgroundGraphRendererBackend* > m_backend;

    UserGraphRangeChangeManager m_userChangeMgr;

};


} // GUI
} // ArgoNavis

#endif // BACKGROUNDGRAPHRENDERER_H
