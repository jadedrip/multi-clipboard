#include "content_parser.h"

#include <QSet>

#include "config_manager.h"

// ============================================================
// 构造
// ============================================================

ContentParser::ContentParser(ConfigManager* config)
{
    if (config != nullptr) {
        m_config = config;
    } else {
        m_ownConfig = std::make_unique<ConfigManager>();
        m_config = m_ownConfig.get();
    }

    const QJsonObject parsingConfig = m_config->getParsingConfig();
    m_splitMode = parsingConfig.value("split_mode").toString("smart");
    m_singleColumnDelimiter = parsingConfig.value("single_column_delimiter").toString("\\n");
    m_singleRowDelimiter = parsingConfig.value("single_row_delimiter").toString("\\t");
    m_stripWhitespace = parsingConfig.value("strip_whitespace").toBool(true);
    m_removeEmptyLines = parsingConfig.value("remove_empty_lines").toBool(true);
    m_removeDuplicates = parsingConfig.value("remove_duplicates").toBool(false);
    m_enableSplitLimits = parsingConfig.value("enable_split_limits").toBool(true);
    m_maxSplitCount = parsingConfig.value("max_split_count").toInt(10);
    m_maxItemLength = parsingConfig.value("max_item_length").toInt(100);
}

ContentParser::~ContentParser() = default;

// ============================================================
// 解析主入口
// ============================================================

QVector<Item> ContentParser::parse(const QString& text, std::optional<SplitMode> mode)
{
    // 确定实际使用的切分模式
    SplitMode effectiveMode = SplitMode::Smart;
    if (mode.has_value()) {
        effectiveMode = mode.value();
    } else if (m_splitMode == "single_column") {
        effectiveMode = SplitMode::SingleColumn;
    } else if (m_splitMode == "single_row") {
        effectiveMode = SplitMode::SingleRow;
    } else {
        effectiveMode = SplitMode::Smart;
    }

    // 按模式解析
    QVector<Item> items;
    switch (effectiveMode) {
    case SplitMode::Smart:
        items = parseSmart(text);
        break;
    case SplitMode::SingleColumn:
        items = parseSingleColumn(text);
        break;
    case SplitMode::SingleRow:
        items = parseSingleRow(text);
        break;
    }

    // 应用后处理
    items = applyPostProcessing(items);

    // 如果拆分结果超过限制，则不拆分，只保留原始条目
    if (shouldSkipSplit(items)) {
        QVector<Item> rawItems = parseRaw(text, 0);
        // 如果 parseRaw 返回空（单行纯文本），则手动创建一个原始条目
        if (rawItems.isEmpty() && !text.trimmed().isEmpty()) {
            Item raw;
            raw.id = "raw_0";
            raw.content = text.trimmed();
            raw.index = 0;
            raw.raw = true;
            rawItems.append(raw);
        }
        return rawItems;
    }

    // 原始条目 + 分隔条目
    QVector<Item> rawItems = parseRaw(text, 0);
    QVector<Item> result = rawItems + items;
    for (int i = 0; i < result.size(); ++i) {
        result[i].index = i;
    }
    return result;
}

QVector<Item> ContentParser::parseFromExcel(const QString& text)
{
    const QString trimmed = text.trimmed();
    const QStringList lines = trimmed.split('\n');
    if (lines.isEmpty()) {
        return {};
    }

    // 按列收集（行 -> 列）
    QVector<QStringList> columns;
    for (const QString& line : lines) {
        const QStringList cells = line.split('\t');
        while (cells.size() > columns.size()) {
            columns.append(QStringList());
        }
        for (int i = 0; i < cells.size(); ++i) {
            columns[i].append(cells[i].trimmed());
        }
    }

    // 按列优先顺序输出条目
    QVector<Item> items;
    int globalIdx = 0;
    for (int colIdx = 0; colIdx < columns.size(); ++colIdx) {
        const QStringList& column = columns[colIdx];
        for (int rowIdx = 0; rowIdx < column.size(); ++rowIdx) {
            Item item;
            item.id = QString("excel_%1_%2").arg(colIdx).arg(rowIdx);
            item.content = column[rowIdx];
            item.index = globalIdx;
            items.append(item);
            ++globalIdx;
        }
    }

    return applyPostProcessing(items);
}

// ============================================================
// 私有解析方法
// ============================================================

bool ContentParser::shouldSkipSplit(const QVector<Item>& items)
{
    if (!m_enableSplitLimits) {
        return false;
    }

    // 检查条目数量是否超限
    if (items.size() > m_maxSplitCount) {
        return true;
    }

    // 检查是否有条目的长度超过限制
    for (const Item& item : items) {
        if (item.content.length() > m_maxItemLength) {
            return true;
        }
    }

    return false;
}

QVector<Item> ContentParser::parseRaw(const QString& text, int startIndex)
{
    if (text.isEmpty() || text.trimmed().isEmpty()) {
        return {};
    }

    const bool hasNewline = text.contains('\n');
    const bool hasTab = text.contains('\t');
    if (!hasNewline && !hasTab) {
        return {};
    }

    Item raw;
    raw.id = QString("raw_%1").arg(startIndex);
    raw.content = text.trimmed();
    raw.index = startIndex;
    raw.raw = true;
    return {raw};
}

QVector<Item> ContentParser::parseSmart(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QStringList lines = trimmed.split('\n');
    bool hasTabs = false;
    for (const QString& line : lines) {
        if (line.contains('\t')) {
            hasTabs = true;
            break;
        }
    }

    // 含制表符按行优先（单行模式），否则按列优先（单列模式）
    if (hasTabs) {
        return parseSingleRow(trimmed);
    }
    return parseSingleColumn(trimmed);
}

QVector<Item> ContentParser::parseSingleColumn(const QString& text)
{
    const QString delimiter = resolveDelimiter(m_singleColumnDelimiter);
    const QStringList lines = text.split(delimiter);

    QVector<Item> items;
    for (int i = 0; i < lines.size(); ++i) {
        Item item;
        item.id = QString("item_%1").arg(i);
        item.content = lines[i];
        item.index = i;
        items.append(item);
    }
    return items;
}

QVector<Item> ContentParser::parseSingleRow(const QString& text)
{
    const QString delimiter = resolveDelimiter(m_singleRowDelimiter);
    const QStringList lines = text.split('\n');

    QVector<Item> items;
    int globalIdx = 0;
    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        const QStringList cells = lines[lineIdx].split(delimiter);
        for (int cellIdx = 0; cellIdx < cells.size(); ++cellIdx) {
            Item item;
            item.id = QString("item_%1_%2").arg(lineIdx).arg(cellIdx);
            item.content = cells[cellIdx];
            item.index = globalIdx;
            items.append(item);
            ++globalIdx;
        }
    }
    return items;
}

QString ContentParser::resolveDelimiter(const QString& delimiter)
{
    // 转义序列映射
    static const QList<QPair<QString, QString>> mappings = {
        {QString("\\n"), QString("\n")},
        {QString("\\t"), QString("\t")},
        {QString("\\r"), QString("\r")},
        {QString("\\v"), QString(QChar(0x0B))},
        {QString("\\f"), QString(QChar(0x0C))},
        {QString("\\0"), QString(QChar(0))}
    };

    QString result = delimiter;
    for (const auto& mapping : mappings) {
        result.replace(mapping.first, mapping.second);
    }
    return result;
}

// ============================================================
// 后处理
// ============================================================

QVector<Item> ContentParser::applyPostProcessing(const QVector<Item>& items)
{
    QVector<Item> result = items;
    if (m_stripWhitespace) {
        result = stripWhitespace(result);
    }
    if (m_removeEmptyLines) {
        result = removeEmptyLines(result);
    }
    if (m_removeDuplicates) {
        result = removeDuplicates(result);
    }

    for (int i = 0; i < result.size(); ++i) {
        result[i].index = i;
    }
    return result;
}

QVector<Item> ContentParser::stripWhitespace(const QVector<Item>& items)
{
    QVector<Item> result = items;
    for (Item& item : result) {
        item.content = item.content.trimmed();
    }
    return result;
}

QVector<Item> ContentParser::removeEmptyLines(const QVector<Item>& items)
{
    QVector<Item> result;
    for (const Item& item : items) {
        if (!item.content.isEmpty()) {
            result.append(item);
        }
    }
    return result;
}

QVector<Item> ContentParser::removeDuplicates(const QVector<Item>& items)
{
    QVector<Item> result;
    QSet<QString> seen;
    for (const Item& item : items) {
        if (!seen.contains(item.content)) {
            seen.insert(item.content);
            result.append(item);
        }
    }
    return result;
}
