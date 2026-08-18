#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QLockFile>

#include <iostream>

#include "utils/logger.h"
#include "utils/config_manager.h"
#include "ui/main_window.h"
#include "ui/font_config.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shobjidl.h>
#endif

// ==================== 匿名命名空间：单实例检测 ====================
namespace {

// 单实例互斥体名称（用于多实例互斥检测）
#ifdef Q_OS_WIN
const wchar_t* kMutexName = L"MultiClipboard_SingleInstance";
#endif

/**
 * @brief 检查是否已有实例在运行
 *
 * Windows: 通过命名互斥体实现；已有实例时广播自定义消息通知其显示窗口。
 * Linux: 通过 QLockFile 实现（基于 PID 锁文件）。
 *
 * @return true 表示本实例可以继续运行，false 表示已有实例
 */
bool checkSingleInstance()
{
#ifdef Q_OS_WIN
    HANDLE mutex = CreateMutexW(nullptr, FALSE, kMutexName);
    if (mutex == nullptr) {
        // 创建失败时允许继续运行
        return true;
    }

    const DWORD lastError = GetLastError();
    if (lastError == ERROR_ALREADY_EXISTS) {
        // 已有实例：广播自定义消息，请求其显示主窗口
        SendNotifyMessageW(HWND_BROADCAST, 0x8001, 0, 0);
        CloseHandle(mutex);
        return false;
    }
    // 互斥体由本进程持有，进程退出时系统自动释放
    return true;
#else
    // Linux: 使用 QLockFile 实现单实例锁
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + QStringLiteral("/.local/share");
    }
    const QString lockPath = baseDir + QStringLiteral("/MultiClipboard/multiclipboard.lock");
    QDir().mkpath(QFileInfo(lockPath).absolutePath());

    static QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);
    // 尝试获取锁，失败说明已有实例在运行
    return lockFile.tryLock(100);
#endif
}

} // namespace

/**
 * @brief 主函数
 */
int main(int argc, char* argv[])
{
    // ========== 第一步：创建 QApplication ==========
    // 注意：日志目录使用可执行文件路径（applicationDirPath），必须先创建应用
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("多元剪贴板"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("MultiClipboard"));
    // 关闭窗口不退出，托盘常驻
    app.setQuitOnLastWindowClosed(false);

    // ========== 第二步：初始化日志（记录后续所有事件） ==========
    Logger& logMgr = Logger::instance();
    logMgr.initialize();
    logMgr.setupGlobalHandler();
    qInfo() << "日志系统初始化完成";

    // ========== 第三步：单实例检测 ==========
    if (!checkSingleInstance()) {
        qInfo() << "检测到已有实例在运行，退出当前实例";
        return 0;
    }
    qInfo() << "单实例检测通过";

    // ========== 第四步：设置 Windows 任务栏图标 ==========
#ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(L"MultiClipboard");
    qInfo() << "AppUserModelID 设置成功";
#endif

    // 设置跨平台默认字体（确保中文正确显示）
    FontConfig::setupApplicationFont(&app);
    qInfo() << "QApplication 创建成功";

    // ========== 第五步：加载配置 ==========
    ConfigManager config;
    qInfo() << QStringLiteral("配置加载成功，窗口置顶: %1")
                   .arg(config.get(QStringLiteral("window.always_on_top"), true).toString());

    // ========== 第六步：创建主窗口 ==========
    MainWindow window(&config);
    qInfo() << "主窗口创建成功";

    // ========== 第七步：显示窗口 ==========
    window.showWindow();
    qInfo() << "窗口显示成功";

    // ========== 第八步：进入事件循环 ==========
    const int exitCode = app.exec();
    qInfo() << QStringLiteral("事件循环退出，退出码: %1").arg(exitCode);
    return exitCode;
}
