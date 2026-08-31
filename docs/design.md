# 多元剪贴板 C++20 设计文档

## 1. 目标

设计并实现「多元剪贴板」原生应用，基于 **C++20 + Qt6**，提供以下功能：

- 智能内容切分（smart / single_column / single_row / Excel 格式）
- 条目拖拽复制（QDrag 原生 + 剪贴板+模拟按键回退双路径）
- 实时搜索过滤（匹配高亮、不匹配淡化）
- 明暗主题切换
- 全局快捷键（RegisterHotKey + 原生事件过滤器）
- 系统托盘常驻
- 持久化条目（复选框固定）
- 纯文本粘贴
- 单实例运行检测
- 日志系统（文件轮转 + 错误日志 + 崩溃报告）
- 列表空白区域拖拽移动窗体（不依赖标题栏）
- 窗口透明度配置（界面底部滑动条，30%~100% 不透明）

## 2. 技术选型

| 项目 | 选择 | 说明 |
| :--- | :--- | :--- |
| 语言标准 | C++20 | 用户指定 |
| GUI 框架 | Qt 6 (qtbase) | 用户规则要求 Qt6 |
| 构建系统 | xmake | 用户规则要求 C++ 工程优先使用 xmake |
| 依赖管理 | vcpkg | qtbase 通过 vcpkg 安装，路径取自环境变量 `vcpkg_root` |
| 平台 | Windows 10/11 为主，Linux 适配保留接口 | 开发环境为 Windows |

## 3. 目录结构

```
├── xmake.lua                     # 构建脚本
├── docs/
│   └── design.md                 # 本设计文档
├── src/
│   ├── main.cpp                  # 主程序入口（单实例检测、日志、App 初始化）
│   ├── core/                     # 核心模块
│   │   ├── item.h                # 条目数据结构
│   │   ├── content_parser.h      # 内容解析器（接口声明）
│   │   ├── content_parser.cpp    # 内容解析器实现
│   │   ├── clipboard_manager.h   # 剪贴板管理器（接口声明）
│   │   ├── clipboard_manager.cpp # 剪贴板管理器实现
│   │   ├── drag_manager.h        # 拖拽管理器（接口声明）
│   │   └── drag_manager.cpp      # 拖拽管理器实现
│   ├── platform/                 # 平台抽象层
│   │   ├── platform_interface.h  # 平台抽象接口（纯虚类）
│   │   ├── platform_factory.h    # 平台工厂（接口声明）
│   │   ├── platform_factory.cpp  # 平台工厂实现
│   │   ├── windows_platform.h    # Windows 平台实现（接口声明）
│   │   ├── windows_platform.cpp  # Windows 平台实现
│   │   ├── windows_hotkey_filter.h # Windows 热键事件过滤器（接口声明）
│   │   ├── windows_hotkey_filter.cpp # Windows 热键事件过滤器实现
│   │   ├── linux_platform.h      # Linux 平台实现（接口声明）
│   │   └── linux_platform.cpp    # Linux 平台实现（条件编译）
│   ├── ui/                       # UI 模块（仅负责界面与交互，业务逻辑在 core 层）
│   │   ├── main_window.h         # 主窗口（接口声明）
│   │   ├── main_window.cpp       # 主窗口实现
│   │   ├── item_widget.h         # 条目卡片组件（接口声明）
│   │   ├── item_widget.cpp       # 条目卡片组件实现
│   │   ├── elide_label.h         # 自动截断文本标签（接口声明）
│   │   ├── elide_label.cpp       # 自动截断文本标签实现
│   │   ├── config_window.h       # 配置窗口（接口声明）
│   │   ├── config_window.cpp     # 配置窗口实现
│   │   ├── hotkey_edit_widget.h  # 热键编辑控件（接口声明）
│   │   ├── hotkey_edit_widget.cpp# 热键编辑控件实现
│   │   ├── theme_manager.h       # 主题管理器（接口声明）
│   │   ├── theme_manager.cpp     # 主题管理器实现（颜色常量 + 样式生成）
│   │   ├── font_config.h         # 字体配置（接口声明）
│   │   ├── font_config.cpp       # 字体配置实现
│   │   ├── window_drag_filter.h  # 窗口拖拽移动过滤器（接口声明）
│   │   └── window_drag_filter.cpp # 窗口拖拽移动过滤器实现
│   └── utils/                    # 工具模块
│       ├── config_manager.h      # 配置管理器（接口声明）
│       ├── config_manager.cpp    # 配置管理器实现
│       ├── hotkey_manager.h      # 全局热键管理器（接口声明）
│       ├── hotkey_manager.cpp    # 全局热键管理器实现
│       ├── logger.h              # 日志管理器（接口声明）
│       └── logger.cpp            # 日志管理器实现
├── resources/
│   ├── resources.qrc             # Qt 资源文件
│   └── icon.ico                  # 应用图标
└── test/
    └── test_content_parser.cpp   # 内容解析器单元测试（Qt Test）
```

## 4. 模块接口设计

### 4.1 Item（条目数据结构）— `core/item.h`

使用普通结构体 + 成员变量定义条目：

```cpp
struct Item {
    QString id;              // 条目唯一标识
    QString content;         // 条目内容
    QString note;            // 备注（仅常驻条目，显示于内容前方）
    int index = 0;           // 显示顺序索引
    bool used = false;       // 是否已使用
    double usedTime = 0.0;   // 使用时间戳
    int usageOrder = -1;     // 使用顺序
    bool persistent = false; // 是否持久化（复选框固定）
    bool raw = false;        // 是否为未分隔的原始条目
};
```

### 4.2 ContentParser（内容解析器）— `core/content_parser.h/.cpp`

| 方法 | 说明 |
| :--- | :--- |
| `enum class SplitMode { Smart, SingleColumn, SingleRow }` | 切分模式 |
| `QVector<Item> parse(const QString& text, SplitMode mode = SplitMode::Smart)` | 解析文本为条目列表（含原始条目） |
| `QVector<Item> parseFromExcel(const QString& text)` | 解析 Excel 复制文本（按列优先） |
| `QVector<Item> parseForced(const QString& text, SplitMode mode = SplitMode::Smart)` | 强制解析：绕过切分限制（超限时也切分为多条） |
| `QVector<QVector<Item>> parseTableRows(const QString& text)` | 检测多行表格并解析为逐行条目（翻页浏览用）；非表格内容返回空 |

私有方法：`parseRaw`、`parseSmart`、`parseSingleColumn`、`parseSingleRow`、`resolveDelimiter`、`applyPostProcessing`、`stripWhitespace`、`removeEmptyLines`、`removeDuplicates`、`shouldSkipSplit`、`isFileListContent`、`isMultiRowTable`。

> 多行表格规则：`parseTableRows` 检测到多行表格（行数 ≥ 2 且至少 2 行含制表符 `\t` 分隔列）时，不整段切碎，而是**逐行独立解析**：每行按原有智能切分逻辑切分为条目列表（仅保留切分后的条目，去除整行的聚合原始条目），返回逐行条目集合（`QVector<QVector<Item>>`）。主窗口一次显示一行切分后的条目，底部翻页条切换 Excel 行。行内条目序号连续，跨行序号由主窗口统一重排。仅在智能模式下检测，显式指定单列/单行模式时保持原有切分行为。

> 特殊规则：`parse` 入口检测到文件列表内容（所有非空行均以 `file://` 开头，即资源管理器复制文件产生的剪贴板文本）时直接返回空列表，避免弹出一堆 file:// 条目。

> 强制解析：`parseForced` 与 `parse` 相同但跳过 `shouldSkipSplit` 限制，供原始条目右键"解析"使用——即使超限（超过 max_split_count / max_item_length）也强制切分为多条。

### 4.3 ClipboardManager（剪贴板管理器）— `core/clipboard_manager.h/.cpp`

继承 `QObject`，负责监控剪贴板变化：

| 成员 | 说明 |
| :--- | :--- |
| `Q_SIGNAL void clipboardChanged(const QString& text)` | 剪贴板内容变化信号 |
| `void startMonitoring()` / `void stopMonitoring()` | 启动/停止监控 |
| `QString getText()` | 获取剪贴板文本 |
| `bool setText(const QString& text)` | 写入剪贴板文本并同步 `lastText` |
| `void clear()` | 清空剪贴板 |
| `bool isTextAvailable()` | 剪贴板是否有文本 |
| `void setMonitorInterval(int ms)` | 设置监控间隔 |
| `void stop()` | 停止监控并清理 |

私有：`QTimer* m_monitorTimer`、`QString m_lastText`、`qint64 m_lastCheckTime`、平台适配器指针。

### 4.4 DragManager（拖拽管理器）— `core/drag_manager.h/.cpp`

继承 `QObject`，负责条目拖拽与复制粘贴：

| 方法 | 说明 |
| :--- | :--- |
| `bool startDrag(QWidget* widget, const Item& item)` | 启动拖拽（路径 A），失败回退路径 B |
| `bool copyToClipboardAndPaste(const Item& item)` | 复制到剪贴板并模拟粘贴 |
| `bool copyOnly(const Item& item)` | 仅复制到剪贴板 |
| `bool copyText(const QString& text)` | 直接复制文本 |

### 4.5 平台抽象层 — `platform/`

`PlatformInterface` 为纯虚基类，定义：

```cpp
class PlatformInterface {
public:
    virtual ~PlatformInterface() = default;
    virtual QString getPlatformName() const = 0;
    virtual bool registerHotkey(int hotkeyId, quint32 modifiers, quint32 key) = 0;
    virtual bool unregisterHotkey(int hotkeyId) = 0;
    virtual void unregisterAllHotkeys() = 0;
    virtual bool installHotkeyListener(std::function<bool(int)> callback) = 0;
    virtual void removeHotkeyListener() = 0;
    virtual QPoint getCursorPosition() = 0;
    virtual bool setCursorPosition(int x, int y) = 0;
    virtual bool simulateMouseClick(int x = -1, int y = -1) = 0;
    virtual bool simulateKeyPress(const QVector<quint32>& keyCodes) = 0;
    virtual bool simulateCtrlV() = 0;
    virtual HWND/WId getWindowAtPosition(int x, int y) = 0;
    virtual bool setForegroundWindow(WId hwnd) = 0;
    virtual bool isFileCopyContent(const QMimeData* mimeData) = 0;
    virtual void cleanup() = 0;
};
```

- `WindowsPlatform`：完整实现（RegisterHotKey、SendInput、WindowFromPoint、QAbstractNativeEventFilter 子类 `WindowsHotkeyFilter`）。
- `LinuxPlatform`：基于 X11 的实现，使用 `#ifdef Q_OS_LINUX` 条件编译，Windows 下不参与编译。
- `PlatformFactory`：单例工厂，Windows 下返回 `WindowsPlatform`，Linux 下返回 `LinuxPlatform`。

### 4.6 ConfigManager（配置管理器）— `utils/config_manager.h/.cpp`

| 方法 | 说明 |
| :--- | :--- |
| `explicit ConfigManager(const QString& configPath = {})` | 加载配置（默认 `%APPDATA%/MultiClipboard/config.json`，Windows） |
| `QVariant get(const QString& key, const QVariant& def = {}) const` | 点分键取值 |
| `void set(const QString& key, const QVariant& value)` | 点分键设值 |
| `void saveConfig()` | 保存到文件 |
| `QJsonObject getWindowConfig()` 等 | 各分区配置 |
| `void updateWindowPosition(int x, int y)` / `updateWindowSize(int w, int h)` | 更新窗口位置/大小 |
| `bool toggleAlwaysOnTop()` | 切换置顶并返回新状态 |
| `QJsonArray getPersistentItems()` / `addPersistentItem(content)` / `removePersistentItem(content)` | 持久化条目管理 |
| `void setPersistentItemNote(const QString& content, const QString& note)` | 设置持久化条目备注（无备注时删除 note 键） |

实现要点：
- 内部使用 `QJsonObject` 保存配置，支持嵌套合并（默认配置 + 用户配置）。
- 默认配置内置在代码中，用户配置缺失的项自动使用默认值。
- 界面透明度配置键为 `ui.opacity`（整数百分比 30~100，默认 100），由主窗口底部滑动条写入。
- 自动弹出阈值配置键为 `window.auto_popup_min_items`（默认 3）：本次解析出的非常驻条目数小于等于该值时，解析与列表更新照常进行，但不自动弹出窗口。
- 持久化条目以 `{ "content": "...", "note": "..." }` 对象存于配置 `persistent_items` 数组。

### 4.7 HotkeyManager（热键管理器）— `utils/hotkey_manager.h/.cpp`

| 方法 | 说明 |
| :--- | :--- |
| `void registerAllHotkeys()` | 注册所有配置快捷键 |
| `void reloadHotkeys()` | 重新加载 |
| `int registerHotkey(const QString& shortcutStr, std::function<void()> cb)` | 注册单个热键 |
| `void setCallbacks(std::function<void()> toggleAlwaysOnTop, ...)` | 设置回调 |
| `bool handleHotkey(int hotkeyId)` | 热键分发 |
| `void stop()` | 停止监听 |

`parseShortcut` 解析 `Ctrl+Shift+T` 为 (modifiers, vkCode)，内置 Windows 虚拟键码映射表。

### 4.8 Logger（日志管理器）— `utils/logger.h/.cpp`

单例（`Q_GLOBAL_STATIC`），功能：
- 文件轮转日志 `multiclipboard.log`（5MB × 5 份）
- 错误日志 `error.log`（仅 ERROR 及以上）
- 崩溃报告 `crash_YYYYMMDD_HHMMSS.log`
- 日志目录：可执行文件所在目录的 `logs` 子目录（便携式，日志跟随程序位置）
- 安装 `qInstallMessageHandler` 捕获 Qt 与全局消息

### 4.9 ThemeManager（主题管理器）— `ui/theme_manager.h/.cpp`

- 定义 `ThemeColors`（QHash<QString, QString> 键值对）颜色变量
- `const ThemeColors& lightTheme()` / `darkTheme()`
- `const ThemeColors& configLightTheme()` / `configDarkTheme()`
- 提供 `getTheme(name)` / `getConfigTheme(name)`

### 4.10 FontConfig（字体配置）— `ui/font_config.h/.cpp`

- `QStringList getChineseFontFamilies()`：按平台返回中文字体列表
- `QString getCssFontFamily()`：生成 QSS `font-family` 回退链
- `int getDefaultFontSize()`
- `void setupApplicationFont(QApplication* app)`：设置应用默认字体

### 4.11 主窗口 — `ui/main_window.h/.cpp`

继承 `QMainWindow`，主要功能：
- 工具栏（搜索框，条目 > 1 时显示）
- 滚动区域条目列表（`ItemWidget` 数组）
- 状态栏（已使用计数 / 搜索结果数）
- 系统托盘（置顶切换、显示、配置、主题切换、退出）
- 剪贴板监控信号连接：内容解析、持久化合并、列表更新**始终执行**；仅当满足自动弹出条件时才弹出窗口
- 持久化条目加载/合并/排序（persistent 优先、raw 其次、index 最后）
- 持久化勾选状态同步到内存数据源（勾选后剪贴板刷新仍常驻置顶）
- 原始条目右键"解析"：强制切分为多条并替换显示（`parseForced`）
- 条目右键"从列表删除"：删除内存条目并刷新显示；常驻条目删除时同步删除配置（`removePersistentItem`）
- 条目右键"删除已复制"：一次删除所有非常驻且已复制（`used`）的条目，常驻条目不受影响
- 窗口自适应高度（最多 8 行）
- 底部透明度控制条（`QSlider`，30%~100%：滑动实时调用 `setWindowOpacity` 预览，防抖保存 `ui.opacity`）
- 自动弹出窗口条件（全部满足）：自动弹出开关开启（`window.auto_popup`）＋ 距上次关闭窗口超过 10 秒 ＋ 本次解析出的**非常驻条目数** > `window.auto_popup_min_items`（默认 3，常驻条目不参与计数）
- 多行表格：检测到多行表格时进入翻页浏览模式——列表一次显示一行 Excel 行切分后的条目（不含整行聚合条目），底部翻页条切换行；翻页时清空搜索、重排行内序号、重建列表
- 列表底部翻页条（滚动条式，充满一行）：`◀ 滑块 第 X/Y ▶`——左右按钮 + 中间行切换滑块（拖动/点击切换行） + 数字标签；仅表格浏览模式显示；显示翻页条时**隐藏底部透明度条**（两者互斥占用底部区域），退出表格模式后恢复
- 表格内容优先自动弹出（不按非常驻条目数阈值计数），保证用户能立即翻页浏览；表格内容一致时不刷新（保持翻页位置）；复制普通内容/清空列表自动退出表格模式
- `closeEvent` 隐藏到托盘、保存窗口位置/大小
- 热键回调（toggleWindow / toggleAlwaysOnTop / clearAll / copyAll / pastePlain）
- 列表空白区域拖拽移动窗体（通过 `WindowDragFilter` 事件过滤器实现）

信号：`windowClosed`、`showRequested`。

### 4.12 条目组件 — `ui/item_widget.h/.cpp` + `ui/elide_label.h/.cpp`

`ItemWidget` 继承 `QFrame`：
- 布局：持久化复选框 + 序号标签 + 内容标签（ElideLabel）+ 状态标签
- 信号：`itemUsed(Item*)`、`itemUnused(Item*)`、`itemCopied(Item*)`、`itemPersistentChanged(Item*, bool)`、`itemNoteRequested(Item*)`、`itemForceParseRequested(Item*)`、`itemDeleteRequested(Item*)`、`itemDeleteCopiedRequested()`
- 鼠标事件：按下记录位置、移动超过 5px 触发拖拽、双击复制、右键菜单
- 样式：normal / used / raw / flash / search_dimmed 五态 × 明暗两主题
- 备注显示：常驻条目有备注时，内容标签显示为 `[备注] 内容`（备注醒目色标签位于内容前方）
- 右键菜单：标记使用 / 复制 / 添加备注（非常驻条目设置备注时自动转为常驻）/ 从列表删除 / 删除已复制（一次删除所有非常驻且已复制的条目）/ 纯文本粘贴；原始条目（raw）额外提供"解析"（强制切分多条）

`ElideLabel` 继承 `QLabel`：paintEvent 中用 `QFontMetrics::elidedText` 截断文本，多行只显示首行加 `...`。

### 4.13 配置窗口 — `ui/config_window.h/.cpp` + `ui/hotkey_edit_widget.h/.cpp`

`ConfigWindow` 继承 `QDialog`：
- 快捷键配置表格（5 行 2 列：功能描述 + HotkeyEditWidget）
- 窗口设置（自动弹出复选框、自动弹出最小条目数 `QSpinBox`（n 条以下不弹出，默认 3）、窗口置顶复选框、透明度滑动条 30%~100%）
- 按钮：重置默认 / 取消 / 确定
- 信号：`hotkeysChanged`、`alwaysOnTopPreview(bool)`、`opacityPreview(int)`
- 保存时写回 `shortcuts.*`、`window.auto_popup`、`window.auto_popup_min_items`、`window.always_on_top`、`ui.opacity`

主窗口联动（实时预览 + 关闭时同步）：
- 置顶复选框 / 透明度滑动条变化时实时发出 `alwaysOnTopPreview` / `opacityPreview`，主窗口即时应用（不落盘）；
- 点击确定后保存配置；配置窗口无论确定/取消关闭，主窗口统一按配置重新应用置顶与透明度（确定 = 新值，取消 = 恢复原状）。

`HotkeyEditWidget` 继承 `QLineEdit`：只读、点击后按下组合键捕获显示，信号 `hotkeySet(QString)`。

### 4.14 窗口拖拽移动过滤器 — `ui/window_drag_filter.h/.cpp`

继承 `QObject`，作为事件过滤器安装在列表滚动容器（`QScrollArea` 的内容部件）上，实现不依赖标题栏的窗体拖动：

| 成员 | 说明 |
| :--- | :--- |
| `explicit WindowDragFilter(QWidget* watched, QWidget* targetWindow, QObject* parent = nullptr)` | 构造函数，自动将过滤器安装到 `watched` 上，`targetWindow` 为要移动的窗口 |
| `bool eventFilter(QObject* watched, QEvent* event) override` | 处理左键按下/移动/释放，移动超过阈值后随鼠标位移移动窗口 |

实现要点：
- 左键按下时记录鼠标全局坐标与目标窗口位置，消费事件使 `watched` 成为鼠标抓取者（保证拖动期间持续收到移动事件）；
- 按住左键移动超过 `m_dragThreshold`（4px）后进入拖拽状态，目标位置 = 按下时窗口位置 + 鼠标位移，调用 `move()` 实时移动；
- 点击条目卡片时事件由 `ItemWidget` 消费，不会传播到过滤器，因此条目拖拽与窗体移动互不干扰；
- 其他事件一律放行，不影响列表滚轮、滚动条等既有交互。

## 5. 关键信号/槽流程

```
用户复制文本
   └─> QClipboard::dataChanged / 定时轮询
        └─> ClipboardManager::_checkClipboard（防抖、过滤文件/图片）
             └─> signal clipboardChanged(text)
                  └─> MainWindow::onClipboardChanged
                       ├─> ContentParser::parse(text) -> items
                       ├─> 合并持久化条目、查重
                       ├─> _applySearchFilter -> _displayItems（重建 ItemWidget）
                       ├─> 切分出多条 -> 托盘弹出窗口；仅 1 条 -> 后台更新
                       └─> 状态栏更新

拖拽条目
   └─> ItemWidget::mouseMoveEvent（超过 5px 阈值）
        └─> DragManager::startDrag
             ├─> 路径 A：QDrag::exec 成功 -> setUsed(true)
             └─> 失败回退路径 B：模拟点击 + 写剪贴板 + Ctrl+V

列表空白区域拖动移动窗体
   └─> scrollContent（列表滚动容器）左键按下
        └─> WindowDragFilter::eventFilter
             ├─> 记录按下位置，进入候选状态（4px 阈值）
             └─> 按住左键移动超过阈值 -> MainWindow::move(按下时位置 + 位移)

全局热键
   └─> Windows 消息 WM_HOTKEY
        └─> WindowsHotkeyFilter::nativeEventFilter
             └─> HotkeyManager::handleHotkey(id) -> 回调
```

## 6. 构建配置（xmake.lua）

```lua
set_project("multiclipboard")
set_version("1.0.0")
set_languages("c++20")
add_defines("NOMINMAX")                       -- Windows 下避免 min/max 宏冲突
add_includedirs("src", "src/core", "src/utils", "src/ui", "src/platform")

target("multiclipboard")
    set_kind("binary")
    add_rules("qt.widgetapp")                 -- Qt Widgets 应用（自动 moc/rcc）
    add_files("src/**.cpp", "src/**.h")
    add_files("resources/resources.qrc")
    if is_plat("windows") then
        add_syslinks("user32", "shell32")
        add_files("resources/app.rc")         -- 图标与版本信息
    end

target("test_content_parser")
    set_kind("binary")
    add_rules("qt.console")
    add_files("test/test_content_parser.cpp", {rules = "qt.moc"})
    add_files("src/core/content_parser.cpp")
    add_files("src/utils/config_manager.cpp")
    add_links("Qt6Test")
```

**Qt SDK 定位**：xmake 的 `qt.widgetapp` 规则通过查找 qmake 定位 Qt SDK，有以下两种方式：

- **方式一（推荐）**：将 Qt 工具目录加入 PATH，xmake 自动从 PATH 中发现 qmake：

  ```powershell
  $env:PATH = "<vcpkg>\installed\x64-windows\tools\Qt6\bin;" + $env:PATH
  xmake f -y -m release
  ```

- **方式二（显式指定）**：`xmake f -y -m release --qt="<vcpkg>\installed\x64-windows\tools\Qt6"`

注意：xmake 检测到 qmake 后通过 `qmake -query` 获取头文件、库、工具等全部路径，因此 vcpkg 的 Qt 布局（`include/Qt6`、`tools/Qt6/bin`）可正确适配。vcpkg 的 qtbase 默认仅含 release 库，**必须使用 `-m release` 构建**。

## 7. 测试计划

使用 Qt Test 框架编写 `test/test_content_parser.cpp`，覆盖内容解析器全部用例：
- 智能模式带制表符 / 不带制表符
- 单列模式、单行模式
- 去除空白、移除空行
- 空文本
- Excel 格式（按列优先）
- 切分限制（超过 max_split_count 或 max_item_length 时不拆分）

## 8. 风险与扩展点

- **Qt 获取**：依赖 vcpkg 安装 qtbase，首次构建耗时较长。
- **Linux 平台**：Windows 环境无法验证 Linux 代码路径，Linux 实现使用条件编译，需在 Linux 环境编译验证。
- **拖拽回退路径**：路径 B（模拟点击 + Ctrl+V）依赖 Windows API。
- **扩展点**：平台抽象层可继续扩展 macOS 实现；主题颜色集中管理便于新增主题。
