// ============================================================
// Windows 平台适配器实现（仅 Windows 编译真实实现）
// 其他平台提供空实现以保证工程结构完整。
// 注意：使用编译器预定义宏 _WIN32（Q_OS_WIN 需包含 Qt 头后才定义，
// 而本判断位于所有 include 之前，无法使用）
// ============================================================

#ifdef _WIN32

#include "windows_platform.h"
#include "windows_hotkey_filter.h"

#include <windows.h>

#include <QApplication>
#include <QDebug>

// ============================================================
// Windows API 常量与结构
// ============================================================

namespace {

constexpr UINT kInputMouse = 0;                                 /**< 鼠标输入类型 */
constexpr UINT kInputKeyboard = 1;                              /**< 键盘输入类型 */
constexpr DWORD kKeyEventFKeyUp = 0x0002;                       /**< 按键释放标志 */
constexpr DWORD kMouseEventFLeftDown = 0x0002;                  /**< 左键按下标志 */
constexpr DWORD kMouseEventFLeftUp = 0x0004;                    /**< 左键释放标志 */
constexpr UINT kVkControl = 0x11;                               /**< VK_CONTROL 虚拟键码 */
constexpr UINT kVkV = 0x56;                                     /**< VK_V 虚拟键码 */

// Windows 剪贴板文件复制格式
const char* kFileCopyFormats[] = {
    "application/x-qt-windows-mime;value=\"Shell IDList Array\"",
    "application/x-qt-windows-mime;value=\"FileGroupDescriptorW\"",
    "application/x-qt-windows-mime;value=\"FileName\"",
    "application/x-qt-windows-mime;value=\"FileNameW\"",
    "application/x-qt-windows-mime;value=\"Preferred DropEffect\"",
};

} // namespace

// ============================================================
// 构造与析构
// ============================================================

WindowsPlatform::WindowsPlatform()
{
    qInfo() << "WindowsPlatform 初始化完成";
}

WindowsPlatform::~WindowsPlatform()
{
    cleanup();
}

QString WindowsPlatform::getPlatformName() const
{
    return QStringLiteral("windows");
}

// ============================================================
// 热键管理
// ============================================================

bool WindowsPlatform::registerHotkey(int hotkeyId, quint32 modifiers, quint32 key)
{
    if (RegisterHotKey(nullptr, hotkeyId, modifiers, key)) {
        m_registeredHotkeys[hotkeyId] = qMakePair(modifiers, key);
        qDebug() << QString("RegisterHotKey 成功: id=%1, mod=%2, key=%3")
                        .arg(hotkeyId).arg(modifiers).arg(key);
        return true;
    }
    qWarning() << QString("RegisterHotKey 失败: id=%1, error=%2").arg(hotkeyId).arg(GetLastError());
    return false;
}

bool WindowsPlatform::unregisterHotkey(int hotkeyId)
{
    if (UnregisterHotKey(nullptr, hotkeyId)) {
        m_registeredHotkeys.remove(hotkeyId);
        return true;
    }
    return false;
}

void WindowsPlatform::unregisterAllHotkeys()
{
    const QList<int> ids = m_registeredHotkeys.keys();
    for (int id : ids) {
        unregisterHotkey(id);
    }
}

bool WindowsPlatform::installHotkeyListener(const std::function<bool(int)>& callback)
{
    if (m_hotkeyFilter == nullptr) {
        m_hotkeyFilter = new WindowsHotkeyFilter(callback);
        qApp->installNativeEventFilter(m_hotkeyFilter);
        qInfo() << "Windows 热键监听器已安装";
        return true;
    }
    return false;
}

void WindowsPlatform::removeHotkeyListener()
{
    if (m_hotkeyFilter != nullptr) {
        qApp->removeNativeEventFilter(m_hotkeyFilter);
        delete m_hotkeyFilter;
        m_hotkeyFilter = nullptr;
        qInfo() << "Windows 热键监听器已移除";
    }
}

// ============================================================
// 鼠标/键盘模拟
// ============================================================

QPoint WindowsPlatform::getCursorPosition()
{
    POINT point;
    GetCursorPos(&point);
    return QPoint(point.x, point.y);
}

bool WindowsPlatform::setCursorPosition(int x, int y)
{
    return SetCursorPos(x, y) != 0;
}

bool WindowsPlatform::simulateMouseClick(int x, int y)
{
    if (x >= 0 && y >= 0) {
        SetCursorPos(x, y);
        Sleep(50);
    }

    INPUT down = {};
    down.type = kInputMouse;
    down.mi.dwFlags = kMouseEventFLeftDown;

    INPUT up = {};
    up.type = kInputMouse;
    up.mi.dwFlags = kMouseEventFLeftUp;

    INPUT inputs[2] = {down, up};
    SendInput(2, inputs, sizeof(INPUT));
    Sleep(50);
    return true;
}

bool WindowsPlatform::simulateKeyPress(const QVector<quint32>& keyCodes)
{
    QVector<INPUT> inputs;
    inputs.reserve(keyCodes.size() * 2);

    // 按下（按顺序）
    for (quint32 code : keyCodes) {
        INPUT in = {};
        in.type = kInputKeyboard;
        in.ki.wVk = code;
        inputs.append(in);
    }
    // 释放（逆序）
    for (int i = keyCodes.size() - 1; i >= 0; --i) {
        INPUT in = {};
        in.type = kInputKeyboard;
        in.ki.wVk = keyCodes[i];
        in.ki.dwFlags = kKeyEventFKeyUp;
        inputs.append(in);
    }

    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    Sleep(50);
    return true;
}

bool WindowsPlatform::simulateCtrlV()
{
    return simulateKeyPress({kVkControl, kVkV});
}

// ============================================================
// 窗口操作
// ============================================================

WId WindowsPlatform::getWindowAtPosition(int x, int y)
{
    POINT pt;
    pt.x = x;
    pt.y = y;
    return reinterpret_cast<WId>(WindowFromPoint(pt));
}

bool WindowsPlatform::setForegroundWindow(WId hwnd)
{
    return ::SetForegroundWindow(reinterpret_cast<HWND>(hwnd)) != 0;
}

// ============================================================
// 剪贴板增强
// ============================================================

bool WindowsPlatform::isFileCopyContent(const QMimeData* mimeData)
{
    if (mimeData == nullptr) {
        return false;
    }
    for (const char* format : kFileCopyFormats) {
        if (mimeData->hasFormat(QString::fromUtf8(format))) {
            return true;
        }
    }
    return false;
}

// ============================================================
// 资源清理
// ============================================================

void WindowsPlatform::cleanup()
{
    removeHotkeyListener();
    unregisterAllHotkeys();
    qInfo() << "WindowsPlatform 资源已清理";
}

#else // Q_OS_WIN 未定义：空实现（保持工程结构完整）

#include "windows_platform.h"

#include <QDebug>

WindowsPlatform::WindowsPlatform()
{
    qWarning() << "WindowsPlatform 仅支持 Windows 平台";
}

WindowsPlatform::~WindowsPlatform()
{
}

QString WindowsPlatform::getPlatformName() const
{
    return QStringLiteral("windows");
}

bool WindowsPlatform::registerHotkey(int, quint32, quint32)
{
    return false;
}

bool WindowsPlatform::unregisterHotkey(int)
{
    return false;
}

void WindowsPlatform::unregisterAllHotkeys()
{
}

bool WindowsPlatform::installHotkeyListener(const std::function<bool(int)>&)
{
    return false;
}

void WindowsPlatform::removeHotkeyListener()
{
}

QPoint WindowsPlatform::getCursorPosition()
{
    return QPoint();
}

bool WindowsPlatform::setCursorPosition(int, int)
{
    return false;
}

bool WindowsPlatform::simulateMouseClick(int, int)
{
    return false;
}

bool WindowsPlatform::simulateKeyPress(const QVector<quint32>&)
{
    return false;
}

bool WindowsPlatform::simulateCtrlV()
{
    return false;
}

WId WindowsPlatform::getWindowAtPosition(int, int)
{
    return 0;
}

bool WindowsPlatform::setForegroundWindow(WId)
{
    return false;
}

bool WindowsPlatform::isFileCopyContent(const QMimeData*)
{
    return false;
}

void WindowsPlatform::cleanup()
{
}

#endif // Q_OS_WIN
