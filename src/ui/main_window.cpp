#include "main_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QLabel>
#include <QStatusBar>
#include <QScrollArea>
#include <QFrame>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QTimer>
#include <QCloseEvent>
#include <QShowEvent>
#include <QIcon>
#include <QPixmap>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDateTime>
#include <QSet>
#include <QStringList>
#include <algorithm>

#include "content_parser.h"
#include "clipboard_manager.h"
#include "drag_manager.h"
#include "config_manager.h"
#include "hotkey_manager.h"
#include "platform_factory.h"
#include "item_widget.h"
#include "config_window.h"
#include "theme_manager.h"
#include "font_config.h"

// ==================== 匿名命名空间：图标路径辅助函数 ====================
namespace {

/**
 * @brief 获取图标文件路径
 *
 * 支持以下场景：
 * 1. 开发环境：直接从 src/resources 读取
 * 2. 部署环境：资源文件在可执行文件同目录的 resources 文件夹
 * 3. 找不到时返回空字符串（由调用方创建纯色图标）
 *
 * @return 图标文件的绝对路径，找不到返回空字符串
 */
QString getIconPath()
{
    QStringList candidates;

    // 1. 开发环境路径（基于当前工作目录）
    candidates << QStringLiteral("src/resources/icon.ico");
    candidates << QStringLiteral("src/resources/icon_32x32.png");

    // 2. 部署路径（可执行文件同目录）
    const QString exeDir = QCoreApplication::applicationDirPath();
    candidates << exeDir + QStringLiteral("/resources/icon.ico");
    candidates << exeDir + QStringLiteral("/resources/icon_32x32.png");
    candidates << exeDir + QStringLiteral("/icon.ico");
    candidates << exeDir + QStringLiteral("/icon_32x32.png");

    // 查找第一个存在的文件
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

} // namespace

/**
 * @brief 构造函数
 * @param config 配置管理器实例
 * @param parent 父部件
 */
MainWindow::MainWindow(ConfigManager* config, QWidget* parent)
    : QMainWindow(parent)
{
    // 配置管理器为空时内部创建
    if (config == nullptr) {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    } else {
        m_config = config;
    }

    qInfo() << "MainWindow 构造开始";

    // 按顺序初始化：属性 -> 核心组件 -> UI -> 窗口标志 -> 托盘 -> 主题
    initProperties();
    initCoreComponents();
    initUi();
    applyWindowFlags();
    initSystemTray();
    applyTheme();

    // 连接外部显示请求信号（单实例检测用）
    connect(this, &MainWindow::showRequested, this, &MainWindow::onShowRequested);

    qInfo() << "MainWindow 构造完成";
}

/**
 * @brief 析构函数（释放非 QObject 的成员）
 */
MainWindow::~MainWindow()
{
    // ContentParser 不是 QObject，需要手动释放
    delete m_contentParser;
    m_contentParser = nullptr;
}

/**
 * @brief 显示窗口（在构造函数完成后调用）
 */
void MainWindow::showWindow()
{
    show();

    // 自动适应条目高度（延迟确保布局完成）
    QTimer::singleShot(50, this, &MainWindow::autoFitWindow);

    // 应用保存的主题
    applyTheme();

    // 注册全局热键（窗口显示后注册，避免控件未就绪）
    if (m_hotkeyManager != nullptr && !m_hotkeysRegistered) {
        m_hotkeyManager->registerAllHotkeys();
        m_hotkeysRegistered = true;
    }

    // 延迟启动剪贴板监控
    QTimer::singleShot(200, this, &MainWindow::startClipboardMonitoring);
}

// ==================== 初始化 ====================

/**
 * @brief 初始化窗口属性
 */
void MainWindow::initProperties()
{
    const QJsonObject windowConfig = m_config->getWindowConfig();

    setWindowTitle(QStringLiteral("多元剪贴板"));
    setGeometry(windowConfig.value(QStringLiteral("x")).toInt(100),
                windowConfig.value(QStringLiteral("y")).toInt(100),
                windowConfig.value(QStringLiteral("width")).toInt(320),
                windowConfig.value(QStringLiteral("height")).toInt(480));
    setMinimumSize(180, 100);

    m_isAlwaysOnTop = windowConfig.value(QStringLiteral("always_on_top")).toBool(true);
    m_autoPopup = windowConfig.value(QStringLiteral("auto_popup")).toBool(true);
    m_lastCloseTime = 0;
    m_usedCounter = 0;
    m_currentTheme = m_config->get(QStringLiteral("ui.theme"), QStringLiteral("light")).toString();

    m_persistentItemsData = m_config->getPersistentItems();
    m_searchText.clear();
}

/**
 * @brief 初始化核心组件（解析器、剪贴板、拖拽、热键管理器）
 */
void MainWindow::initCoreComponents()
{
    // 内容解析器（非 QObject，需手动释放）
    m_contentParser = new ContentParser(m_config);

    // 剪贴板管理器
    m_clipboardManager = new ClipboardManager(m_config, this);

    // 拖拽管理器
    m_dragManager = new DragManager(m_config, m_clipboardManager, this);

    // 热键管理器并设置回调
    m_hotkeyManager = new HotkeyManager(m_config, this);
    m_hotkeyManager->setCallbacks(
        [this]() { toggleAlwaysOnTop(); },
        [this]() { toggleWindow(); },
        [this]() { clearAllItems(); },
        [this]() { copyAllItems(); },
        [this]() { pastePlainText(); }
    );
    qInfo() << "热键管理器初始化成功（热键未注册）";
}

/**
 * @brief 初始化界面布局
 */
void MainWindow::initUi()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    initToolbar(mainLayout);
    initContentArea(mainLayout);
    initStatusBar();

    loadPersistentItems();
}

/**
 * @brief 初始化工具栏（搜索框）
 * @param parentLayout 父布局
 */
void MainWindow::initToolbar(QVBoxLayout* parentLayout)
{
    const QString fontCss = FontConfig::getCssFontFamily();

    m_toolbar = new QFrame(this);
    m_toolbar->setFixedHeight(38);
    m_toolbar->setStyleSheet(
        "QFrame {"
        "    background-color: #f6f8fa;"
        "    border-bottom: 1px solid #d0d7de;"
        "}");

    auto* toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(8, 4, 4, 4);
    toolbarLayout->setSpacing(6);

    // 搜索框
    m_searchEdit = new QLineEdit(m_toolbar);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
    m_searchEdit->setFixedHeight(28);
    m_searchEdit->setMinimumWidth(100);
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit {"
        "    background-color: #ffffff;"
        "    border: 1px solid #d0d7de;"
        "    border-radius: 4px;"
        "    padding: 0 8px;"
        "    font-size: 12px;"
        "    font-family: %1;"
        "    color: #24292f;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #0969da;"
        "    outline: none;"
        "}"
        "QLineEdit::placeholder {"
        "    color: #909399;"
        "}")
        .arg(fontCss));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchChanged);
    m_searchEdit->setAcceptDrops(false);
    m_searchEdit->setVisible(false);
    toolbarLayout->addWidget(m_searchEdit);

    parentLayout->addWidget(m_toolbar);
    m_toolbar->setFixedHeight(0); // 初始无条目时工具栏折叠
}

/**
 * @brief 设置搜索框可见性，同时折叠/展开工具栏
 * @param visible 是否可见
 */
void MainWindow::setSearchVisible(bool visible)
{
    m_searchEdit->setVisible(visible);
    m_searchEdit->setEnabled(visible);
    if (!visible) {
        m_searchEdit->clear();
    }
    m_toolbar->setFixedHeight(visible ? 38 : 0);
}

/**
 * @brief 初始化内容区域（滚动条目列表）
 * @param parentLayout 父布局
 */
void MainWindow::initContentArea(QVBoxLayout* parentLayout)
{
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    // 禁止横向滚动条，避免长文本溢出导致横向滚动
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 使用 objectName 选择器代替脆弱的层级选择器
    auto* scrollContent = new QWidget(m_scrollArea);
    scrollContent->setObjectName(QStringLiteral("scrollContent"));
    scrollContent->setStyleSheet(QStringLiteral("QWidget#scrollContent { background-color: transparent; }"));

    m_itemsLayout = new QVBoxLayout(scrollContent);
    m_itemsLayout->setContentsMargins(2, 4, 2, 4);
    m_itemsLayout->setSpacing(4);

    // 底部弹性空间，条目少时不会撑满整个区域
    m_itemsLayout->addStretch();

    m_scrollArea->setWidget(scrollContent);

    parentLayout->addWidget(m_scrollArea, 1);
}

/**
 * @brief 初始化状态栏
 */
void MainWindow::initStatusBar()
{
    const QString fontCss = FontConfig::getCssFontFamily();

    m_statusBar = new QStatusBar(this);
    m_statusBar->setFixedHeight(28);
    m_statusBar->setStyleSheet(QString(
        "QStatusBar {"
        "    background-color: #f6f8fa;"
        "    border-top: 1px solid #d0d7de;"
        "    font-size: 11px;"
        "    font-family: %1;"
        "    color: #656d76;"
        "}"
        "QStatusBar::item {"
        "    border: none;"
        "}")
        .arg(fontCss));

    m_statusLabel = new QLabel(QStringLiteral("等待剪贴板内容..."), m_statusBar);
    m_statusLabel->setStyleSheet(
        QStringLiteral("color: #656d76; padding: 0px 10px; background: transparent;"));
    m_statusBar->addWidget(m_statusLabel);
    setStatusBar(m_statusBar);
}

/**
 * @brief 初始化系统托盘
 */
void MainWindow::initSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "系统托盘不可用";
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(QStringLiteral("多元剪贴板 - 剪贴板切分工具"));

    // 加载图标，找不到时使用纯色图标
    const QString iconPath = getIconPath();
    if (!iconPath.isEmpty()) {
        const QIcon icon(iconPath);
        m_trayIcon->setIcon(icon);
        // 同时设置窗口图标和应用程序图标
        setWindowIcon(icon);
        QGuiApplication::setWindowIcon(icon);
        qInfo() << QStringLiteral("图标加载成功: %1").arg(iconPath);
    } else {
        QPixmap pixmap(32, 32);
        pixmap.fill(QColor(QStringLiteral("#1976d2")));
        m_trayIcon->setIcon(QIcon(pixmap));
        qWarning() << "图标文件未找到，使用纯色图标";
    }

    auto* trayMenu = new QMenu(nullptr);

    // 窗口置顶（第一位）
    const QString pinText = m_isAlwaysOnTop ? QStringLiteral("\u2713 窗口置顶") : QStringLiteral("窗口置顶");
    m_trayPinAction = trayMenu->addAction(pinText);
    connect(m_trayPinAction, &QAction::triggered, this, [this]() {
        // 延迟执行避免菜单冲突
        QTimer::singleShot(0, this, &MainWindow::toggleAlwaysOnTop);
    });

    trayMenu->addSeparator();

    QAction* showAction = trayMenu->addAction(QStringLiteral("显示窗口"));
    connect(showAction, &QAction::triggered, this, &MainWindow::showFromTray);

    QAction* configAction = trayMenu->addAction(QStringLiteral("配置"));
    connect(configAction, &QAction::triggered, this, &MainWindow::showConfigWindow);

    // 主题切换
    const QString themeText = (m_currentTheme == QLatin1String("light"))
        ? QStringLiteral("切换暗色主题") : QStringLiteral("切换亮色主题");
    m_trayThemeAction = trayMenu->addAction(themeText);
    connect(m_trayThemeAction, &QAction::triggered, this, &MainWindow::toggleTheme);

    trayMenu->addSeparator();

    QAction* exitAction = trayMenu->addAction(QStringLiteral("退出"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::quitApplication);

    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_trayIcon->show();
}

/**
 * @brief 应用窗口标志（移除最大/最小化按钮，按需置顶）
 */
void MainWindow::applyWindowFlags()
{
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMinimizeButtonHint;
    flags &= ~Qt::WindowMaximizeButtonHint;
    if (m_isAlwaysOnTop) {
        flags |= Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
}

/**
 * @brief 更新窗口标题显示置顶状态
 */
void MainWindow::updateOnTopUi()
{
    const QString title = m_isAlwaysOnTop
        ? QStringLiteral("多元剪贴板 \U0001F4CC")
        : QStringLiteral("多元剪贴板");
    setWindowTitle(title);
}

/**
 * @brief 延迟启动剪贴板监控
 */
void MainWindow::startClipboardMonitoring()
{
    if (m_clipboardManager == nullptr) {
        return;
    }
    // 连接剪贴板变化信号
    connect(m_clipboardManager, &ClipboardManager::clipboardChanged,
            this, &MainWindow::onClipboardChanged);
    m_clipboardManager->startMonitoring();
    qInfo() << "剪贴板监控已启动";
}

// ==================== 剪贴板事件 ====================

/**
 * @brief 剪贴板内容变化处理：多行自动弹出，单行后台更新
 * @param text 剪贴板新文本
 */
void MainWindow::onClipboardChanged(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    if (!m_autoPopup) {
        return;
    }
    // 关闭后 10 秒内不自动弹出
    if (QDateTime::currentSecsSinceEpoch() - m_lastCloseTime < 10) {
        return;
    }

    qInfo() << QStringLiteral("剪贴板新内容: %1 字符").arg(text.size());

    // 解析剪贴板内容
    QVector<Item> newItems = m_contentParser->parse(text);
    if (newItems.isEmpty()) {
        return;
    }

    // 合并持久化条目
    newItems = mergePersistentItems(newItems);

    // 内容一致时忽略
    if (isSameContent(newItems)) {
        return;
    }

    m_allItems = newItems;
    applySearchFilter();

    // 多行内容自动弹出窗口，单行后台更新
    if (text.contains(QLatin1Char('\n'))) {
        showFromTray();
        qInfo() << QStringLiteral("多行剪贴板，自动弹出: %1 条目").arg(newItems.size());
    } else {
        qInfo() << QStringLiteral("单行剪贴板，后台更新: %1 条目").arg(newItems.size());
    }

    m_statusLabel->setText(QStringLiteral("已解析 %1 个条目").arg(newItems.size()));
}

/**
 * @brief 比较新旧条目是否内容一致
 * @param newItems 新条目列表
 * @return 是否一致
 */
bool MainWindow::isSameContent(const QVector<Item>& newItems)
{
    if (newItems.size() != m_items.size()) {
        return false;
    }
    for (int i = 0; i < newItems.size(); ++i) {
        if (newItems[i].content.trimmed() != m_items[i].content.trimmed()) {
            return false;
        }
    }
    return true;
}

// ==================== 搜索过滤 ====================

/**
 * @brief 搜索文本变化处理：实时过滤条目列表
 * @param text 搜索文本
 */
void MainWindow::onSearchChanged(const QString& text)
{
    m_searchText = text.trimmed().toLower();
    applySearchFilter();
}

/**
 * @brief 应用搜索过滤器：不隐藏条目，匹配的高亮，不匹配的淡化
 */
void MainWindow::applySearchFilter()
{
    m_items = m_allItems;
    displayItems();

    if (!m_searchText.isEmpty()) {
        // 统计匹配条目数
        int matchCount = 0;
        for (const Item& item : m_allItems) {
            if (item.content.toLower().contains(m_searchText)) {
                ++matchCount;
            }
        }
        m_statusLabel->setText(QStringLiteral("搜索结果: %1 个").arg(matchCount));
    } else {
        updateUsedCounter();
    }
}

// ==================== 条目显示 ====================

/**
 * @brief 显示条目列表
 */
void MainWindow::displayItems()
{
    // 移除旧的条目组件和底部弹性空间
    while (m_itemsLayout->count() > 0) {
        QLayoutItem* child = m_itemsLayout->takeAt(0);
        if (child->widget() != nullptr) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_itemWidgets.clear();

    // 为每条数据创建组件
    for (const Item& item : m_allItems) {
        auto* widget = new ItemWidget(item, m_config, m_dragManager);
        connect(widget, &ItemWidget::itemUsed, this, &MainWindow::onItemUsed);
        connect(widget, &ItemWidget::itemUnused, this, &MainWindow::onItemUnused);
        connect(widget, &ItemWidget::itemCopied, this, &MainWindow::onItemCopied);
        connect(widget, &ItemWidget::itemPersistentChanged, this, &MainWindow::onItemPersistentChanged);

        // 搜索匹配时正常显示，不匹配时淡化
        if (!m_searchText.isEmpty()) {
            const bool match = item.content.toLower().contains(m_searchText);
            widget->setSearchMatch(match);
        }

        m_itemsLayout->addWidget(widget);
        m_itemWidgets.append(widget);
    }

    // 底部弹性空间，条目少时不会撑满整个区域
    m_itemsLayout->addStretch();

    // 搜索框可见性：基于原始条目数（<=1 时隐藏）
    const int totalCount = m_allItems.size();
    setSearchVisible(totalCount > 1);

    // 自动调整窗口高度适配条目数（延迟到布局完成后）
    QTimer::singleShot(0, this, &MainWindow::autoFitWindow);

    updateUsedCounter();
}

/**
 * @brief 条目标记为已使用处理
 * @param item 条目
 */
void MainWindow::onItemUsed(Item* item)
{
    if (item->usageOrder == -1) {
        ++m_usedCounter;
        item->usageOrder = m_usedCounter;
    }
    updateUsedCounter();
}

/**
 * @brief 条目标记为未使用处理
 * @param item 条目
 */
void MainWindow::onItemUnused(Item* item)
{
    if (item->usageOrder != -1) {
        --m_usedCounter;
        item->usageOrder = -1;
    }
    updateUsedCounter();
}

/**
 * @brief 条目复制处理
 * @param item 条目
 */
void MainWindow::onItemCopied(Item* item)
{
    m_statusLabel->setText(QStringLiteral("已复制: %1...").arg(item->content.left(20)));
    QTimer::singleShot(2000, this, &MainWindow::updateStatusLabel);
}

/**
 * @brief 更新已使用计数器显示
 */
void MainWindow::updateUsedCounter()
{
    int used = 0;
    for (const Item& item : m_items) {
        if (item.used) {
            ++used;
        }
    }
    m_statusLabel->setText(QStringLiteral("%1/%2 已使用").arg(used).arg(m_items.size()));
}

/**
 * @brief 更新状态栏标签
 */
void MainWindow::updateStatusLabel()
{
    if (!m_items.isEmpty()) {
        updateUsedCounter();
    } else {
        m_statusLabel->setText(QStringLiteral("等待剪贴板内容..."));
    }
}

// ==================== 持久化条目管理 ====================

/**
 * @brief 加载持久化条目
 */
void MainWindow::loadPersistentItems()
{
    if (m_persistentItemsData.isEmpty()) {
        return;
    }

    for (int idx = 0; idx < m_persistentItemsData.size(); ++idx) {
        const QJsonObject data = m_persistentItemsData.at(idx).toObject();
        Item item;
        item.id = QStringLiteral("persistent_%1").arg(idx);
        item.content = data.value(QStringLiteral("content")).toString();
        item.index = idx;
        item.persistent = true;
        m_items.append(item);
        m_allItems.append(item);
    }

    if (!m_items.isEmpty()) {
        displayItems();
    }
}

/**
 * @brief 合并持久化条目到新条目列表，确保 persistent 条目放最前面
 * @param newItems 新条目列表
 * @return 合并后的条目列表
 */
QVector<Item> MainWindow::mergePersistentItems(const QVector<Item>& newItems)
{
    // 收集现有持久化条目的内容集合
    QSet<QString> persistentContents;
    for (const Item& item : m_items) {
        if (item.persistent) {
            persistentContents.insert(item.content);
        }
    }

    // 新条目中与持久化内容相同的标记为持久化
    QVector<Item> merged = newItems;
    for (Item& item : merged) {
        if (persistentContents.contains(item.content)) {
            item.persistent = true;
        }
    }

    // 追加未出现在新条目中的持久化条目
    for (const Item& item : m_items) {
        if (!item.persistent) {
            continue;
        }
        bool exists = false;
        for (const Item& ni : merged) {
            if (ni.content == item.content) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            merged.append(item);
        }
    }

    // 排序：持久化在前，其次原始条目，最后按索引升序
    std::stable_sort(merged.begin(), merged.end(), [](const Item& a, const Item& b) {
        if (a.persistent != b.persistent) {
            return a.persistent > b.persistent;
        }
        if (a.raw != b.raw) {
            return a.raw > b.raw;
        }
        return a.index < b.index;
    });

    // 重新编号索引
    for (int idx = 0; idx < merged.size(); ++idx) {
        merged[idx].index = idx;
    }

    return merged;
}

/**
 * @brief 条目持久化状态变化处理
 * @param item 条目
 * @param persistent 是否持久化
 */
void MainWindow::onItemPersistentChanged(Item* item, bool persistent)
{
    if (persistent) {
        m_config->addPersistentItem(item->content);
    } else {
        m_config->removePersistentItem(item->content);
    }
}

// ==================== 配置窗口 ====================

/**
 * @brief 显示配置窗口
 */
void MainWindow::showConfigWindow()
{
    if (m_configWindow == nullptr || !m_configWindow->isVisible()) {
        // 旧的隐藏窗口直接销毁，避免内存泄漏
        if (m_configWindow != nullptr) {
            m_configWindow->deleteLater();
        }
        m_configWindow = new ConfigWindow(m_config, this);
        connect(m_configWindow, &ConfigWindow::hotkeysChanged,
                this, &MainWindow::onHotkeysChanged);
        m_configWindow->setTheme(m_currentTheme);
        m_configWindow->show();
    } else {
        m_configWindow->activateWindow();
    }
}

/**
 * @brief 热键配置变化处理
 */
void MainWindow::onHotkeysChanged()
{
    qInfo() << "热键配置已更新，重新加载热键";
    if (m_hotkeyManager != nullptr) {
        m_hotkeyManager->reloadHotkeys();
    }
}

// ==================== 热键操作 ====================

/**
 * @brief 切换窗口显示/隐藏
 */
void MainWindow::toggleWindow()
{
    if (isVisible()) {
        hide();
    } else {
        showFromTray();
    }
}

/**
 * @brief 清空所有条目
 */
void MainWindow::clearAllItems()
{
    // 移除所有条目组件和底部弹性空间
    while (m_itemsLayout->count() > 0) {
        QLayoutItem* child = m_itemsLayout->takeAt(0);
        if (child->widget() != nullptr) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_itemWidgets.clear();
    m_items.clear();
    m_usedCounter = 0;

    // 重新添加底部弹性空间
    m_itemsLayout->addStretch();

    // 清空后折叠工具栏
    setSearchVisible(false);

    m_statusLabel->setText(QStringLiteral("已清空所有条目"));
    QTimer::singleShot(2000, this, &MainWindow::updateStatusLabel);
}

/**
 * @brief 复制所有条目
 */
void MainWindow::copyAllItems()
{
    if (m_items.isEmpty()) {
        return;
    }

    // 将所有条目内容按换行符拼接后复制
    QStringList parts;
    parts.reserve(m_items.size());
    for (const Item& item : m_items) {
        parts << item.content;
    }
    m_dragManager->copyText(parts.join(QLatin1Char('\n')));

    m_statusLabel->setText(QStringLiteral("已复制 %1 个条目").arg(m_items.size()));
    QTimer::singleShot(2000, this, &MainWindow::updateStatusLabel);
}

/**
 * @brief 纯文本粘贴：获取剪贴板内容，去除格式后重新写入并粘贴
 */
void MainWindow::pastePlainText()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QString text = clipboard->text();
    if (!text.isEmpty()) {
        clipboard->setText(text);
        // 使用平台抽象层模拟 Ctrl+V
        auto platform = PlatformFactory::platform();
        if (platform) {
            platform->simulateCtrlV();
        }
        m_statusLabel->setText(QStringLiteral("已纯文本粘贴"));
        QTimer::singleShot(2000, this, &MainWindow::updateStatusLabel);
    }
}

// ==================== 主题 ====================

/**
 * @brief 切换明暗主题
 */
void MainWindow::toggleTheme()
{
    m_currentTheme = (m_currentTheme == QLatin1String("light"))
        ? QStringLiteral("dark") : QStringLiteral("light");
    m_config->set(QStringLiteral("ui.theme"), m_currentTheme);
    m_config->saveConfig();

    // 更新托盘菜单中的主题切换文案
    if (m_trayThemeAction != nullptr) {
        m_trayThemeAction->setText(m_currentTheme == QLatin1String("light")
            ? QStringLiteral("切换暗色主题") : QStringLiteral("切换亮色主题"));
    }

    applyTheme();
}

/**
 * @brief 应用主题样式
 */
void MainWindow::applyTheme()
{
    const ThemeColors theme = ThemeManager::getTheme(m_currentTheme);
    const QString fontCss = FontConfig::getCssFontFamily();

    // 主窗口背景
    setStyleSheet(QStringLiteral("QMainWindow { background-color: %1; }")
                      .arg(theme.value(QStringLiteral("window_bg"))));

    // 状态栏
    m_statusBar->setStyleSheet(QString(
        "QStatusBar {"
        "    background-color: %1;"
        "    border-top: 1px solid %2;"
        "    font-size: 11px;"
        "    font-family: %3;"
        "    color: %4;"
        "}"
        "QStatusBar::item { border: none; }")
        .arg(theme.value(QStringLiteral("status_bar_bg")))
        .arg(theme.value(QStringLiteral("status_bar_border")))
        .arg(fontCss)
        .arg(theme.value(QStringLiteral("status_bar_text"))));
    m_statusLabel->setStyleSheet(QStringLiteral("color: %1; padding: 0px 10px; background: transparent;")
                                     .arg(theme.value(QStringLiteral("status_bar_text"))));

    // 搜索框
    if (m_searchEdit != nullptr) {
        m_searchEdit->setStyleSheet(QString(
            "QLineEdit {"
            "    background-color: %1;"
            "    border: 1px solid %2;"
            "    border-radius: 4px;"
            "    padding: 0 8px;"
            "    font-size: 12px;"
            "    font-family: %3;"
            "    color: %4;"
            "}"
            "QLineEdit:focus {"
            "    border-color: %5;"
            "    outline: none;"
            "}"
            "QLineEdit::placeholder {"
            "    color: %6;"
            "}")
            .arg(theme.value(QStringLiteral("search_bg")))
            .arg(theme.value(QStringLiteral("search_border")))
            .arg(fontCss)
            .arg(theme.value(QStringLiteral("search_text")))
            .arg(theme.value(QStringLiteral("search_border_focus")))
            .arg(theme.value(QStringLiteral("search_placeholder"))));
    }

    // 工具栏背景
    if (m_toolbar != nullptr) {
        m_toolbar->setStyleSheet(QString(
            "QFrame {"
            "    background-color: %1;"
            "    border-bottom: 1px solid %2;"
            "}")
            .arg(theme.value(QStringLiteral("toolbar_bg")))
            .arg(theme.value(QStringLiteral("toolbar_border"))));
    }

    // 滚动区域
    if (m_scrollArea != nullptr) {
        m_scrollArea->setStyleSheet(QString(
            "QScrollArea {"
            "    border: none;"
            "    background-color: %1;"
            "}"
            "QScrollBar:vertical {"
            "    width: 6px;"
            "    background: %2;"
            "    margin: 2px;"
            "}"
            "QScrollBar::handle:vertical {"
            "    background-color: %3;"
            "    border-radius: 3px;"
            "    min-height: 30px;"
            "}"
            "QScrollBar::handle:vertical:hover {"
            "    background-color: %4;"
            "}"
            "QScrollBar::add-line:vertical,"
            "QScrollBar::sub-line:vertical {"
            "    height: 0px;"
            "}"
            "QScrollBar::add-page:vertical,"
            "QScrollBar::sub-page:vertical {"
            "    background: %2;"
            "}")
            .arg(theme.value(QStringLiteral("content_bg")))
            .arg(theme.value(QStringLiteral("scrollbar_bg")))
            .arg(theme.value(QStringLiteral("scrollbar_handle")))
            .arg(theme.value(QStringLiteral("scrollbar_handle_hover"))));
    }

    // 更新所有条目组件的主题
    for (ItemWidget* widget : m_itemWidgets) {
        widget->setTheme(m_currentTheme);
    }

    // 更新配置窗口主题
    if (m_configWindow != nullptr && m_configWindow->isVisible()) {
        m_configWindow->setTheme(m_currentTheme);
    }
}

// ==================== 窗口操作 ====================

/**
 * @brief 设置窗口置顶
 * @param onTop 是否置顶
 */
void MainWindow::setAlwaysOnTop(bool onTop)
{
    if (m_isAlwaysOnTop == onTop) {
        return;
    }
    m_isAlwaysOnTop = onTop;

    const QRect geo = geometry();
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMinimizeButtonHint;
    flags &= ~Qt::WindowMaximizeButtonHint;
    if (onTop) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    setGeometry(geo);
    updateOnTopUi();

    // 同步托盘菜单文字
    if (m_trayPinAction != nullptr) {
        m_trayPinAction->setText(onTop ? QStringLiteral("\u2713 窗口置顶") : QStringLiteral("窗口置顶"));
    }

    show();

    m_config->set(QStringLiteral("window.always_on_top"), onTop);
    m_config->saveConfig();
}

/**
 * @brief 切换窗口置顶状态
 */
void MainWindow::toggleAlwaysOnTop()
{
    setAlwaysOnTop(!m_isAlwaysOnTop);
}

/**
 * @brief 自动调整窗口高度适配条目数（最多 8 行）
 */
void MainWindow::autoFitWindow()
{
    const int count = m_allItems.size();
    if (count == 0) {
        return;
    }

    // 行数上限
    const int maxRows = 8;
    const int visibleRows = qMin(count, maxRows);

    // 组件高度常量
    const int toolbarH = 38;
    const int statusbarH = 28;
    const int contentMargin = 8; // 4px top + 4px bottom
    const int itemH = m_itemWidgets.isEmpty() ? 36 : m_itemWidgets.first()->height();
    const int spacing = 4;

    // 内容高度 = 条目 * (高度 + 间距) - 最后一项无间距
    const int contentH = visibleRows * itemH + (visibleRows - 1) * spacing;

    // 窗口总高度
    const int newHeight = toolbarH + contentMargin + contentH + statusbarH + 4;

    // 保持当前宽度不变
    resize(width(), newHeight);
}

/**
 * @brief 从托盘恢复显示
 */
void MainWindow::showFromTray()
{
    show();
    autoFitWindow();
    raise();
    activateWindow();
}

/**
 * @brief 收到外部显示请求
 */
void MainWindow::onShowRequested()
{
    showFromTray();
}

/**
 * @brief 托盘图标激活处理
 * @param reason 激活原因
 */
void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showFromTray();
    }
}

/**
 * @brief 完全退出应用
 */
void MainWindow::quitApplication()
{
    qInfo() << "用户请求退出";

    // 停止剪贴板监控
    if (m_clipboardManager != nullptr) {
        m_clipboardManager->stop();
    }
    // 注销全局热键
    if (m_hotkeyManager != nullptr) {
        m_hotkeyManager->stop();
    }
    // 隐藏托盘图标
    if (m_trayIcon != nullptr) {
        m_trayIcon->hide();
    }

    QApplication::quit();
}

// ==================== Qt 事件 ====================

/**
 * @brief 关闭事件：最小化到托盘并保存窗口位置/大小
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
    qInfo() << "关闭 -> 托盘";

    m_lastCloseTime = QDateTime::currentSecsSinceEpoch();

    // 保存窗口位置和大小
    m_config->updateWindowPosition(x(), y());
    m_config->updateWindowSize(width(), height());

    event->ignore();
    hide();
}

/**
 * @brief 窗口显示事件
 */
void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
}
