#include "platform_factory.h"

#include <QDebug>
#include <mutex>

#ifdef Q_OS_WIN
#include "windows_platform.h"
#elif defined(Q_OS_LINUX)
#include "linux_platform.h"
#endif

namespace {

// 全局平台适配器实例（单例）
std::shared_ptr<PlatformInterface> g_platform;
std::once_flag g_platformFlag;

} // namespace

// ============================================================
// 平台工厂
// ============================================================

std::shared_ptr<PlatformInterface> PlatformFactory::platform()
{
    // 首次调用时根据当前操作系统创建对应平台适配器
    std::call_once(g_platformFlag, []() {
#ifdef Q_OS_WIN
        g_platform = std::make_shared<WindowsPlatform>();
        qInfo() << "已创建 Windows 平台适配器";
#elif defined(Q_OS_LINUX)
        g_platform = std::make_shared<LinuxPlatform>();
        qInfo() << "已创建 Linux 平台适配器";
#else
        qWarning() << "不支持的操作系统，平台适配器不可用";
#endif
    });
    return g_platform;
}

void PlatformFactory::reset()
{
    if (g_platform) {
        g_platform->cleanup();
        g_platform.reset();
    }
}
