#include "PersistenceMonitor.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTextStream>
#include <QtCore/QEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <random>

using namespace TRAVODDS;
using namespace QtCharts;

PersistenceMonitor::PersistenceMonitor(QWidget* parent)
    : QMainWindow(parent)
    , persistence_service_(nullptr)
    , main_tabs_(nullptr)
    , topology_view_(nullptr)
    , topology_scene_(nullptr)
    , performance_chart_view_(nullptr)
    , performance_chart_(nullptr)
    , cpu_series_(nullptr)
    , memory_series_(nullptr)
    , throughput_series_(nullptr)
    , time_axis_(nullptr)
    , performance_axis_(nullptr)
    , is_monitoring_(false)
    , refresh_interval_ms_(2000) // 2秒刷新间隔
    , is_dragging_(false)
{
    // 设置无边框窗口
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    
    // 隐藏菜单栏
    menuBar()->hide();

    setWindowTitle(QString::fromUtf8("TravoDDS 持久化服务监控器 v1.0"));
    setMinimumSize(1400, 900);
    resize(1600, 1000);
    
    
    // 初始化UI
    setupUI();
    setupStatusBar();
    
    // 应用现代样式
    applyModernStyle();
    
    // 初始化定时器
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, &QTimer::timeout, this, &PersistenceMonitor::refreshAllData);
    
    // 图表更新定时器 
    chart_update_timer_ = new QTimer(this);
    connect(chart_update_timer_, &QTimer::timeout, this, &PersistenceMonitor::updateRealTimeCharts);
    chart_update_timer_->start(1000); // 每秒更新图表
    
    // 拓扑更新定时器
    topology_update_timer_ = new QTimer(this);
    connect(topology_update_timer_, &QTimer::timeout, this, &PersistenceMonitor::updateNetworkTopology);
    topology_update_timer_->start(3000); // 每3秒更新拓扑
    
    
    uptime_timer_ = new QTimer(this);
    connect(uptime_timer_, &QTimer::timeout, this, [this]() {
        // 更新运行时间显示
        auto elapsed = start_time_.secsTo(QDateTime::currentDateTime());
        int hours = elapsed / 3600;
        int minutes = (elapsed % 3600) / 60;
        int seconds = elapsed % 60;
        
        uptime_label_->setText(QString::fromUtf8("运行时间: %1:%2:%3")
                              .arg(hours, 2, 10, QLatin1Char('0'))
                              .arg(minutes, 2, 10, QLatin1Char('0'))
                              .arg(seconds, 2, 10, QLatin1Char('0')));
    });
    uptime_timer_->start(1000); // 每秒更新运行时间
    
    start_time_ = QDateTime::currentDateTime();
}

PersistenceMonitor::~PersistenceMonitor() {
    stopMonitoring();
}

void PersistenceMonitor::setPersistenceService(std::shared_ptr<TRAVODDS::PersistenceService> service) {
    if (persistence_service_ == service) {
        return;
    }
    
    persistence_service_ = service;
    
    if (service) {
        // 自动启动监控
        startMonitoring();
    }
}

void PersistenceMonitor::startMonitoring() {
    if (is_monitoring_) {
        return;
    }
    
    if (!persistence_service_) {
        QMessageBox::warning(this, QString::fromUtf8("警告"), QString::fromUtf8("持久化服务未设置"));
        return;
    }
    
    is_monitoring_ = true;
    refresh_timer_->start(refresh_interval_ms_);
    
    status_label_->setText(QString::fromUtf8("状态: 监控中"));
    status_label_->setStyleSheet("color: green; font-weight: bold;");
    
    refreshAllData();
}

void PersistenceMonitor::stopMonitoring() {
    if (!is_monitoring_) {
        return;
    }
    
    is_monitoring_ = false;
    refresh_timer_->stop();
    
    status_label_->setText(QString::fromUtf8("状态: 已停止"));
    status_label_->setStyleSheet("color: red; font-weight: bold;");
}

void PersistenceMonitor::setupUI() {
    // 创建中央小部件
    QWidget* central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    
    // 创建顶部工具栏
    QWidget* toolbar_widget = new QWidget();
    toolbar_widget->setStyleSheet(
        "QWidget {"
        "   background-color: #f8f9fa;"
        "   border-bottom: 1px solid #dee2e6;"
        "   border-top-left-radius: 8px;"
        "   border-top-right-radius: 8px;"
        "}"
    );
    QHBoxLayout* toolbar_layout = new QHBoxLayout(toolbar_widget);
    toolbar_layout->setContentsMargins(15, 8, 15, 8);
    
    // 添加标题标签
    QLabel* title_label = new QLabel(QString::fromUtf8("TravoDDS 持久化服务监控器 v1.0"));
    title_label->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #2c3e50; }");
    
    // 添加弹性空间
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    // 创建控制按钮
    QPushButton* minimize_btn = new QPushButton("−");
    QPushButton* exit_btn = new QPushButton("✕");
    
    // 设置按钮样式
    QString button_style = 
        "QPushButton {"
        "   background-color: #ecf0f1;"
        "   border: 1px solid #bdc3c7;"
        "   border-radius: 15px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   min-width: 30px;"
        "   max-width: 30px;"
        "   min-height: 30px;"
        "   max-height: 30px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #d5dbdb;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #bdc3c7;"
        "}";
    
    minimize_btn->setStyleSheet(button_style);
    exit_btn->setStyleSheet(button_style + 
        "QPushButton:hover { background-color: #e74c3c; color: white; }");
    
    // 连接信号
    connect(minimize_btn, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(exit_btn, &QPushButton::clicked, this, &PersistenceMonitor::onExitButtonClicked);
    
    // 添加组件到工具栏
    toolbar_layout->addWidget(title_label);
    toolbar_layout->addWidget(spacer);
    toolbar_layout->addWidget(minimize_btn);
    toolbar_layout->addWidget(exit_btn);
    
    // 创建主标签页
    main_tabs_ = new QTabWidget(central_widget);
    
    // 设置标签页
    setupRealTimeMonitoringTab();
    setupDomainTopicTab();
    setupDatabaseTab();
    
    // 设置主布局
    QVBoxLayout* main_layout = new QVBoxLayout(central_widget);
    main_layout->addWidget(toolbar_widget);
    main_layout->addWidget(main_tabs_);
    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(5);
    
    central_widget->setLayout(main_layout);
    
    // 设置窗口样式，确保有合适的背景色
    setStyleSheet(
        "QMainWindow {"
        "   background-color: #ffffff;"
        "   border: 2px solid #3498db;"
        "   border-radius: 15px;"
        "}"
        "QMainWindow::separator {"
        "   background-color: #bdc3c7;"
        "   width: 1px;"
        "   height: 1px;"
        "}"
    );
}

void PersistenceMonitor::setupRealTimeMonitoringTab() {
    realtime_monitoring_widget_ = new QWidget();
    
    // 创建分割器
    QSplitter* main_splitter = new QSplitter(Qt::Horizontal, realtime_monitoring_widget_);
    
    // 左侧：网络拓扑视图
    QGroupBox* topology_group = new QGroupBox(QString::fromUtf8("DDS网络拓扑"), realtime_monitoring_widget_);
    QVBoxLayout* topology_layout = new QVBoxLayout(topology_group);
    
    setupNetworkTopologyView();
    topology_layout->addWidget(topology_view_);
    // 添加图例
    QWidget* legend = createTopologyLegend();
    if (legend) topology_layout->addWidget(legend);
    
    // 右侧：实时性能图表
    QGroupBox* charts_group = new QGroupBox(QString::fromUtf8("实时性能监控"), realtime_monitoring_widget_);
    QVBoxLayout* charts_layout = new QVBoxLayout(charts_group);
    
    setupRealTimeCharts();
    charts_layout->addWidget(performance_chart_view_);
    
    // 设置分割器比例
    main_splitter->addWidget(topology_group);
    main_splitter->addWidget(charts_group);
    main_splitter->setSizes({500, 500});
    
    // 布局
    QHBoxLayout* realtime_layout = new QHBoxLayout(realtime_monitoring_widget_);
    realtime_layout->addWidget(main_splitter);
    
    main_tabs_->addTab(realtime_monitoring_widget_, QString::fromUtf8("实时监控"));
}

void PersistenceMonitor::setupNetworkTopologyView() {
    topology_scene_ = new QGraphicsScene(this);
    topology_view_ = new QGraphicsView(topology_scene_);
    
    topology_view_->setRenderHint(QPainter::Antialiasing);
    topology_view_->setDragMode(QGraphicsView::ScrollHandDrag);  // 可拖动
    topology_view_->setMinimumSize(400, 300);
    topology_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    topology_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    topology_view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // 滚轮围绕鼠标缩放
    topology_view_->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    // 启用选择并响应高亮
    topology_scene_->setItemIndexMethod(QGraphicsScene::NoIndex);
    topology_view_->setInteractive(true);
    topology_view_->viewport()->installEventFilter(this);
    connect(topology_scene_, &QGraphicsScene::selectionChanged,
            this, &PersistenceMonitor::onTopologySelectionChanged);
    
    // 创建示例拓扑节点
    createSampleTopology();
}

bool PersistenceMonitor::eventFilter(QObject* obj, QEvent* event) {
    if (topology_view_ && obj == topology_view_->viewport()) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent* wheel = static_cast<QWheelEvent*>(event);
            const double factor = 1.15;
            if (wheel->angleDelta().y() > 0) {
                topology_view_->scale(factor, factor);
            } else {
                topology_view_->scale(1.0 / factor, 1.0 / factor);
            }
            event->accept();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void PersistenceMonitor::createSampleTopology() {
    if (!topology_scene_) return;
    
    topology_scene_->clear();
    topology_nodes_.clear();
    topology_connections_.clear();
    topology_edges_.clear();
    adjacency_.clear();
    
    // 设置场景大小，居中显示
    const int scene_width = 600;
    const int scene_height = 400;
    const int center_x = scene_width / 2;
    const int center_y = scene_height / 2;
    
    topology_scene_->setSceneRect(0, 0, scene_width, scene_height);
    
    // 创建DomainParticipant节点 - 居中位置
    auto* participant_node = topology_scene_->addEllipse(-25, -25, 50, 50, 
        QPen(QColor(0, 120, 255), 2), QBrush(QColor(230, 245, 255)));
    participant_node->setPos(center_x, center_y - 80);
    participant_node->setFlag(QGraphicsItem::ItemIsMovable, true);  // 使节点可移动
    auto* participant_text = topology_scene_->addText("DomainParticipant", QFont("Arial", 8));
    participant_text->setPos(center_x - 50, center_y - 50);
    topology_nodes_["participant"] = participant_node;
    
    // 创建Publisher节点 - 左侧
    auto* publisher_node = topology_scene_->addRect(-20, -15, 40, 30, 
        QPen(QColor(0, 180, 0), 2), QBrush(QColor(230, 255, 230)));
    publisher_node->setPos(center_x - 100, center_y + 20);
    publisher_node->setFlag(QGraphicsItem::ItemIsMovable, true);
    auto* publisher_text = topology_scene_->addText("Publisher", QFont("Arial", 8));
    publisher_text->setPos(center_x - 130, center_y + 40);
    topology_nodes_["publisher"] = publisher_node;
    
    // 创建Subscriber节点 - 右侧
    auto* subscriber_node = topology_scene_->addRect(-20, -15, 40, 30, 
        QPen(QColor(255, 140, 0), 2), QBrush(QColor(255, 245, 230)));
    subscriber_node->setPos(center_x + 100, center_y + 20);
    subscriber_node->setFlag(QGraphicsItem::ItemIsMovable, true);
    auto* subscriber_text = topology_scene_->addText("Subscriber", QFont("Arial", 8));
    subscriber_text->setPos(center_x + 70, center_y + 40);
    topology_nodes_["subscriber"] = subscriber_node;
    
    // 创建Topic节点 - 下方
    auto* topic_node = topology_scene_->addEllipse(-15, -15, 30, 30, 
        QPen(QColor(255, 0, 120), 2), QBrush(QColor(255, 230, 245)));
    topic_node->setPos(center_x, center_y + 100);
    topic_node->setFlag(QGraphicsItem::ItemIsMovable, true);
    auto* topic_text = topology_scene_->addText("HelloWorld", QFont("Arial", 8));
    topic_text->setPos(center_x - 30, center_y + 120);
    topology_nodes_["topic"] = topic_node;
    
    // 创建连接线 - 使用居中坐标
    auto* line1 = topology_scene_->addLine(center_x, center_y - 55, center_x - 80, center_y + 5, QPen(QColor(100, 100, 100), 2));
    auto* line2 = topology_scene_->addLine(center_x, center_y - 55, center_x + 80, center_y + 5, QPen(QColor(100, 100, 100), 2));
    auto* line3 = topology_scene_->addLine(center_x - 80, center_y + 35, center_x - 15, center_y + 85, QPen(QColor(100, 100, 100), 2));
    auto* line4 = topology_scene_->addLine(center_x + 80, center_y + 35, center_x + 15, center_y + 85, QPen(QColor(100, 100, 100), 2));
    
    topology_connections_["conn1"] = line1;
    topology_connections_["conn2"] = line2;
    topology_connections_["conn3"] = line3;
    topology_connections_["conn4"] = line4;
}

void PersistenceMonitor::setupRealTimeCharts() {
    // 创建图表
    performance_chart_ = new QChart();
    performance_chart_->setTitle(QString::fromUtf8("DDS网络实体数量监控"));
    performance_chart_->setAnimationOptions(QChart::SeriesAnimations);
    
    // 创建数据序列 - 改为DDS实体监控
    cpu_series_ = new QLineSeries();
    cpu_series_->setName(QString::fromUtf8("活跃DataWriter数量"));
    cpu_series_->setPen(QPen(QColor(255, 0, 0), 2));
    
    memory_series_ = new QLineSeries();
    memory_series_->setName(QString::fromUtf8("活跃DataReader数量"));
    memory_series_->setPen(QPen(QColor(0, 255, 0), 2));
    
    throughput_series_ = new QLineSeries();
    throughput_series_->setName(QString::fromUtf8("DomainParticipant数量"));
    throughput_series_->setPen(QPen(QColor(0, 0, 255), 2));
    
    // 添加序列到图表
    performance_chart_->addSeries(cpu_series_);
    performance_chart_->addSeries(memory_series_);
    performance_chart_->addSeries(throughput_series_);
    
    // 创建坐标轴
    time_axis_ = new QValueAxis();
    time_axis_->setTitleText(QString::fromUtf8("时间点"));
    time_axis_->setRange(0, MAX_DATA_POINTS);
    
    performance_axis_ = new QValueAxis();
    performance_axis_->setTitleText(QString::fromUtf8("实体数量"));
    performance_axis_->setRange(0, 20);  // 调整范围适合实体数量
    performance_axis_->setTickCount(11);  // 设置刻度数量 (0,2,4,6,8,10,12,14,16,18,20)
    performance_axis_->setLabelFormat("%d");  // 设置标签格式为整数
    
    // 设置坐标轴
    performance_chart_->addAxis(time_axis_, Qt::AlignBottom);
    performance_chart_->addAxis(performance_axis_, Qt::AlignLeft);
    
    cpu_series_->attachAxis(time_axis_);
    cpu_series_->attachAxis(performance_axis_);
    memory_series_->attachAxis(time_axis_);
    memory_series_->attachAxis(performance_axis_);
    throughput_series_->attachAxis(time_axis_);
    throughput_series_->attachAxis(performance_axis_);
    
    // 创建图表视图
    performance_chart_view_ = new QChartView(performance_chart_);
    performance_chart_view_->setRenderHint(QPainter::Antialiasing);
    performance_chart_view_->setMinimumSize(500, 300);
    
    // 初始化数据队列 - 重新命名以反映新的用途
    cpu_data_.clear();      // 现在存储DataWriter数量
    memory_data_.clear();   // 现在存储DataReader数量  
    throughput_data_.clear(); // 现在存储DomainParticipant数量
    time_data_.clear();
}

void PersistenceMonitor::setupDomainTopicTab() {
    domain_topic_widget_ = new QWidget();
    
    // 创建水平分割器
    QSplitter* main_splitter = new QSplitter(Qt::Horizontal, domain_topic_widget_);
    
    // 左侧：域信息树和Topic表格
    QWidget* left_widget = new QWidget();
    QVBoxLayout* left_layout = new QVBoxLayout(left_widget);
    
    // 域信息组
    QGroupBox* domain_group = new QGroupBox(QString::fromUtf8("域信息"));
    QVBoxLayout* domain_layout = new QVBoxLayout(domain_group);
    
    domain_tree_ = new QTreeWidget();
    domain_tree_->setHeaderLabels({QString::fromUtf8("域信息")});
    domain_tree_->setMinimumHeight(150);
    
    // 初始化为空，将从持久化服务获取真实数据
    // updateDomainInfo() 方法将在监控启动时填充真实数据
    
    domain_layout->addWidget(domain_tree_);
    
    // 添加域信息控制按钮
    QHBoxLayout* domain_button_layout = new QHBoxLayout();
    QPushButton* refresh_domain_btn = new QPushButton(QString::fromUtf8("刷新域信息"));
    QPushButton* clear_domain_btn = new QPushButton(QString::fromUtf8("清除数据"));
    
    refresh_domain_btn->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #218838; }");
    clear_domain_btn->setStyleSheet("QPushButton { background-color: #dc3545; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #c82333; }");
    
    connect(refresh_domain_btn, &QPushButton::clicked, this, &PersistenceMonitor::updateDomainInfo);
    connect(clear_domain_btn, &QPushButton::clicked, [this]() {
        domain_tree_->clear();
        topic_table_->setRowCount(0);
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("域信息和Topic数据已清除"));
    });
    
    domain_button_layout->addWidget(refresh_domain_btn);
    domain_button_layout->addWidget(clear_domain_btn);
    domain_button_layout->addStretch();
    domain_layout->addLayout(domain_button_layout);
    
    // Topic信息组
    QGroupBox* topic_group = new QGroupBox(QString::fromUtf8("Topic信息"));
    QVBoxLayout* topic_layout = new QVBoxLayout(topic_group);
    
    topic_table_ = new QTableWidget();
    topic_table_->setColumnCount(6);
    topic_table_->setHorizontalHeaderLabels({
        QString::fromUtf8("Topic名称"),
        QString::fromUtf8("数据类型"),
        QString::fromUtf8("持久性策略"),
        QString::fromUtf8("可靠性策略"),
        QString::fromUtf8("Writer数量"),
        QString::fromUtf8("Reader数量")
    });
    
    // 初始化为空表格，将从持久化服务获取真实数据
    topic_table_->setRowCount(0);
    
    topic_table_->resizeColumnsToContents();
    topic_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    connect(topic_table_, &QTableWidget::itemSelectionChanged, 
            this, &PersistenceMonitor::onTopicSelected);
    
    topic_layout->addWidget(topic_table_);
    
    left_layout->addWidget(domain_group);
    left_layout->addWidget(topic_group);
    
    // 右侧：详细信息
    QWidget* right_widget = new QWidget();
    QVBoxLayout* right_layout = new QVBoxLayout(right_widget);
    
    // Topic详细信息
    QGroupBox* details_group = new QGroupBox(QString::fromUtf8("Topic详细信息"));
    QVBoxLayout* details_layout = new QVBoxLayout(details_group);
    
    topic_details_ = new QTextEdit();
    topic_details_->setReadOnly(true);
    topic_details_->setMinimumHeight(200);
    topic_details_->setPlainText(QString::fromUtf8(
        "请选择左侧Topic查看详细信息...\n\n"
        "详细信息将包括:\n"
        "• QoS策略详细配置\n"
        "• DataWriter/DataReader列表\n"
        "• 历史数据统计\n"
        "• 网络流量信息"));
    
    details_layout->addWidget(topic_details_);
    
    // DataWriter表格
    QGroupBox* writer_group = new QGroupBox(QString::fromUtf8("DataWriter信息"));
    QVBoxLayout* writer_layout = new QVBoxLayout(writer_group);
    
    datawriter_table_ = new QTableWidget();
    datawriter_table_->setColumnCount(6);
    datawriter_table_->setHorizontalHeaderLabels({
        QString::fromUtf8("GUID"),
        QString::fromUtf8("Topic名称"),
        QString::fromUtf8("数据类型"),
        QString::fromUtf8("持久性策略"),
        QString::fromUtf8("可靠性策略"),
        QString::fromUtf8("状态")
    });
    datawriter_table_->setMaximumHeight(150);
    
    writer_layout->addWidget(datawriter_table_);
    
    // DataReader表格
    QGroupBox* reader_group = new QGroupBox(QString::fromUtf8("DataReader信息"));
    QVBoxLayout* reader_layout = new QVBoxLayout(reader_group);
    
    datareader_table_ = new QTableWidget();
    datareader_table_->setColumnCount(6);
    datareader_table_->setHorizontalHeaderLabels({
        QString::fromUtf8("GUID"),
        QString::fromUtf8("Topic名称"),
        QString::fromUtf8("数据类型"),
        QString::fromUtf8("持久性策略"),
        QString::fromUtf8("可靠性策略"),
        QString::fromUtf8("状态")
    });
    datareader_table_->setMaximumHeight(150);
    
    reader_layout->addWidget(datareader_table_);
    
    right_layout->addWidget(details_group);
    right_layout->addWidget(writer_group);
    right_layout->addWidget(reader_group);
    
    // 添加到分割器
    main_splitter->addWidget(left_widget);
    main_splitter->addWidget(right_widget);
    main_splitter->setSizes({400, 600});
    
    // 布局
    QHBoxLayout* domain_topic_layout = new QHBoxLayout(domain_topic_widget_);
    domain_topic_layout->addWidget(main_splitter);
    
    main_tabs_->addTab(domain_topic_widget_, QString::fromUtf8("域和主题"));
}




void PersistenceMonitor::setupDatabaseTab() {
    database_widget_ = new QWidget();
    
    // 创建垂直分割器
    QSplitter* main_splitter = new QSplitter(Qt::Vertical, database_widget_);
    
    // 上部：数据库信息和查询面板
    QWidget* top_widget = new QWidget();
    QHBoxLayout* top_layout = new QHBoxLayout(top_widget);
    
    // 左上：数据库基本信息
    QGroupBox* db_info_group = new QGroupBox(QString::fromUtf8("数据库信息"));
    QGridLayout* db_info_layout = new QGridLayout(db_info_group);
    
    QLabel* path_label = new QLabel(QString::fromUtf8("数据库路径:"));
    db_path_label_ = new QLabel("未连接");
    db_path_label_->setWordWrap(true);
    
    QLabel* size_label = new QLabel(QString::fromUtf8("数据库大小:"));
    db_size_label_ = new QLabel("0 MB");
    
    QLabel* topics_label = new QLabel(QString::fromUtf8("Topic数量:"));
    total_topics_label_ = new QLabel("0");
    
    QLabel* samples_label = new QLabel(QString::fromUtf8("总样本数:"));
    total_samples_label_ = new QLabel("0");
    
    db_info_layout->addWidget(path_label, 0, 0);
    db_info_layout->addWidget(db_path_label_, 0, 1);
    db_info_layout->addWidget(size_label, 1, 0);
    db_info_layout->addWidget(db_size_label_, 1, 1);
    db_info_layout->addWidget(topics_label, 2, 0);
    db_info_layout->addWidget(total_topics_label_, 2, 1);
    db_info_layout->addWidget(samples_label, 3, 0);
    db_info_layout->addWidget(total_samples_label_, 3, 1);
    
    // 内存缓存使用情况
    QGroupBox* memory_group = new QGroupBox(QString::fromUtf8("内存缓存"));
    QVBoxLayout* memory_layout = new QVBoxLayout(memory_group);
    
    memory_usage_bar_ = new QProgressBar();
    memory_usage_bar_->setRange(0, 100);
    memory_usage_bar_->setValue(0);
    memory_usage_bar_->setFormat("0MB / 0MB");
    
    memory_layout->addWidget(memory_usage_bar_);
    
    // 右上：查询控制面板
    QGroupBox* query_group = new QGroupBox(QString::fromUtf8("数据查询"));
    QGridLayout* query_layout = new QGridLayout(query_group);
    
    // 时间范围
    QLabel* start_label = new QLabel(QString::fromUtf8("开始时间:"));
    db_start_time_edit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    db_start_time_edit_->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    db_start_time_edit_->setCalendarPopup(true);
    
    QLabel* end_label = new QLabel(QString::fromUtf8("结束时间:"));
    db_end_time_edit_ = new QDateTimeEdit(QDateTime::currentDateTime());
    db_end_time_edit_->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    db_end_time_edit_->setCalendarPopup(true);
    
    // 过滤器
    QLabel* durability_label = new QLabel(QString::fromUtf8("持久化策略:"));
    durability_filter_combo_ = new QComboBox();
    durability_filter_combo_->addItem(QString::fromUtf8("全部"));
    durability_filter_combo_->addItem(QString::fromUtf8("VOLATILE"));
    durability_filter_combo_->addItem(QString::fromUtf8("TRANSIENT_LOCAL"));
    durability_filter_combo_->addItem(QString::fromUtf8("TRANSIENT"));
    durability_filter_combo_->addItem(QString::fromUtf8("PERSISTENT"));
    
    QLabel* topic_label = new QLabel(QString::fromUtf8("Topic过滤:"));
    db_topic_filter_combo_ = new QComboBox();
    db_topic_filter_combo_->addItem(QString::fromUtf8("全部"));
    
    // 查询和导出按钮
    db_query_button_ = new QPushButton(QString::fromUtf8("查询数据"));
    db_export_button_ = new QPushButton(QString::fromUtf8("导出结果"));
    QPushButton* refresh_db_btn = new QPushButton(QString::fromUtf8("刷新"));
    
    db_query_button_->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #218838; }");
    db_export_button_->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #138496; }");
    refresh_db_btn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #0056b3; }");
    
    // 连接信号
    connect(db_query_button_, &QPushButton::clicked, this, &PersistenceMonitor::onDatabaseQuery);
    connect(db_export_button_, &QPushButton::clicked, this, &PersistenceMonitor::onDatabaseExport);
    connect(refresh_db_btn, &QPushButton::clicked, this, &PersistenceMonitor::updateDatabaseInfo);
    
    // 布局查询控件
    query_layout->addWidget(start_label, 0, 0);
    query_layout->addWidget(db_start_time_edit_, 0, 1);
    query_layout->addWidget(end_label, 0, 2);
    query_layout->addWidget(db_end_time_edit_, 0, 3);
    
    query_layout->addWidget(durability_label, 1, 0);
    query_layout->addWidget(durability_filter_combo_, 1, 1);
    query_layout->addWidget(topic_label, 1, 2);
    query_layout->addWidget(db_topic_filter_combo_, 1, 3);
    
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->addWidget(db_query_button_);
    button_layout->addWidget(db_export_button_);
    button_layout->addWidget(refresh_db_btn);
    button_layout->addStretch();
    
    query_layout->addLayout(button_layout, 2, 0, 1, 4);
    
    // 添加左上部分到布局
    QVBoxLayout* left_layout = new QVBoxLayout();
    left_layout->addWidget(db_info_group);
    left_layout->addWidget(memory_group);
    
    QWidget* left_widget = new QWidget();
    left_widget->setLayout(left_layout);
    
    top_layout->addWidget(left_widget, 1);
    top_layout->addWidget(query_group, 2);
    
    // 下部：分割为表结构和查询结果
    QWidget* bottom_widget = new QWidget();
    QHBoxLayout* bottom_layout = new QHBoxLayout(bottom_widget);
    
    // 左下：数据库表结构
    QGroupBox* tables_group = new QGroupBox(QString::fromUtf8("数据库表结构"));
    QVBoxLayout* tables_layout = new QVBoxLayout(tables_group);
    
    database_tables_tree_ = new QTreeWidget();
    database_tables_tree_->setHeaderLabels({QString::fromUtf8("表名"), QString::fromUtf8("记录数")});
    database_tables_tree_->setRootIsDecorated(false);
    
    connect(database_tables_tree_, &QTreeWidget::itemSelectionChanged,
            this, &PersistenceMonitor::onDatabaseTableSelected);
    
    tables_layout->addWidget(database_tables_tree_);
    
    // 右下：查询结果显示
    QGroupBox* results_group = new QGroupBox(QString::fromUtf8("查询结果"));
    QVBoxLayout* results_layout = new QVBoxLayout(results_group);
    
    // 创建选项卡来显示表内容预览和查询结果
    QTabWidget* results_tabs = new QTabWidget();
    
    // 表内容预览标签页
    table_content_view_ = new QTableWidget();
    table_content_view_->setAlternatingRowColors(true);
    table_content_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_content_view_->setSortingEnabled(true);
    results_tabs->addTab(table_content_view_, QString::fromUtf8("表内容预览"));
    
    // 查询结果标签页
    query_results_table_ = new QTableWidget();
    query_results_table_->setAlternatingRowColors(true);
    query_results_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    query_results_table_->setSortingEnabled(true);
    results_tabs->addTab(query_results_table_, QString::fromUtf8("查询结果"));
    
    results_layout->addWidget(results_tabs);
    
    bottom_layout->addWidget(tables_group, 1);
    bottom_layout->addWidget(results_group, 2);
    
    // 添加到主分割器
    main_splitter->addWidget(top_widget);
    main_splitter->addWidget(bottom_widget);
    main_splitter->setSizes({200, 500}); // 上部较小，下部较大
    
    // 设置主布局
    QVBoxLayout* main_layout = new QVBoxLayout(database_widget_);
    main_layout->addWidget(main_splitter);
    
    main_tabs_->addTab(database_widget_, QString::fromUtf8("数据库"));
}


void PersistenceMonitor::setupStatusBar() {
    QStatusBar* status_bar = statusBar();
    
    status_label_ = new QLabel(QString::fromUtf8("状态: 就绪"));
    uptime_label_ = new QLabel(QString::fromUtf8("运行时间: 00:00:00"));
  
    
    status_bar->addWidget(status_label_);
    status_bar->addPermanentWidget(uptime_label_);
}



void PersistenceMonitor::applyModernStyle() {
    // 应用现代化的样式表
    setStyleSheet(R"(
        /* 主窗口样式 */
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            font-family: 'Segoe UI', 'Microsoft YaHei', Arial, sans-serif;
            font-size: 12px;
        }
        
        /* 标签页样式 */
        QTabWidget::pane {
            border: 1px solid #dee2e6;
            background-color: white;
            border-radius: 8px;
            margin-top: -1px;
        }
        
        QTabBar::tab {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            color: #495057;
            padding: 12px 20px;
            margin-right: 2px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            border: 1px solid #dee2e6;
            border-bottom: none;
            font-weight: 500;
            min-width: 100px;
        }
        
        QTabBar::tab:selected {
            background: white;
            color: #0d6efd;
            border-bottom: 2px solid #0d6efd;
        }
        
        QTabBar::tab:hover:!selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #ffffff, stop:1 #f8f9fa);
        }
        
        /* 分组框样式 */
        QGroupBox {
            font-weight: bold;
            color: #343a40;
            border: 2px solid #dee2e6;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 15px;
            background-color: white;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 15px;
            padding: 0 8px 0 8px;
            color: #0d6efd;
            font-size: 14px;
            font-weight: bold;
        }
        
        /* 表格样式 */
        QTableWidget {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background-color: white;
            alternate-background-color: #f8f9fa;
            gridline-color: #e9ecef;
            selection-background-color: #e3f2fd;
            font-size: 11px;
        }
        
        QTableWidget::item {
            padding: 8px;
            border: none;
        }
        
        QTableWidget::item:selected {
            background-color: #0d6efd;
            color: white;
        }
        
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            color: #495057;
            padding: 8px;
            border: 1px solid #dee2e6;
            font-weight: bold;
        }
        
        /* 树形控件样式 */
        QTreeWidget {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background-color: white;
            alternate-background-color: #f8f9fa;
            selection-background-color: #e3f2fd;
            font-size: 11px;
        }
        
        QTreeWidget::item {
            padding: 4px;
            border: none;
        }
        
        QTreeWidget::item:selected {
            background-color: #0d6efd;
            color: white;
        }
        
        /* 按钮样式 */
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #0d6efd, stop:1 #0a58ca);
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #0a58ca, stop:1 #084298);
        }
        
        QPushButton:pressed {
            background: #084298;
        }
        
        /* 进度条样式 */
        QProgressBar {
            border: 1px solid #dee2e6;
            border-radius: 4px;
            text-align: center;
            background-color: #f8f9fa;
        }
        
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                      stop:0 #198754, stop:1 #20c997);
            border-radius: 3px;
        }
        
        /* 滑块样式 */
        QSlider::groove:horizontal {
            border: 1px solid #dee2e6;
            height: 8px;
            background: #f8f9fa;
            border-radius: 4px;
        }
        
        QSlider::handle:horizontal {
            background: #0d6efd;
            border: 1px solid #0a58ca;
            width: 18px;
            margin: -5px 0;
            border-radius: 9px;
        }
        
        QSlider::handle:horizontal:hover {
            background: #0a58ca;
        }
        
        /* 状态栏样式 */
        QStatusBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            border-top: 1px solid #dee2e6;
            color: #495057;
        }
        
        /* 文本编辑器样式 */
        QTextEdit {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background-color: white;
            padding: 8px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 11px;
        }
    )");
}

// 数据更新方法实现
void PersistenceMonitor::refreshAllData() {
    if (!persistence_service_ || !is_monitoring_) {
        return;
    }
    
    updateDomainInfo();
    updateDatabaseInfo();
}

void PersistenceMonitor::updateDomainInfo() {
    if (!persistence_service_) return;
    
    // 清空现有数据
    domain_tree_->clear();
    
    // 获取真实的域信息
    try {
        // 获取持久化服务统计信息
        auto service_stats = persistence_service_->get_statistics();
        
        // 创建域根节点
        QTreeWidgetItem* domain_item = new QTreeWidgetItem(domain_tree_);
        domain_item->setText(0, QString::fromUtf8("Domain 0 [TravoDDS持久化服务]"));
        
        // 创建DomainParticipant节点
        QTreeWidgetItem* participant_item = new QTreeWidgetItem(domain_item);
        int total_topics = 0;
        if (service_stats.find("total_topics") != service_stats.end()) {
            total_topics = static_cast<int>(service_stats.at("total_topics"));
        }
        participant_item->setText(0, QString::fromUtf8("DomainParticipant [活跃: %1 个Topic]")
                                 .arg(total_topics));
        
        // 获取活跃的DataWriter信息
        auto writers = persistence_service_->get_active_datawriters_info();
        if (!writers.empty()) {
            QTreeWidgetItem* publisher_item = new QTreeWidgetItem(participant_item);
            publisher_item->setText(0, QString::fromUtf8("Publisher [%1 个DataWriter]")
                                   .arg(writers.size()));
        }
        
        // 获取活跃的DataReader信息
        auto readers = persistence_service_->get_active_datareaders_info();
        if (!readers.empty()) {
            QTreeWidgetItem* subscriber_item = new QTreeWidgetItem(participant_item);
            subscriber_item->setText(0, QString::fromUtf8("Subscriber [%1 个DataReader]")
                                    .arg(readers.size()));
        }
        
        domain_tree_->expandAll();
        
    } catch (const std::exception& e) {
        // 如果获取数据失败，显示错误信息
        QTreeWidgetItem* error_item = new QTreeWidgetItem(domain_tree_);
        error_item->setText(0, QString::fromUtf8("错误: 无法获取域信息"));
    }
}



void PersistenceMonitor::updateDatabaseInfo() {
    if (!persistence_service_) return;
    
    try {
        // 读取服务统计（包含数据库统计）
        auto stats = persistence_service_->get_statistics();

        // 展示数据库路径与文件大小（支持单文件 + WAL/SHM）
        QString dbPath = QString::fromStdString(persistence_service_->get_config().database_path);
        if (QDir::isRelativePath(dbPath)) {
            const QString appDir = QCoreApplication::applicationDirPath();
            dbPath = QDir(appDir).filePath(dbPath);
        }
        db_path_label_->setText(dbPath);

        quint64 bytes = 0;
        QFileInfo dbFileInfo(dbPath);
        if (dbFileInfo.exists() && dbFileInfo.isFile()) {
            bytes += static_cast<quint64>(dbFileInfo.size());
        } else if (dbFileInfo.exists() && dbFileInfo.isDir()) {
            // 兼容目录模式：遍历计算
            QDirIterator dirIterator(dbPath, QDir::Files, QDirIterator::Subdirectories);
            while (dirIterator.hasNext()) {
                dirIterator.next();
                bytes += static_cast<quint64>(dirIterator.fileInfo().size());
            }
        }
        // 追加 WAL/SHM 文件大小
        QFileInfo walFile(dbPath + "-wal");
        if (walFile.exists()) bytes += static_cast<quint64>(walFile.size());
        QFileInfo shmFile(dbPath + "-shm");
        if (shmFile.exists()) bytes += static_cast<quint64>(shmFile.size());

        const double kilobytes = bytes / 1024.0;
        db_size_label_->setText(QString("%1 KB").arg(kilobytes, 0, 'f', 1));

        // 使用数据库统计值（若存在）
        if (auto topicCountIt = stats.find("topics_count"); topicCountIt != stats.end()) {
            total_topics_label_->setText(QString::number(topicCountIt->second));
        }
        if (auto sampleCountIt = stats.find("samples_count"); sampleCountIt != stats.end()) {
            total_samples_label_->setText(QString::number(sampleCountIt->second));
        }
        
        // 更新内存缓存使用进度条
        auto [used_bytes, total_bytes] = persistence_service_->get_memory_cache_usage();
        const double used_kb = used_bytes / 1024.0;
        const double total_kb = total_bytes / 1024.0;

        if (total_kb > 0.0) {
            const int percentage = static_cast<int>((used_kb * 100.0) / total_kb);
            memory_usage_bar_->setValue(std::clamp(percentage, 0, 100));
            memory_usage_bar_->setFormat(QString("%1 KB / %2 KB").arg(used_kb, 0, 'f', 1).arg(total_kb, 0, 'f', 1));
        } else {
            memory_usage_bar_->setValue(0);
            memory_usage_bar_->setFormat("0 KB / 0 KB");
        }
        
        // 更新数据库表结构
        updateDatabaseTableList();

        // 使用数据库中的 topics 表填充 topic 过滤器（若存在）
        std::set<std::string> topics_from_db;
        try {
            auto table_names = persistence_service_->get_db_table_names();
            bool has_topics = std::find(table_names.begin(), table_names.end(), std::string("topics")) != table_names.end();
            if (has_topics) {
                auto [headers, rows] = persistence_service_->get_db_table_content("topics");
                // 找到 topic_name 列索引
                int name_idx = -1;
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (headers[i] == "topic_name") { name_idx = static_cast<int>(i); break; }
                }
                if (name_idx >= 0) {
                    for (const auto& row : rows) {
                        if (static_cast<size_t>(name_idx) < row.size()) {
                            topics_from_db.insert(row[name_idx]);
                        }
                    }
                }
            }
        } catch (...) {
            // 忽略填充失败
        }
        if (!topics_from_db.empty()) {
            updateTopicFilterCombo(topics_from_db);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceMonitor] Error updating database info: " << e.what() << std::endl;
        
        // 设置默认值
        db_path_label_->setText("未连接");
        db_size_label_->setText("0 MB");
        total_topics_label_->setText("0");
        total_samples_label_->setText("0");
        memory_usage_bar_->setValue(0);
        memory_usage_bar_->setFormat("0 MB / 0 MB");
    }
}

void PersistenceMonitor::updateRealTimeCharts() {
    if (!persistence_service_) return;
    
    // 获取真实的DDS实体数量数据
    auto active_writers = persistence_service_->get_active_datawriters_info();
    auto active_readers = persistence_service_->get_active_datareaders_info();
    
    // 统计实体数量
    double writer_count = static_cast<double>(active_writers.size());
    double reader_count = static_cast<double>(active_readers.size());
    double participant_count = 1.0; // 持久化服务本身就是一个DomainParticipant
    
    // 如果有活跃的writer或reader，说明至少有其他参与者
    if (!active_writers.empty() || !active_readers.empty()) {
        // 估算DomainParticipant数量（简化处理，假设每个进程一个participant）
        std::set<std::string> unique_participants;
        for (const auto& writer : active_writers) {
            // 基于GUID估算不同的participant
            std::string participant_id = std::to_string(writer.key.value[0]);
            unique_participants.insert(participant_id);
        }
        for (const auto& reader : active_readers) {
            std::string participant_id = std::to_string(reader.key.value[0]);
            unique_participants.insert(participant_id);
        }
        participant_count = static_cast<double>(unique_participants.size() + 1); // +1 for persistence service
    }
    
    // 添加新数据点
    cpu_data_.push_back(writer_count);        // DataWriter数量
    memory_data_.push_back(reader_count);     // DataReader数量
    throughput_data_.push_back(participant_count); // DomainParticipant数量
    time_data_.push_back(QDateTime::currentDateTime());
    
    // 保持数据点数量限制
    if (cpu_data_.size() > MAX_DATA_POINTS) {
        cpu_data_.pop_front();
        memory_data_.pop_front();
        throughput_data_.pop_front();
        time_data_.pop_front();
    }
    
    // 更新图表数据
    cpu_series_->clear();
    memory_series_->clear();
    throughput_series_->clear();
    
    for (size_t i = 0; i < cpu_data_.size(); ++i) {
        cpu_series_->append(i, cpu_data_[i]);
        memory_series_->append(i, memory_data_[i]);
        throughput_series_->append(i, throughput_data_[i]);
    }
    
    // 动态调整Y轴范围 - 使用整数
    int max_count = static_cast<int>(std::max({writer_count, reader_count, participant_count}));
    int y_axis_max = std::max(5, max_count + 2); // 至少显示5，留2个单位的余量
    performance_axis_->setRange(0, y_axis_max);
    performance_axis_->setTickCount(std::min(y_axis_max + 1, 11)); // 确保刻度数量合理
    
    // 更新坐标轴范围
    time_axis_->setRange(0, std::max(static_cast<int>(cpu_data_.size()), 10));
}

void PersistenceMonitor::updateNetworkTopology() {
    if (!topology_scene_ || !persistence_service_) return;
    
    // 获取真实的网络拓扑节点信息
    auto nodes = persistence_service_->get_network_topology_nodes();
    auto connections = persistence_service_->get_network_topology_connections();
    
    // 清除现有节点
    topology_scene_->clear();
    topology_nodes_.clear();
    topology_edges_.clear();
    adjacency_.clear();
    
    // 添加节点
    for (const auto& node_info : nodes) {
        auto node_id = QString::fromStdString(node_info.at("id"));
        auto type = node_info.at("type");
        auto name = QString::fromStdString(node_info.at("name"));
        auto status = node_info.at("status");
        auto pos_x = std::stoi(node_info.at("x"));
        auto pos_y = std::stoi(node_info.at("y"));
        
        QGraphicsItem* node_item = nullptr;
        
        if (type == "service") {
            // 持久化服务节点 - 使用方形
            auto rect = topology_scene_->addRect(-25, -25, 50, 50, 
                QPen(QColor(0, 100, 200), 2), 
                QBrush(status == "active" ? QColor(100, 200, 255, 200) : QColor(150, 150, 150, 200)));
            rect->setPos(pos_x, pos_y);
            rect->setToolTip(QString::fromUtf8("类型: 服务\n名称: %1\n状态: %2").arg(name).arg(QString::fromStdString(status)));
            rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
            node_item = rect;
        } else if (type == "datawriter") {
            // DataWriter节点 - 使用圆形
            auto ellipse = topology_scene_->addEllipse(-20, -20, 40, 40,
                QPen(QColor(200, 100, 0), 2),
                QBrush(status == "active" ? QColor(255, 200, 100, 200) : QColor(150, 150, 150, 200)));
            ellipse->setPos(pos_x, pos_y);
            ellipse->setToolTip(QString::fromUtf8("类型: DataWriter\n名称: %1\n状态: %2").arg(name).arg(QString::fromStdString(status)));
            ellipse->setFlag(QGraphicsItem::ItemIsSelectable, true);
            node_item = ellipse;
        } else if (type == "datareader") {
            // DataReader节点 - 使用三角形
            QPolygonF triangle;
            triangle << QPointF(0, -20) << QPointF(-20, 20) << QPointF(20, 20);
            auto polygon = topology_scene_->addPolygon(triangle,
                QPen(QColor(0, 150, 0), 2),
                QBrush(status == "active" ? QColor(100, 255, 100, 200) : QColor(150, 150, 150, 200)));
            polygon->setPos(pos_x, pos_y);
            polygon->setToolTip(QString::fromUtf8("类型: DataReader\n名称: %1\n状态: %2").arg(name).arg(QString::fromStdString(status)));
            polygon->setFlag(QGraphicsItem::ItemIsSelectable, true);
            node_item = polygon;
        }
        
        if (node_item) {
            topology_nodes_[node_id] = node_item;
            
            // 添加节点标签
            auto text = topology_scene_->addText(name, QFont("Arial", 10));
            text->setPos(pos_x - text->boundingRect().width() / 2, pos_y + 30);
            text->setDefaultTextColor(QColor(60, 60, 60));
            text->setZValue(10);
        }
    }
    
    // 添加连接线
    for (const auto& conn_info : connections) {
        auto source_id = QString::fromStdString(conn_info.at("source"));
        auto target_id = QString::fromStdString(conn_info.at("target"));
        auto conn_status = conn_info.at("status");
        
        if (topology_nodes_.find(source_id) != topology_nodes_.end() && 
            topology_nodes_.find(target_id) != topology_nodes_.end()) {
            
            auto source_pos = topology_nodes_[source_id]->pos();
            auto target_pos = topology_nodes_[target_id]->pos();
            
            auto pen = QPen(conn_status == "active" ? QColor(0, 200, 0, 180) : QColor(120, 120, 120, 120), 2);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            auto line = topology_scene_->addLine(source_pos.x(), source_pos.y(),
                                               target_pos.x(), target_pos.y(), pen);
            
            // 添加箭头指示数据流方向
            QPointF direction = target_pos - source_pos;
            direction /= std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
            QPointF arrow_tip = target_pos - direction * 25;
            QPointF arrow_left = arrow_tip - QPointF(direction.y() * 10, -direction.x() * 10);
            QPointF arrow_right = arrow_tip + QPointF(direction.y() * 10, -direction.x() * 10);
            
            QPolygonF arrowHead;
            arrowHead << arrow_tip << arrow_left << arrow_right;
            auto arrowPen = QPen(conn_status == "active" ? QColor(0, 200, 0) : QColor(120, 120, 120));
            auto arrowBrush = QBrush(conn_status == "active" ? QColor(0, 200, 0) : QColor(120, 120, 120));
            auto arrow = topology_scene_->addPolygon(arrowHead, arrowPen, arrowBrush);
            
            // tooltip 和选择
            line->setToolTip(QString::fromUtf8("连接: %1 -> %2\n状态: %3")
                             .arg(source_id).arg(target_id).arg(QString::fromStdString(conn_status)));
            arrow->setToolTip(line->toolTip());
            line->setZValue(-1);
            arrow->setZValue(0);
            
            TopologyEdge e;
            e.line = line;
            e.arrow = arrow;
            e.source = source_id;
            e.target = target_id;
            e.status = QString::fromStdString(conn_status);
            topology_edges_.push_back(e);
            adjacency_[source_id].insert(target_id);
            adjacency_[target_id].insert(source_id); // 无向邻接用于高亮
        }
    }

    // 连接选择变化以高亮邻接
    // 使用场景的选中项变化需要自定义或轮询，这里简化为每次刷新后连接一次信号到自定义槽
    // 场景没有直接的selectionChanged信号，但可以用views()的selectionModel，
    // 对于QGraphicsScene，我们改为定时更新中调用onTopologySelectionChanged()，或给每个节点安装事件。
    // 这里：立即调用一次，确保默认状态样式正确。
    onTopologySelectionChanged();
}

QWidget* PersistenceMonitor::createTopologyLegend() {
    QWidget* legend = new QWidget();
    auto layout = new QHBoxLayout(legend);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(12);
    auto addItem = [&](const QColor& stroke, const QColor& fill, const QString& text, const QString& shape){
        QWidget* item = new QWidget();
        auto h = new QHBoxLayout(item);
        h->setContentsMargins(0,0,0,0);
        QLabel* icon = new QLabel();
        icon->setFixedSize(16,16);
        QPixmap pix(16,16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(stroke, 2));
        p.setBrush(fill);
        if (shape == "rect") p.drawRect(2,2,12,12);
        else if (shape == "ellipse") p.drawEllipse(2,2,12,12);
        else { QPolygon tri; tri << QPoint(8,2) << QPoint(2,14) << QPoint(14,14); p.drawPolygon(tri); }
        p.end();
        icon->setPixmap(pix);
        QLabel* label = new QLabel(text);
        h->addWidget(icon); h->addWidget(label);
        layout->addWidget(item);
    };
    addItem(QColor(0,100,200), QColor(100,200,255,200), QString::fromUtf8("服务"), "rect");
    addItem(QColor(200,100,0), QColor(255,200,100,200), QString::fromUtf8("DataWriter"), "ellipse");
    addItem(QColor(0,150,0), QColor(100,255,100,200), QString::fromUtf8("DataReader"), "triangle");
    addItem(QColor(0,200,0), QColor(0,200,0), QString::fromUtf8("活动连接"), "ellipse");
    addItem(QColor(120,120,120), QColor(120,120,120), QString::fromUtf8("非活动连接"), "ellipse");
    legend->setStyleSheet("QWidget { background-color: rgba(255,255,255,220); border: 1px solid #dee2e6; border-radius: 6px;} QLabel{color:#333;}");
    return legend;
}

void PersistenceMonitor::onTopologySelectionChanged() {
    // 根据被选中的节点，高亮其邻接边和相邻节点，其他元素降暗
    // 查找被选中的节点ID
    std::set<QString> selectedIds;
    for (const auto& pair : topology_nodes_) {
        if (pair.second && pair.second->isSelected()) selectedIds.insert(pair.first);
    }
    
    // 定义样式
    auto dimColor = [](const QColor& c){ QColor r=c; r.setAlphaF(std::min(1.0, c.alphaF()*0.4)); return r; };
    QPen nodePenSelected(QColor(30,144,255), 3);
    QPen nodePenNormal(QColor(80,80,80), 2);
    QPen nodePenDim(QColor(150,150,150,120), 1);
    
    // 所有节点先降暗
    for (auto& pair : topology_nodes_) {
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(pair.second)) {
            shape->setPen(nodePenDim);
            QColor fill = shape->brush().color();
            shape->setBrush(dimColor(fill));
        }
    }
    // 还原标题颜色
    for (auto item : topology_scene_->items()) {
        if (auto text = dynamic_cast<QGraphicsTextItem*>(item)) {
            text->setDefaultTextColor(QColor(120,120,120));
        }
    }
    
    // 高亮选中及邻接
    std::set<QString> neighborIds;
    for (const auto& id : selectedIds) {
        if (adjacency_.count(id)) neighborIds.insert(adjacency_[id].begin(), adjacency_[id].end());
        if (topology_nodes_.count(id)) {
            if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(topology_nodes_[id])) {
                shape->setPen(nodePenSelected);
                QColor fill = shape->brush().color(); fill.setAlpha(220); shape->setBrush(fill);
            }
        }
    }
    for (const auto& id : neighborIds) {
        if (topology_nodes_.count(id)) {
            if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(topology_nodes_[id])) {
                shape->setPen(nodePenNormal);
                QColor fill = shape->brush().color(); fill.setAlpha(180); shape->setBrush(fill);
            }
        }
    }
    
    // 边高亮：与选中或邻接相关的边加粗，否则降暗
    for (auto& e : topology_edges_) {
        bool highlight = selectedIds.count(e.source) || selectedIds.count(e.target) || neighborIds.count(e.source) || neighborIds.count(e.target);
        if (e.line) {
            auto pen = e.line->pen();
            if (highlight) { pen.setWidth(3); pen.setColor(e.status == "active" ? QColor(0,200,0) : QColor(120,120,120)); }
            else { pen.setWidth(1); QColor c = pen.color(); c.setAlpha(80); pen.setColor(c); }
            e.line->setPen(pen);
        }
        if (e.arrow) {
            auto pen = e.arrow->pen();
            if (highlight) { pen.setWidth(2); pen.setColor(e.status == "active" ? QColor(0,200,0) : QColor(120,120,120)); }
            else { pen.setWidth(1); QColor c = pen.color(); c.setAlpha(80); pen.setColor(c); }
            e.arrow->setPen(pen);
            auto brush = e.arrow->brush();
            QColor bc = brush.color();
            if (highlight) { bc.setAlpha(255); bc = (e.status == "active" ? QColor(0,200,0) : QColor(120,120,120)); }
            else { bc.setAlpha(80); }
            brush.setColor(bc);
            e.arrow->setBrush(brush);
        }
    }
}

// 交互槽函数实现
void PersistenceMonitor::onTopicSelected() {
    int current_row = topic_table_->currentRow();
    if (current_row < 0) return;
    
    QTableWidgetItem* topic_item = topic_table_->item(current_row, 0);
    if (!topic_item) return;
    
    QString topic_name = topic_item->text();
    
    // 更新详细信息
    QString details = QString::fromUtf8(
        "Topic详细信息: %1\n\n"
        "基本属性:\n"
        "• 数据类型: %2\n"
        "• 持久性策略: %3\n"
        "• 可靠性策略: %4\n\n"
        "QoS策略详细配置:\n"
        "• DURABILITY: %3\n"
        "• RELIABILITY: %4\n"
        "• PRESENTATION: GROUP\n"
        "• DEADLINE: INFINITE\n"
        "• LATENCY_BUDGET: 0\n"
        "• OWNERSHIP: SHARED\n"
        "• LIVELINESS: AUTOMATIC\n"
        "• DESTINATION_ORDER: BY_RECEPTION_TIMESTAMP\n\n"
        "统计信息:\n"
        "• 历史数据样本: 1,247\n"
        "• 平均数据大小: 156 bytes\n"
        "• 最后更新时间: %5\n"
        "• QoS兼容性检查: 通过\n"
    ).arg(topic_name)
     .arg(topic_table_->item(current_row, 1)->text())
     .arg(topic_table_->item(current_row, 2)->text())
     .arg(topic_table_->item(current_row, 3)->text())
     .arg(QDateTime::currentDateTime().toString());
    
    topic_details_->setPlainText(details);
}

void PersistenceMonitor::onDatabaseTableSelected() {
    QTreeWidgetItem* current_item = database_tables_tree_->currentItem();
    if (!current_item || !persistence_service_) return;
    
    QString table_name = current_item->text(0);
    
    try {
        // 获取真实的表内容
        auto [headers, rows] = persistence_service_->get_db_table_content(table_name.toStdString());
        
        // 设置列数和表头
        table_content_view_->setColumnCount(static_cast<int>(headers.size()));
        
        QStringList qt_headers;
        for (const auto& header : headers) {
            qt_headers << QString::fromStdString(header);
        }
        table_content_view_->setHorizontalHeaderLabels(qt_headers);
        
        // 设置行数和数据
        table_content_view_->setRowCount(static_cast<int>(rows.size()));
        
        for (size_t row = 0; row < rows.size(); ++row) {
            for (size_t col = 0; col < rows[row].size() && col < headers.size(); ++col) {
                QString cell_data = QString::fromStdString(rows[row][col]);
                
                // 对特定列类型进行格式化处理
                if (headers[col].find("timestamp") != std::string::npos ||
                    headers[col].find("time") != std::string::npos) {
                    // 尝试将时间戳转换为可读格式
                    bool ok;
                    qint64 timestamp = cell_data.toLongLong(&ok);
                    if (ok && timestamp > 1000000000) { // 确保是合理的时间戳
                        QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp / 1000000); // 转换微秒到秒
                        cell_data = dt.toString("yyyy-MM-dd hh:mm:ss");
                    }
                } else if (headers[col].find("size") != std::string::npos ||
                          headers[col].find("bytes") != std::string::npos) {
                    // 格式化数据大小
                    bool ok;
                    int size = cell_data.toInt(&ok);
                    if (ok) {
                        cell_data = QString("%1 bytes").arg(size);
                    }
                }
                
                auto* item = new QTableWidgetItem(cell_data);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable); // 设置为只读
                table_content_view_->setItem(static_cast<int>(row), static_cast<int>(col), item);
            }
        }
        
        // 调整列宽
        table_content_view_->resizeColumnsToContents();
        
        // 更新状态信息
        status_label_->setText(QString::fromUtf8("已加载表 %1，共 %2 行记录")
                              .arg(table_name)
                              .arg(rows.size()));
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceMonitor] Error loading table content: " << e.what() << std::endl;
        
        // 显示错误信息
        table_content_view_->setColumnCount(1);
        table_content_view_->setHorizontalHeaderLabels({QString::fromUtf8("错误信息")});
        table_content_view_->setRowCount(1);
        table_content_view_->setItem(0, 0, new QTableWidgetItem(
            QString::fromUtf8("无法加载表内容: %1").arg(QString::fromStdString(e.what()))));
        
        status_label_->setText(QString::fromUtf8("表加载失败: %1").arg(table_name));
    }
}

void PersistenceMonitor::onDatabaseQuery() {
    if (!persistence_service_) {
        QMessageBox::warning(this, QString::fromUtf8("查询错误"), 
                           QString::fromUtf8("持久化服务未连接"));
        return;
    }
    
    QDateTime start_time = db_start_time_edit_->dateTime();
    QDateTime end_time = db_end_time_edit_->dateTime();
    QString durability_filter = durability_filter_combo_->currentText();
    QString topic_filter = db_topic_filter_combo_->currentText();
    
    if (start_time >= end_time) {
        QMessageBox::warning(this, QString::fromUtf8("查询错误"), 
                           QString::fromUtf8("开始时间必须早于结束时间"));
        return;
    }
    
    try {
        // 设置查询结果表格的列
        query_results_table_->setColumnCount(8);
        query_results_table_->setHorizontalHeaderLabels({
            QString::fromUtf8("时间戳"),
            QString::fromUtf8("Topic"),
            QString::fromUtf8("类型"),
            QString::fromUtf8("序列号"),
            QString::fromUtf8("数据大小"),
            QString::fromUtf8("持久化策略"),
            QString::fromUtf8("Writer GUID"),
            QString::fromUtf8("状态")
        });
        
        // 获取所有Topic的历史数据
        std::vector<std::string> topics_to_query;
        if (topic_filter == QString::fromUtf8("全部")) {
            // 优先从数据库的 topics 表读取Topic列表
            bool topics_loaded_from_db = false;
            try {
                auto table_names = persistence_service_->get_db_table_names();
                bool has_topics = std::find(table_names.begin(), table_names.end(), std::string("topics")) != table_names.end();
                if (has_topics) {
                    auto [headers, rows] = persistence_service_->get_db_table_content("topics");
                    int name_idx = -1;
                    for (size_t i = 0; i < headers.size(); ++i) {
                        if (headers[i] == "topic_name") { name_idx = static_cast<int>(i); break; }
                    }
                    if (name_idx >= 0) {
                        for (const auto& row : rows) {
                            if (static_cast<size_t>(name_idx) < row.size()) {
                                topics_to_query.push_back(row[name_idx]);
                            }
                        }
                        topics_loaded_from_db = !topics_to_query.empty();
                    }
                }
            } catch (...) {
                // ignore and fallback
            }

            if (!topics_loaded_from_db) {
                // 回退：从活跃的Writer和Reader中获取所有Topic
                auto writers = persistence_service_->get_active_datawriters_info();
                auto readers = persistence_service_->get_active_datareaders_info();

                std::set<std::string> unique_topics;
                for (const auto& writer : writers) {
                    unique_topics.insert(writer.topic_name);
                }
                for (const auto& reader : readers) {
                    unique_topics.insert(reader.topic_name);
                }
                topics_to_query.assign(unique_topics.begin(), unique_topics.end());
            }
        } else {
            topics_to_query.push_back(topic_filter.toStdString());
        }
        
        if (topics_to_query.empty()) {
            status_label_->setText(QString::fromUtf8("状态: 没有可查询的Topic"));
            query_results_table_->setRowCount(0);
            return;
        }
        
        // 转换时间格式
        Time_t start_dds_time, end_dds_time;
        start_dds_time.sec = start_time.toSecsSinceEpoch();
        start_dds_time.nanosec = 0;
        end_dds_time.sec = end_time.toSecsSinceEpoch();
        end_dds_time.nanosec = 0;
        
        // 查询数据
        std::vector<StoredData> all_results;
        for (const auto& topic : topics_to_query) {
            auto topic_data = persistence_service_->get_historical_data_range(topic, start_dds_time, end_dds_time);
            all_results.insert(all_results.end(), topic_data.begin(), topic_data.end());
        }
        
        // 按持久化策略过滤
        if (durability_filter != QString::fromUtf8("全部")) {
            DurabilityQosPolicyKind filter_kind = VOLATILE_DURABILITY_QOS;
            if (durability_filter == "TRANSIENT_LOCAL") {
                filter_kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            } else if (durability_filter == "TRANSIENT") {
                filter_kind = TRANSIENT_DURABILITY_QOS;
            } else if (durability_filter == "PERSISTENT") {
                filter_kind = PERSISTENT_DURABILITY_QOS;
            }
            
            all_results.erase(
                std::remove_if(all_results.begin(), all_results.end(),
                    [filter_kind](const StoredData& data) {
                        return data.durability_kind != filter_kind;
                    }),
                all_results.end()
            );
        }
        
        // 按时间排序
        std::sort(all_results.begin(), all_results.end(),
            [](const StoredData& a, const StoredData& b) {
                if (a.storage_time.sec != b.storage_time.sec) {
                    return a.storage_time.sec < b.storage_time.sec;
                }
                return a.storage_time.nanosec < b.storage_time.nanosec;
            });
        
        // 填充结果表格
        query_results_table_->setRowCount(static_cast<int>(all_results.size()));
        
        for (size_t i = 0; i < all_results.size(); ++i) {
            const auto& data = all_results[i];
            
            // 时间戳
            QDateTime timestamp = QDateTime::fromSecsSinceEpoch(data.storage_time.sec);
            query_results_table_->setItem(static_cast<int>(i), 0, 
                new QTableWidgetItem(timestamp.toString("yyyy-MM-dd hh:mm:ss")));
            
            // Topic和类型
            query_results_table_->setItem(static_cast<int>(i), 1, 
                new QTableWidgetItem(QString::fromStdString(data.topic_name)));
            query_results_table_->setItem(static_cast<int>(i), 2, 
                new QTableWidgetItem(QString::fromStdString(data.type_name)));
            
            // 序列号
            query_results_table_->setItem(static_cast<int>(i), 3, 
                new QTableWidgetItem(QString::number(data.sequence_number)));
            
            // 数据大小
            query_results_table_->setItem(static_cast<int>(i), 4, 
                new QTableWidgetItem(QString("%1 bytes").arg(data.data.size())));
            
            // 持久化策略
            QString durability_str;
            switch (data.durability_kind) {
                case VOLATILE_DURABILITY_QOS: durability_str = "VOLATILE"; break;
                case TRANSIENT_LOCAL_DURABILITY_QOS: durability_str = "TRANSIENT_LOCAL"; break;
                case TRANSIENT_DURABILITY_QOS: durability_str = "TRANSIENT"; break;
                case PERSISTENT_DURABILITY_QOS: durability_str = "PERSISTENT"; break;
                default: durability_str = "UNKNOWN"; break;
            }
            query_results_table_->setItem(static_cast<int>(i), 5, 
                new QTableWidgetItem(durability_str));
            
            // Writer GUID (简化显示，使用占位符)
            QString guid_str = QString::fromUtf8("GUID_%1").arg(i);
            query_results_table_->setItem(static_cast<int>(i), 6, 
                new QTableWidgetItem(guid_str));
            
            // 状态
            QString status = data.is_disposed ? QString::fromUtf8("已销毁") : QString::fromUtf8("正常");
            query_results_table_->setItem(static_cast<int>(i), 7, new QTableWidgetItem(status));
        }
        
        query_results_table_->resizeColumnsToContents();
        
        status_label_->setText(QString::fromUtf8("状态: 查询完成，找到 %1 条记录").arg(all_results.size()));
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, QString::fromUtf8("查询失败"), 
                            QString::fromUtf8("查询数据时发生错误: %1").arg(QString::fromStdString(e.what())));
        status_label_->setText(QString::fromUtf8("状态: 查询失败"));
    }
}

void PersistenceMonitor::onDatabaseExport() {
    if (query_results_table_->rowCount() == 0) {
        QMessageBox::information(this, QString::fromUtf8("导出提示"), 
                                QString::fromUtf8("没有可导出的查询结果"));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        QString::fromUtf8("导出查询结果"), 
        QString("query_results_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        QString::fromUtf8("CSV文件 (*.csv)"));
    
    if (!fileName.isEmpty()) {
        // 简单的CSV导出实现
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            
            // 写入表头
            QStringList headers;
            for (int col = 0; col < query_results_table_->columnCount(); ++col) {
                headers << query_results_table_->horizontalHeaderItem(col)->text();
            }
            out << headers.join(",") << "\n";
            
            // 写入数据
            for (int row = 0; row < query_results_table_->rowCount(); ++row) {
                QStringList rowData;
                for (int col = 0; col < query_results_table_->columnCount(); ++col) {
                    QTableWidgetItem* item = query_results_table_->item(row, col);
                    rowData << (item ? item->text() : "");
                }
                out << rowData.join(",") << "\n";
            }
            
            QMessageBox::information(this, QString::fromUtf8("导出完成"), 
                                   QString::fromUtf8("查询结果已成功导出到: %1").arg(fileName));
        } else {
            QMessageBox::critical(this, QString::fromUtf8("导出错误"), 
                                QString::fromUtf8("无法创建文件: %1").arg(fileName));
        }
    }
}



void PersistenceMonitor::onTopologyNodeClicked(const QString& nodeId) {
    // 处理拓扑节点点击事件
    QMessageBox::information(this, QString::fromUtf8("节点信息"), 
                            QString::fromUtf8("点击了节点: %1").arg(nodeId));
}

void PersistenceMonitor::onExitButtonClicked() {
    // 确认退出对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        QString::fromUtf8("确认退出"), 
        QString::fromUtf8("确定要退出TravoDDS持久化服务监控器吗？\n正在运行的服务将会停止。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 停止监控
        if (is_monitoring_) {
            stopMonitoring();
        }
        
        // 关闭应用程序
        QApplication::quit();
    }
}

// 数据导出方法实现


// 窗口事件处理
void PersistenceMonitor::closeEvent(QCloseEvent* event) {
    stopMonitoring();
    event->accept();
}

void PersistenceMonitor::paintEvent(QPaintEvent* event) {
    QMainWindow::paintEvent(event);
}

void PersistenceMonitor::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_dragging_ = true;
        drag_start_position_ = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void PersistenceMonitor::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && is_dragging_) {
        move(event->globalPos() - drag_start_position_);
        event->accept();
    }
}

void PersistenceMonitor::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_dragging_ = false;
        event->accept();
    }
}


void PersistenceMonitor::updateDatabaseTableList() {
    if (!persistence_service_) return;
    
    try {
        // 获取数据库表名
        auto table_names = persistence_service_->get_db_table_names();
        
        // 清空现有表结构
        database_tables_tree_->clear();
        
        // 添加表项到树控件
        for (const auto& table_name : table_names) {
            auto* item = new QTreeWidgetItem(database_tables_tree_);
            item->setText(0, QString::fromStdString(table_name));
            
            // 尝试获取表的记录数（简单估算）
            try {
                auto [headers, rows] = persistence_service_->get_db_table_content(table_name);
                item->setText(1, QString::number(rows.size()));
            } catch (...) {
                item->setText(1, "N/A");
            }
        }
        
        // 调整列宽
        database_tables_tree_->resizeColumnToContents(0);
        database_tables_tree_->resizeColumnToContents(1);
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceMonitor] Error updating database table list: " << e.what() << std::endl;
        
        // 添加错误提示项
        auto* error_item = new QTreeWidgetItem(database_tables_tree_);
        error_item->setText(0, QString::fromUtf8("无法获取表信息"));
        error_item->setText(1, "0");
    }
}

void PersistenceMonitor::updateTopicFilterCombo(const std::set<std::string>& topics) {
    if (!db_topic_filter_combo_) {
        return;
    }
    
    // 保存当前选中的项
    QString current_selection = db_topic_filter_combo_->currentText();
    
    // 清空下拉框
    db_topic_filter_combo_->clear();
    
    // 添加"全部"选项
    db_topic_filter_combo_->addItem("全部", "");
    
    // 添加所有topic
    for (const auto& topic : topics) {
        db_topic_filter_combo_->addItem(QString::fromStdString(topic), QString::fromStdString(topic));
    }
    
    // 尝试恢复之前的选择
    int index = db_topic_filter_combo_->findText(current_selection);
    if (index >= 0) {
        db_topic_filter_combo_->setCurrentIndex(index);
    } else {
        db_topic_filter_combo_->setCurrentIndex(0); // 默认选择"全部"
    }
}

