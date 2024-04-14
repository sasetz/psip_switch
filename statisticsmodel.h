#pragma once

#include "shared_storage.h"
#include "shared_storage_handle.h"
#include <QAbstractItemModel>
#include <qabstractitemmodel.h>
#include <qtimer.h>

class StatisticsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    StatisticsModel(const SharedStorageHandle & handle, interface currentInterface, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

private slots:
    void updateStats();

private:
    mutable SharedStorageHandle storageHandle_m;
    interface currentInterface_m;
    QTimer timer_m;
};
