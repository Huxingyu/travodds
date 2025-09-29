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
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsPolygonItem>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QPoint>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtGui/QMouseEvent>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QBarCategoryAxis>
#include <memory>
#include <map>
#include <deque>
#include <set>
#include <vector>

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


    void exportDataToExcel(const QString& filename,
                          const QDateTime& start_time,
                          const QDateTime& end_time,
                          const QString& topic_filter = "");

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
    
    /**
     * @brief 事件过滤（用于拓扑视图缩放等）
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    // 数据更新槽函数
    /**
     * @brief 更新域信息显示
     */
    void updateDomainInfo();
    

    
    /**
     * @brief 更新数据库信息显示
     */
    void updateDatabaseInfo();
    
    /**
     * @brief 刷新所有数据
     */
    void refreshAllData();
    
    /**
     * @brief 更新实时图表数据
     */
    void updateRealTimeCharts();
    
    /**
     * @brief 更新网络拓扑图
     */
    void updateNetworkTopology();
    
    // 交互槽函数
    /**
     * @brief Topic选择响应
     */
    void onTopicSelected();
    
    /**
     * @brief 数据库表选择响应
     */
    void onDatabaseTableSelected();
    

    
    /**
     * @brief 拓扑节点点击响应
     */
    void onTopologyNodeClicked(const QString& nodeId);
    
    /**
     * @brief 退出按钮点击响应
     */
    void onExitButtonClicked();
    
    /**
     * @brief 数据库查询响应
     */
    void onDatabaseQuery();
    
    /**
     * @brief 数据库数据导出响应
     */
    void onDatabaseExport();

    // UI初始化
    /**
     * @brief 设置用户界面
     */
    void setupUI();
    
    
    /**
     * @brief 设置状态栏
     */
    void setupStatusBar();
    
    /**
     * @brief 设置实时监控标签页
     */
    void setupRealTimeMonitoringTab();
    
    /**
     * @brief 设置域和Topic标签页
     */
    void setupDomainTopicTab();
    
    /**
     * @brief 设置数据库标签页
     */
    void setupDatabaseTab();
    
    /**
     * @brief 设置网络拓扑视图
     */
    void setupNetworkTopologyView();
    
    /**
     * @brief 创建示例网络拓扑
     */
    void createSampleTopology();
    
    /**
     * @brief 设置实时图表
     */
    void setupRealTimeCharts();
    
    /**
     * @brief 创建拓扑图例组件
     */
    QWidget* createTopologyLegend();
    

    
    // 数据更新方法
    /**
     * @brief 更新数据库表结构列表
     */
    void updateDatabaseTableList();
    
    /**
     * @brief 更新Topic过滤器下拉框
     */
    void updateTopicFilterCombo(const std::set<std::string>& topics);
    
    /**
     * @brief 拓扑选择改变响应（高亮邻接元素）
     */
    void onTopologySelectionChanged();
    
    // 样式设置
    /**
     * @brief 应用现代化样式
     */
    void applyModernStyle();
    

    
    
private:
    // 成员变量
    std::shared_ptr<TRAVODDS::PersistenceService> persistence_service_;  ///< 持久化服务实例
    
    // 主要UI组件
    QTabWidget* main_tabs_;                                              ///< 主标签页控件
    
    // 实时监控相关组件
    QWidget* realtime_monitoring_widget_;                                ///< 实时监控标签页
    QGraphicsView* topology_view_;                                       ///< 网络拓扑图视图
    QGraphicsScene* topology_scene_;                                     ///< 网络拓扑图场景
    QtCharts::QChartView* performance_chart_view_;                       ///< 性能图表视图
    QtCharts::QChart* performance_chart_;                                ///< 性能图表
    QtCharts::QLineSeries* cpu_series_;                                  ///< CPU使用率序列
    QtCharts::QLineSeries* memory_series_;                               ///< 内存使用率序列
    QtCharts::QLineSeries* throughput_series_;                           ///< 吞吐量序列
    QtCharts::QValueAxis* time_axis_;                                    ///< 时间轴
    QtCharts::QValueAxis* performance_axis_;                             ///< 性能轴
    
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
    
    // 数据库查询相关组件
    QDateTimeEdit* db_start_time_edit_;                                  ///< 数据库查询开始时间
    QDateTimeEdit* db_end_time_edit_;                                    ///< 数据库查询结束时间
    QComboBox* durability_filter_combo_;                                ///< 持久化策略过滤器
    QComboBox* db_topic_filter_combo_;                                   ///< Topic过滤器
    QPushButton* db_query_button_;                                       ///< 数据库查询按钮
    QPushButton* db_export_button_;                                      ///< 数据库导出按钮
    QTableWidget* query_results_table_;                                  ///< 查询结果表格
    
    // 状态栏组件
    QLabel* status_label_;                                               ///< 状态标签
    QLabel* uptime_label_;                                               ///< 运行时间标签
    QLabel* heartbeat_label_;                                            ///< 心跳状态标签
    
    // 定时器
    QTimer* refresh_timer_;                                              ///< 数据刷新定时器
    QTimer* uptime_timer_;                                               ///< 运行时间定时器
    QTimer* chart_update_timer_;                                         ///< 图表更新定时器
    QTimer* topology_update_timer_;                                      ///< 拓扑更新定时器
    QTimer* heartbeat_timer_;                                            ///< 心跳检测定时器
    QDateTime start_time_;                                               ///< 启动时间
    
    // 状态变量
    bool is_monitoring_;                                                 ///< 是否正在监控
    int refresh_interval_ms_;                                            ///< 刷新间隔（毫秒）
    
    // 数据缓存 (用于实时图表)
    std::deque<double> cpu_data_;                                        ///< CPU使用率数据队列
    std::deque<double> memory_data_;                                     ///< 内存使用率数据队列
    std::deque<double> throughput_data_;                                 ///< 吞吐量数据队列
    std::deque<QDateTime> time_data_;                                    ///< 时间数据队列
    static constexpr int MAX_DATA_POINTS = 100;                         ///< 最大数据点数
    
    // 拓扑节点映射
    std::map<QString, QGraphicsItem*> topology_nodes_;                   ///< 拓扑节点映射
    std::map<QString, QGraphicsItem*> topology_connections_;             ///< 拓扑连接映射
    
    struct TopologyEdge {
        QGraphicsLineItem* line {nullptr};
        QGraphicsPolygonItem* arrow {nullptr};
        QString source;
        QString target;
        QString status;
    };
    std::vector<TopologyEdge> topology_edges_;                           ///< 边集合（含箭头）
    std::map<QString, std::set<QString>> adjacency_;                     ///< 邻接表
    
    // 窗口拖拽相关
    bool is_dragging_;                                                   ///< 是否正在拖拽
    QPoint drag_start_position_;                                         ///< 拖拽起始位置
    
    // 缓存管理
    struct CacheEntry {
        QDateTime timestamp;
        QVariant data;
    };
    std::map<QString, std::deque<CacheEntry>> data_cache_;               ///< 内存数据缓存 (LRU)
};
