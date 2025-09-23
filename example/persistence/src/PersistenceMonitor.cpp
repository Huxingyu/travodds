#include "PersistenceMonitor.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <QFont>

using namespace TRAVODDS;

PersistenceMonitor::PersistenceMonitor(QWidget* parent)
    : QMainWindow(parent)
    , persistence_service_(nullptr)
    , main_tabs_(nullptr)
    , is_monitoring_(false)
    , refresh_interval_ms_(2000) // 2秒刷新间隔
    , is_dragging_(false)
{
    // 设置无边框窗口
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    menuBar()->hide();

    QFont font("Microsoft YaHei");

    setWindowTitle(QString::fromUtf8("TravoDDS 持久化服务监控器 v1.0"));
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // 设置应用程序图标
    setupIcons();
    
    // 初始化UI
    setupUI();
    setupMenuBar();
    setupStatusBar();
    
    // 应用现代样式
    applyModernStyle();
    
    // 初始化定时器
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, &QTimer::timeout, this, &PersistenceMonitor::refreshAllData);
    
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
    // 设置全局样式表，使用现代化设计
    setStyleSheet(R"(
        /* 主窗口样式 */
        QMainWindow {
            background-color: #f8f9fa;
            font-family: 'Segoe UI', 'Microsoft YaHei', Arial, sans-serif;
        }
        
        /* 标签页样式 */
        QTabWidget::pane {
            border: 1px solid #dee2e6;
            background-color: white;
            border-radius: 8px;
        }
        
        QTabWidget::tab-bar {
            alignment: left;
        }
        
        QTabBar::tab {
            background-color: #e9ecef;
            color: #495057;
            padding: 12px 20px;
            margin-right: 2px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            border: 1px solid #dee2e6;
            border-bottom: none;
            font-weight: 500;
        }
        
        QTabBar::tab:selected {
            background-color: white;
            color: #0d6efd;
            border-bottom: 2px solid #0d6efd;
        }
        
        QTabBar::tab:hover:!selected {
            background-color: #f8f9fa;
        }
        
        /* 分组框样式 */
        QGroupBox {
            font-weight: bold;
            color: #343a40;
            border: 2px solid #dee2e6;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 10px;
            background-color: white;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 15px;
            padding: 0 8px 0 8px;
            color: #0d6efd;
            font-size: 14px;
        }
        
        /* 表格样式 */
        QTableWidget {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background-color: white;
            alternate-background-color: #f8f9fa;
            gridline-color: #e9ecef;
            selection-background-color: #e3f2fd;
        }
        
        QTableWidget::item {
            padding: 8px;
            border: none;
        }
        
        QTableWidget::item:selected {
            background-color: #bbdefb;
            color: #1565c0;
        }
        
        QHeaderView::section {
            background-color: #e9ecef;
            color: #495057;
            padding: 10px;
            border: none;
            border-right: 1px solid #dee2e6;
            font-weight: bold;
        }
        
        /* 树形控件样式 */
        QTreeWidget {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background-color: white;
            alternate-background-color: #f8f9fa;
            selection-background-color: #e3f2fd;
        }
        
        QTreeWidget::item {
            padding: 6px;
            border: none;
        }
        
        QTreeWidget::item:selected {
            background-color: #bbdefb;
            color: #1565c0;
        }
        
        /* 标签样式 */
        QLabel {
            color: #495057;
            font-size: 13px;
        }
        
        /* 进度条样式 */
        QProgressBar {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background-color: #f8f9fa;
            text-align: center;
            font-weight: bold;
        }
        
        QProgressBar::chunk {
            background-color: #28a745;
            border-radius: 5px;
        }
        
        /* 分割器样式 */
        QSplitter::handle {
            background-color: #dee2e6;
            width: 2px;
            height: 2px;
        }
        
        QSplitter::handle:hover {
            background-color: #0d6efd;
        }
    )");
    
    // 创建中央部件
    auto* central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    
    // 创建主布局，使用更现代的间距
    auto* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(15, 50, 15, 15); // 更合适的边距
    main_layout->setSpacing(10); // 添加组件间距
    
    // 创建选项卡
    main_tabs_ = new QTabWidget();
    
    setupDomainTopicTab();
    setupDatabaseTab();
    
    main_layout->addWidget(main_tabs_);
}

void PersistenceMonitor::setupDomainTopicTab() {
    domain_topic_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(domain_topic_widget_);
    layout->setSpacing(15); // 增加组件间距
    layout->setContentsMargins(15, 15, 15, 15); // 添加边距
    
    // 创建分割器
    auto* splitter = new QSplitter(Qt::Horizontal);
    
    // 左侧：域和主题树
    auto* left_widget = new QWidget();
    auto* left_layout = new QVBoxLayout(left_widget);
    left_layout->setSpacing(10); // 左侧布局间距
    
    auto* domain_group = new QGroupBox(QString::fromUtf8("域和主题结构"));
    auto* domain_layout = new QVBoxLayout(domain_group);
    
    domain_tree_ = new QTreeWidget();
    domain_tree_->setHeaderLabels({QString::fromUtf8("名称"), QString::fromUtf8("类型"), 
                                   QString::fromUtf8("持久化策略"), QString::fromUtf8("可靠性策略")});
    domain_tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    
    domain_layout->addWidget(domain_tree_);
    left_layout->addWidget(domain_group);
    
    // 右侧：详细信息
    auto* right_widget = new QWidget();
    auto* right_layout = new QVBoxLayout(right_widget);
    right_layout->setSpacing(12); // 右侧布局间距
    
    // 主题统计表格
    auto* topic_stats_group = new QGroupBox(QString::fromUtf8("主题统计"));
    auto* topic_stats_layout = new QVBoxLayout(topic_stats_group);
    
    topic_table_ = new QTableWidget();
    topic_table_->setColumnCount(4);
    topic_table_->setHorizontalHeaderLabels({
        QString::fromUtf8("主题名称"), QString::fromUtf8("DataWriter数量"), 
        QString::fromUtf8("DataReader数量"), QString::fromUtf8("类型名称")
    });
    topic_table_->horizontalHeader()->setStretchLastSection(true);
    topic_stats_layout->addWidget(topic_table_);
    
    // 主题详细信息
    auto* topic_group = new QGroupBox(QString::fromUtf8("主题详细信息"));
    auto* topic_layout = new QVBoxLayout(topic_group);
    
    topic_details_ = new QTextEdit();
    topic_details_->setReadOnly(true);
    topic_details_->setMaximumHeight(200);
    topic_layout->addWidget(topic_details_);
    
    // DataWriter表格
    auto* writer_group = new QGroupBox("DataWriter");
    auto* writer_layout = new QVBoxLayout(writer_group);
    
    datawriter_table_ = new QTableWidget();
    datawriter_table_->setColumnCount(6);
    datawriter_table_->setHorizontalHeaderLabels({
        QString::fromUtf8("句柄"), QString::fromUtf8("主题"), QString::fromUtf8("类型"), 
        QString::fromUtf8("持久化策略"), QString::fromUtf8("可靠性策略"), QString::fromUtf8("状态")
    });
    datawriter_table_->horizontalHeader()->setStretchLastSection(true);
    writer_layout->addWidget(datawriter_table_);
    
    // DataReader表格
    auto* reader_group = new QGroupBox("DataReader");
    auto* reader_layout = new QVBoxLayout(reader_group);
    
    datareader_table_ = new QTableWidget();
    datareader_table_->setColumnCount(6);
    datareader_table_->setHorizontalHeaderLabels({
        QString::fromUtf8("句柄"), QString::fromUtf8("主题"), QString::fromUtf8("类型"), 
        QString::fromUtf8("持久化策略"), QString::fromUtf8("可靠性策略"), QString::fromUtf8("状态")
    });
    datareader_table_->horizontalHeader()->setStretchLastSection(true);
    reader_layout->addWidget(datareader_table_);
    
    right_layout->addWidget(topic_stats_group);
    right_layout->addWidget(topic_group);
    right_layout->addWidget(writer_group);
    right_layout->addWidget(reader_group);
    
    // 添加运行统计表格到右侧
    auto* stats_group = new QGroupBox(QString::fromUtf8("运行统计"));
    auto* stats_layout = new QVBoxLayout(stats_group);
    
    stats_table_ = new QTableWidget();
    stats_table_->setColumnCount(2);
    stats_table_->setHorizontalHeaderLabels({QString::fromUtf8("统计项"), QString::fromUtf8("值")});
    stats_table_->horizontalHeader()->setStretchLastSection(true);
    stats_layout->addWidget(stats_table_);
    
    right_layout->addWidget(stats_group);
    
    splitter->addWidget(left_widget);
    splitter->addWidget(right_widget);
    splitter->setSizes({350, 950}); // 更好的比例：左侧350px，右侧950px
    
    layout->addWidget(splitter);
    
    main_tabs_->addTab(domain_topic_widget_, QString::fromUtf8("域和主题"));
    
    // 连接选择信号
    connect(domain_tree_, &QTreeWidget::itemSelectionChanged, 
            this, &PersistenceMonitor::onTopicSelected);
}

void PersistenceMonitor::setupDatabaseTab() {
    database_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(database_widget_);
    layout->setSpacing(15); // 数据库页面组件间距
    layout->setContentsMargins(15, 15, 15, 15); // 数据库页面边距
    
    // 数据库基本信息
    auto* info_group = new QGroupBox(QString::fromUtf8("数据库信息"));
    auto* info_layout = new QGridLayout(info_group);
    
    info_layout->addWidget(new QLabel(QString::fromUtf8("数据库路径:")), 0, 0);
    db_path_label_ = new QLabel(QString::fromUtf8("未设置"));
    info_layout->addWidget(db_path_label_, 0, 1);
    
    info_layout->addWidget(new QLabel(QString::fromUtf8("数据库大小:")), 1, 0);
    db_size_label_ = new QLabel("0 MB");
    info_layout->addWidget(db_size_label_, 1, 1);
    
    info_layout->addWidget(new QLabel(QString::fromUtf8("总主题数:")), 2, 0);
    total_topics_label_ = new QLabel("0");
    info_layout->addWidget(total_topics_label_, 2, 1);
    
    info_layout->addWidget(new QLabel(QString::fromUtf8("总样本数:")), 3, 0);
    total_samples_label_ = new QLabel("0");
    info_layout->addWidget(total_samples_label_, 3, 1);
    
    // 内存和磁盘使用情况
    auto* usage_group = new QGroupBox(QString::fromUtf8("资源使用情况"));
    auto* usage_layout = new QGridLayout(usage_group);
    
    usage_layout->addWidget(new QLabel(QString::fromUtf8("内存缓存使用:")), 0, 0);
    memory_usage_bar_ = new QProgressBar();
    memory_usage_bar_->setTextVisible(true);
    usage_layout->addWidget(memory_usage_bar_, 0, 1);
    
    usage_layout->addWidget(new QLabel(QString::fromUtf8("磁盘使用:")), 1, 0);
    disk_usage_bar_ = new QProgressBar();
    disk_usage_bar_->setTextVisible(true);
    usage_layout->addWidget(disk_usage_bar_, 1, 1);
    
    // 数据库表内容可视化
    auto* tables_group = new QGroupBox(QString::fromUtf8("数据库表内容"));
    auto* tables_layout = new QHBoxLayout(tables_group);
    tables_layout->setSpacing(12); // 表内容组件间距
    
    // 左侧：表列表
    auto* left_tables_widget = new QWidget();
    auto* left_tables_layout = new QVBoxLayout(left_tables_widget);
    left_tables_layout->setSpacing(8); // 左侧组件间距
    left_tables_layout->setContentsMargins(8, 8, 8, 8); // 左侧边距
    
    auto* table_list_label = new QLabel(QString::fromUtf8("数据库表列表"));
    table_list_label->setStyleSheet("font-weight: bold; margin: 5px;");
    left_tables_layout->addWidget(table_list_label);
    
    database_tables_tree_ = new QTreeWidget();
    database_tables_tree_->setHeaderLabels({QString::fromUtf8("表名"), QString::fromUtf8("记录数")});
    database_tables_tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    left_tables_layout->addWidget(database_tables_tree_);
    
    // 右侧：表内容
    auto* right_tables_widget = new QWidget();
    auto* right_tables_layout = new QVBoxLayout(right_tables_widget);
    right_tables_layout->setSpacing(8); // 右侧组件间距
    right_tables_layout->setContentsMargins(8, 8, 8, 8); // 右侧边距
    
    auto* content_label = new QLabel(QString::fromUtf8("表内容 (点击左侧表名查看)"));
    content_label->setStyleSheet("font-weight: bold; margin: 5px;");
    right_tables_layout->addWidget(content_label);
    
    table_content_view_ = new QTableWidget();
    table_content_view_->setAlternatingRowColors(true);
    table_content_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_content_view_->setSortingEnabled(true);
    right_tables_layout->addWidget(table_content_view_);
    
    // 设置分割器
    auto* tables_splitter = new QSplitter(Qt::Horizontal);
    tables_splitter->addWidget(left_tables_widget);
    tables_splitter->addWidget(right_tables_widget);
    tables_splitter->setSizes({250, 750}); // 左侧表列表较窄，右侧内容更宽
    
    tables_layout->addWidget(tables_splitter);
    
    // 连接表选择信号
    connect(database_tables_tree_, &QTreeWidget::itemSelectionChanged, 
            this, &PersistenceMonitor::onDatabaseTableSelected);
    
    layout->addWidget(info_group);
    layout->addWidget(usage_group);
    layout->addWidget(tables_group);
    
    main_tabs_->addTab(database_widget_, QString::fromUtf8("数据库状态"));
}

void PersistenceMonitor::setupMenuBar() {
    auto* menu_bar = menuBar();
    
    // 文件菜单
    auto* file_menu = menu_bar->addMenu(QString::fromUtf8("文件"));
    
    auto* exit_action = file_menu->addAction(QString::fromUtf8("退出"));
    connect(exit_action, &QAction::triggered, this, &QWidget::close);
    
    // 帮助菜单
    auto* help_menu = menu_bar->addMenu(QString::fromUtf8("帮助"));
    
    auto* about_action = help_menu->addAction(QString::fromUtf8("关于"));
    connect(about_action, &QAction::triggered, [this]() {
        QMessageBox::about(this, QString::fromUtf8("关于"), 
            QString::fromUtf8("TravoDDS 持久化服务监控器 v1.0\n\n"
            "用于监控和管理TravoDDS持久化服务的Qt应用程序\n"
            "支持实时查看域、主题、数据库状态和运行统计\n\n"
            "© 2025 AGIROS"));
    });
}

void PersistenceMonitor::setupStatusBar() {
    auto* status_bar = statusBar();
    
    status_label_ = new QLabel(QString::fromUtf8("状态: 未启动"));
    status_label_->setStyleSheet("color: gray; font-weight: bold;");
    
    uptime_label_ = new QLabel(QString::fromUtf8("运行时间: 00:00:00"));
    
    status_bar->addWidget(status_label_);
    status_bar->addPermanentWidget(uptime_label_);
}

void PersistenceMonitor::applyModernStyle() {
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        
        QTabWidget::pane {
            border: 1px solid #c0c0c0;
            background-color: white;
        }
        
        QTabWidget::tab-bar {
            alignment: left;
        }
        
        QTabBar::tab {
            background-color: #e1e1e1;
            border: 1px solid #c0c0c0;
            border-bottom-color: #C2C7CB;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            min-width: 8ex;
            padding: 8px;
        }
        
        QTabBar::tab:selected, QTabBar::tab:hover {
            background-color: white;
        }
        
        QTabBar::tab:selected {
            border-color: #9B9B9B;
            border-bottom-color: white;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 5px;
            margin: 3px;
            padding-top: 10px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #106ebe;
        }
        
        QPushButton:pressed {
            background-color: #005a9e;
        }
        
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        
        QTableWidget {
            gridline-color: #e0e0e0;
            background-color: white;
            alternate-background-color: #f9f9f9;
        }
        
        QTableWidget::item {
            padding: 4px;
        }
        
        QTreeWidget {
            background-color: white;
            alternate-background-color: #f9f9f9;
        }
        
        QProgressBar {
            border: 2px solid grey;
            border-radius: 5px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 3px;
        }
    )");
}

void PersistenceMonitor::setupIcons() {
    // 这里可以设置应用程序图标
    // setWindowIcon(QIcon(":/icons/persistence_monitor.png"));
}

void PersistenceMonitor::updateDomainInfo() {
    if (!persistence_service_) {
        return;
    }
    
    // 清空域树
    domain_tree_->clear();
    
    // 创建根节点
    auto* root_item = new QTreeWidgetItem(domain_tree_);
    root_item->setText(0, "Domain 0");
    root_item->setText(1, "Domain");
    root_item->setExpanded(true);
    
    // 获取主题信息并添加到树中
    // TODO: 从持久化服务获取实际的主题信息
    // 这里先添加示例数据
    auto stats = persistence_service_->get_statistics();
    
    for (const auto& [key, value] : stats) {
        if (key.find("topic_") == 0) {
            auto* topic_item = new QTreeWidgetItem(root_item);
            topic_item->setText(0, QString::fromStdString(key));
            topic_item->setText(1, "Topic");
            topic_item->setText(2, "PERSISTENT");
            topic_item->setText(3, "RELIABLE");
        }
    }
}

void PersistenceMonitor::updateTopicInfo() {
    updateTopicTable();
    updateDataWriterTable();
    updateDataReaderTable();
}

void PersistenceMonitor::updateTopicTable() {
    // 使用 get_active_datawriters_info 和 get_active_datareaders_info
    if (!persistence_service_) {
        return;
    }
    
    auto writers = persistence_service_->get_active_datawriters_info();
    auto readers = persistence_service_->get_active_datareaders_info();

    std::set<std::string> topics;
    for(const auto& writer : writers) topics.insert(writer.topic_name);
    for(const auto& reader : readers) topics.insert(reader.topic_name);

    topic_table_->setRowCount(0);
    topic_table_->setRowCount(topics.size());
    int row = 0;
    for (const auto& topic_name : topics) {
        int writer_count = std::count_if(writers.begin(), writers.end(), 
            [&](const auto& w){ return w.topic_name == topic_name; });
        int reader_count = std::count_if(readers.begin(), readers.end(), 
            [&](const auto& r){ return r.topic_name == topic_name; });

        topic_table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(topic_name)));
        topic_table_->setItem(row, 1, new QTableWidgetItem(QString::number(writer_count)));
        topic_table_->setItem(row, 2, new QTableWidgetItem(QString::number(reader_count)));
        
        // 获取第一个实体的type名称
        std::string type_name = "Unknown";
        if (!writers.empty()) {
            auto writer_it = std::find_if(writers.begin(), writers.end(),
                [&](const auto& w) { return w.topic_name == topic_name; });
            if (writer_it != writers.end()) {
                type_name = writer_it->type_name;
            }
        } else if (!readers.empty()) {
            auto reader_it = std::find_if(readers.begin(), readers.end(),
                [&](const auto& r) { return r.topic_name == topic_name; });
            if (reader_it != readers.end()) {
                type_name = reader_it->type_name;
            }
        }
        topic_table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(type_name)));
        row++;
    }
}

void PersistenceMonitor::updateDataWriterTable() {
    datawriter_table_->setRowCount(0);
    
    if (!persistence_service_) {
        return;
    }
    
    try {
        // 使用新的内存接口获取活跃DataWriter信息
        auto datawriter_infos = persistence_service_->get_active_datawriters_info();
        
        for (size_t i = 0; i < datawriter_infos.size(); ++i) {
            const auto& info = datawriter_infos[i];
            
            datawriter_table_->insertRow(i);
            
            // GUID
            std::stringstream guid_stream;
            guid_stream << "0x" << std::hex << std::uppercase 
                       << info.key.value[0] << info.key.value[1] 
                       << info.key.value[2] << info.key.value[3];
            datawriter_table_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(guid_stream.str())));
            
            // Topic Name
            datawriter_table_->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(info.topic_name.c_str())));
            
            // Type Name
            datawriter_table_->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(info.type_name.c_str())));
            
            // Durability
            QString durability_str;
            switch (info.durability_kind) {
                case VOLATILE_DURABILITY_QOS:
                    durability_str = "VOLATILE";
                    break;
                case TRANSIENT_LOCAL_DURABILITY_QOS:
                    durability_str = "TRANSIENT_LOCAL";
                    break;
                case TRANSIENT_DURABILITY_QOS:
                    durability_str = "TRANSIENT";
                    break;
                case PERSISTENT_DURABILITY_QOS:
                    durability_str = "PERSISTENT";
                    break;
                default:
                    durability_str = "UNKNOWN";
                    break;
            }
            datawriter_table_->setItem(i, 3, new QTableWidgetItem(durability_str));
            
            // Reliability
            QString reliability_str;
            switch (info.reliability_kind) {
                case BEST_EFFORT_RELIABILITY_QOS:
                    reliability_str = "BEST_EFFORT";
                    break;
                case RELIABLE_RELIABILITY_QOS:
                    reliability_str = "RELIABLE";
                    break;
                default:
                    reliability_str = "UNKNOWN";
                    break;
            }
            datawriter_table_->setItem(i, 4, new QTableWidgetItem(reliability_str));
            
            // Status - 直接从内存中的info获取
            QString status_str = info.is_active ? QString::fromUtf8("活跃") : QString::fromUtf8("非活跃");
            datawriter_table_->setItem(i, 5, new QTableWidgetItem(status_str));
        }
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceMonitor] Error updating DataWriter table: " << e.what() << std::endl;
    }
}

void PersistenceMonitor::updateDataReaderTable() {
    datareader_table_->setRowCount(0);
    
    if (!persistence_service_) {
        return;
    }
    
    try {
        // 使用新的内存接口获取活跃DataReader信息
        auto datareader_infos = persistence_service_->get_active_datareaders_info();
        
        for (size_t i = 0; i < datareader_infos.size(); ++i) {
            const auto& info = datareader_infos[i];
            
            datareader_table_->insertRow(i);
            
            // GUID
            std::stringstream guid_stream;
            guid_stream << "0x" << std::hex << std::uppercase 
                       << info.key.value[0] << info.key.value[1] 
                       << info.key.value[2] << info.key.value[3];
            datareader_table_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(guid_stream.str())));
            
            // Topic Name
            datareader_table_->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(info.topic_name.c_str())));
            
            // Type Name  
            datareader_table_->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(info.type_name.c_str())));
            
            // Durability
            QString durability_str;
            switch (info.durability_kind) {
                case VOLATILE_DURABILITY_QOS:
                    durability_str = "VOLATILE";
                    break;
                case TRANSIENT_LOCAL_DURABILITY_QOS:
                    durability_str = "TRANSIENT_LOCAL";
                    break;
                case TRANSIENT_DURABILITY_QOS:
                    durability_str = "TRANSIENT";
                    break;
                case PERSISTENT_DURABILITY_QOS:
                    durability_str = "PERSISTENT";
                    break;
                default:
                    durability_str = "UNKNOWN";
                    break;
            }
            datareader_table_->setItem(i, 3, new QTableWidgetItem(durability_str));
            
            // Reliability
            QString reliability_str;
            switch (info.reliability_kind) {
                case BEST_EFFORT_RELIABILITY_QOS:
                    reliability_str = "BEST_EFFORT";
                    break;
                case RELIABLE_RELIABILITY_QOS:
                    reliability_str = "RELIABLE";
                    break;
                default:
                    reliability_str = "UNKNOWN";
                    break;
            }
            datareader_table_->setItem(i, 4, new QTableWidgetItem(reliability_str));
            
            // Status - 直接从内存中的info获取
            QString status_str = info.is_active ? QString::fromUtf8("活跃") : QString::fromUtf8("非活跃");
            datareader_table_->setItem(i, 5, new QTableWidgetItem(status_str));
        }
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceMonitor] Error updating DataReader table: " << e.what() << std::endl;
    }
}

void PersistenceMonitor::updateDatabaseInfo() {
    if (!persistence_service_) {
        return;
    }
    
    auto table_names = persistence_service_->get_db_table_names();
    auto stats = persistence_service_->get_statistics();
    
    // 更新基本信息
    db_path_label_->setText(QString::fromStdString(persistence_service_->get_config().database_path));
    
    auto it_db_size = stats.find("database_size_bytes");
    if (it_db_size != stats.end()) {
        double size_mb = it_db_size->second / (1024.0 * 1024.0);
        db_size_label_->setText(QString("%1 MB").arg(size_mb, 0, 'f', 2));
    }
    
    auto it_topics = stats.find("total_topics");
    if (it_topics != stats.end()) {
        total_topics_label_->setText(QString::number(it_topics->second));
    }
    
    auto it_samples = stats.find("total_samples");
    if (it_samples != stats.end()) {
        total_samples_label_->setText(QString::number(it_samples->second));
    }
    
    // 更新数据库表树状列表
    database_tables_tree_->clear();
    auto* db_root = new QTreeWidgetItem(database_tables_tree_, {"Database Tables", "Records"});
    
    // 获取每个表的记录数
    for (const auto& name : table_names) {
        auto content = persistence_service_->get_db_table_content(name);
        auto* table_item = new QTreeWidgetItem(db_root, {
            QString::fromStdString(name), 
            QString::number(content.second.size())
        });
    }
    db_root->setExpanded(true);
    
    updateDatabaseStats();
}

void PersistenceMonitor::updateMemoryCacheInfo() {
    if (!persistence_service_) {
        return;
    }
    
    auto usage = persistence_service_->get_memory_cache_usage();
    size_t current_bytes = usage.first;
    size_t max_bytes = usage.second;

    if (max_bytes > 0) {
        double current_mb = current_bytes / (1024.0 * 1024.0);
        double max_mb = max_bytes / (1024.0 * 1024.0);
        int percentage = static_cast<int>((current_mb / max_mb) * 100);
        
        memory_usage_bar_->setMaximum(100);
        memory_usage_bar_->setValue(percentage);
        memory_usage_bar_->setFormat(QString("%1 / %2 MB (%3%)")
            .arg(current_mb, 0, 'f', 2)
            .arg(max_mb, 0, 'f', 2)
            .arg(percentage));
    }
}

void PersistenceMonitor::updateDatabaseStats() {
    if (!persistence_service_) {
        return;
    }
    
    auto stats = persistence_service_->get_statistics();
    
    stats_table_->setRowCount(stats.size());
    int row = 0;
    
    for (const auto& [key, value] : stats) {
        stats_table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(key)));
        stats_table_->setItem(row, 1, new QTableWidgetItem(QString::number(value)));
        ++row;
    }
}

void PersistenceMonitor::refreshAllData() {
    if (!persistence_service_ || !is_monitoring_) {
        return;
    }

    // 更新所有UI部分
    updateTopicTable();
    updateDataWriterTable();
    updateDataReaderTable();
    updateDatabaseInfo();
    updateMemoryCacheInfo();
}

void PersistenceMonitor::onTopicSelected() {
    auto selected_items = domain_tree_->selectedItems();
    if (selected_items.isEmpty()) {
        stats_table_->clear();
        return;
    }
    
    auto* item = selected_items.first();
    
    // 如果是根节点，则不处理
    if (item->parent() == nullptr) {
        return;
    }
    
    std::string table_name = item->text(0).toStdString();
    if (!persistence_service_) {
        return;
    }
    
    auto content = persistence_service_->get_db_table_content(table_name);
    
    // 使用 stats_table_ 来显示表内容
    stats_table_->clear();
    if (!content.first.empty()) {
        stats_table_->setColumnCount(content.first.size());
        QStringList headers;
        for (const auto& header : content.first) {
            headers << QString::fromStdString(header);
        }
        stats_table_->setHorizontalHeaderLabels(headers);
        
        stats_table_->setRowCount(content.second.size());
        for (int row = 0; row < static_cast<int>(content.second.size()); ++row) {
            for (int col = 0; col < static_cast<int>(content.second[row].size()); ++col) {
                stats_table_->setItem(row, col, new QTableWidgetItem(QString::fromStdString(content.second[row][col])));
            }
        }
        stats_table_->resizeColumnsToContents();
    }
}

void PersistenceMonitor::onDatabaseTableSelected() {
    auto selected_items = database_tables_tree_->selectedItems();
    if (selected_items.isEmpty()) {
        table_content_view_->clear();
        table_content_view_->setRowCount(0);
        table_content_view_->setColumnCount(0);
        return;
    }
    
    auto* item = selected_items.first();
    
    // 如果是根节点，则不处理
    if (item->parent() == nullptr) {
        return;
    }
    
    std::string table_name = item->text(0).toStdString();
    if (!persistence_service_) {
        return;
    }
    
    auto content = persistence_service_->get_db_table_content(table_name);
    
    // 更新表内容显示
    table_content_view_->clear();
    if (!content.first.empty()) {
        table_content_view_->setColumnCount(content.first.size());
        QStringList headers;
        for (const auto& header : content.first) {
            headers << QString::fromStdString(header);
        }
        table_content_view_->setHorizontalHeaderLabels(headers);
        
        table_content_view_->setRowCount(content.second.size());
        for (int row = 0; row < static_cast<int>(content.second.size()); ++row) {
            for (int col = 0; col < static_cast<int>(content.second[row].size()); ++col) {
                table_content_view_->setItem(row, col, new QTableWidgetItem(QString::fromStdString(content.second[row][col])));
            }
        }
        table_content_view_->resizeColumnsToContents();
    }
}

void PersistenceMonitor::closeEvent(QCloseEvent* event) {
    stopMonitoring();
    QMainWindow::closeEvent(event);
}

void PersistenceMonitor::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    drawRoundedBackground();
    drawTitleBar();
}

void PersistenceMonitor::drawRoundedBackground() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    
    // 设置背景色
    QBrush backgroundBrush(QColor(245, 245, 245, 255)); // #f5f5f5
    painter.setBrush(backgroundBrush);
    
    // 绘制圆角矩形背景
    int radius = 15;
    painter.drawRoundedRect(rect(), radius, radius);
    
    // 绘制边框
    QPen borderPen(QColor(200, 200, 200, 255), 2); // #c8c8c8
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
}

void PersistenceMonitor::drawTitleBar() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // 绘制标题栏背景
    int titleBarHeight = 40;
    QRect titleBarRect(0, 0, width(), titleBarHeight);
    
    QBrush titleBrush(QColor(0, 120, 212, 255)); // #0078d4
    painter.setBrush(titleBrush);
    painter.setPen(Qt::NoPen);
    
    // 只绘制顶部圆角
    QPainterPath path;
    int radius = 15;
    path.moveTo(radius, 0);
    path.lineTo(width() - radius, 0);
    path.arcTo(width() - 2 * radius, 0, 2 * radius, 2 * radius, 90, -90);
    path.lineTo(width(), titleBarHeight);
    path.lineTo(0, titleBarHeight);
    path.lineTo(0, radius);
    path.arcTo(0, 0, 2 * radius, 2 * radius, 180, -90);
    path.closeSubpath();
    
    painter.drawPath(path);
    
    // 绘制标题文本
    QFont titleFont("Microsoft YaHei", 12, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor(255, 255, 255));
    
    QRect textRect(20, 0, width() - 40, titleBarHeight);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, 
                     QString::fromUtf8("TravoDDS 持久化服务监控器 v1.0"));
    
    // 绘制关闭按钮区域（可选）
    QRect closeButtonRect(width() - 35, 5, 30, 30);
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.setBrush(Qt::NoBrush);
    
    // 绘制X符号
    int margin = 8;
    painter.drawLine(closeButtonRect.left() + margin, closeButtonRect.top() + margin,
                     closeButtonRect.right() - margin, closeButtonRect.bottom() - margin);
    painter.drawLine(closeButtonRect.right() - margin, closeButtonRect.top() + margin,
                     closeButtonRect.left() + margin, closeButtonRect.bottom() - margin);
}

void PersistenceMonitor::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击在标题栏区域
        if (event->y() <= 40) {
            // 检查是否点击关闭按钮
            QRect closeButtonRect(width() - 35, 5, 30, 30);
            if (closeButtonRect.contains(event->pos())) {
                close();
                return;
            }
            
            // 开始拖拽
            is_dragging_ = true;
            drag_start_position_ = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
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
