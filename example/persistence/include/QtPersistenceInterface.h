#pragma once

#include "PersistenceService.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <memory>

TRAVODDS_NAMESPACE_BEGIN
class PersistenceService;
TRAVODDS_NAMESPACE_END

/**
 * @brief Qt可视化接口类
 */
class QtPersistenceInterface : public QObject {
    Q_OBJECT

public:
    explicit QtPersistenceInterface(std::shared_ptr<TRAVODDS::PersistenceService> service, QObject* parent = nullptr);
    virtual ~QtPersistenceInterface() = default;

    std::shared_ptr<TRAVODDS::PersistenceService> getPersistenceService() const { 
        return persistence_service_; 
    }

signals:
    void writerDiscovered(const QString& topic, const QString& type);
    void readerDiscovered(const QString& topic, const QString& type);
    void dataStored(const QString& topic, qint64 dataSize);
    void historicalDataSent(const QString& topic, qint64 count);
    void statisticsUpdated();

public slots:
    void refreshData();

private:
    std::shared_ptr<TRAVODDS::PersistenceService> persistence_service_;
};
