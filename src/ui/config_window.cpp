#include "config_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QMessageBox>
#include <QFont>
#include <QShowEvent>

#include "config_manager.h"
#include "theme_manager.h"
#include "hotkey_edit_widget.h"

// ==================== 匿名命名空间：样式生成辅助函数 ====================
namespace {

/**
 * @brief 生成次要按钮样式（QSS）
 * @param theme 主题颜色
 * @return 样式字符串
 */
QString generateButtonSecondaryStyle(const ThemeColors& theme)
{
    return QString(
               "QPushButton {"
               "    background-color: %1;"
               "    border: 1px solid %2;"
               "    border-radius: 6px;"
               "    padding: 8px 22px;"
               "    min-width: 88px;"
               "    color: %3;"
               "    font-family: \"Microsoft YaHei\";"
               "}"
               "QPushButton:hover {"
               "    background-color: %4;"
               "    border-color: %2;"
               "}"
               "QPushButton:pressed {"
               "    background-color: %5;"
               "    border-color: %2;"
               "}")
        .arg(theme.value(QStringLiteral("button_secondary_bg")))
        .arg(theme.value(QStringLiteral("button_secondary_border")))
        .arg(theme.value(QStringLiteral("button_secondary_text")))
        .arg(theme.value(QStringLiteral("button_secondary_hover")))
        .arg(theme.value(QStringLiteral("button_secondary_pressed")));
}

/**
 * @brief 生成主要按钮样式（QSS）
 * @param theme 主题颜色
 * @return 样式字符串
 */
QString generateButtonPrimaryStyle(const ThemeColors& theme)
{
    return QString(
               "QPushButton {"
               "    background-color: %1;"
               "    color: %2;"
               "    border: none;"
               "    border-radius: 6px;"
               "    padding: 8px 22px;"
               "    min-width: 88px;"
               "    font-family: \"Microsoft YaHei\";"
               "}"
               "QPushButton:hover {"
               "    background-color: %3;"
               "}"
               "QPushButton:pressed {"
               "    background-color: %4;"
               "}")
        .arg(theme.value(QStringLiteral("button_primary_bg")))
        .arg(theme.value(QStringLiteral("button_primary_text")))
        .arg(theme.value(QStringLiteral("button_primary_hover")))
        .arg(theme.value(QStringLiteral("button_primary_pressed")));
}

/**
 * @brief 生成编辑框样式（QSS）
 * @param theme 主题颜色
 * @return 样式字符串
 */
QString generateEditStyle(const ThemeColors& theme)
{
    return QString(
               "QLineEdit {"
               "    background-color: %1;"
               "    border: 1px solid %2;"
               "    border-radius: 4px;"
               "    padding: 4px 8px;"
               "    font-family: \"Microsoft YaHei\", \"Consolas\", monospace;"
               "    font-size: 12px;"
               "    color: %3;"
               "    selection-background-color: %4;"
               "}"
               "QLineEdit:focus {"
               "    border-color: %5;"
               "    background-color: %6;"
               "}")
        .arg(theme.value(QStringLiteral("edit_bg")))
        .arg(theme.value(QStringLiteral("edit_border")))
        .arg(theme.value(QStringLiteral("edit_text")))
        .arg(theme.value(QStringLiteral("header_bg")))
        .arg(theme.value(QStringLiteral("edit_focus_border")))
        .arg(theme.value(QStringLiteral("edit_focus_bg")));
}

/**
 * @brief 生成分区组框样式（QSS）
 * @param theme 主题颜色
 * @return 样式字符串
 */
QString generateGroupBoxStyle(const ThemeColors& theme)
{
    return QString(
               "QGroupBox {"
               "    border: 1px solid %1;"
               "    border-radius: 6px;"
               "    margin-top: 12px;"
               "    padding-top: 18px;"
               "    background-color: %2;"
               "    font-weight: bold;"
               "    color: %3;"
               "}"
               "QGroupBox::title {"
               "    subcontrol-origin: margin;"
               "    left: 14px;"
               "    padding: 0 6px;"
               "}")
        .arg(theme.value(QStringLiteral("group_box_border")))
        .arg(theme.value(QStringLiteral("group_box_bg")))
        .arg(theme.value(QStringLiteral("group_box_title")));
}

/**
 * @brief 生成复选框样式（QSS）
 * @param theme 主题颜色
 * @return 样式字符串
 */
QString generateCheckBoxStyle(const ThemeColors& theme)
{
    return QString(
               "QCheckBox {"
               "    color: %1;"
               "    background: transparent;"
               "    spacing: 8px;"
               "}"
               "QCheckBox::indicator {"
               "    width: 16px;"
               "    height: 16px;"
               "    border: 1px solid %2;"
               "    border-radius: 3px;"
               "    background-color: %3;"
               "}"
               "QCheckBox::indicator:hover {"
               "    border-color: %4;"
               "}"
               "QCheckBox::indicator:checked {"
               "    background-color: %5;"
               "    border-color: %6;"
               "}")
        .arg(theme.value(QStringLiteral("checkbox_text")))
        .arg(theme.value(QStringLiteral("checkbox_border")))
        .arg(theme.value(QStringLiteral("checkbox_bg")))
        .arg(theme.value(QStringLiteral("checkbox_hover_border")))
        .arg(theme.value(QStringLiteral("checkbox_checked_bg")))
        .arg(theme.value(QStringLiteral("checkbox_checked_border")));
}

/**
 * @brief 生成水平滑动条样式（QSS）
 * @param theme 主题颜色
 * @return 样式字符串
 */
QString generateSliderStyle(const ThemeColors& theme)
{
    return QString(
               "QSlider::groove:horizontal {"
               "    height: 4px;"
               "    background: %1;"
               "    border-radius: 2px;"
               "}"
               "QSlider::handle:horizontal {"
               "    width: 14px;"
               "    margin: -5px 0;"
               "    border-radius: 7px;"
               "    background: %2;"
               "}")
        .arg(theme.value(QStringLiteral("slider_groove")))
        .arg(theme.value(QStringLiteral("slider_handle")));
}

} // namespace

/**
 * @brief 构造函数
 * @param config 配置管理器实例
 * @param parent 父部件
 */
ConfigWindow::ConfigWindow(ConfigManager* config, QWidget* parent)
    : QDialog(parent)
{
    // 配置管理器为空时内部创建
    if (config == nullptr) {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    } else {
        m_config = config;
    }

    setWindowTitle(QStringLiteral("多元剪贴板 - 配置"));
    setFixedSize(520, 560);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_currentTheme = m_config->get(QStringLiteral("ui.theme"), QStringLiteral("light")).toString();

    initUi();
}

ConfigWindow::~ConfigWindow() = default;

/**
 * @brief 初始化界面
 */
void ConfigWindow::initUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    initHotkeySection(layout);
    initWindowSection(layout);
    initButtonSection(layout);

    applyTheme();
}

/**
 * @brief 应用主题样式
 */
void ConfigWindow::applyTheme()
{
    const ThemeColors theme = ThemeManager::getConfigTheme(m_currentTheme);

    // 对话框背景
    setStyleSheet(QStringLiteral("QDialog { background-color: %1; }")
                      .arg(theme.value(QStringLiteral("dialog_bg"))));

    // 快捷键分区组框
    if (m_groupBox != nullptr) {
        m_groupBox->setStyleSheet(generateGroupBoxStyle(theme));
    }

    // 窗口设置分区组框
    if (m_windowGroupBox != nullptr) {
        m_windowGroupBox->setStyleSheet(generateGroupBoxStyle(theme));
    }

    // 窗口设置复选框（自动弹出、置顶）
    if (m_autoPopupCheckbox != nullptr) {
        m_autoPopupCheckbox->setStyleSheet(generateCheckBoxStyle(theme));
    }
    if (m_alwaysOnTopCheckbox != nullptr) {
        m_alwaysOnTopCheckbox->setStyleSheet(generateCheckBoxStyle(theme));
    }

    // 透明度滑动条与百分比标签
    if (m_opacitySlider != nullptr) {
        m_opacitySlider->setStyleSheet(generateSliderStyle(theme));
    }
    if (m_opacityValueLabel != nullptr) {
        m_opacityValueLabel->setStyleSheet(
            QStringLiteral("color: %1; background: transparent; font-size: 12px;")
                .arg(theme.value(QStringLiteral("checkbox_text"))));
    }

    // 快捷键表格
    if (m_table != nullptr) {
        m_table->setStyleSheet(QString(
                                   "QTableWidget {"
                                   "    gridline-color: %1;"
                                   "    font-family: \"Microsoft YaHei\", sans-serif;"
                                   "    font-size: 12px;"
                                   "    border: 1px solid %2;"
                                   "    border-radius: 4px;"
                                   "    background-color: %3;"
                                   "    alternate-background-color: %4;"
                                   "}"
                                   "QTableWidget::item {"
                                   "    padding: 8px 10px;"
                                   "    color: %5;"
                                   "}"
                                   "QHeaderView::section {"
                                   "    background-color: %6;"
                                   "    padding: 6px;"
                                   "    font-weight: bold;"
                                   "    color: %7;"
                                   "    border: 1px solid %8;"
                                   "    font-family: \"Microsoft YaHei\", sans-serif;"
                                   "    font-size: 12px;"
                                   "}")
                                   .arg(theme.value(QStringLiteral("table_grid")))
                                   .arg(theme.value(QStringLiteral("table_border")))
                                   .arg(theme.value(QStringLiteral("table_bg")))
                                   .arg(theme.value(QStringLiteral("table_alternate")))
                                   .arg(theme.value(QStringLiteral("edit_text")))
                                   .arg(theme.value(QStringLiteral("header_bg")))
                                   .arg(theme.value(QStringLiteral("header_text")))
                                   .arg(theme.value(QStringLiteral("header_border"))));
    }

    // 提示标签
    if (m_hintLabel != nullptr) {
        m_hintLabel->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                       .arg(theme.value(QStringLiteral("hint_text"))));
    }

    // 按钮
    if (m_resetButton != nullptr) {
        m_resetButton->setStyleSheet(generateButtonSecondaryStyle(theme));
    }
    if (m_cancelButton != nullptr) {
        m_cancelButton->setStyleSheet(generateButtonSecondaryStyle(theme));
    }
    if (m_okButton != nullptr) {
        m_okButton->setStyleSheet(generateButtonPrimaryStyle(theme));
    }

    // 热键编辑控件
    for (auto it = m_hotkeyEdits.constBegin(); it != m_hotkeyEdits.constEnd(); ++it) {
        it.value()->setTheme(m_currentTheme);
    }
}

/**
 * @brief 设置主题
 * @param themeName 主题名称，"light" 或 "dark"
 */
void ConfigWindow::setTheme(const QString& themeName)
{
    m_currentTheme = themeName;
    applyTheme();
}

/**
 * @brief 初始化快捷键配置分区
 * @param parentLayout 父布局
 */
void ConfigWindow::initHotkeySection(QVBoxLayout* parentLayout)
{
    m_groupBox = new QGroupBox(QStringLiteral("快捷键配置"), this);
    m_groupBox->setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));

    auto* layout = new QVBoxLayout(m_groupBox);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // 快捷键表格：5 行 2 列
    m_table = new QTableWidget(5, 2, m_groupBox);
    m_table->setHorizontalHeaderLabels({ QStringLiteral("功能"), QStringLiteral("快捷键") });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(1, 180);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    // 设置行高确保编辑控件可见
    for (int row = 0; row < 5; ++row) {
        m_table->setRowHeight(row, 36);
    }

    // 快捷键定义（键名 -> 功能描述）
    struct ShortcutInfo
    {
        QString key;
        QString desc;
    };
    const QVector<ShortcutInfo> shortcuts = {
        { QStringLiteral("toggle_window"), QStringLiteral("显示/隐藏窗口") },
        { QStringLiteral("toggle_always_on_top"), QStringLiteral("切换窗口置顶") },
        { QStringLiteral("clear_all"), QStringLiteral("清空所有条目") },
        { QStringLiteral("copy_all"), QStringLiteral("复制所有条目") },
        { QStringLiteral("paste_plain"), QStringLiteral("纯文本粘贴") },
    };

    for (int row = 0; row < shortcuts.size(); ++row) {
        // 功能描述列（不可编辑）
        auto* descItem = new QTableWidgetItem(shortcuts[row].desc);
        descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 0, descItem);

        // 快捷键编辑列
        auto* editWidget = new HotkeyEditWidget(m_table);
        const QString currentHotkey = m_config->get(QStringLiteral("shortcuts.") + shortcuts[row].key,
                                                    QString()).toString();
        editWidget->setText(currentHotkey);
        const QString key = shortcuts[row].key;
        connect(editWidget, &HotkeyEditWidget::hotkeySet,
                this, [this, key](const QString& hotkey) {
                    // 热键修改仅记录日志，保存由确定按钮统一处理
                    qInfo() << QStringLiteral("热键修改: %1 -> %2").arg(key, hotkey);
                });
        m_hotkeyEdits.insert(key, editWidget);
        m_table->setCellWidget(row, 1, editWidget);
    }

    layout->addWidget(m_table);

    m_hintLabel = new QLabel(QStringLiteral("提示：点击快捷键列后按下新的按键组合"), m_groupBox);
    m_hintLabel->setFont(QFont(QStringLiteral("Microsoft YaHei"), 10));
    layout->addWidget(m_hintLabel);

    parentLayout->addWidget(m_groupBox);
}

/**
 * @brief 初始化窗口设置分区
 * @param parentLayout 父布局
 */
void ConfigWindow::initWindowSection(QVBoxLayout* parentLayout)
{
    m_windowGroupBox = new QGroupBox(QStringLiteral("窗口设置"), this);
    m_windowGroupBox->setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));

    auto* layout = new QVBoxLayout(m_windowGroupBox);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // 自动弹出复选框
    m_autoPopupCheckbox = new QCheckBox(QStringLiteral("检测到剪贴板改变时自动弹出主界面"), m_windowGroupBox);
    m_autoPopupCheckbox->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    const bool autoPopup = m_config->get(QStringLiteral("window.auto_popup"), true).toBool();
    m_autoPopupCheckbox->setChecked(autoPopup);
    layout->addWidget(m_autoPopupCheckbox);

    // 自动弹出最小条目数（非常驻条目数小于等于该值时仅后台更新，不弹出）
    auto* minItemsRow = new QHBoxLayout();
    minItemsRow->setSpacing(8);

    auto* minItemsLabel = new QLabel(QStringLiteral("自动弹出最小条目数"), m_windowGroupBox);
    minItemsLabel->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    minItemsRow->addWidget(minItemsLabel);

    m_autoPopupMinSpin = new QSpinBox(m_windowGroupBox);
    m_autoPopupMinSpin->setRange(1, 10);
    m_autoPopupMinSpin->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    const int savedMinItems = qBound(1, m_config->get(QStringLiteral("window.auto_popup_min_items"), 3).toInt(), 10);
    m_autoPopupMinSpin->setValue(savedMinItems);
    m_autoPopupMinSpin->setToolTip(QStringLiteral("本次解析出的非常驻条目数小于等于该值时，不自动弹出窗口（常驻条目不参与计数）"));
    minItemsRow->addWidget(m_autoPopupMinSpin);

    auto* minItemsHint = new QLabel(QStringLiteral("条（非常驻）"), m_windowGroupBox);
    minItemsHint->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    minItemsRow->addWidget(minItemsHint);
    minItemsRow->addStretch(1);
    layout->addLayout(minItemsRow);

    // 窗口置顶复选框（变化时实时预览，不落盘）
    m_alwaysOnTopCheckbox = new QCheckBox(QStringLiteral("窗口置顶显示"), m_windowGroupBox);
    m_alwaysOnTopCheckbox->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    const bool alwaysOnTop = m_config->get(QStringLiteral("window.always_on_top"), true).toBool();
    m_alwaysOnTopCheckbox->setChecked(alwaysOnTop);
    connect(m_alwaysOnTopCheckbox, &QCheckBox::toggled,
            this, &ConfigWindow::alwaysOnTopPreview);
    layout->addWidget(m_alwaysOnTopCheckbox);

    // 透明度滑动条行
    auto* opacityRow = new QHBoxLayout();
    opacityRow->setSpacing(8);

    auto* opacityTitle = new QLabel(QStringLiteral("透明度"), m_windowGroupBox);
    opacityTitle->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    opacityRow->addWidget(opacityTitle);

    m_opacitySlider = new QSlider(Qt::Horizontal, m_windowGroupBox);
    m_opacitySlider->setRange(30, 100);
    m_opacitySlider->setFixedHeight(20);
    const int savedOpacity = qBound(30, m_config->get(QStringLiteral("ui.opacity"), 100).toInt(), 100);
    m_opacitySlider->setValue(savedOpacity);
    opacityRow->addWidget(m_opacitySlider, 1);

    m_opacityValueLabel = new QLabel(QStringLiteral("%1%").arg(savedOpacity), m_windowGroupBox);
    m_opacityValueLabel->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    m_opacityValueLabel->setFixedWidth(38);
    m_opacityValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    opacityRow->addWidget(m_opacityValueLabel);

    layout->addLayout(opacityRow);

    // 滑动时实时预览透明度并更新百分比标签（不落盘）
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_opacityValueLabel->setText(QStringLiteral("%1%").arg(value));
        emit opacityPreview(value);
    });

    parentLayout->addWidget(m_windowGroupBox);
}

/**
 * @brief 初始化按钮分区
 * @param parentLayout 父布局
 */
void ConfigWindow::initButtonSection(QVBoxLayout* parentLayout)
{
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();

    m_resetButton = new QPushButton(QStringLiteral("重置默认"), this);
    m_resetButton->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    connect(m_resetButton, &QPushButton::clicked, this, &ConfigWindow::onReset);
    buttonLayout->addWidget(m_resetButton);

    m_cancelButton = new QPushButton(QStringLiteral("取消"), this);
    m_cancelButton->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    connect(m_cancelButton, &QPushButton::clicked, this, &ConfigWindow::reject);
    buttonLayout->addWidget(m_cancelButton);

    m_okButton = new QPushButton(QStringLiteral("确定"), this);
    m_okButton->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    connect(m_okButton, &QPushButton::clicked, this, &ConfigWindow::onOk);
    buttonLayout->addWidget(m_okButton);

    parentLayout->addLayout(buttonLayout);
}

/**
 * @brief 重置为默认快捷键
 */
void ConfigWindow::onReset()
{
    const QHash<QString, QString> defaultShortcuts = {
        { QStringLiteral("toggle_window"), QStringLiteral("Ctrl+Shift+M") },
        { QStringLiteral("toggle_always_on_top"), QStringLiteral("Ctrl+Shift+T") },
        { QStringLiteral("clear_all"), QStringLiteral("Ctrl+Shift+X") },
        { QStringLiteral("copy_all"), QStringLiteral("Ctrl+Shift+C") },
        { QStringLiteral("paste_plain"), QStringLiteral("Ctrl+Shift+V") },
    };

    for (auto it = defaultShortcuts.constBegin(); it != defaultShortcuts.constEnd(); ++it) {
        if (m_hotkeyEdits.contains(it.key())) {
            m_hotkeyEdits.value(it.key())->setText(it.value());
        }
    }

    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已重置为默认快捷键"));
}

/**
 * @brief 确定按钮：保存配置并发出 hotkeysChanged 信号
 */
void ConfigWindow::onOk()
{
    // 保存所有快捷键配置
    for (auto it = m_hotkeyEdits.constBegin(); it != m_hotkeyEdits.constEnd(); ++it) {
        const QString hotkey = it.value()->text().trimmed();
        m_config->set(QStringLiteral("shortcuts.") + it.key(), hotkey);
    }

    // 保存自动弹出配置
    if (m_autoPopupCheckbox != nullptr) {
        m_config->set(QStringLiteral("window.auto_popup"), m_autoPopupCheckbox->isChecked());
    }
    if (m_autoPopupMinSpin != nullptr) {
        m_config->set(QStringLiteral("window.auto_popup_min_items"), m_autoPopupMinSpin->value());
    }

    // 保存窗口置顶配置
    if (m_alwaysOnTopCheckbox != nullptr) {
        m_config->set(QStringLiteral("window.always_on_top"), m_alwaysOnTopCheckbox->isChecked());
    }

    // 保存透明度配置
    if (m_opacitySlider != nullptr) {
        m_config->set(QStringLiteral("ui.opacity"), m_opacitySlider->value());
    }

    m_config->saveConfig();
    emit hotkeysChanged();
    qInfo() << QStringLiteral("配置已保存");

    accept();
}

/**
 * @brief 窗口显示事件：激活窗口
 */
void ConfigWindow::showEvent(QShowEvent* event)
{
    activateWindow();
    QDialog::showEvent(event);
}
