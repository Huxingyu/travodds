#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QPoint>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtGui/QMouseEvent>
#include <memory>

#include "PersistenceService.h"

// 前向声明
class QtPersistenceInterface;

TRAVODDS_NAMESPACE_BEGIN
class PersistenceService;
TRAVODDS_NAMESPACE_END


class PersistenceMonitor : public QMainWindow {
    Q_OBJECT

public:
    explicit PersistenceMonitor(QWidget* parent = nullptr);
    ~PersistenceMonitor();

    // 设置持久化服务实例
    void setPersistenceService(std::shared_ptr<TRAVODDS::PersistenceService> service);
    
    // 启动/停止监控
    void startMonitoring();
    void stopMonitoring();

protected:
    void closeEvent(QCloseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    // 数据更新槽函数
    void updateDomainInfo();
    void updateTopicInfo();
    void updateDatabaseInfo();
    void refreshAllData();
    
    // 交互槽函数
    void onTopicSelected();

private:
    // UI初始化
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupDomainTopicTab();
    void setupDatabaseTab();
    
    // 数据更新方法
    void updateTopicTable();
    void updateDataWriterTable();
    void updateDataReaderTable();
    void updateDatabaseStats();
    void updateMemoryCacheInfo();
    
    // 样式设置
    void applyModernStyle();
    void setupIcons();
    
    // 绘制方法
    void drawRoundedBackground();
    void drawTitleBar();
    
    // 成员变量
    std::shared_ptr<TRAVODDS::PersistenceService> persistence_service_;
    std::shared_ptr<QtPersistenceInterface> qt_interface_;
    
    // 主要UI组件
    QTabWidget* main_tabs_;
    
    // 域和主题相关组件
    QWidget* domain_topic_widget_;
    QTreeWidget* domain_tree_;
    QTableWidget* topic_table_;
    QTableWidget* datawriter_table_;
    QTableWidget* datareader_table_;
    QTextEdit* topic_details_;
    
    // 数据库相关组件
    QWidget* database_widget_;
    QLabel* db_path_label_;
    QLabel* db_size_label_;
    QLabel* total_topics_label_;
    QLabel* total_samples_label_;
    QProgressBar* memory_usage_bar_;
    QProgressBar* disk_usage_bar_;
    QTableWidget* stats_table_;
    
    // 状态栏组件
    QLabel* status_label_;
    QLabel* uptime_label_;
    
    // 定时器
    QTimer* refresh_timer_;
    QTimer* uptime_timer_;
    QDateTime start_time_;
    
    // 状态变量
    bool is_monitoring_;
    int refresh_interval_ms_;
    
    // 窗口拖拽相关
    bool is_dragging_;
    QPoint drag_start_position_;
};
