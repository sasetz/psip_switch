#pragma once

#include "shared_storage_handle.h"
#include <QAbstractTableModel>
#include <qtimer.h>

class SessionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SessionsModel(const SharedStorageHandle & handle, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

private slots:
    void updateSessions();

private:
    mutable SharedStorageHandle storageHandle_m;
    QTimer timer_m;
};
