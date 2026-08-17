// ============================================================
// Linux 平台适配器实现（基于 X11）
// 注意：本文件仅在 Q_OS_LINUX 下编译真实实现，
// 其他平台提供空实现以保证工程结构完整。
// ============================================================

#ifdef Q_OS_LINUX

#include "linux_platform.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

#include <QDebug>
#include <chrono>

namespace {

// Windows 风格修饰键位标志（与 WindowsPlatform 一致）
constexpr quint32 kModAlt = 0x0001;
constexpr quint32 kModControl = 0x0002;
constexpr quint32 kModShift = 0x0004;
constexpr quint32 kModWin = 0x0008;

// 常用虚拟键码
constexpr quint32 kVkControl = 0x11;    /**< VK_CONTROL */
constexpr quint32 kVkV = 0x56;          /**< VK_V */

} // namespace

// ============================================================
// 构造与析构
// ============================================================

LinuxPlatform::LinuxPlatform()
{
    m_display = XOpenDisplay(nullptr);
    if (m_display != nullptr) {
        qInfo() << "X11 显示连接成功";
    } else {
        qWarning() << "X11 显示连接失败";
    }
    qInfo() << "LinuxPlatform 初始化完成";
}

LinuxPlatform::~LinuxPlatform()
{
    cleanup();
}

QString LinuxPlatform::getPlatformName() const
{
    return QStringLiteral("linux");
}

// ============================================================
// 热键管理
// ============================================================

bool LinuxPlatform::registerHotkey(int hotkeyId, quint32 modifiers, quint32 key)
{
    if (m_display == nullptr) {
        qWarning() << "Xlib 不可用，无法注册热键";
        return false;
    }

    const quint32 xModifiers = convertModifiers(modifiers);
    const quint32 xKeycode = keyToKeycode(key);
    if (xKeycode == 0) {
        qWarning() << QString("无法转换键码: %1").arg(key);
        return false;
    }

    // 在根窗口上捕获按键（GrabModeAsync）
    XGrabKey(m_display, static_cast<KeyCode>(xKeycode), xModifiers,
             DefaultRootWindow(m_display), True, GrabModeAsync, GrabModeAsync);

    m_hotkeyCallbacks[hotkeyId] = qMakePair(xModifiers, xKeycode);
    qDebug() << QString("XGrabKey 成功: id=%1").arg(hotkeyId);
    return true;
}

bool LinuxPlatform::unregisterHotkey(int hotkeyId)
{
    if (m_display == nullptr) {
        return false;
    }
    auto it = m_hotkeyCallbacks.find(hotkeyId);
    if (it != m_hotkeyCallbacks.end()) {
        XUngrabKey(m_display, static_cast<KeyCode>(it.value().second),
                   it.value().first, DefaultRootWindow(m_display));
        m_hotkeyCallbacks.remove(hotkeyId);
        return true;
    }
    return false;
}

void LinuxPlatform::unregisterAllHotkeys()
{
    const QList<int> ids = m_hotkeyCallbacks.keys();
    for (int id : ids) {
        unregisterHotkey(id);
    }
}

bool LinuxPlatform::installHotkeyListener(const std::function<bool(int)>& callback)
{
    if (m_display == nullptr) {
        qWarning() << "Xlib 不可用，无法安装热键监听器";
        return false;
    }
    m_callback = callback;
    m_listenerRunning = true;
    m_listenerThread = new std::thread([this]() { eventLoop(m_callback); });
    qInfo() << "Linux 热键监听器已安装";
    return true;
}

void LinuxPlatform::removeHotkeyListener()
{
    m_listenerRunning = false;
    if (m_listenerThread != nullptr) {
        if (m_listenerThread->joinable()) {
            m_listenerThread->join();
        }
        delete m_listenerThread;
        m_listenerThread = nullptr;
    }
    qInfo() << "Linux 热键监听器已移除";
}

// ============================================================
// 鼠标/键盘模拟
// ============================================================

QPoint LinuxPlatform::getCursorPosition()
{
    if (m_display == nullptr) {
        return QPoint(0, 0);
    }
    Window root, child;
    int rootX = 0, rootY = 0, winX = 0, winY = 0;
    quint32 mask = 0;
    if (XQueryPointer(m_display, DefaultRootWindow(m_display), &root, &child,
                      &rootX, &rootY, &winX, &winY, &mask)) {
        return QPoint(rootX, rootY);
    }
    return QPoint(0, 0);
}

bool LinuxPlatform::setCursorPosition(int x, int y)
{
    if (m_display == nullptr) {
        return false;
    }
    XWarpPointer(m_display, None, DefaultRootWindow(m_display), 0, 0, 0, 0, x, y);
    XFlush(m_display);
    return true;
}

bool LinuxPlatform::simulateMouseClick(int x, int y)
{
    if (m_display == nullptr) {
        return false;
    }
    if (x >= 0 && y >= 0) {
        setCursorPosition(x, y);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    XTestFakeButtonEvent(m_display, 1, True, CurrentTime);
    XTestFakeButtonEvent(m_display, 1, False, CurrentTime);
    XFlush(m_display);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return true;
}

bool LinuxPlatform::simulateKeyPress(const QVector<quint32>& keyCodes)
{
    if (m_display == nullptr) {
        return false;
    }
    // 按下（按顺序）
    for (quint32 code : keyCodes) {
        const KeyCode keycode = static_cast<KeyCode>(keyToKeycode(code));
        if (keycode != 0) {
            XTestFakeKeyEvent(m_display, keycode, True, CurrentTime);
        }
    }
    // 释放（逆序）
    for (int i = keyCodes.size() - 1; i >= 0; --i) {
        const KeyCode keycode = static_cast<KeyCode>(keyToKeycode(keyCodes[i]));
        if (keycode != 0) {
            XTestFakeKeyEvent(m_display, keycode, False, CurrentTime);
        }
    }
    XFlush(m_display);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return true;
}

bool LinuxPlatform::simulateCtrlV()
{
    return simulateKeyPress({kVkControl, kVkV});
}

// ============================================================
// 窗口操作
// ============================================================

WId LinuxPlatform::getWindowAtPosition(int x, int y)
{
    if (m_display == nullptr) {
        return 0;
    }
    Window root = DefaultRootWindow(m_display);
    Window child = None;
    int rootX = 0, rootY = 0, winX = 0, winY = 0;
    quint32 mask = 0;
    if (XQueryPointer(m_display, root, &root, &child, &rootX, &rootY, &winX, &winY, &mask)) {
        return reinterpret_cast<WId>(child != None ? child : root);
    }
    return 0;
}

bool LinuxPlatform::setForegroundWindow(WId hwnd)
{
    if (m_display == nullptr || hwnd == 0) {
        return false;
    }
    const Window window = static_cast<Window>(hwnd);
    XRaiseWindow(m_display, window);
    XSetInputFocus(m_display, window, RevertToParent, CurrentTime);
    XFlush(m_display);
    return true;
}

// ============================================================
// 剪贴板增强
// ============================================================

bool LinuxPlatform::isFileCopyContent(const QMimeData* mimeData)
{
    Q_UNUSED(mimeData);
    // Linux 下不检测文件复制格式，始终返回 false
    return false;
}

// ============================================================
// 私有方法
// ============================================================

quint32 LinuxPlatform::convertModifiers(quint32 winModifiers)
{
    quint32 xModifiers = 0;
    if (winModifiers & kModControl) {
        xModifiers |= ControlMask;
    }
    if (winModifiers & kModShift) {
        xModifiers |= ShiftMask;
    }
    if (winModifiers & kModAlt) {
        xModifiers |= Mod1Mask; // Alt
    }
    if (winModifiers & kModWin) {
        xModifiers |= Mod4Mask; // Super/Win
    }
    return xModifiers;
}

quint32 LinuxPlatform::keyToKeycode(quint32 key)
{
    if (m_display == nullptr) {
        return 0;
    }

    // 将 Windows 虚拟键码映射为 X11 KeySym
    KeySym keysym = 0;
    if (key >= 0x41 && key <= 0x5A) {
        // 字母键 A-Z
        keysym = XK_a + (key - 0x41);
    } else if (key >= 0x30 && key <= 0x39) {
        // 数字键 0-9
        keysym = XK_0 + (key - 0x30);
    } else {
        switch (key) {
        case 0x70: keysym = XK_F1; break;
        case 0x71: keysym = XK_F2; break;
        case 0x72: keysym = XK_F3; break;
        case 0x73: keysym = XK_F4; break;
        case 0x74: keysym = XK_F5; break;
        case 0x75: keysym = XK_F6; break;
        case 0x76: keysym = XK_F7; break;
        case 0x77: keysym = XK_F8; break;
        case 0x78: keysym = XK_F9; break;
        case 0x79: keysym = XK_F10; break;
        case 0x7A: keysym = XK_F11; break;
        case 0x7B: keysym = XK_F12; break;
        case 0x20: keysym = XK_space; break;
        case 0x09: keysym = XK_Tab; break;
        case 0x0D: keysym = XK_Return; break;
        case 0x1B: keysym = XK_Escape; break;
        case 0x08: keysym = XK_BackSpace; break;
        case 0x2E: keysym = XK_Delete; break;
        case 0x2D: keysym = XK_Insert; break;
        case 0x24: keysym = XK_Home; break;
        case 0x23: keysym = XK_End; break;
        case 0x21: keysym = XK_Page_Up; break;
        case 0x22: keysym = XK_Page_Down; break;
        case 0x26: keysym = XK_Up; break;
        case 0x28: keysym = XK_Down; break;
        case 0x25: keysym = XK_Left; break;
        case 0x27: keysym = XK_Right; break;
        default:
            return 0;
        }
    }

    if (keysym == 0) {
        return 0;
    }
    return static_cast<quint32>(XKeysymToKeycode(m_display, keysym));
}

void LinuxPlatform::eventLoop(const std::function<bool(int)>& callback)
{
    // 后台线程事件循环：轮询 X11 事件并分发热键
    while (m_listenerRunning && m_display != nullptr) {
        while (XPending(m_display) > 0) {
            XEvent event;
            XNextEvent(m_display, &event);
            if (event.type == KeyPress) {
                const quint32 keycode = static_cast<quint32>(event.xkey.keycode);
                const quint32 state = event.xkey.state;
                for (auto it = m_hotkeyCallbacks.begin(); it != m_hotkeyCallbacks.end(); ++it) {
                    // 匹配键码与修饰键状态（忽略 CapsLock/NumLock）
                    const quint32 relevantState = state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
                    if (it.value().second == keycode && relevantState == it.value().first) {
                        callback(it.key());
                        break;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ============================================================
// 资源清理
// ============================================================

void LinuxPlatform::cleanup()
{
    removeHotkeyListener();
    unregisterAllHotkeys();
    if (m_display != nullptr) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
    qInfo() << "LinuxPlatform 资源已清理";
}

#else // Q_OS_LINUX 未定义：空实现（保持工程结构完整）

#include "linux_platform.h"

#include <QDebug>

LinuxPlatform::LinuxPlatform()
{
    qWarning() << "LinuxPlatform 仅支持 Linux 平台";
}

LinuxPlatform::~LinuxPlatform()
{
}

QString LinuxPlatform::getPlatformName() const
{
    return QStringLiteral("linux");
}

bool LinuxPlatform::registerHotkey(int, quint32, quint32)
{
    return false;
}

bool LinuxPlatform::unregisterHotkey(int)
{
    return false;
}

void LinuxPlatform::unregisterAllHotkeys()
{
}

bool LinuxPlatform::installHotkeyListener(const std::function<bool(int)>&)
{
    return false;
}

void LinuxPlatform::removeHotkeyListener()
{
}

QPoint LinuxPlatform::getCursorPosition()
{
    return QPoint(0, 0);
}

bool LinuxPlatform::setCursorPosition(int, int)
{
    return false;
}

bool LinuxPlatform::simulateMouseClick(int, int)
{
    return false;
}

bool LinuxPlatform::simulateKeyPress(const QVector<quint32>&)
{
    return false;
}

bool LinuxPlatform::simulateCtrlV()
{
    return false;
}

WId LinuxPlatform::getWindowAtPosition(int, int)
{
    return 0;
}

bool LinuxPlatform::setForegroundWindow(WId)
{
    return false;
}

bool LinuxPlatform::isFileCopyContent(const QMimeData*)
{
    return false;
}

void LinuxPlatform::cleanup()
{
}

#endif // Q_OS_LINUX
