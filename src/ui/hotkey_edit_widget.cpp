#include "hotkey_edit_widget.h"

#include <QKeyEvent>

#include "theme_manager.h"

// ==================== 匿名命名空间：样式生成辅助函数 ====================
namespace {

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

} // namespace

/**
 * @brief 构造函数
 * @param parent 父部件
 */
HotkeyEditWidget::HotkeyEditWidget(QWidget* parent)
    : QLineEdit(parent)
{
    setReadOnly(true);
    setAlignment(Qt::AlignCenter);
    setMinimumHeight(28);
    setTheme(QStringLiteral("light"));
}

/**
 * @brief 设置主题
 * @param themeName 主题名称，"light" 或 "dark"
 */
void HotkeyEditWidget::setTheme(const QString& themeName)
{
    ThemeColors theme = ThemeManager::getConfigTheme(themeName);
    setStyleSheet(generateEditStyle(theme));
}

/**
 * @brief 按键事件：捕获修饰键 + 主键组合
 */
void HotkeyEditWidget::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    // 单独按下修饰键时不处理，等待主键
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Alt || key == Qt::Key_Meta) {
        return;
    }

    // 收集修饰键名称
    QStringList modifiers;
    if (event->modifiers() & Qt::ControlModifier) {
        modifiers << QStringLiteral("Ctrl");
    }
    if (event->modifiers() & Qt::ShiftModifier) {
        modifiers << QStringLiteral("Shift");
    }
    if (event->modifiers() & Qt::AltModifier) {
        modifiers << QStringLiteral("Alt");
    }
    if (event->modifiers() & Qt::MetaModifier) {
        modifiers << QStringLiteral("Win");
    }

    // 获取主键名称，不支持的按键直接忽略
    QString keyName = getKeyName(key);
    if (keyName.isEmpty()) {
        return;
    }

    QString hotkeyStr;
    if (!modifiers.isEmpty()) {
        hotkeyStr = modifiers.join(QLatin1Char('+')) + QStringLiteral("+") + keyName;
    } else {
        hotkeyStr = keyName;
    }

    setText(hotkeyStr);
    emit hotkeySet(hotkeyStr);
}

/**
 * @brief 获取按键名称（Qt 键码 -> 显示名）
 * @param key Qt 键码
 * @return 按键名称，不支持时返回空字符串
 */
QString HotkeyEditWidget::getKeyName(int key) const
{
    switch (key) {
    // 字母键
    case Qt::Key_A: return QStringLiteral("A");
    case Qt::Key_B: return QStringLiteral("B");
    case Qt::Key_C: return QStringLiteral("C");
    case Qt::Key_D: return QStringLiteral("D");
    case Qt::Key_E: return QStringLiteral("E");
    case Qt::Key_F: return QStringLiteral("F");
    case Qt::Key_G: return QStringLiteral("G");
    case Qt::Key_H: return QStringLiteral("H");
    case Qt::Key_I: return QStringLiteral("I");
    case Qt::Key_J: return QStringLiteral("J");
    case Qt::Key_K: return QStringLiteral("K");
    case Qt::Key_L: return QStringLiteral("L");
    case Qt::Key_M: return QStringLiteral("M");
    case Qt::Key_N: return QStringLiteral("N");
    case Qt::Key_O: return QStringLiteral("O");
    case Qt::Key_P: return QStringLiteral("P");
    case Qt::Key_Q: return QStringLiteral("Q");
    case Qt::Key_R: return QStringLiteral("R");
    case Qt::Key_S: return QStringLiteral("S");
    case Qt::Key_T: return QStringLiteral("T");
    case Qt::Key_U: return QStringLiteral("U");
    case Qt::Key_V: return QStringLiteral("V");
    case Qt::Key_W: return QStringLiteral("W");
    case Qt::Key_X: return QStringLiteral("X");
    case Qt::Key_Y: return QStringLiteral("Y");
    case Qt::Key_Z: return QStringLiteral("Z");
    // 数字键
    case Qt::Key_0: return QStringLiteral("0");
    case Qt::Key_1: return QStringLiteral("1");
    case Qt::Key_2: return QStringLiteral("2");
    case Qt::Key_3: return QStringLiteral("3");
    case Qt::Key_4: return QStringLiteral("4");
    case Qt::Key_5: return QStringLiteral("5");
    case Qt::Key_6: return QStringLiteral("6");
    case Qt::Key_7: return QStringLiteral("7");
    case Qt::Key_8: return QStringLiteral("8");
    case Qt::Key_9: return QStringLiteral("9");
    // 功能键
    case Qt::Key_F1: return QStringLiteral("F1");
    case Qt::Key_F2: return QStringLiteral("F2");
    case Qt::Key_F3: return QStringLiteral("F3");
    case Qt::Key_F4: return QStringLiteral("F4");
    case Qt::Key_F5: return QStringLiteral("F5");
    case Qt::Key_F6: return QStringLiteral("F6");
    case Qt::Key_F7: return QStringLiteral("F7");
    case Qt::Key_F8: return QStringLiteral("F8");
    case Qt::Key_F9: return QStringLiteral("F9");
    case Qt::Key_F10: return QStringLiteral("F10");
    case Qt::Key_F11: return QStringLiteral("F11");
    case Qt::Key_F12: return QStringLiteral("F12");
    // 控制键
    case Qt::Key_Space: return QStringLiteral("SPACE");
    case Qt::Key_Tab: return QStringLiteral("TAB");
    case Qt::Key_Enter: return QStringLiteral("ENTER");
    case Qt::Key_Return: return QStringLiteral("ENTER");
    case Qt::Key_Escape: return QStringLiteral("ESC");
    case Qt::Key_Backspace: return QStringLiteral("BACKSPACE");
    case Qt::Key_Delete: return QStringLiteral("DELETE");
    case Qt::Key_Insert: return QStringLiteral("INSERT");
    case Qt::Key_Home: return QStringLiteral("HOME");
    case Qt::Key_End: return QStringLiteral("END");
    case Qt::Key_PageUp: return QStringLiteral("PAGEUP");
    case Qt::Key_PageDown: return QStringLiteral("PAGEDOWN");
    case Qt::Key_Up: return QStringLiteral("UP");
    case Qt::Key_Down: return QStringLiteral("DOWN");
    case Qt::Key_Left: return QStringLiteral("LEFT");
    case Qt::Key_Right: return QStringLiteral("RIGHT");
    // 符号键
    case Qt::Key_Minus: return QStringLiteral("-");
    case Qt::Key_Plus: return QStringLiteral("+");
    case Qt::Key_Equal: return QStringLiteral("=");
    case Qt::Key_Comma: return QStringLiteral(",");
    case Qt::Key_Period: return QStringLiteral(".");
    case Qt::Key_Slash: return QStringLiteral("/");
    case Qt::Key_BracketLeft: return QStringLiteral("[");
    case Qt::Key_BracketRight: return QStringLiteral("]");
    case Qt::Key_Semicolon: return QStringLiteral(";");
    case Qt::Key_Apostrophe: return QStringLiteral("'");
    case Qt::Key_Backslash: return QStringLiteral("\\");
    case Qt::Key_QuoteLeft: return QStringLiteral("`");
    default:
        return QString();
    }
}
