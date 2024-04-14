#include "sessionsmodel.h"

using std::chrono::duration_cast, std::chrono::seconds;

SessionsModel::SessionsModel(const SharedStorageHandle & handle, QObject *parent)
    : QAbstractTableModel(parent),
    storageHandle_m{handle},
    timer_m{this}
{
    timer_m.setInterval(MAC_UPDATE_TIMER.count());
    connect(&timer_m, &QTimer::timeout, this, &SessionsModel::updateSessions);
    timer_m.start();
}

QVariant SessionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return QString("token");
        case 1:
            return QString("timeout");
        }
    }
    return QVariant();
}

int SessionsModel::rowCount(const QModelIndex & parent) const
{
    auto guard = storageHandle_m.guard();
    return guard->sessions.size();
}

int SessionsModel::columnCount(const QModelIndex & parent) const
{
    return 2;
}

QVariant SessionsModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    auto guard = storageHandle_m.guard();
    int currentRow = 0;
    for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
    {
        if (currentRow != index.row())
        {
            currentRow++;
            continue;
        }

        switch (index.column())
        {
        case 0:
            return QVariant(QString("%1").arg(it->token));
        case 1:
            return QVariant(QString("%1 s").arg(duration_cast<seconds>(it->expiration.timeLeft()).count()));
        default:
            qDebug("Unknown column! %d", index.column());
            return QVariant();
        }
    }
    return QVariant();
}

void SessionsModel::updateSessions()
{
    beginResetModel();
    endResetModel();
}
