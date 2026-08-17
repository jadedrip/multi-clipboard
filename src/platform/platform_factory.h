#pragma once

#include <memory>

#include "platform_interface.h"

/**
 * @brief 平台工厂
 *
 * 负责检测当前操作系统并创建对应的平台适配器实例（单例）。
 */
class PlatformFactory
{
public:
    /**
     * @brief 获取平台适配器实例（单例，线程安全）
     * @return 平台适配器共享指针
     */
    static std::shared_ptr<PlatformInterface> platform();

    /**
     * @brief 重置单例实例（释放资源）
     */
    static void reset();
};
