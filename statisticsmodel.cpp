#include "statisticsmodel.h"
#include "settings.h"
#include <qnamespace.h>

StatisticsModel::StatisticsModel(const SharedStorageHandle & handle, interface currentInterface, QObject *parent)
    : QAbstractTableModel(parent),
      storageHandle_m{handle},
      currentInterface_m{currentInterface},
      timer_m{this}
{
    timer_m.setInterval(STATS_REFRESH_TIMER.count());
    connect(&timer_m, &QTimer::timeout, this, &StatisticsModel::updateStats);
    timer_m.start();
}

QVariant StatisticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return QString("protocol");
        case 1:
            return QString("in");
        case 2:
            return QString("out");
        }
    }
    return QVariant();
}

int StatisticsModel::rowCount(const QModelIndex & parent) const
{
    auto guard = storageHandle_m.guard();
    int count = 0;
    for (auto it = guard->statisticsTable.begin(); it != guard->statisticsTable.end(); it++)
    {
        if (it->first.target == currentInterface_m)
        {
            count++;
        }
    }

    return count;
}

int StatisticsModel::columnCount(const QModelIndex & parent) const
{
    return 3;
}

QVariant StatisticsModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    int currentStat = 0;
    auto guard = storageHandle_m.guard();
    for (auto it = guard->statisticsTable.begin(); it != guard->statisticsTable.end(); it++)
    {
        if (it->first.target != currentInterface_m)
        {
            continue;
        }

        if (currentStat != index.row())
        {
            currentStat++;
            continue;
        }

        switch (index.column())
        {
        case 0:
            return QVariant(QString("%1").arg(protocolToString(it->first.protocol).c_str()));
        case 1:
            return QVariant(QString("%1").arg(it->second.input));
        case 2:
            return QVariant(QString("%1").arg(it->second.output));
        default:
            qDebug("Unknown column! %d", index.column());
            return QVariant();
        }
    }
    return QVariant();
}

void StatisticsModel::updateStats()
{
    beginResetModel();
    endResetModel();
}
