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

TRAVODDS_NAMESPACE_BEGIN
class PersistenceService;
TRAVODDS_NAMESPACE_END

/**
 * @brief 持久化服务监控界面
 * 
 * 基于Qt5的现代化GUI界面，用于实时监控和管理DDS持久化服务。
 * 提供域信息、Topic状态、数据库内容、性能统计等多维度的可视化。
 */
class PersistenceMonitor : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * 
     * @param parent 父窗口指针，默认为nullptr
     */
    explicit PersistenceMonitor(QWidget* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~PersistenceMonitor();

    /**
     * @brief 设置持久化服务实例
     * 
     * 关联监控界面与持久化服务，开始数据同步。
     * 
     * @param service 持久化服务的共享指针
     */
    void setPersistenceService(std::shared_ptr<TRAVODDS::PersistenceService> service);
    
    /**
     * @brief 启动监控
     * 
     * 开始定时刷新界面数据，显示实时状态。
     */
    void startMonitoring();
    
    /**
     * @brief 停止监控
     * 
     * 停止数据刷新，保持界面当前状态。
     */
    void stopMonitoring();

protected:
    /**
     * @brief 窗口关闭事件处理
     * 
     * @param event 关闭事件
     */
    void closeEvent(QCloseEvent* event) override;
    
    /**
     * @brief 窗口绘制事件处理
     * 
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent* event) override;
    
    /**
     * @brief 鼠标按下事件处理
     * 
     * 用于实现窗口拖拽功能。
     * 
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent* event) override;
    
    /**
     * @brief 鼠标移动事件处理
     * 
     * 用于实现窗口拖拽功能。
     * 
     * @param event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent* event) override;
    
    /**
     * @brief 鼠标释放事件处理
     * 
     * 用于实现窗口拖拽功能。
     * 
     * @param event 鼠标事件
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    // 数据更新槽函数
    /**
     * @brief 更新域信息显示
     */
    void updateDomainInfo();
    
    /**
     * @brief 更新Topic信息显示
     */
    void updateTopicInfo();
    
    /**
     * @brief 更新数据库信息显示
     */
    void updateDatabaseInfo();
    
    /**
     * @brief 刷新所有数据
     */
    void refreshAllData();
    
    // 交互槽函数
    /**
     * @brief Topic选择响应
     */
    void onTopicSelected();
    
    /**
     * @brief 数据库表选择响应
     */
    void onDatabaseTableSelected();

private:
    // UI初始化
    /**
     * @brief 设置用户界面
     */
    void setupUI();
    
    /**
     * @brief 设置菜单栏
     */
    void setupMenuBar();
    
    /**
     * @brief 设置状态栏
     */
    void setupStatusBar();
    
    /**
     * @brief 设置域和Topic标签页
     */
    void setupDomainTopicTab();
    
    /**
     * @brief 设置数据库标签页
     */
    void setupDatabaseTab();
    
    // 数据更新方法
    /**
     * @brief 更新Topic表格数据
     */
    void updateTopicTable();
    
    /**
     * @brief 更新DataWriter表格数据
     */
    void updateDataWriterTable();
    
    /**
     * @brief 更新DataReader表格数据
     */
    void updateDataReaderTable();
    
    /**
     * @brief 更新数据库统计信息
     */
    void updateDatabaseStats();
    
    /**
     * @brief 更新内存缓存信息
     */
    void updateMemoryCacheInfo();
    
    // 样式设置
    /**
     * @brief 应用现代化样式
     */
    void applyModernStyle();
    
    /**
     * @brief 设置图标
     */
    void setupIcons();
    
    // 绘制方法
    /**
     * @brief 绘制圆角背景
     */
    void drawRoundedBackground();
    
    /**
     * @brief 绘制标题栏
     */
    void drawTitleBar();
    
    // 成员变量
    std::shared_ptr<TRAVODDS::PersistenceService> persistence_service_;  ///< 持久化服务实例
    
    // 主要UI组件
    QTabWidget* main_tabs_;                                              ///< 主标签页控件
    
    // 域和主题相关组件
    QWidget* domain_topic_widget_;                                       ///< 域和Topic标签页
    QTreeWidget* domain_tree_;                                           ///< 域信息树形控件
    QTableWidget* topic_table_;                                          ///< Topic信息表格
    QTableWidget* datawriter_table_;                                     ///< DataWriter信息表格
    QTableWidget* datareader_table_;                                     ///< DataReader信息表格
    QTextEdit* topic_details_;                                           ///< Topic详细信息文本框
    
    // 数据库相关组件
    QWidget* database_widget_;                                           ///< 数据库标签页
    QLabel* db_path_label_;                                              ///< 数据库路径标签
    QLabel* db_size_label_;                                              ///< 数据库大小标签
    QLabel* total_topics_label_;                                         ///< 总Topic数标签
    QLabel* total_samples_label_;                                        ///< 总样本数标签
    QProgressBar* memory_usage_bar_;                                     ///< 内存使用进度条
    QProgressBar* disk_usage_bar_;                                       ///< 磁盘使用进度条
    QTableWidget* stats_table_;                                          ///< 统计信息表格
    
    // 数据库表内容可视化组件
    QTreeWidget* database_tables_tree_;                                  ///< 数据库表树形控件
    QTableWidget* table_content_view_;                                   ///< 表内容查看器
    
    // 状态栏组件
    QLabel* status_label_;                                               ///< 状态标签
    QLabel* uptime_label_;                                               ///< 运行时间标签
    
    // 定时器
    QTimer* refresh_timer_;                                              ///< 数据刷新定时器
    QTimer* uptime_timer_;                                               ///< 运行时间定时器
    QDateTime start_time_;                                               ///< 启动时间
    
    // 状态变量
    bool is_monitoring_;                                                 ///< 是否正在监控
    int refresh_interval_ms_;                                            ///< 刷新间隔（毫秒）
    
    // 窗口拖拽相关
    bool is_dragging_;                                                   ///< 是否正在拖拽
    QPoint drag_start_position_;                                         ///< 拖拽起始位置
};
