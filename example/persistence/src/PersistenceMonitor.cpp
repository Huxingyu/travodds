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
        // 启动定时器定期刷新数据
        refresh_timer_->start(1000); // 每秒刷新
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
    // 创建中央部件
    auto* central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    
    // 创建主布局，为标题栏留出空间
    auto* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(10, 45, 10, 10); // 顶部留出标题栏空间
    
    // 创建选项卡
    main_tabs_ = new QTabWidget();
    
    setupDomainTopicTab();
    setupDatabaseTab();
    
    main_layout->addWidget(main_tabs_);
}

void PersistenceMonitor::setupDomainTopicTab() {
    domain_topic_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(domain_topic_widget_);
    
    // 创建分割器
    auto* splitter = new QSplitter(Qt::Horizontal);
    
    // 左侧：域和主题树
    auto* left_widget = new QWidget();
    auto* left_layout = new QVBoxLayout(left_widget);
    
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
    
    right_layout->addWidget(topic_group);
    right_layout->addWidget(writer_group);
    right_layout->addWidget(reader_group);
    
    splitter->addWidget(left_widget);
    splitter->addWidget(right_widget);
    splitter->setSizes({400, 800});
    
    layout->addWidget(splitter);
    
    main_tabs_->addTab(domain_topic_widget_, QString::fromUtf8("域和主题"));
    
    // 连接选择信号
    connect(domain_tree_, &QTreeWidget::itemSelectionChanged, 
            this, &PersistenceMonitor::onTopicSelected);
}

void PersistenceMonitor::setupDatabaseTab() {
    database_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(database_widget_);
    
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
    
    // 数据库统计表
    auto* stats_group = new QGroupBox(QString::fromUtf8("运行统计"));
    auto* stats_layout = new QVBoxLayout(stats_group);
    
    stats_table_ = new QTableWidget();
    stats_table_->setColumnCount(2);
    stats_table_->setHorizontalHeaderLabels({QString::fromUtf8("统计项"), QString::fromUtf8("值")});
    stats_table_->horizontalHeader()->setStretchLastSection(true);
    stats_layout->addWidget(stats_table_);
    
    layout->addWidget(info_group);
    layout->addWidget(usage_group);
    layout->addWidget(stats_group);
    
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
    // TODO: 实现主题表格更新
    // 从持久化服务获取主题信息并更新表格
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
    
    updateDatabaseStats();
    updateMemoryCacheInfo();
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

void PersistenceMonitor::updateMemoryCacheInfo() {
    if (!persistence_service_) {
        return;
    }
    
    auto stats = persistence_service_->get_statistics();
    
    // 更新内存使用进度条
    auto it_cache_size = stats.find("memory_cache_size");
    auto it_cache_bytes = stats.find("memory_cache_bytes");
    
    if (it_cache_size != stats.end() && it_cache_bytes != stats.end()) {
        int max_size_mb = persistence_service_->get_config().memory_cache_size_mb;
        double used_mb = it_cache_bytes->second / (1024.0 * 1024.0);
        int percentage = static_cast<int>((used_mb / max_size_mb) * 100);
        
        memory_usage_bar_->setMaximum(100);
        memory_usage_bar_->setValue(percentage);
        memory_usage_bar_->setFormat(QString("%1 MB / %2 MB (%3%)")
                                     .arg(used_mb, 0, 'f', 1)
                                     .arg(max_size_mb)
                                     .arg(percentage));
    }
    
    // 磁盘使用（示例）
    disk_usage_bar_->setMaximum(100);
    disk_usage_bar_->setValue(25);
    disk_usage_bar_->setFormat("1.2 GB / 5.0 GB (25%)");
}

void PersistenceMonitor::refreshAllData() {
    if (!persistence_service_) {
        return;
    }
    
    updateDomainInfo();
    updateTopicInfo();
    updateDatabaseInfo();
}

void PersistenceMonitor::onTopicSelected() {
    auto selected_items = domain_tree_->selectedItems();
    if (selected_items.isEmpty()) {
        topic_details_->clear();
        return;
    }
    
    auto* item = selected_items.first();
    QString topic_name = item->text(0);
    
    // 显示主题详细信息
    QString details = QString::fromUtf8(
        "主题名称: %1\n"
        "类型: %2\n"
        "持久化策略: %3\n"
        "可靠性策略: %4\n"
        "发现时间: %5\n"
    ).arg(item->text(0))
     .arg(item->text(1))
     .arg(item->text(2))
     .arg(item->text(3))
     .arg(QDateTime::currentDateTime().toString());
    
    topic_details_->setPlainText(details);
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
