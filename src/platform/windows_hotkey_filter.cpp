// ============================================================
// Windows 热键事件过滤器（仅 Windows 编译真实实现）
// 其他平台提供空实现以保证工程结构完整。
// 注意：使用编译器预定义宏 _WIN32（Q_OS_WIN 需包含 Qt 头后才定义，
// 而本判断位于所有 include 之前，无法使用）
// ============================================================

#ifdef _WIN32

#include "windows_hotkey_filter.h"

#include <QByteArray>

#include <windows.h>

namespace {

constexpr UINT kWM_Hotkey = 0x0312;         /**< WM_HOTKEY 消息 */

} // namespace

WindowsHotkeyFilter::WindowsHotkeyFilter(const std::function<bool(int)>& callback)
    : m_callback(callback)
{
}

bool WindowsHotkeyFilter::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType == "windows_generic_MSG") {
        const MSG* msg = static_cast<const MSG*>(message);
        if (msg != nullptr && msg->message == kWM_Hotkey) {
            if (m_callback(static_cast<int>(msg->wParam))) {
                if (result != nullptr) {
                    *result = 0;
                }
                return true;
            }
        }
    }
    return false;
}

#else // Q_OS_WIN 未定义：空实现（保持工程结构完整）

#include "windows_hotkey_filter.h"

WindowsHotkeyFilter::WindowsHotkeyFilter(const std::function<bool(int)>&)
{
}

bool WindowsHotkeyFilter::nativeEventFilter(const QByteArray&, void*, qintptr*)
{
    return false;
}

#endif // Q_OS_WIN
