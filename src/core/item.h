#pragma once

#include <QString>

/**
 * @brief 条目数据结构
 *
 * 表示剪贴板内容切分后的一个条目。
 */
struct Item
{
    QString id;                 /**< 条目唯一标识 */
    QString content;            /**< 条目内容 */
    QString note;               /**< 备注（仅常驻条目，显示于内容前方） */
    int index = 0;              /**< 显示顺序索引（从 0 开始） */
    bool used = false;          /**< 是否已使用 */
    double usedTime = 0.0;      /**< 使用时间戳（Unix 秒） */
    int usageOrder = -1;        /**< 使用顺序（-1 表示从未使用） */
    bool persistent = false;    /**< 是否持久化（勾选复选框固定，防止被覆盖） */
    bool raw = false;           /**< 是否为未分隔的原始条目 */
};
