#include "item_widget.h"

#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QSizePolicy>
#include <QFont>
#include <QFontDatabase>
#include <QTimer>
#include <QDateTime>

#include "elide_label.h"
#include "content_parser.h"
#include "drag_manager.h"
#include "config_manager.h"
#include "font_config.h"
#include "platform_factory.h"

// ==================== 匿名命名空间：样式常量与辅助函数 ====================
namespace {

// ---------- 亮色主题样式 ----------

const char* kStyleNormalCardLight =
    "ItemWidget {"
    "    background-color: #ffffff;"
    "    border: 2px solid #909399;"
    "    border-radius: 6px;"
    "}"
    "ItemWidget:hover {"
    "    background-color: #f5f7fa;"
    "    border-color: #606266;"
    "}";

const char* kStyleNormalIndexLight =
    "color: #606266; font-weight: bold; background: transparent; font-size: 11px;";
const char* kStyleNormalContentLight =
    "color: #303133; background: transparent;";

const char* kStyleUsedCardLight =
    "ItemWidget {"
    "    background-color: #f0f9eb;"
    "    border: 2px solid #67c23a;"
    "    border-radius: 6px;"
    "}"
    "ItemWidget:hover {"
    "    background-color: #e8f5e1;"
    "    border-color: #529b2e;"
    "}";

const char* kStyleUsedIndexLight =
    "color: #808080; font-weight: bold; background: transparent; font-size: 11px;";
const char* kStyleUsedContentLight =
    "color: #606266; text-decoration: line-through; background: transparent;";
const char* kStyleUsedCheckLight =
    "color: #67c23a; font-weight: bold; background: transparent; font-size: 13px;";

const char* kStyleRawCardLight =
    "ItemWidget {"
    "    background-color: #f9f0ff;"
    "    border: 2px solid #9254de;"
    "    border-radius: 6px;"
    "}"
    "ItemWidget:hover {"
    "    background-color: #f3d9fa;"
    "    border-color: #722ed1;"
    "}";

const char* kStyleRawIndexLight =
    "color: #9254de; font-weight: bold; background: transparent; font-size: 8px;";
const char* kStyleRawContentLight =
    "color: #531dab; font-style: italic; background: transparent;";
const char* kStyleRawCheckLight =
    "color: #9254de; font-weight: bold; background: transparent; font-size: 9px;";

const char* kStyleCheckboxLight =
    "QCheckBox {"
    "    background: transparent;"
    "    spacing: 0px;"
    "}"
    "QCheckBox::indicator {"
    "    width: 14px;"
    "    height: 14px;"
    "    border: 1px solid #909399;"
    "    border-radius: 2px;"
    "    background-color: #ffffff;"
    "}"
    "QCheckBox::indicator:hover {"
    "    border-color: #4096ff;"
    "    background-color: #ecf5ff;"
    "}"
    "QCheckBox::indicator:checked {"
    "    background-color: #4096ff;"
    "    border-color: #4096ff;"
    "}"
    "QCheckBox::indicator:checked:hover {"
    "    background-color: #337ecc;"
    "    border-color: #337ecc;"
    "}";

const char* kStyleFlashLight =
    "ItemWidget {"
    "    background-color: #dbeafe;"
    "    border: 2px solid #3b82f6;"
    "    border-radius: 6px;"
    "}";

// 搜索不匹配淡化样式（亮色）
const char* kStyleSearchDimmedLight =
    "ItemWidget {"
    "    background-color: #fafafa;"
    "    border: 2px solid #e0e0e0;"
    "    border-radius: 6px;"
    "}";
const char* kStyleSearchDimmedContentLight =
    "color: #b0b0b0; background: transparent;";

// ---------- 暗色主题样式 ----------

const char* kStyleNormalCardDark =
    "ItemWidget {"
    "    background-color: #161b22;"
    "    border: 2px solid #30363d;"
    "    border-radius: 6px;"
    "}"
    "ItemWidget:hover {"
    "    background-color: #21262d;"
    "    border-color: #8b949e;"
    "}";

const char* kStyleNormalIndexDark =
    "color: #8b949e; font-weight: bold; background: transparent; font-size: 11px;";
const char* kStyleNormalContentDark =
    "color: #c9d1d9; background: transparent;";

const char* kStyleUsedCardDark =
    "ItemWidget {"
    "    background-color: #0e4429;"
    "    border: 2px solid #238636;"
    "    border-radius: 6px;"
    "}"
    "ItemWidget:hover {"
    "    background-color: #0d3d24;"
    "    border-color: #2ea043;"
    "}";

const char* kStyleUsedIndexDark =
    "color: #484f58; font-weight: bold; background: transparent; font-size: 11px;";
const char* kStyleUsedContentDark =
    "color: #8b949e; text-decoration: line-through; background: transparent;";
const char* kStyleUsedCheckDark =
    "color: #238636; font-weight: bold; background: transparent; font-size: 13px;";

const char* kStyleRawCardDark =
    "ItemWidget {"
    "    background-color: #2d1f47;"
    "    border: 2px solid #8957e5;"
    "    border-radius: 6px;"
    "}"
    "ItemWidget:hover {"
    "    background-color: #3d2a5a;"
    "    border-color: #a371f7;"
    "}";

const char* kStyleRawIndexDark =
    "color: #8957e5; font-weight: bold; background: transparent; font-size: 8px;";
const char* kStyleRawContentDark =
    "color: #d2a8ff; font-style: italic; background: transparent;";
const char* kStyleRawCheckDark =
    "color: #8957e5; font-weight: bold; background: transparent; font-size: 9px;";

const char* kStyleCheckboxDark =
    "QCheckBox {"
    "    background: transparent;"
    "    spacing: 0px;"
    "}"
    "QCheckBox::indicator {"
    "    width: 14px;"
    "    height: 14px;"
    "    border: 1px solid #30363d;"
    "    border-radius: 2px;"
    "    background-color: #21262d;"
    "}"
    "QCheckBox::indicator:hover {"
    "    border-color: #58a6ff;"
    "    background-color: #1c3d5c;"
    "}"
    "QCheckBox::indicator:checked {"
    "    background-color: #58a6ff;"
    "    border-color: #58a6ff;"
    "}"
    "QCheckBox::indicator:checked:hover {"
    "    background-color: #4794e8;"
    "    border-color: #4794e8;"
    "}";

const char* kStyleFlashDark =
    "ItemWidget {"
    "    background-color: #162332;"
    "    border: 2px solid #58a6ff;"
    "    border-radius: 6px;"
    "}";

// 搜索不匹配淡化样式（暗色）
const char* kStyleSearchDimmedDark =
    "ItemWidget {"
    "    background-color: #0d1117;"
    "    border: 2px solid #21262d;"
    "    border-radius: 6px;"
    "}";
const char* kStyleSearchDimmedContentDark =
    "color: #484f58; background: transparent;";

// ---------- 样式映射表 ----------

using StyleMap = QHash<QString, QHash<QString, QString>>;

/**
 * @brief 构建主题样式映射表（light/dark -> 样式键 -> 样式字符串）
 */
const StyleMap& styleMap()
{
    static StyleMap map = [] {
        StyleMap m;

        // 亮色主题
        QHash<QString, QString> light;
        light.insert(QStringLiteral("normal_card"), QString::fromUtf8(kStyleNormalCardLight));
        light.insert(QStringLiteral("normal_index"), QString::fromUtf8(kStyleNormalIndexLight));
        light.insert(QStringLiteral("normal_content"), QString::fromUtf8(kStyleNormalContentLight));
        light.insert(QStringLiteral("used_card"), QString::fromUtf8(kStyleUsedCardLight));
        light.insert(QStringLiteral("used_index"), QString::fromUtf8(kStyleUsedIndexLight));
        light.insert(QStringLiteral("used_content"), QString::fromUtf8(kStyleUsedContentLight));
        light.insert(QStringLiteral("used_check"), QString::fromUtf8(kStyleUsedCheckLight));
        light.insert(QStringLiteral("raw_card"), QString::fromUtf8(kStyleRawCardLight));
        light.insert(QStringLiteral("raw_index"), QString::fromUtf8(kStyleRawIndexLight));
        light.insert(QStringLiteral("raw_content"), QString::fromUtf8(kStyleRawContentLight));
        light.insert(QStringLiteral("raw_check"), QString::fromUtf8(kStyleRawCheckLight));
        light.insert(QStringLiteral("checkbox"), QString::fromUtf8(kStyleCheckboxLight));
        light.insert(QStringLiteral("flash"), QString::fromUtf8(kStyleFlashLight));
        light.insert(QStringLiteral("search_dimmed"), QString::fromUtf8(kStyleSearchDimmedLight));
        light.insert(QStringLiteral("search_dimmed_content"), QString::fromUtf8(kStyleSearchDimmedContentLight));
        m.insert(QStringLiteral("light"), light);

        // 暗色主题
        QHash<QString, QString> dark;
        dark.insert(QStringLiteral("normal_card"), QString::fromUtf8(kStyleNormalCardDark));
        dark.insert(QStringLiteral("normal_index"), QString::fromUtf8(kStyleNormalIndexDark));
        dark.insert(QStringLiteral("normal_content"), QString::fromUtf8(kStyleNormalContentDark));
        dark.insert(QStringLiteral("used_card"), QString::fromUtf8(kStyleUsedCardDark));
        dark.insert(QStringLiteral("used_index"), QString::fromUtf8(kStyleUsedIndexDark));
        dark.insert(QStringLiteral("used_content"), QString::fromUtf8(kStyleUsedContentDark));
        dark.insert(QStringLiteral("used_check"), QString::fromUtf8(kStyleUsedCheckDark));
        dark.insert(QStringLiteral("raw_card"), QString::fromUtf8(kStyleRawCardDark));
        dark.insert(QStringLiteral("raw_index"), QString::fromUtf8(kStyleRawIndexDark));
        dark.insert(QStringLiteral("raw_content"), QString::fromUtf8(kStyleRawContentDark));
        dark.insert(QStringLiteral("raw_check"), QString::fromUtf8(kStyleRawCheckDark));
        dark.insert(QStringLiteral("checkbox"), QString::fromUtf8(kStyleCheckboxDark));
        dark.insert(QStringLiteral("flash"), QString::fromUtf8(kStyleFlashDark));
        dark.insert(QStringLiteral("search_dimmed"), QString::fromUtf8(kStyleSearchDimmedDark));
        dark.insert(QStringLiteral("search_dimmed_content"), QString::fromUtf8(kStyleSearchDimmedContentDark));
        m.insert(QStringLiteral("dark"), dark);

        return m;
    }();
    return map;
}

/**
 * @brief 获取适配当前平台的中文字体族名称
 * @return 字体族名称，无可用字体时返回空字符串（使用系统默认字体）
 */
QString getFontFamily()
{
    const QStringList families = FontConfig::getChineseFontFamilies();
    const QStringList availableFamilies = QFontDatabase::families();
    const QSet<QString> availableSet(availableFamilies.begin(), availableFamilies.end());

    for (const QString& family : families) {
        if (availableSet.contains(family)) {
            return family;
        }
    }
    // 回退：返回空字符串，使用系统默认字体
    return QString();
}

} // namespace

/**
 * @brief 构造函数
 * @param item 条目数据
 * @param config 配置管理器实例
 * @param dragManager 拖拽管理器实例
 * @param parent 父部件
 */
ItemWidget::ItemWidget(const Item& item,
                       ConfigManager* config,
                       DragManager* dragManager,
                       QWidget* parent)
    : QFrame(parent)
    , m_item(item)
{
    // 配置管理器为空时内部创建
    if (config == nullptr) {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    } else {
        m_config = config;
    }

    // 拖拽管理器为空时内部创建
    if (dragManager == nullptr) {
        m_ownDragManager = std::make_unique<DragManager>(m_config);
        m_dragManager = m_ownDragManager.get();
    } else {
        m_dragManager = dragManager;
    }

    // 读取界面配置
    const QJsonObject uiConfig = m_config->getUiConfig();
    m_itemHeight = uiConfig.value(QStringLiteral("item_height")).toInt(36);
    m_currentTheme = m_config->get(QStringLiteral("ui.theme"), QStringLiteral("light")).toString();

    m_isDragging = false;

    initUi();
}

ItemWidget::~ItemWidget() = default;

/**
 * @brief 初始化界面
 */
void ItemWidget::initUi()
{
    setObjectName(QStringLiteral("itemCard"));
    setFixedHeight(m_itemHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(4);

    // 持久化复选框
    m_persistentCheckbox = new QCheckBox(this);
    m_persistentCheckbox->setChecked(m_item.persistent);
    m_persistentCheckbox->setFixedWidth(18);
    connect(m_persistentCheckbox, &QCheckBox::stateChanged,
            this, &ItemWidget::onPersistentChanged);

    // 获取平台适配的中文字体
    const QString fontFamily = getFontFamily();

    // 序号标签
    m_indexLabel = new QLabel(QStringLiteral("%1.").arg(m_item.index + 1), this);
    m_indexLabel->setFont(QFont(fontFamily, 8, QFont::Bold));
    m_indexLabel->setFixedWidth(22);
    m_indexLabel->setAlignment(Qt::AlignCenter);
    m_indexLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // 内容标签（自动截断）
    const int fontSize = m_config->get(QStringLiteral("ui.font_size"), 10).toInt();
    m_contentLabel = new ElideLabel(m_item.content, this);
    m_contentLabel->setFont(QFont(fontFamily, fontSize));
    m_contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_contentLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // 状态标签（√ 标记）
    m_statusLabel = new QLabel(this);
    m_statusLabel->setFixedWidth(20);
    m_statusLabel->setFont(QFont(fontFamily, 9));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    layout->addWidget(m_persistentCheckbox);
    layout->addWidget(m_indexLabel);
    layout->addWidget(m_contentLabel, 1);
    layout->addWidget(m_statusLabel);

    updateTooltip();

    applyStyle(false);
}

/**
 * @brief 更新 tooltip，鼠标悬停时显示完整文本内容
 */
void ItemWidget::updateTooltip()
{
    const QString content = m_item.content;
    const QString elided = m_contentLabel->displayText();
    if (elided != content || content.contains('\n')) {
        QString prefix = m_item.raw ? QStringLiteral("[原始] ") : QString();
        setToolTip(prefix + content);
    } else {
        setToolTip(QString());
    }
}

/**
 * @brief 设置主题
 * @param theme 主题名称，"light" 或 "dark"
 */
void ItemWidget::setTheme(const QString& theme)
{
    m_currentTheme = theme;
    updateStatusDisplay();
}

/**
 * @brief 设置搜索匹配状态
 * @param match true 表示匹配搜索词（正常显示），false 表示不匹配（淡化显示）
 */
void ItemWidget::setSearchMatch(bool match)
{
    if (!match) {
        setStyleSheet(styleKey(QStringLiteral("search_dimmed")));
        m_contentLabel->setStyleSheet(styleKey(QStringLiteral("search_dimmed_content")));
    }
    // 匹配时不做额外处理，保持原有样式（由 updateStatusDisplay 控制）
}

/**
 * @brief 获取当前主题的样式字符串
 * @param key 样式键名
 */
QString ItemWidget::styleKey(const QString& key) const
{
    const StyleMap& map = styleMap();
    auto themeIt = map.constFind(m_currentTheme);
    const QHash<QString, QString>& themeMap = (themeIt != map.constEnd())
        ? themeIt.value()
        : map.value(QStringLiteral("light"));
    return themeMap.value(key, QString());
}

// ==================== 样式管理 ====================

/**
 * @brief 根据使用状态和是否为原始条目应用完整样式
 * @param isUsed 是否已使用
 */
void ItemWidget::applyStyle(bool isUsed)
{
    if (m_item.raw) {
        applyRawStyle(isUsed);
    } else if (isUsed) {
        applyUsedStyle();
    } else {
        applyNormalStyle();
    }
}

/**
 * @brief 应用未使用样式：浅灰白背景 + 深色文字，高对比度
 */
void ItemWidget::applyNormalStyle()
{
    setStyleSheet(styleKey(QStringLiteral("normal_card")));
    m_persistentCheckbox->setStyleSheet(styleKey(QStringLiteral("checkbox")));
    m_indexLabel->setStyleSheet(styleKey(QStringLiteral("normal_index")));
    m_contentLabel->setStyleSheet(styleKey(QStringLiteral("normal_content")));
    m_statusLabel->setStyleSheet(QStringLiteral("color: transparent; background: transparent;"));
    m_statusLabel->setText(QString());
}

/**
 * @brief 应用已使用样式：淡绿背景 + 灰色文字删除线，柔和不刺眼
 */
void ItemWidget::applyUsedStyle()
{
    setStyleSheet(styleKey(QStringLiteral("used_card")));
    m_persistentCheckbox->setStyleSheet(styleKey(QStringLiteral("checkbox")));
    m_indexLabel->setStyleSheet(styleKey(QStringLiteral("used_index")));
    m_contentLabel->setStyleSheet(styleKey(QStringLiteral("used_content")));
    m_statusLabel->setText(QStringLiteral("\u2713"));
    m_statusLabel->setStyleSheet(styleKey(QStringLiteral("used_check")));
}

/**
 * @brief 应用未分隔原始条目样式：紫色背景 + 紫色边框，醒目区分
 * @param isUsed 是否已使用
 */
void ItemWidget::applyRawStyle(bool isUsed)
{
    setStyleSheet(styleKey(QStringLiteral("raw_card")));
    m_persistentCheckbox->setStyleSheet(styleKey(QStringLiteral("checkbox")));
    m_indexLabel->setStyleSheet(styleKey(QStringLiteral("raw_index")));

    if (isUsed) {
        // 已使用：内容加删除线，显示 √ 标记
        QString contentStyle = styleKey(QStringLiteral("raw_content"));
        contentStyle.replace(QLatin1Char(';'), QStringLiteral("; text-decoration: line-through;"));
        m_contentLabel->setStyleSheet(contentStyle);
        m_statusLabel->setText(QStringLiteral("\u2713"));
        m_statusLabel->setStyleSheet(styleKey(QStringLiteral("raw_check")));
    } else {
        m_contentLabel->setStyleSheet(styleKey(QStringLiteral("raw_content")));
        m_statusLabel->setStyleSheet(QStringLiteral("color: transparent; background: transparent;"));
        m_statusLabel->setText(QString());
    }
}

/**
 * @brief 更新状态显示（根据 used 状态重新应用样式）
 */
void ItemWidget::updateStatusDisplay()
{
    applyStyle(m_item.used);
}

/**
 * @brief 持久化复选框状态变化处理
 * @param state 复选框状态
 */
void ItemWidget::onPersistentChanged(int state)
{
    m_item.persistent = (state == Qt::Checked);
    emit itemPersistentChanged(&m_item, m_item.persistent);
}

// ==================== 状态管理 ====================

/**
 * @brief 设置条目使用状态
 * @param used 是否已使用
 */
void ItemWidget::setUsed(bool used)
{
    if (m_item.used == used) {
        return;
    }

    m_item.used = used;
    m_item.usedTime = used ? QDateTime::currentSecsSinceEpoch() : 0.0;
    updateStatusDisplay();

    if (used) {
        emit itemUsed(&m_item);
    } else {
        emit itemUnused(&m_item);
    }
}

// ==================== 鼠标事件 ====================

/**
 * @brief 鼠标按下事件：记录按下位置，供后续移动判断使用
 *
 * 双击由 Qt 原生 mouseDoubleClickEvent 处理，无需额外定时器。
 */
void ItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressPos = event->pos();
        m_isDragging = false;
    }
    QFrame::mousePressEvent(event);
}

/**
 * @brief 鼠标移动事件：左键按住且移动超过阈值时启动拖拽
 *
 * event->buttons() 确保只有按住左键时才会触发拖拽。
 */
void ItemWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_isDragging && (event->buttons() & Qt::LeftButton)) {
        const int distance = (event->pos() - m_pressPos).manhattanLength();
        if (distance > kDragThreshold) {
            m_isDragging = true;
            startDrag();
        }
    }
    QFrame::mouseMoveEvent(event);
}

/**
 * @brief 鼠标双击事件：复制到剪贴板
 *
 * Qt 原生双击检测，双击时一定没有触发 mouseMoveEvent（否则 Qt 不会判定为双击）。
 */
void ItemWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        onDoubleClick();
    }
    QFrame::mouseDoubleClickEvent(event);
}

/**
 * @brief 双击处理：复制到剪贴板
 */
void ItemWidget::onDoubleClick()
{
    m_dragManager->copyOnly(m_item);
    emit itemCopied(&m_item);

    // 配置允许时双击后标记为已使用
    const bool markUsed = m_config->get(QStringLiteral("ui.mark_used_after_double_click"), true).toBool();
    if (markUsed) {
        setUsed(true);
    }

    flashHighlight();
}

/**
 * @brief 高亮闪烁效果（短暂蓝色高亮后恢复）
 */
void ItemWidget::flashHighlight()
{
    setStyleSheet(styleKey(QStringLiteral("flash")));
    m_persistentCheckbox->setStyleSheet(styleKey(QStringLiteral("checkbox")));
    repaint();

    QTimer::singleShot(200, this, &ItemWidget::restoreFlashStyle);
}

/**
 * @brief 恢复闪烁前的样式
 */
void ItemWidget::restoreFlashStyle()
{
    updateStatusDisplay();
}

/**
 * @brief 启动拖拽操作（仅拖拽成功后才标注为已使用）
 */
void ItemWidget::startDrag()
{
    m_isDragging = false;

    const bool completed = m_dragManager->startDrag(this, m_item);

    if (completed) {
        setUsed(true);
    }
}

/**
 * @brief 右键菜单事件：标记使用状态、复制、纯文本粘贴
 */
void ItemWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    // 标记使用状态（根据当前状态显示相反操作）
    if (m_item.used) {
        QAction* markUnusedAction = menu.addAction(QStringLiteral("标记为未使用"));
        connect(markUnusedAction, &QAction::triggered, this, [this]() { setUsed(false); });
    } else {
        QAction* markUsedAction = menu.addAction(QStringLiteral("标记为已使用"));
        connect(markUsedAction, &QAction::triggered, this, [this]() { setUsed(true); });
    }

    QAction* copyAction = menu.addAction(QStringLiteral("复制到剪贴板"));
    connect(copyAction, &QAction::triggered, this, [this]() { copyToClipboard(); });

    menu.addSeparator();

    QAction* pastePlainAction = menu.addAction(QStringLiteral("纯文本粘贴"));
    connect(pastePlainAction, &QAction::triggered, this, [this]() { pastePlainText(); });

    menu.exec(event->globalPos());
}

/**
 * @brief 纯文本粘贴：复制条目内容并模拟 Ctrl+V 粘贴到目标位置
 */
void ItemWidget::pastePlainText()
{
    m_dragManager->copyOnly(m_item);
    emit itemCopied(&m_item);

    // 使用平台抽象层模拟 Ctrl+V
    auto platform = PlatformFactory::platform();
    if (platform) {
        platform->simulateCtrlV();
    }
}

/**
 * @brief 复制条目内容到剪贴板
 */
void ItemWidget::copyToClipboard()
{
    m_dragManager->copyOnly(m_item);
    emit itemCopied(&m_item);
}

// ==================== 数据更新 ====================

/**
 * @brief 设置条目数据
 * @param item 条目数据
 */
void ItemWidget::setItem(const Item& item)
{
    m_item = item;
    m_contentLabel->setFullText(item.content);
    m_indexLabel->setText(QStringLiteral("%1.").arg(item.index + 1));
    m_persistentCheckbox->setChecked(item.persistent);
    updateTooltip();
    updateStatusDisplay();
}
