#include "../include/QtPersistenceInterface.h"
#include <QTimer>
#include <QApplication>

QtPersistenceInterface::QtPersistenceInterface(std::shared_ptr<TRAVODDS::PersistenceService> service, QObject* parent)
    : QObject(parent)
    , persistence_service_(std::move(service))
{
    // 设置定时器定期刷新
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QtPersistenceInterface::refreshData);
    timer->start(1000); // 每秒刷新一次
}

void QtPersistenceInterface::refreshData() {
    if (!persistence_service_) {
        return;
    }
    
    // 发射统计更新信号
    emit statisticsUpdated();
}
