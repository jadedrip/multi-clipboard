# 多元剪贴板 (MultiClipboard) — C++20 / Qt6

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Language](https://img.shields.io/badge/Language-C%2B%2B20-orange.svg)
![Framework](https://img.shields.io/badge/Framework-Qt6-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)

一款高效的剪贴板内容切分与拖拽工具：将多行数据自动切分为独立条目，拖拽或双击即可逐个粘贴到表单输入框。

## 主要用途

多元剪贴板解决"同一批数据需要逐个填入多个输入框"的重复劳动问题。典型场景：

- **Excel 表格数据填入网页表单**：在 Excel 中选中一列或多列数据复制，条目自动切分后，逐个拖拽填入网页/系统的表单输入框，无需反复切换窗口复制粘贴
- **批量单据录入**：把表格中的编号、姓名、金额等数据依次录入业务系统、后台管理系统
- **信息登记与问卷填写**：将名单、联系方式等重复性数据逐条粘贴到登记页面
- **重复性文本输入**：将代码片段、模板语句等逐条粘贴到编辑器的不同输入框

只需"复制一次、逐个拖拽"，即可将一整块数据分发到多个目标位置。

## 功能特性

- **智能内容切分**：自动识别剪贴板内容格式（换行 / 制表符 / Excel 表格），切分为独立条目
- **条目拖拽复制**：拖拽条目到外部应用输入框即完成粘贴（原生 QDrag 优先，失败自动回退剪贴板 + 模拟按键）
- **实时搜索过滤**：输入关键词实时过滤条目，匹配条目高亮、不匹配淡化
- **明暗主题切换**：内置 light / dark 两套配色，系统托盘一键切换
- **全局快捷键**：通过 Windows `RegisterHotKey` 注册，无需打开窗口即可操作核心功能
- **系统托盘常驻**：关闭窗口隐藏到托盘，随时唤起
- **持久化条目**：勾选复选框固定常用条目，不受新内容覆盖影响
- **纯文本粘贴**：一键去除格式，纯文本粘贴到目标位置
- **单实例运行**：重复启动时自动唤醒已有实例
- **完整日志**：文件轮转日志 + 错误日志 + 崩溃报告

## 快捷键

| 快捷键 | 功能 |
| :--- | :--- |
| `Ctrl+Shift+M` | 切换窗口显示 / 隐藏 |
| `Ctrl+Shift+T` | 切换窗口置顶 |
| `Ctrl+Shift+X` | 清空所有条目 |
| `Ctrl+Shift+C` | 复制所有条目 |
| `Ctrl+Shift+V` | 纯文本粘贴 |

以上快捷键可在配置窗口（托盘菜单 → 配置）中自定义。

## 环境要求

| 项目 | 要求 |
| :--- | :--- |
| 操作系统 | Windows 10/11（主支持）；Linux（X11）适配代码保留 |
| 编译器 | MSVC 2022+（Windows）/ GCC 11+（Linux） |
| 构建系统 | [xmake](https://xmake.io) v2.8+ |
| Qt | Qt 6（qtbase），Windows 通过 vcpkg 安装，Linux 通过系统包管理器 |
| C++ 标准 | C++20 |

## 构建与运行

### Windows

1. **安装依赖**

   - 安装 Visual Studio（含 MSVC 与 Windows SDK）
   - 安装 xmake：`winget install xmake` 或从官网下载
   - 安装 vcpkg 并安装 qtbase（仅 release）：

     ```powershell
     git clone https://github.com/microsoft/vcpkg
     .\vcpkg\bootstrap-vcpkg.bat
     .\vcpkg\vcpkg install qtbase:x64-windows
     ```

2. **配置并构建**

   将 vcpkg 的 Qt 工具目录加入 PATH 后直接配置（推荐方式）：

   ```powershell
   $env:PATH = "$env:VCPKG_ROOT\installed\x64-windows\tools\Qt6\bin;" + $env:PATH
   xmake f -y -m release
   xmake
   ```

   > 若不想修改 PATH，也可显式指定 Qt SDK：
   > `xmake f -y -m release --qt="<vcpkg>\installed\x64-windows\tools\Qt6"`
   >
   > 注意：vcpkg 的 qtbase 默认仅提供 release 库，请务必使用 `-m release`。

3. **运行**

   ```powershell
   xmake r multiclipboard
   # 或直接运行构建产物
   .\build\windows\x64\release\multiclipboard.exe
   ```

### Linux（X11）

```bash
sudo apt install qt6-base-dev xmake
xmake f -y -m release
xmake
xmake r multiclipboard
```

## 运行测试

```bash
xmake r test_content_parser
```

测试使用 Qt Test 框架，覆盖内容解析器的全部用例（智能 / 单列 / 单行 / Excel 格式、空白处理、空文本等）。退出码为 0 即全部通过；如需保存详细报告：

```bash
xmake r test_content_parser -o test_result.txt,txt
```

## 打包

产物统一输出到 `dist/` 目录，Windows 与 Linux 打包方式不同：

### Windows

| 命令 | 作用 |
| :--- | :--- |
| `xmake` | 仅编译（不打包） |
| `xmake pack` | 先编译，再打包 release 产物到 `dist/` |
| `xmake f --enable_pack=true` | 配置"构建后自动打包"，之后 `xmake` 编译完自动打包到 `dist/` |

打包目录内容：

- `multiclipboard.exe`：主程序
- `Qt6*.dll`：实际依赖的 Qt 运行库（按 PE 导入表精确收集）
- 第三方依赖 DLL（freetype / harfbuzz / libpng / ICU / pcre2 等）
- `plugins/`：Qt 插件目录（platforms / styles / imageformats 等）
- VC++ 运行库（vcruntime140 / msvcp140 等，检测到 VS 时自动拷贝）
- `qt.conf`：使 Qt 以打包目录为前缀查找插件，脱离 vcpkg 环境可运行

> 打包逻辑位于 `scripts/pack.lua`（xmake Lua 模块，由 `xmake pack` 任务调用）：采用 PE 导入表解析收集依赖，
> 仅打包实际用到的 DLL，完成后自动做完整性校验。
> 可将 `dist/` 目录整体拷贝到目标机器直接运行（Windows 10+ 无需额外安装运行库）。

### Linux

使用 linuxdeploy 生成自包含的 AppImage（无需目标机安装 Qt6）：

```bash
bash scripts/pack_linux.sh [版本号]   # 版本号缺省时自动读取 xmake.lua
```

产物：`dist/multiclipboard-<版本>-linux-x86_64.AppImage`。

> 打包逻辑位于 `scripts/pack_linux.sh`：编译 release → linuxdeploy + plugin-qt 收集 Qt 依赖
> → 补充 desktop/图标 → 生成 AppImage。工具首次运行自动下载到 `~/ldt`（可设 `LINUXDEPLOY_HOME` 覆盖）。

## 使用说明

1. **复制内容**：在 Excel、文本编辑器等应用中复制多行数据
2. **自动切分**：工具自动捕获剪贴板内容并切分为独立条目（多行内容自动弹出窗口）
3. **拖拽粘贴**：从工具窗口拖拽条目到目标输入框完成粘贴；双击条目复制到剪贴板

- **复选框**：勾选条目左侧复选框，将其固定为持久化条目
- **右键菜单**：标记已使用 / 未使用、复制、纯文本粘贴
- **搜索框**：条目数大于 1 时自动出现，输入关键词实时过滤
- **托盘菜单**：置顶切换、显示窗口、配置、切换主题、退出

## 配置说明

配置文件 `config.json` 默认生成于用户配置目录（Windows：`%APPDATA%\MultiClipboard\config.json`；Linux：`~/.config/MultiClipboard/config.json`），支持以下配置项：

| 配置项 | 说明 | 默认值 |
| :--- | :--- | :--- |
| `window.always_on_top` | 窗口是否置顶 | `true` |
| `window.auto_popup` | 剪贴板变化时是否自动弹出窗口 | `true` |
| `parsing.split_mode` | 切分模式（smart / single_column / single_row） | `smart` |
| `parsing.strip_whitespace` | 是否去除条目首尾空白 | `true` |
| `ui.theme` | 主题（dark / light） | `dark` |
| `shortcuts.*` | 各功能快捷键 | 见快捷键表 |

日志文件位于可执行文件所在目录的 `logs` 子目录（便携式，日志跟随程序位置）。

## 目录结构

```
├── xmake.lua                  # 构建脚本（xmake）
├── scripts/                   # 构建辅助脚本
│   ├── pack.lua               # Windows 打包模块（xmake pack：收集 release 产物到 dist）
│   └── pack_linux.sh          # Linux 打包脚本（linuxdeploy 生成 AppImage）
├── docs/                      # 设计文档
│   ├── design.md              # 设计文档
│   ├── architecture.md        # 架构说明
│   └── usage.md               # 使用说明
├── src/
│   ├── main.cpp               # 主程序入口（单实例检测、日志初始化）
│   ├── core/                  # 核心业务模块（不依赖 UI）
│   │   ├── item.h             # 条目数据结构
│   │   ├── content_parser.*   # 内容解析器
│   │   ├── clipboard_manager.*# 剪贴板监控
│   │   └── drag_manager.*     # 拖拽 / 复制粘贴
│   ├── platform/              # 平台抽象层（接口 + Windows/Linux 实现 + 工厂）
│   ├── ui/                    # UI 模块（仅界面与交互，通过信号与核心层通信）
│   │   ├── main_window.*      # 主窗口
│   │   ├── item_widget.*      # 条目卡片
│   │   ├── elide_label.*      # 自动截断文本标签
│   │   ├── config_window.*    # 配置窗口
│   │   ├── hotkey_edit_widget.* # 热键捕获控件
│   │   ├── theme_manager.*    # 主题配色
│   │   └── font_config.*      # 字体配置
│   └── utils/                 # 工具模块
│       ├── config_manager.*   # 配置管理（JSON）
│       ├── hotkey_manager.*   # 全局热键管理
│       └── logger.*           # 日志系统
├── resources/
│   ├── resources.qrc          # Qt 资源清单
│   ├── app.rc                 # Windows 资源（图标 / 版本信息）
│   └── icons/                 # 应用图标（多尺寸）
└── test/
    └── test_content_parser.cpp # 内容解析器单元测试
```

## 技术实现

- **模块解耦**：核心业务（`core/`）、平台适配（`platform/`）、UI（`ui/`）三层分离，UI 通过 Qt 信号 / 槽与核心层交互
- **平台抽象**：`PlatformInterface` 纯虚基类 + `PlatformFactory` 单例工厂，Windows 用 `RegisterHotKey` / `SendInput` / `WindowFromPoint`，Linux 用 X11/XTest
- **拖拽双路径**：优先 `QDrag`，失败回退"写剪贴板 + 模拟点击 + Ctrl+V"
- **剪贴板监控**：`QTimer` 轮询 + 防抖（300ms）+ 文件 / 图片内容过滤
- **内容解析**：智能 / 单列 / 单行 / Excel 四种模式，支持去空白、去空行、去重、长度限制

## 开发指南

- 运行测试：`xmake r test_content_parser`
- 构建调试版：`xmake f -y -m debug`（需 vcpkg 安装 qtbase 的 debug 库 `qtbase:x64-windows-debug`）
- 代码规范：中文注释（Javadoc 风格）、每类独立文件、UI 与业务分离、复用重复逻辑到函数 / 类

详细设计见 [docs/design.md](docs/design.md)、[docs/architecture.md](docs/architecture.md)。

## 许可证

[MIT License](LICENSE)

## 贡献

欢迎提交 Issue 与 Pull Request，详见 [CONTRIBUTING.md](CONTRIBUTING.md)。
