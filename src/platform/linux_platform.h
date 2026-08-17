#pragma once

#include <QHash>
#include <QPair>
#include <functional>
#include <thread>

#include "platform_interface.h"

// Xlib Display 前向声明（避免在头文件中引入 X11 依赖）
struct _XDisplay;
using Display = _XDisplay;

/**
 * @brief Linux 平台适配器实现
 *
 * 实现 Linux (X11) 平台的具体 API：
 * - XGrabKey 全局热键
 * - XTest 鼠标/键盘模拟（X11 扩展）
 * - XQueryPointer/XTranslateCoordinates 窗口操作
 *
 * 注意：本文件仅在使用 Q_OS_LINUX 时参与编译（Windows 下由 xmake 排除）。
 */
class LinuxPlatform : public PlatformInterface
{
public:
    LinuxPlatform();
    ~LinuxPlatform() override;

    // 平台信息
    QString getPlatformName() const override;

    // 热键管理
    bool registerHotkey(int hotkeyId, quint32 modifiers, quint32 key) override;
    bool unregisterHotkey(int hotkeyId) override;
    void unregisterAllHotkeys() override;
    bool installHotkeyListener(const std::function<bool(int)>& callback) override;
    void removeHotkeyListener() override;

    // 鼠标/键盘模拟
    QPoint getCursorPosition() override;
    bool setCursorPosition(int x, int y) override;
    bool simulateMouseClick(int x, int y) override;
    bool simulateKeyPress(const QVector<quint32>& keyCodes) override;
    bool simulateCtrlV() override;

    // 窗口操作
    WId getWindowAtPosition(int x, int y) override;
    bool setForegroundWindow(WId hwnd) override;

    // 剪贴板增强
    bool isFileCopyContent(const QMimeData* mimeData) override;

    // 资源清理
    void cleanup() override;

private:
    /**
     * @brief 将 Windows 风格修饰键转换为 X11 修饰键掩码
     * @param winModifiers Windows 修饰键位标志
     * @return X11 修饰键掩码
     */
    quint32 convertModifiers(quint32 winModifiers);

    /**
     * @brief 将 Windows 虚拟键码转换为 X11 KeyCode
     * @param key Windows 虚拟键码
     * @return X11 KeyCode，无效返回 0
     */
    quint32 keyToKeycode(quint32 key);

    /**
     * @brief 热键事件循环（后台线程运行）
     * @param callback 热键回调
     */
    void eventLoop(const std::function<bool(int)>& callback);

    Display* m_display = nullptr;                               /**< X11 显示连接 */
    QHash<int, QPair<quint32, quint32>> m_hotkeyCallbacks;      /**< 已注册热键表（id -> (modifiers, keycode)） */
    std::function<bool(int)> m_callback;                        /**< 热键回调 */
    bool m_listenerRunning = false;                             /**< 事件循环运行标志 */
    std::thread* m_listenerThread = nullptr;                    /**< 事件循环线程 */
};
