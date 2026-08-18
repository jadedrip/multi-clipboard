#pragma once

#include <QString>
#include <QVector>
#include <optional>
#include <memory>

#include "item.h"

class ConfigManager;

/**
 * @brief 切分模式枚举
 *
 * 定义文本切分的三种模式。
 */
enum class SplitMode
{
    Smart = 0,          /**< 智能模式：自动检测内容格式 */
    SingleColumn = 1,   /**< 单列模式：仅按换行符切分 */
    SingleRow = 2       /**< 单行模式：按制表符切分 */
};

/**
 * @brief 内容解析器
 *
 * 负责将剪贴板文本切分为多个条目，支持智能模式、单列模式和单行模式。
 */
class ContentParser
{
public:
    /**
     * @brief 构造函数
     * @param config 配置管理器实例，为空时内部创建
     */
    explicit ContentParser(ConfigManager* config = nullptr);

    /**
     * @brief 析构函数
     *
     * 显式声明以释放 std::unique_ptr<ConfigManager>（避免不完整类型问题）。
     */
    ~ContentParser();

    /**
     * @brief 解析文本并切分为条目列表
     *
     * 返回的列表包含分隔后的条目和未分隔的原始条目（raw=true）。
     *
     * @param text 原始文本
     * @param mode 切分模式，nullopt 表示使用配置中的模式
     * @return 条目列表
     */
    QVector<Item> parse(const QString& text, std::optional<SplitMode> mode = std::nullopt);

    /**
     * @brief 解析 Excel 复制的文本
     *
     * Excel 复制的文本使用制表符分隔列、换行符分隔行，按列优先顺序输出。
     *
     * @param text Excel 复制的文本
     * @return 条目列表
     */
    QVector<Item> parseFromExcel(const QString& text);

private:
    /**
     * @brief 检查拆分结果是否超过限制
     * @param items 拆分后的条目列表
     * @return true 表示超过限制，应跳过拆分
     */
    bool shouldSkipSplit(const QVector<Item>& items);

    /**
     * @brief 解析未分隔的原始条目
     * @param text 原始文本
     * @param startIndex 起始索引
     * @return 原始条目列表（raw=true），无换行/制表符时返回空
     */
    QVector<Item> parseRaw(const QString& text, int startIndex);

    /**
     * @brief 智能模式解析：含制表符按行优先，否则按列优先
     * @param text 原始文本
     * @return 条目列表
     */
    QVector<Item> parseSmart(const QString& text);

    /**
     * @brief 单列模式解析：按换行符切分，每行作为一个条目
     * @param text 原始文本
     * @return 条目列表
     */
    QVector<Item> parseSingleColumn(const QString& text);

    /**
     * @brief 单行模式解析：按制表符切分，每个单元格作为一个条目
     * @param text 原始文本
     * @return 条目列表
     */
    QVector<Item> parseSingleRow(const QString& text);

    /**
     * @brief 解析转义的分隔符（如 "\\n" -> 换行符）
     * @param delimiter 分隔符字符串
     * @return 解析后的分隔符
     */
    QString resolveDelimiter(const QString& delimiter);

    /**
     * @brief 规范化行尾：\r\n 与 \r 均转换为 \n
     *
     * 保证 Windows（\r\n）、Unix（\n）、旧 Mac（\r）三种换行风格的文本
     * 都能被统一按 \n 切分，避免行尾残留 \r。
     *
     * @param text 原始文本
     * @return 规范化后的文本
     */
    QString normalizeLineEndings(const QString& text);

    /**
     * @brief 应用后处理规则（去除空白、移除空行、去重）
     * @param items 原始条目列表
     * @return 处理后的条目列表
     */
    QVector<Item> applyPostProcessing(const QVector<Item>& items);

    /**
     * @brief 去除条目中的空白字符
     * @param items 条目列表
     * @return 处理后的条目列表
     */
    QVector<Item> stripWhitespace(const QVector<Item>& items);

    /**
     * @brief 移除空行条目
     * @param items 条目列表
     * @return 处理后的条目列表
     */
    QVector<Item> removeEmptyLines(const QVector<Item>& items);

    /**
     * @brief 移除重复条目（保留首次出现）
     * @param items 条目列表
     * @return 处理后的条目列表
     */
    QVector<Item> removeDuplicates(const QVector<Item>& items);

    ConfigManager* m_config = nullptr;                              /**< 配置管理器指针 */
    std::unique_ptr<ConfigManager> m_ownConfig;                     /**< 自有的配置管理器（config 为空时使用） */
    QString m_splitMode = "smart";                                  /**< 切分模式字符串 */
    QString m_singleColumnDelimiter = "\\n";                        /**< 单列分隔符 */
    QString m_singleRowDelimiter = "\\t";                           /**< 单行分隔符 */
    bool m_stripWhitespace = true;                                  /**< 是否去除空白 */
    bool m_removeEmptyLines = true;                                 /**< 是否移除空行 */
    bool m_removeDuplicates = false;                                /**< 是否去重 */
    bool m_enableSplitLimits = true;                                /**< 是否启用切分限制 */
    int m_maxSplitCount = 10;                                       /**< 最大切分条目数 */
    int m_maxItemLength = 100;                                      /**< 单个条目最大长度 */
};
