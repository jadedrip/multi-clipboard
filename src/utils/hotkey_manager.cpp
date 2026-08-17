#include "hotkey_manager.h"

#include <QJsonObject>

#include "platform/platform_factory.h"
#include "utils/config_manager.h"

// ============================================================
// 构造与析构
// ============================================================

HotkeyManager::HotkeyManager(ConfigManager* config, QObject* parent)
    : QObject(parent)
{
    if (config != nullptr) {
        m_config = config;
    } else {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    }

    m_platform = PlatformFactory::platform();
}

HotkeyManager::~HotkeyManager() = default;

// ============================================================
// 快捷键注册
// ============================================================

void HotkeyManager::registerAllHotkeys()
{
    const QJsonObject shortcutConfig = m_config->getShortcutConfig();

    // 快捷键名 -> 回调（调用内部回调成员）
    const QList<QPair<QString, std::function<void()>>> shortcuts = {
        {"toggle_always_on_top", [this]() { if (m_toggleAlwaysOnTopCb) m_toggleAlwaysOnTopCb(); }},
        {"toggle_window",        [this]() { if (m_toggleWindowCb) m_toggleWindowCb(); }},
        {"clear_all",            [this]() { if (m_clearAllCb) m_clearAllCb(); }},
        {"copy_all",             [this]() { if (m_copyAllCb) m_copyAllCb(); }},
        {"paste_plain",          [this]() { if (m_pastePlainCb) m_pastePlainCb(); }},
    };

    for (const auto& shortcut : shortcuts) {
        const QString shortcutStr = shortcutConfig.value(shortcut.first).toString();
        if (shortcutStr.isEmpty()) {
            continue;
        }
        const int hotkeyId = registerHotkey(shortcutStr, shortcut.second);
        if (hotkeyId > 0) {
            m_registeredShortcuts[shortcut.first] = hotkeyId;
            qInfo() << QString("快捷键 [%1] 注册成功 (id=%2)").arg(shortcut.first).arg(hotkeyId);
        } else {
            qWarning() << QString("快捷键 [%1] 注册失败: %2").arg(shortcut.first).arg(shortcutStr);
        }
    }

    // 安装热键监听器
    m_platform->installHotkeyListener([this](int hotkeyId) { return handleHotkey(hotkeyId); });

    qInfo() << QString("HotkeyManager 已注册 %1 个快捷键").arg(m_registeredShortcuts.size());
}

void HotkeyManager::reloadHotkeys()
{
    unregisterAllHotkeys();
    m_registeredShortcuts.clear();
    registerAllHotkeys();
}

int HotkeyManager::registerHotkey(const QString& shortcutStr, const std::function<void()>& callback)
{
    const QPair<quint32, quint32> parsed = parseShortcut(shortcutStr);
    const quint32 modifiers = parsed.first;
    const quint32 key = parsed.second;

    if (key == 0) {
        qWarning() << QString("解析快捷键失败: %1").arg(shortcutStr);
        return 0;
    }

    const int hotkeyId = m_hotkeyIdCounter;
    m_hotkeyIdCounter += 1;

    if (m_platform->registerHotkey(hotkeyId, modifiers, key)) {
        m_hotkeys[hotkeyId] = callback;
        qDebug() << QString("热键注册成功: id=%1").arg(hotkeyId);
        return hotkeyId;
    }

    qWarning() << QString("热键注册失败: %1").arg(shortcutStr);
    return 0;
}

bool HotkeyManager::unregisterHotkey(int hotkeyId)
{
    if (m_platform->unregisterHotkey(hotkeyId)) {
        m_hotkeys.remove(hotkeyId);
        return true;
    }
    return false;
}

void HotkeyManager::unregisterAllHotkeys()
{
    m_platform->unregisterAllHotkeys();
    m_hotkeys.clear();
}

// ============================================================
// 回调设置
// ============================================================

void HotkeyManager::setCallbacks(const std::function<void()>& toggleAlwaysOnTop,
                                 const std::function<void()>& toggleWindow,
                                 const std::function<void()>& clearAll,
                                 const std::function<void()>& copyAll,
                                 const std::function<void()>& pastePlain)
{
    m_toggleAlwaysOnTopCb = toggleAlwaysOnTop;
    m_toggleWindowCb = toggleWindow;
    m_clearAllCb = clearAll;
    m_copyAllCb = copyAll;
    m_pastePlainCb = pastePlain;
}

// ============================================================
// 热键处理
// ============================================================

bool HotkeyManager::handleHotkey(int hotkeyId)
{
    auto it = m_hotkeys.find(hotkeyId);
    if (it != m_hotkeys.end()) {
        qDebug() << QString("处理热键: id=%1").arg(hotkeyId);
        it.value()();
        return true;
    }
    return false;
}

void HotkeyManager::stop()
{
    qInfo() << "HotkeyManager.stop 开始";
    m_platform->removeHotkeyListener();
    unregisterAllHotkeys();
    qInfo() << "HotkeyManager.stop 完成";
}

// ============================================================
// 私有方法
// ============================================================

QPair<quint32, quint32> HotkeyManager::parseShortcut(const QString& shortcutStr)
{
    // Windows 虚拟键码映射
    static const QHash<QString, quint32> vkMap = {
        {"A", 0x41}, {"B", 0x42}, {"C", 0x43}, {"D", 0x44}, {"E", 0x45},
        {"F", 0x46}, {"G", 0x47}, {"H", 0x48}, {"I", 0x49}, {"J", 0x4A},
        {"K", 0x4B}, {"L", 0x4C}, {"M", 0x4D}, {"N", 0x4E}, {"O", 0x4F},
        {"P", 0x50}, {"Q", 0x51}, {"R", 0x52}, {"S", 0x53}, {"T", 0x54},
        {"U", 0x55}, {"V", 0x56}, {"W", 0x57}, {"X", 0x58}, {"Y", 0x59},
        {"Z", 0x5A},
        {"0", 0x30}, {"1", 0x31}, {"2", 0x32}, {"3", 0x33}, {"4", 0x34},
        {"5", 0x35}, {"6", 0x36}, {"7", 0x37}, {"8", 0x38}, {"9", 0x39},
        {"F1", 0x70}, {"F2", 0x71}, {"F3", 0x72}, {"F4", 0x73}, {"F5", 0x74},
        {"F6", 0x75}, {"F7", 0x76}, {"F8", 0x77}, {"F9", 0x78}, {"F10", 0x79},
        {"F11", 0x7A}, {"F12", 0x7B},
        {"SPACE", 0x20}, {"TAB", 0x09}, {"ENTER", 0x0D}, {"ESC", 0x1B},
        {"BACKSPACE", 0x08}, {"DELETE", 0x2E}, {"INSERT", 0x2D},
        {"HOME", 0x24}, {"END", 0x23}, {"PAGEUP", 0x21}, {"PAGEDOWN", 0x22},
        {"UP", 0x26}, {"DOWN", 0x28}, {"LEFT", 0x25}, {"RIGHT", 0x27}
    };

    // Windows 修饰键位标志
    constexpr quint32 kModAlt = 0x0001;
    constexpr quint32 kModControl = 0x0002;
    constexpr quint32 kModShift = 0x0004;
    constexpr quint32 kModWin = 0x0008;

    quint32 modifiers = 0;
    quint32 key = 0;

    const QStringList parts = shortcutStr.split('+');
    for (const QString& part : parts) {
        const QString upper = part.trimmed().toUpper();
        if (upper == "CTRL") {
            modifiers |= kModControl;
        } else if (upper == "ALT") {
            modifiers |= kModAlt;
        } else if (upper == "SHIFT") {
            modifiers |= kModShift;
        } else if (upper == "WIN") {
            modifiers |= kModWin;
        } else if (vkMap.contains(upper)) {
            key = vkMap.value(upper);
        }
    }

    return qMakePair(modifiers, key);
}
