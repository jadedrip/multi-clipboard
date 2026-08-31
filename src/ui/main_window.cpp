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
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDateTime>
#include <QSet>
#include <QStringList>
#include <QSlider>
#include <QPushButton>
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
#include "window_drag_filter.h"

// ==================== 匿名命名空间：图标路径辅助函数 ====================
namespace {

/**
 * @brief 获取图标文件路径
 *
 * 支持以下场景：
 * 1. Qt 资源系统（编译进可执行文件，最可靠）
 * 2. 开发环境：从 src/resources 读取
 * 3. 部署环境：资源文件在可执行文件同目录的 resources 文件夹
 * 4. 找不到时返回空字符串（由调用方创建纯色图标）
 *
 * @return 图标文件的绝对路径或资源路径，找不到返回空字符串
 */
QString getIconPath()
{
    // 0. Qt 资源系统路径（qrc 已注册，编译进可执行文件，始终存在）
    const QStringList resourceCandidates = {
        QStringLiteral(":/icons/icon_256x256.png"),
        QStringLiteral(":/icons/icon_128x128.png"),
        QStringLiteral(":/icons/icon_64x64.png"),
        QStringLiteral(":/icons/icon_48x48.png"),
        QStringLiteral(":/icons/icon_32x32.png"),
    };
    for (const QString& path : resourceCandidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }

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
    m_autoPopupMinItems = windowConfig.value(QStringLiteral("auto_popup_min_items")).toInt(3);
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
    initPagerBar(mainLayout);
    initStatusBar();
    initOpacityControl(mainLayout);

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

    // 允许通过列表空白区域按住左键拖动移动窗体（不依赖标题栏）
    new WindowDragFilter(scrollContent, this, this);

    parentLayout->addWidget(m_scrollArea, 1);
}

/**
 * @brief 初始化底部多行表格翻页条（◀ 第 X/Y ▶）
 * @param parentLayout 父布局
 */
void MainWindow::initPagerBar(QVBoxLayout* parentLayout)
{
    // 翻页条容器（样式在 applyTheme 中随主题统一刷新），默认隐藏
    m_pagerBar = new QFrame(this);
    m_pagerBar->setObjectName(QStringLiteral("pagerBar"));
    m_pagerBar->setFixedHeight(30);
    m_pagerBar->setVisible(false);

    auto* barLayout = new QHBoxLayout(m_pagerBar);
    barLayout->setContentsMargins(10, 2, 10, 2);
    barLayout->setSpacing(8);

    // 上一行按钮（◀）
    m_prevPageBtn = new QPushButton(QStringLiteral("\u25C0"), m_pagerBar);
    m_prevPageBtn->setObjectName(QStringLiteral("pagerButton"));
    m_prevPageBtn->setFixedSize(24, 22);
    m_prevPageBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_prevPageBtn, &QPushButton::clicked, this, &MainWindow::onPrevPage);

    // 行切换滑块（占满中间，拖动切换表格行）
    m_pagerSlider = new QSlider(Qt::Horizontal, m_pagerBar);
    m_pagerSlider->setObjectName(QStringLiteral("pagerSlider"));
    m_pagerSlider->setRange(0, 0);
    m_pagerSlider->setFocusPolicy(Qt::NoFocus);
    connect(m_pagerSlider, &QSlider::valueChanged, this, &MainWindow::onPagerSliderChanged);

    // 翻页标签（第 X/Y）
    m_pagerLabel = new QLabel(QStringLiteral("第 1/1"), m_pagerBar);
    m_pagerLabel->setObjectName(QStringLiteral("pagerLabel"));
    m_pagerLabel->setAlignment(Qt::AlignCenter);
    m_pagerLabel->setMinimumWidth(64);

    // 下一行按钮（▶）
    m_nextPageBtn = new QPushButton(QStringLiteral("\u25B6"), m_pagerBar);
    m_nextPageBtn->setObjectName(QStringLiteral("pagerButton"));
    m_nextPageBtn->setFixedSize(24, 22);
    m_nextPageBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_nextPageBtn, &QPushButton::clicked, this, &MainWindow::onNextPage);

    // 滚动条式布局：◀ 按钮 + 滑块占满 + 数字标签 + ▶ 按钮，充满一行
    barLayout->addWidget(m_prevPageBtn);
    barLayout->addWidget(m_pagerSlider, 1);
    barLayout->addWidget(m_pagerLabel);
    barLayout->addWidget(m_nextPageBtn);

    parentLayout->addWidget(m_pagerBar);
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
 * @brief 初始化底部透明度控制条（滑动条 + 百分比标签）
 * @param parentLayout 父布局
 */
void MainWindow::initOpacityControl(QVBoxLayout* parentLayout)
{
    // 读取已保存的透明度并限制在合法范围（30~100）
    const int savedOpacity = m_config->get(QStringLiteral("ui.opacity"), 100).toInt();
    const int initialOpacity = qBound(30, savedOpacity, 100);

    // 控制条容器（样式在 applyTheme 中随主题统一刷新）
    m_opacityBar = new QFrame(this);
    m_opacityBar->setObjectName(QStringLiteral("opacityBar"));
    m_opacityBar->setFixedHeight(34);

    auto* barLayout = new QHBoxLayout(m_opacityBar);
    barLayout->setContentsMargins(10, 2, 10, 2);
    barLayout->setSpacing(6);

    // 标题标签
    auto* titleLabel = new QLabel(QStringLiteral("透明度"), m_opacityBar);
    titleLabel->setObjectName(QStringLiteral("opacityTitleLabel"));

    // 透明度滑动条（30%~100% 不透明）
    m_opacitySlider = new QSlider(Qt::Horizontal, m_opacityBar);
    m_opacitySlider->setObjectName(QStringLiteral("opacitySlider"));
    m_opacitySlider->setRange(30, 100);
    m_opacitySlider->setValue(initialOpacity);
    m_opacitySlider->setFixedHeight(20);

    // 百分比标签（固定宽度避免滑动时布局跳动）
    m_opacityValueLabel = new QLabel(QStringLiteral("%1%").arg(initialOpacity), m_opacityBar);
    m_opacityValueLabel->setObjectName(QStringLiteral("opacityValueLabel"));
    m_opacityValueLabel->setFixedWidth(38);
    m_opacityValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    barLayout->addWidget(titleLabel);
    barLayout->addWidget(m_opacitySlider, 1);
    barLayout->addWidget(m_opacityValueLabel);

    parentLayout->addWidget(m_opacityBar);

    // 防抖保存定时器：滑动停止 300ms 后才写盘，避免频繁 IO
    m_opacitySaveTimer = new QTimer(this);
    m_opacitySaveTimer->setSingleShot(true);
    m_opacitySaveTimer->setInterval(300);
    connect(m_opacitySaveTimer, &QTimer::timeout, this, [this]() {
        m_config->saveConfig();
    });

    // 滑动时实时预览透明度并更新百分比标签
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        applyOpacity(value);
        m_opacityValueLabel->setText(QStringLiteral("%1%").arg(value));
        m_config->set(QStringLiteral("ui.opacity"), value);
        m_opacitySaveTimer->start();
    });

    // 应用初始透明度
    applyOpacity(initialOpacity);
}

/**
 * @brief 应用窗口不透明度
 * @param percent 不透明度百分比（30~100）
 */
void MainWindow::applyOpacity(int percent)
{
    // Qt 的 setWindowOpacity 取值范围为 0.0~1.0，与百分比一一对应
    const int clamped = qBound(30, percent, 100);
    setWindowOpacity(clamped / 100.0);
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
 * @brief 剪贴板内容变化处理：解析与列表更新始终执行，满足条件时才自动弹出
 * @param text 剪贴板新文本
 *
 * 自动弹出与否只影响窗口显示，不影响内容解析、持久化合并与列表刷新。
 */
void MainWindow::onClipboardChanged(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }

    qInfo() << QStringLiteral("剪贴板新内容: %1 字符").arg(text.size());

    // 多行表格：进入表格翻页浏览模式（每行按原逻辑切分，翻页条切换行）
    const QVector<QVector<Item>> tableRows = m_contentParser->parseTableRows(text);
    if (!tableRows.isEmpty()) {
        handleTableContent(tableRows);
        return;
    }

    // 普通内容：退出表格模式
    if (m_activeTableRowIndex >= 0) {
        m_activeTableRowIndex = -1;
        m_tableRows.clear();
        updatePagerState();
    }

    // 解析剪贴板内容（与是否自动弹出无关，始终执行）
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

    // 更新内存数据源并刷新界面
    m_allItems = newItems;
    applySearchFilter();
    m_statusLabel->setText(QStringLiteral("已解析 %1 个条目").arg(newItems.size()));
    qInfo() << QStringLiteral("已解析 %1 个条目，非常驻 %2 个")
                   .arg(newItems.size())
                   .arg(std::count_if(newItems.begin(), newItems.end(),
                                      [](const Item& it) { return !it.persistent; }));

    // 满足自动弹出条件时才弹出窗口，否则仅后台更新
    if (shouldAutoPopup(newItems)) {
        showFromTray();
        qInfo() << QStringLiteral("自动弹出窗口");
    } else {
        qInfo() << QStringLiteral("仅后台更新（未满足自动弹出条件）");
    }
}

/**
 * @brief 处理多行表格内容：进入表格翻页浏览模式
 * @param tableRows 表格逐行条目
 */
void MainWindow::handleTableContent(const QVector<QVector<Item>>& tableRows)
{
    // 表格内容一致时不刷新，保持当前翻页位置
    if (!isSameTableContent(tableRows)) {
        m_tableRows = tableRows;
        m_activeTableRowIndex = -1;
        enterTableRow(0);
        m_statusLabel->setText(QStringLiteral("表格 %1 行，翻页浏览").arg(tableRows.size()));
        qInfo() << QStringLiteral("多行表格: %1 行").arg(tableRows.size());
    }

    // 表格内容优先弹出窗口（不按非常驻条目数阈值计数，保证用户能立即翻页浏览）
    if (m_autoPopup && QDateTime::currentSecsSinceEpoch() - m_lastCloseTime >= 10) {
        showFromTray();
    }
}

/**
 * @brief 判断是否自动弹出窗口
 * @param items 解析合并后的条目列表
 * @return 是否弹出
 *
 * 同时满足以下条件才弹出：
 * 1. 自动弹出开关开启；
 * 2. 距上次关闭窗口超过 10 秒；
 * 3. 本次解析出的非常驻条目数大于阈值（常驻条目不参与计数）。
 */
bool MainWindow::shouldAutoPopup(const QVector<Item>& items)
{
    // 自动弹出开关关闭时仅后台更新
    if (!m_autoPopup) {
        return false;
    }
    // 关闭后 10 秒内不自动弹出
    if (QDateTime::currentSecsSinceEpoch() - m_lastCloseTime < 10) {
        return false;
    }
    // 统计非常驻条目数（常驻条目已存在于列表，不参与计数）
    int nonPersistentCount = 0;
    for (const Item& it : items) {
        if (!it.persistent) {
            ++nonPersistentCount;
        }
    }
    return nonPersistentCount > m_autoPopupMinItems;
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
        connect(widget, &ItemWidget::itemNoteRequested, this, &MainWindow::onItemNoteRequested);
        connect(widget, &ItemWidget::itemForceParseRequested, this, &MainWindow::onItemForceParseRequested);
        connect(widget, &ItemWidget::itemDeleteRequested, this, &MainWindow::onItemDeleteRequested);
        connect(widget, &ItemWidget::itemDeleteCopiedRequested, this, &MainWindow::onDeleteCopiedRequested);

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

    updatePagerState();

    updateUsedCounter();
}

/**
 * @brief 翻页条上一行处理：切换当前显示的表格行
 */
void MainWindow::onPrevPage()
{
    if (m_activeTableRowIndex <= 0) {
        return;
    }
    enterTableRow(m_activeTableRowIndex - 1);
}

/**
 * @brief 翻页条下一行处理：切换当前显示的表格行
 */
void MainWindow::onNextPage()
{
    if (m_activeTableRowIndex < 0 || m_activeTableRowIndex >= m_tableRows.size() - 1) {
        return;
    }
    enterTableRow(m_activeTableRowIndex + 1);
}

/**
 * @brief 进入指定表格行：显示该行切分后的条目并刷新列表
 * @param rowIndex 表格行索引（0 起）
 */
void MainWindow::enterTableRow(int rowIndex)
{
    if (m_tableRows.isEmpty() || rowIndex < 0 || rowIndex >= m_tableRows.size()) {
        return;
    }

    m_activeTableRowIndex = rowIndex;

    // 显示列表 = 常驻条目（固定保持显示） + 当前行切分条目，常驻优先在前
    m_allItems = mergePersistentItems(m_tableRows.at(rowIndex));

    // 行内序号连续（从 0 开始），保持与条目组件序号一致
    for (int i = 0; i < m_allItems.size(); ++i) {
        m_allItems[i].index = i;
    }

    // 表格模式下搜索不跨行保留，避免旧搜索词干扰新行显示
    m_searchText.clear();
    m_searchEdit->clear();

    applySearchFilter();
    updatePagerState();

    qInfo() << QStringLiteral("表格切换到第 %1/%2 行，%3 个条目")
                   .arg(rowIndex + 1)
                   .arg(m_tableRows.size())
                   .arg(m_allItems.size());
}

/**
 * @brief 比较新表格与当前表格内容是否一致（避免无谓刷新丢失翻页位置）
 * @param newRows 新表格逐行条目
 * @return 是否一致
 */
bool MainWindow::isSameTableContent(const QVector<QVector<Item>>& newRows) const
{
    if (newRows.size() != m_tableRows.size()) {
        return false;
    }
    for (int r = 0; r < newRows.size(); ++r) {
        const QVector<Item>& newRow = newRows.at(r);
        const QVector<Item>& oldRow = m_tableRows.at(r);
        if (newRow.size() != oldRow.size()) {
            return false;
        }
        for (int i = 0; i < newRow.size(); ++i) {
            if (newRow.at(i).content.trimmed() != oldRow.at(i).content.trimmed()) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 翻页条滑块变化处理：切换当前显示的表格行
 * @param value 表格行索引
 */
void MainWindow::onPagerSliderChanged(int value)
{
    if (value != m_activeTableRowIndex) {
        enterTableRow(value);
    }
}

/**
 * @brief 更新翻页条状态：按当前表格行刷新滑块/标签/显隐
 *
 * 翻页条显示时隐藏底部透明度条（两条件互斥占用同一底部区域）。
 */
void MainWindow::updatePagerState()
{
    if (m_pagerBar == nullptr || m_opacityBar == nullptr) {
        return;
    }

    // 非表格模式或无表格数据时隐藏翻页条，恢复透明度条
    if (m_activeTableRowIndex < 0 || m_tableRows.isEmpty()) {
        m_pagerBar->setVisible(false);
        m_opacityBar->setVisible(true);
        return;
    }

    const int total = m_tableRows.size();
    const int current = qBound(0, m_activeTableRowIndex, total - 1);

    // 同步滑块（blockSignals 避免 valueChanged 回环触发 enterTableRow）
    m_pagerSlider->blockSignals(true);
    m_pagerSlider->setRange(0, total - 1);
    m_pagerSlider->setValue(current);
    m_pagerSlider->blockSignals(false);

    m_pagerLabel->setText(QStringLiteral("第 %1/%2").arg(current + 1).arg(total));
    m_prevPageBtn->setEnabled(current > 0);
    m_nextPageBtn->setEnabled(current < total - 1);

    m_pagerBar->setVisible(true);
    m_opacityBar->setVisible(false);
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
        item.note = data.value(QStringLiteral("note")).toString();
        item.index = idx;
        item.persistent = true;
        m_persistentItems.append(item);
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
    // 收集常驻条目运行时缓存的内容集合（启动加载 + 运行中勾选）
    QSet<QString> persistentContents;
    for (const Item& item : m_persistentItems) {
        persistentContents.insert(item.content);
    }

    // 新条目中与常驻内容相同的标记为持久化，并继承备注
    QVector<Item> merged = newItems;
    for (Item& item : merged) {
        if (persistentContents.contains(item.content)) {
            item.persistent = true;
            // 从常驻缓存继承备注
            for (const Item& p : m_persistentItems) {
                if (p.content == item.content) {
                    item.note = p.note;
                    break;
                }
            }
        }
    }

    // 追加未出现在新条目中的常驻条目
    for (const Item& item : m_persistentItems) {
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
 *
 * 除写入配置外，同步更新内存数据源（m_items/m_allItems）中对应条目的
 * 持久化状态，避免下次剪贴板刷新时勾选状态被覆盖丢失。
 *
 * @param item 条目
 * @param persistent 是否持久化
 */
void MainWindow::onItemPersistentChanged(Item* item, bool persistent)
{
    // 同步运行时常驻缓存（表格模式固定显示的依据）
    auto pIt = std::find_if(m_persistentItems.begin(), m_persistentItems.end(),
                            [&](const Item& p) { return p.content == item->content; });
    if (persistent) {
        if (pIt == m_persistentItems.end()) {
            Item persistentItem = *item;
            persistentItem.persistent = true;
            m_persistentItems.append(persistentItem);
        } else {
            pIt->note = item->note;
        }
        m_config->addPersistentItem(item->content);
    } else {
        if (pIt != m_persistentItems.end()) {
            m_persistentItems.erase(pIt);
        }
        m_config->removePersistentItem(item->content);
    }

    // 同步内存数据源：按内容匹配更新对应条目的持久化状态
    for (Item& it : m_items) {
        if (it.content == item->content) {
            it.persistent = persistent;
        }
    }
    for (Item& it : m_allItems) {
        if (it.content == item->content) {
            it.persistent = persistent;
        }
    }
}

/**
 * @brief 条目备注变化处理：同步内存数据源并保存到配置
 * @param item 条目
 */
void MainWindow::onItemNoteRequested(Item* item)
{
    // 同步常驻缓存中的备注
    for (Item& p : m_persistentItems) {
        if (p.content == item->content) {
            p.note = item->note;
            break;
        }
    }

    // 同步内存数据源：按内容匹配更新对应条目的备注
    for (Item& it : m_items) {
        if (it.content == item->content) {
            it.note = item->note;
        }
    }
    for (Item& it : m_allItems) {
        if (it.content == item->content) {
            it.note = item->note;
        }
    }

    m_config->setPersistentItemNote(item->content, item->note);
}

/**
 * @brief 条目强制解析处理：绕过切分限制解析并替换当前列表
 *
 * 用于 raw 条目即使超过切分限制也能强制切分为多条。
 * 解析结果替换当前列表（原 raw 条目保留为第一条）。
 *
 * @param item 条目
 */
void MainWindow::onItemForceParseRequested(Item* item)
{
    const QString content = item->content;
    QVector<Item> parsed = m_contentParser->parseForced(content);
    if (parsed.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("解析失败：没有可切分的条目"));
        return;
    }

    qInfo() << QStringLiteral("强制解析: %1 字符 -> %2 条目").arg(content.size()).arg(parsed.size());

    // 合并持久化条目并替换当前列表
    QVector<Item> newItems = mergePersistentItems(parsed);

    // 内容一致时忽略（避免重复刷新）
    if (isSameContent(newItems)) {
        return;
    }

    m_allItems = newItems;
    applySearchFilter();

    // 解析出多条时自动弹出窗口
    if (parsed.size() > 1) {
        showFromTray();
    }

    m_statusLabel->setText(QStringLiteral("已解析 %1 个条目").arg(newItems.size()));
}

/**
 * @brief 条目删除处理：从内存列表删除并刷新，常驻条目同步删除配置
 *
 * 按条目标识（id）精确匹配删除，避免误删同内容的其他条目。
 * 常驻条目删除时同步调用 removePersistentItem，防止下次剪贴板刷新被重新合并回来。
 *
 * @param item 条目
 */
void MainWindow::onItemDeleteRequested(Item* item)
{
    if (item == nullptr) {
        return;
    }

    // 常驻条目删除时同步删除配置与常驻缓存（防止合并时重新出现）
    if (item->persistent) {
        m_config->removePersistentItem(item->content);
        m_persistentItems.erase(std::remove_if(m_persistentItems.begin(), m_persistentItems.end(),
                                  [&](const Item& p) { return p.content == item->content; }),
                                m_persistentItems.end());
    }

    // 按 id 精确匹配，从全部条目中删除
    const QString id = item->id;
    m_allItems.erase(std::remove_if(m_allItems.begin(), m_allItems.end(),
                        [&id](const Item& it) { return it.id == id; }),
                     m_allItems.end());

    // 表格模式下同步删除当前行条目，避免翻页后条目"复活"
    if (m_activeTableRowIndex >= 0 && m_activeTableRowIndex < m_tableRows.size()) {
        QVector<Item>& row = m_tableRows[m_activeTableRowIndex];
        row.erase(std::remove_if(row.begin(), row.end(),
                    [&id](const Item& it) { return it.id == id; }),
                  row.end());
        for (int i = 0; i < row.size(); ++i) {
            row[i].index = i;
        }
    }

    qInfo() << QStringLiteral("删除条目: %1").arg(item->content.left(20));

    // 刷新显示（applySearchFilter 会用 m_allItems 重建 m_items）
    applySearchFilter();
}

/**
 * @brief 删除所有已复制条目：删除所有非常驻且已复制的条目并刷新
 *
 * 常驻条目（persistent）不受影响；已复制条目通过 used 标记识别。
 */
void MainWindow::onDeleteCopiedRequested()
{
    const int before = m_allItems.size();
    m_allItems.erase(std::remove_if(m_allItems.begin(), m_allItems.end(),
                        [](const Item& it) { return !it.persistent && it.used; }),
                     m_allItems.end());
    const int removed = before - m_allItems.size();
    if (removed == 0) {
        m_statusLabel->setText(QStringLiteral("没有可删除的已复制条目"));
        return;
    }

    // 表格模式下当前行同步为删除后的结果（剔除常驻条目，避免翻页后"复活"或混入）
    if (m_activeTableRowIndex >= 0 && m_activeTableRowIndex < m_tableRows.size()) {
        QVector<Item> rowItems;
        for (const Item& it : m_allItems) {
            if (!it.persistent) {
                rowItems.append(it);
            }
        }
        for (int i = 0; i < rowItems.size(); ++i) {
            rowItems[i].index = i;
        }
        m_tableRows[m_activeTableRowIndex] = rowItems;
    }

    qInfo() << QStringLiteral("删除已复制条目: %1 个").arg(removed);
    applySearchFilter();
    m_statusLabel->setText(QStringLiteral("已删除 %1 个已复制条目").arg(removed));
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
        // 置顶/透明度实时预览（不落盘）
        connect(m_configWindow, &ConfigWindow::alwaysOnTopPreview,
                this, &MainWindow::onAlwaysOnTopPreview);
        connect(m_configWindow, &ConfigWindow::opacityPreview,
                this, [this](int percent) { applyOpacity(percent); });
        // 配置窗口关闭后统一同步（确定 = 新值，取消 = 恢复原状）
        connect(m_configWindow, &QDialog::finished,
                this, &MainWindow::syncConfigWindowSettings);
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

/**
 * @brief 窗口置顶实时预览处理（不落盘）
 * @param on 是否置顶
 */
void MainWindow::onAlwaysOnTopPreview(bool on)
{
    // 仅实时切换窗口标志，不更新状态、不写配置；
    // 最终置顶状态由配置窗口关闭时的 syncConfigWindowSettings 统一同步
    const QRect geo = geometry();
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMinimizeButtonHint;
    flags &= ~Qt::WindowMaximizeButtonHint;
    if (on) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    setGeometry(geo);
    show();
}

/**
 * @brief 配置窗口关闭后，按配置重新应用窗口置顶与透明度
 *        确定 = 应用新值，取消 = 恢复原状
 */
void MainWindow::syncConfigWindowSettings()
{
    const bool onTop = m_config->get(QStringLiteral("window.always_on_top"), true).toBool();
    if (m_isAlwaysOnTop != onTop) {
        // 配置值变化：走标准置顶设置流程（更新状态并保存）
        setAlwaysOnTop(onTop);
    } else {
        // 值未变，但预览可能已临时修改窗口标志，强制恢复与配置一致
        applyWindowFlags();
        show();
    }

    // 同步透明度（预览已实时应用，这里确保与配置一致）
    applyOpacity(m_config->get(QStringLiteral("ui.opacity"), 100).toInt());
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

    // 退出表格模式
    m_tableRows.clear();
    m_activeTableRowIndex = -1;
    updatePagerState();

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

    // 底部透明度控制条
    if (m_opacityBar != nullptr) {
        m_opacityBar->setStyleSheet(QString(
            "QFrame#opacityBar {"
            "    background-color: %1;"
            "    border-top: 1px solid %2;"
            "}"
            "QLabel#opacityTitleLabel, QLabel#opacityValueLabel {"
            "    color: %3;"
            "    font-size: 11px;"
            "    font-family: %4;"
            "}"
            "QSlider#opacitySlider::groove:horizontal {"
            "    height: 4px;"
            "    background: %5;"
            "    border-radius: 2px;"
            "}"
            "QSlider#opacitySlider::handle:horizontal {"
            "    width: 12px;"
            "    margin: -4px 0;"
            "    border-radius: 6px;"
            "    background: %6;"
            "}")
            .arg(theme.value(QStringLiteral("toolbar_bg")))
            .arg(theme.value(QStringLiteral("toolbar_border")))
            .arg(theme.value(QStringLiteral("status_bar_text")))
            .arg(fontCss)
            .arg(theme.value(QStringLiteral("scrollbar_handle")))
            .arg(theme.value(QStringLiteral("search_border_focus"))));
    }

    // 底部多行表格翻页条
    if (m_pagerBar != nullptr) {
        m_pagerBar->setStyleSheet(QString(
            "QFrame#pagerBar {"
            "    background-color: %1;"
            "    border-top: 1px solid %2;"
            "}"
            "QLabel#pagerLabel {"
            "    color: %3;"
            "    font-size: 12px;"
            "    font-family: %4;"
            "}"
            "QPushButton#pagerButton {"
            "    background-color: transparent;"
            "    border: 1px solid %2;"
            "    border-radius: 4px;"
            "    color: %3;"
            "    font-size: 12px;"
            "}"
            "QPushButton#pagerButton:hover {"
            "    background-color: %5;"
            "}"
            "QPushButton#pagerButton:disabled {"
            "    color: %6;"
            "    border-color: %6;"
            "}"
            "QSlider#pagerSlider::groove:horizontal {"
            "    height: 4px;"
            "    background: %5;"
            "    border-radius: 2px;"
            "}"
            "QSlider#pagerSlider::handle:horizontal {"
            "    width: 12px;"
            "    margin: -4px 0;"
            "    border-radius: 6px;"
            "    background: %7;"
            "}")
            .arg(theme.value(QStringLiteral("toolbar_bg")))
            .arg(theme.value(QStringLiteral("toolbar_border")))
            .arg(theme.value(QStringLiteral("status_bar_text")))
            .arg(fontCss)
            .arg(theme.value(QStringLiteral("scrollbar_handle")))
            .arg(theme.value(QStringLiteral("search_placeholder")))
            .arg(theme.value(QStringLiteral("search_border_focus"))));
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
    // 翻页条与透明度条互斥显示（翻页条显示时透明度条隐藏）
    const int opacityBarH = (m_opacityBar != nullptr && m_opacityBar->isVisible()) ? 34 : 0;
    const int pagerBarH = (m_pagerBar != nullptr && m_pagerBar->isVisible()) ? 30 : 0;
    const int contentMargin = 8; // 4px top + 4px bottom
    const int itemH = m_itemWidgets.isEmpty() ? 36 : m_itemWidgets.first()->height();
    const int spacing = 4;

    // 内容高度 = 条目 * (高度 + 间距) - 最后一项无间距
    const int contentH = visibleRows * itemH + (visibleRows - 1) * spacing;

    // 窗口总高度
    const int newHeight = toolbarH + contentMargin + contentH + statusbarH + opacityBarH + pagerBarH + 4;

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
