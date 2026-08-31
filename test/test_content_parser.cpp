#include <QtTest/QtTest>
#include <QStringList>

#include "core/content_parser.h"
#include "utils/config_manager.h"

/**
 * @brief 内容解析器单元测试
 *
 * 覆盖内容解析器的全部核心用例。
 * 注意：未分隔的原始条目（raw）位于列表首位。
 */
class TestContentParser : public QObject
{
    Q_OBJECT

private slots:
    // ==================== 智能模式 ====================

    void testParseSmartModeWithTabs()
    {
        // 智能模式解析带制表符的文本（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("a\tb\tc\nd\te\tf");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 7);
        // 原始条目位于首位
        QVERIFY(items[0].raw);
        QCOMPARE(items[0].content, text);
        // 分割条目
        QCOMPARE(items[1].content, QStringLiteral("a"));
        QCOMPARE(items[2].content, QStringLiteral("b"));
        QCOMPARE(items[3].content, QStringLiteral("c"));
        QCOMPARE(items[4].content, QStringLiteral("d"));
    }

    void testParseSmartModeWithoutTabs()
    {
        // 智能模式解析不带制表符的文本（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("line1\nline2\nline3");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        QCOMPARE(items[0].content, text);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    // ==================== 多行表格 ====================

    void testParseTableRowsMultiRow()
    {
        // 多行表格：每行按原逻辑切分为条目（不含整行聚合原始条目），翻页逐行浏览
        ContentParser parser;
        const QString text = QStringLiteral("姓名\t年龄\n张三\t25\n李四\t30");

        const QVector<QVector<Item>> rows = parser.parseTableRows(text);

        QCOMPARE(rows.size(), 3);
        // 第一行：表头，切分为 2 个单元格，无 raw 聚合条目
        QCOMPARE(rows[0].size(), 2);
        QCOMPARE(rows[0][0].content, QStringLiteral("姓名"));
        QCOMPARE(rows[0][1].content, QStringLiteral("年龄"));
        // 第二行：张三/25
        QCOMPARE(rows[1].size(), 2);
        QCOMPARE(rows[1][0].content, QStringLiteral("张三"));
        QCOMPARE(rows[1][1].content, QStringLiteral("25"));
        // 第三行：李四/30
        QCOMPARE(rows[2].size(), 2);
        QCOMPARE(rows[2][0].content, QStringLiteral("李四"));
        QCOMPARE(rows[2][1].content, QStringLiteral("30"));
    }

    void testParseTableRowsNotTable()
    {
        // 非多行表格内容返回空（单行含制表符）
        ContentParser parser;

        const QVector<QVector<Item>> rows1 = parser.parseTableRows(QStringLiteral("a\tb\tc"));
        QVERIFY(rows1.isEmpty());

        // 普通多行文本（无制表符）
        const QVector<QVector<Item>> rows2 = parser.parseTableRows(QStringLiteral("line1\nline2"));
        QVERIFY(rows2.isEmpty());

        // 文件列表内容
        const QVector<QVector<Item>> rows3 = parser.parseTableRows(QStringLiteral("file:///c:/a.txt\nfile:///c:/b.txt"));
        QVERIFY(rows3.isEmpty());
    }

    void testParseTableRowsSkipEmptyLines()
    {
        // 空行被跳过，不参与行计数
        ContentParser parser;
        const QString text = QStringLiteral("a\tb\n\nc\td\n\ne\tf");

        const QVector<QVector<Item>> rows = parser.parseTableRows(text);

        QCOMPARE(rows.size(), 3);
        QCOMPARE(rows[0][0].content, QStringLiteral("a"));
        QCOMPARE(rows[1][1].content, QStringLiteral("d"));
        QCOMPARE(rows[2][1].content, QStringLiteral("f"));
    }

    // ==================== 文件列表忽略 ====================

    void testParseFileListIgnored()
    {
        // 资源管理器复制多个文件产生的 file:// 路径列表应被忽略（不解析为条目）
        ContentParser parser;
        const QString text = QStringLiteral("file:///C:/dir/a.txt\nfile:///C:/dir/b.txt\nfile:///C:/dir/c.txt");

        const QVector<Item> items = parser.parse(text);

        QVERIFY(items.isEmpty());
    }

    void testParseSingleFileIgnored()
    {
        // 单个文件复制（单行 file://）同样忽略，不进入剪贴板历史
        ContentParser parser;
        const QString text = QStringLiteral("file:///C:/dir/a.txt");

        QVERIFY(parser.parse(text).isEmpty());
    }

    void testParseMixedWithFileListLine()
    {
        // 混合内容（file:// 行 + 普通行）不是文件列表，应正常解析
        ContentParser parser;
        const QString text = QStringLiteral("file:///C:/dir/a.txt\nhello");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 3); // 原始 + 2 行
        QCOMPARE(items[1].content, QStringLiteral("file:///C:/dir/a.txt"));
        QCOMPARE(items[2].content, QStringLiteral("hello"));
    }

    // ==================== 行尾兼容性 ====================

    void testParseWindowsLineEndings()
    {
        // Windows \r\n 换行：原始条目应规范化行尾，条目内容不残留 \r
        ContentParser parser;
        const QString text = QStringLiteral("line1\r\nline2\r\nline3");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        // 原始条目行尾已规范化为 \n
        QCOMPARE(items[0].content, QStringLiteral("line1\nline2\nline3"));
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    void testParseOldMacLineEndings()
    {
        // 旧 Mac \r 换行：同样按 \n 切分
        ContentParser parser;
        const QString text = QStringLiteral("line1\rline2\rline3");

        const QVector<Item> items = parser.parse(text);

        QCOMPARE(items.size(), 4);
        QCOMPARE(items[0].content, QStringLiteral("line1\nline2\nline3"));
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    void testParseExcelWindowsLineEndings()
    {
        // Excel 复制的 Windows 文本：\r\n 分行、\t 分列，内容不残留 \r
        ContentParser parser;
        const QString text = QStringLiteral("姓名\t年龄\r\n张三\t25\r\n李四\t30");

        const QVector<Item> items = parser.parseFromExcel(text);

        QCOMPARE(items.size(), 6);
        QCOMPARE(items[0].content, QStringLiteral("姓名"));
        QCOMPARE(items[1].content, QStringLiteral("张三"));
        QCOMPARE(items[2].content, QStringLiteral("李四"));
        QCOMPARE(items[3].content, QStringLiteral("年龄"));
        QCOMPARE(items[4].content, QStringLiteral("25"));
        QCOMPARE(items[5].content, QStringLiteral("30"));
    }

    // ==================== 指定切分模式 ====================

    void testParseSingleColumnMode()
    {
        // 单列模式解析（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("line1\nline2\nline3");

        const QVector<Item> items = parser.parse(text, SplitMode::SingleColumn);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
    }

    void testParseSingleRowMode()
    {
        // 单行模式解析（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("a\tb\tc");

        const QVector<Item> items = parser.parse(text, SplitMode::SingleRow);

        QCOMPARE(items.size(), 4);
        QVERIFY(items[0].raw);
        QCOMPARE(items[0].content, text);
        QCOMPARE(items[1].content, QStringLiteral("a"));
        QCOMPARE(items[2].content, QStringLiteral("b"));
        QCOMPARE(items[3].content, QStringLiteral("c"));
    }

    // ==================== 后处理 ====================

    void testStripWhitespace()
    {
        // 去除空白字符
        ContentParser parser;
        const QString text = QStringLiteral("  line1  \n  line2  ");

        const QVector<Item> items = parser.parse(text);

        // 原始条目 + 2 个分割条目
        QCOMPARE(items.size(), 3);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
    }

    void testRemoveEmptyLines()
    {
        // 移除空行（包含原始条目）
        ContentParser parser;
        const QString text = QStringLiteral("line1\n\nline2\n\n\nline3");

        const QVector<Item> items = parser.parse(text);

        // 原始条目 + 3 个分割条目
        QCOMPARE(items.size(), 4);
        QCOMPARE(items[1].content, QStringLiteral("line1"));
        QCOMPARE(items[2].content, QStringLiteral("line2"));
        QCOMPARE(items[3].content, QStringLiteral("line3"));
    }

    // ==================== 边界情况 ====================

    void testParseEmptyText()
    {
        // 解析空文本
        ContentParser parser;
        const QVector<Item> items = parser.parse(QString());

        QCOMPARE(items.size(), 0);
    }

    // ==================== 强制解析 ====================

    void testParseForcedOverSplitCount()
    {
        // 超过切分数量限制：parse 仅返回原始条目，parseForced 强制切分为多条
        ContentParser parser;
        QStringList lines;
        for (int i = 1; i <= 11; ++i) {
            lines << QStringLiteral("line%1").arg(i);
        }
        const QString text = lines.join('\n');

        // 正常解析：仅保留原始条目（超出默认 10 条限制）
        const QVector<Item> normalItems = parser.parse(text);
        QCOMPARE(normalItems.size(), 1);
        QVERIFY(normalItems[0].raw);
        QCOMPARE(normalItems[0].content, text);

        // 强制解析：切分为原始 + 11 行
        const QVector<Item> forcedItems = parser.parseForced(text);
        QCOMPARE(forcedItems.size(), 12);
        QVERIFY(forcedItems[0].raw);
        QCOMPARE(forcedItems[1].content, QStringLiteral("line1"));
        QCOMPARE(forcedItems[2].content, QStringLiteral("line2"));
        QCOMPARE(forcedItems[11].content, QStringLiteral("line11"));
    }

    void testParseForcedOverItemLength()
    {
        // 超过单条目长度限制：parse 仅返回原始条目，parseForced 强制切分
        ContentParser parser;
        const QString longLine = QString(120, QLatin1Char('a'));
        const QString text = QStringLiteral("short\n%1").arg(longLine);

        // 正常解析：含超长条目，仅保留原始条目
        const QVector<Item> normalItems = parser.parse(text);
        QCOMPARE(normalItems.size(), 1);
        QVERIFY(normalItems[0].raw);

        // 强制解析：切分为原始 + short + 超长行
        const QVector<Item> forcedItems = parser.parseForced(text);
        QCOMPARE(forcedItems.size(), 3);
        QVERIFY(forcedItems[0].raw);
        QCOMPARE(forcedItems[1].content, QStringLiteral("short"));
        QCOMPARE(forcedItems[2].content, longLine);
    }

    void testParseForcedFileListStillIgnored()
    {
        // 强制解析同样忽略文件列表内容（file:// 不是可切分条目）
        ContentParser parser;
        const QString text = QStringLiteral("file:///C:/dir/a.txt\nfile:///C:/dir/b.txt");

        QVERIFY(parser.parseForced(text).isEmpty());
    }

    void testParseForcedSameAsParseWithinLimits()
    {
        // 未超过限制时，强制解析结果与正常解析一致
        ContentParser parser;
        const QString text = QStringLiteral("a\tb\tc\nd\te\tf");

        const QVector<Item> forcedItems = parser.parseForced(text);
        const QVector<Item> normalItems = parser.parse(text);

        QCOMPARE(forcedItems.size(), normalItems.size());
        for (int i = 0; i < normalItems.size(); ++i) {
            QCOMPARE(forcedItems[i].content, normalItems[i].content);
            QCOMPARE(forcedItems[i].raw, normalItems[i].raw);
        }
    }

    // ==================== Excel 格式 ====================

    void testParseExcelFormat()
    {
        // 解析 Excel 格式文本（按列优先顺序输出）
        ContentParser parser;
        const QString text = QStringLiteral("姓名\t年龄\t城市\n张三\t25\t北京\n李四\t30\t上海");

        const QVector<Item> items = parser.parseFromExcel(text);

        QCOMPARE(items.size(), 9);
        QCOMPARE(items[0].content, QStringLiteral("姓名"));
        QCOMPARE(items[1].content, QStringLiteral("张三"));
        QCOMPARE(items[2].content, QStringLiteral("李四"));
        QCOMPARE(items[3].content, QStringLiteral("年龄"));
        QCOMPARE(items[4].content, QStringLiteral("25"));
        QCOMPARE(items[5].content, QStringLiteral("30"));
    }
};

QTEST_MAIN(TestContentParser)
#include "test_content_parser.moc"
