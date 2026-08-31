// ============================================================
// Linux 平台适配器实现（基于 X11）
// 注意：本文件仅在 Linux 编译真实实现，
// 其他平台提供空实现以保证工程结构完整。
// 注意：使用编译器预定义宏 __linux__（Q_OS_LINUX 需包含 Qt 头后才定义，
// 而本判断位于所有 include 之前，无法使用）
// ============================================================

#ifdef __linux__

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

/**
 * @brief X11 异步错误处理器
 *
 * XGrabKey 等请求的 BadAccess（热键已被占用）等错误以异步方式到达，
 * Xlib 默认错误处理器会直接退出进程。此处仅记录错误并返回 0，
 * 避免热键冲突时程序崩溃。
 *
 * @param display 出错的显示连接
 * @param event 错误事件
 * @return 0 表示错误已处理（不终止进程）
 */
int x11ErrorHandler(Display* display, XErrorEvent* event)
{
    char buffer[256] = { 0 };
    XGetErrorText(display, event->error_code, buffer, sizeof(buffer) - 1);
    qWarning() << QString("X11 请求错误: %1 (请求码=%2, 错误码=%3)")
                      .arg(QString::fromLocal8Bit(buffer))
                      .arg(event->request_code)
                      .arg(event->error_code);
    return 0;
}

} // namespace

// ============================================================
// 构造与析构
// ============================================================

LinuxPlatform::LinuxPlatform()
{
    // Xlib 线程安全初始化：热键监听线程与主线程并发访问同一 Display，
    // 必须在使用任何 Xlib 函数之前调用
    XInitThreads();

    // 安装错误处理器：热键冲突（BadAccess）等异步错误只记录不退出
    XSetErrorHandler(x11ErrorHandler);

    m_display = XOpenDisplay(nullptr);
    if (m_display != nullptr) {
        qInfo() << "X11 显示连接成功";
    } else {
        qWarning() << "X11 显示连接失败（若为 Wayland 会话，全局热键不受支持）";
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

    const Window root = DefaultRootWindow(m_display);

    // X11 全局热键经典问题：CapsLock(LockMask)/NumLock(Mod2Mask) 开启时，
    // 按键事件的修饰键状态会带上对应掩码，若只注册基础组合则热键失效。
    // 因此对每个热键注册全部 4 种组合变体。
    const QVector<quint32> combos = {
        xModifiers,
        xModifiers | LockMask,              // CapsLock 开启
        xModifiers | Mod2Mask,              // NumLock 开启
        xModifiers | LockMask | Mod2Mask,   // 两者均开启
    };

    QList<QPair<quint32, quint32>> grabs;
    for (quint32 combo : combos) {
        XGrabKey(m_display, static_cast<KeyCode>(xKeycode), combo, root,
                 True, GrabModeAsync, GrabModeAsync);
        grabs.append(qMakePair(combo, xKeycode));
    }

    // 记录基础组合用于事件匹配；记录全部变体用于解除注册
    m_hotkeyCallbacks[hotkeyId] = qMakePair(xModifiers, xKeycode);
    m_registeredGrabs[hotkeyId] = grabs;
    qDebug() << QString("XGrabKey 注册成功: id=%1（4 种修饰键变体）").arg(hotkeyId);
    return true;
}

bool LinuxPlatform::unregisterHotkey(int hotkeyId)
{
    if (m_display == nullptr) {
        return false;
    }
    auto it = m_registeredGrabs.find(hotkeyId);
    if (it != m_registeredGrabs.end()) {
        const Window root = DefaultRootWindow(m_display);
        for (const auto& grab : it.value()) {
            XUngrabKey(m_display, static_cast<KeyCode>(grab.second), grab.first, root);
        }
        m_registeredGrabs.remove(hotkeyId);
    }
    m_hotkeyCallbacks.remove(hotkeyId);
    return true;
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
        // 注意：Window 与 WId 在 Linux 下类型不同，需用 static_cast 转换
        return static_cast<WId>(child != None ? child : root);
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
        case 0x11: keysym = XK_Control_L; break; // VK_CONTROL
        case 0x5B: keysym = XK_Super_L; break;   // VK_LWIN
        case 0x5C: keysym = XK_Super_R; break;   // VK_RWIN
        default:
            break;
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
