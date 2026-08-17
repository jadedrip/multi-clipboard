#pragma once

#include <QString>
#include <QPoint>
#include <QVector>
#include <QMimeData>
#include <QtGui/qwindowdefs.h>
#include <functional>

/**
 * @brief 平台抽象接口
 *
 * 定义跨平台需要实现的能力，包括：热键管理、鼠标/键盘模拟、窗口操作、光标位置获取。
 */
class PlatformInterface
{
public:
    virtual ~PlatformInterface() = default;

    // ==================== 平台信息 ====================

    /**
     * @brief 获取平台名称
     * @return 平台名称（如 "windows"、"linux"）
     */
    virtual QString getPlatformName() const = 0;

    // ==================== 热键管理 ====================

    /**
     * @brief 注册全局热键
     * @param hotkeyId 热键 ID
     * @param modifiers 修饰键组合（Windows 风格位标志）
     * @param key 虚拟键码
     * @return 是否注册成功
     */
    virtual bool registerHotkey(int hotkeyId, quint32 modifiers, quint32 key) = 0;

    /**
     * @brief 注销全局热键
     * @param hotkeyId 热键 ID
     * @return 是否注销成功
     */
    virtual bool unregisterHotkey(int hotkeyId) = 0;

    /**
     * @brief 注销所有全局热键
     */
    virtual void unregisterAllHotkeys() = 0;

    /**
     * @brief 安装热键监听器
     * @param callback 热键触发回调，接收热键 ID，返回是否已处理
     * @return 是否安装成功
     */
    virtual bool installHotkeyListener(const std::function<bool(int)>& callback) = 0;

    /**
     * @brief 移除热键监听器
     */
    virtual void removeHotkeyListener() = 0;

    // ==================== 鼠标/键盘模拟 ====================

    /**
     * @brief 获取当前光标位置
     * @return (x, y) 坐标
     */
    virtual QPoint getCursorPosition() = 0;

    /**
     * @brief 设置光标位置
     * @param x 横坐标
     * @param y 纵坐标
     * @return 是否设置成功
     */
    virtual bool setCursorPosition(int x, int y) = 0;

    /**
     * @brief 模拟鼠标左键点击
     * @param x 目标横坐标（-1 表示使用当前位置）
     * @param y 目标纵坐标（-1 表示使用当前位置）
     * @return 是否执行成功
     */
    virtual bool simulateMouseClick(int x = -1, int y = -1) = 0;

    /**
     * @brief 模拟按键按下并释放
     * @param keyCodes 虚拟键码序列
     * @return 是否执行成功
     */
    virtual bool simulateKeyPress(const QVector<quint32>& keyCodes) = 0;

    /**
     * @brief 模拟 Ctrl+V 粘贴操作
     * @return 是否执行成功
     */
    virtual bool simulateCtrlV() = 0;

    // ==================== 窗口操作 ====================

    /**
     * @brief 获取指定位置的窗口句柄
     * @param x 横坐标
     * @param y 纵坐标
     * @return 窗口句柄，失败返回 0
     */
    virtual WId getWindowAtPosition(int x, int y) = 0;

    /**
     * @brief 将指定窗口置于前台
     * @param hwnd 窗口句柄
     * @return 是否设置成功
     */
    virtual bool setForegroundWindow(WId hwnd) = 0;

    // ==================== 剪贴板增强 ====================

    /**
     * @brief 检查剪贴板内容是否为文件复制操作
     * @param mimeData QMimeData 对象
     * @return true 表示是文件复制内容
     */
    virtual bool isFileCopyContent(const QMimeData* mimeData) = 0;

    // ==================== 资源清理 ====================

    /**
     * @brief 清理平台相关资源
     */
    virtual void cleanup() = 0;
};
