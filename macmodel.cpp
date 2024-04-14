#include "macmodel.h"
#include "settings.h"
#include <chrono>

using std::chrono::duration_cast, std::chrono::seconds;

MacModel::MacModel(const SharedStorageHandle & handle, QObject *parent)
    : QAbstractTableModel(parent),
    storageHandle_m{handle},
    timer_m{this}
{
    timer_m.setInterval(MAC_UPDATE_TIMER.count());
    connect(&timer_m, &QTimer::timeout, this, &MacModel::updateMac);
    timer_m.start();
}

QVariant MacModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return QString("address");
        case 1:
            return QString("interface");
        case 2:
            return QString("timeout");
        }
    }
    return QVariant();
}

int MacModel::rowCount(const QModelIndex & parent) const
{
    auto guard = storageHandle_m.guard();
    return guard->macTable.size();
}

int MacModel::columnCount(const QModelIndex & parent) const
{
    return 3;
}

QVariant MacModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    auto guard = storageHandle_m.guard();
    int currentRow = 0;
    for (auto it = guard->macTable.begin(); it != guard->macTable.end(); it++)
    {
        if (currentRow != index.row())
        {
            currentRow++;
            continue;
        }

        switch (index.column())
        {
        case 0:
            return QVariant(QString("%1").arg(it->first.to_string().c_str()));
        case 1:
            return QVariant(QString("%1").arg(it->second.interface.name().c_str()));
        case 2:
            return QVariant(QString("%1 s").arg(duration_cast<seconds>(it->second.expiration.timeLeft()).count()));
        default:
            qDebug("Unknown column! %d", index.column());
            return QVariant();
        }
    }
    return QVariant();
}

void MacModel::updateMac()
{
    beginResetModel();
    endResetModel();
}
