# 更新日志

本工程的变更遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 风格，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [1.0.0] - 2026-08-17

### 新增

- 首个可发布版本：基于 C++20 + Qt6 的完整实现
- 核心模块：内容解析器（智能 / 单列 / 单行 / Excel 模式）、剪贴板监控、拖拽管理器
- 平台抽象层：`PlatformInterface` + `PlatformFactory`，Windows 完整实现、Linux 适配代码
- UI 模块：主窗口、条目卡片、配置窗口、热键捕获控件、主题 / 字体配置
- 工具模块：配置管理（JSON）、全局热键、日志系统（轮转 + 错误日志 + 崩溃报告）
- 主入口：单实例检测（Windows 互斥体 / Linux 锁文件）
- 单元测试：内容解析器 Qt Test 用例（8 个）
- 构建：xmake 构建脚本，支持 Windows（vcpkg + MSVC）与 Linux
- 工程化：MIT 许可证、README、贡献指南、GitHub Actions CI
