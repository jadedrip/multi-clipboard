-- =============================================================
-- 多元剪贴板 C++20 版构建脚本（xmake）
-- 依赖：Qt6（qtbase，通过 vcpkg 安装）
-- 配置方式（Windows，PowerShell）：
--   方式一（推荐）：将 Qt 工具目录加入 PATH 后直接配置
--     $env:PATH = "<vcpkg_root>\installed\x64-windows\tools\Qt6\bin;" + $env:PATH
--     xmake f -y -m release
--   方式二（显式指定）：xmake f -y -m release --qt="<vcpkg_root>\installed\x64-windows\tools\Qt6"
-- 注意：vcpkg 的 qtbase 默认仅包含 release 库，请使用 -m release 构建
-- =============================================================

set_project("multiclipboard")
set_version("1.1.0")
set_languages("c++17")

-- Windows 下避免 min/max 宏与 Qt 冲突
add_defines("NOMINMAX")

-- MSVC 编译时声明源文件为 UTF-8，避免中文字符串字面量按本地代码页（GBK）解码导致乱码
-- （Windows 默认工具链为 MSVC；如需 MinGW 可自行替换为 -finput-charset=utf-8）
if is_plat("windows") then
    add_cxxflags("/utf-8")
end

-- 源码中统一使用 "xxx.h" 形式引用（同目录）或 "模块/xxx.h" 形式引用（跨目录），
-- 需把 src 根目录及各子模块目录加入包含路径
add_includedirs("src", "src/core", "src/utils", "src/ui", "src/platform")

-- Windows 平台链接系统库
if is_plat("windows") then
    add_syslinks("user32", "shell32")
end

-- =============================================================
-- 选项：构建完成后是否自动打包（默认关闭）
-- 需要自动打包时：xmake f --enable_pack=true
-- 单独打包：xmake pack（打包前自动编译）
-- =============================================================
option("enable_pack")
    set_default(false)
    set_showmenu(true)
    set_description("构建完成后自动打包 release 产物到 dist/windows")

-- =============================================================
-- 打包任务：xmake pack（先编译，再打包 release 产物到 dist/windows）
-- =============================================================
task("pack")
    set_category("package")
    on_run(function ()
        -- 打包前先编译
        import("core.base.task")
        task.run("build")
        local pack = import("scripts.pack")
        pack.run(os.scriptdir())
    end)
    set_menu {
        usage   = "xmake pack",
        description = "编译并打包 release 构建产物（exe / Qt 依赖 DLL / 插件）到 dist/windows",
        options = {}
    }

-- =============================================================
-- 主程序
-- =============================================================
target("multiclipboard")
    set_kind("binary")
    add_rules("qt.widgetapp")
    add_files("src/**.cpp", "src/**.h")
    add_files("resources/resources.qrc")
    if is_plat("windows") then
        -- 窗口图标嵌入（.rc 资源文件）
        add_files("resources/app.rc")
    end
    -- 构建后按选项自动打包（默认关闭，可通过 xmake f --enable_pack=true 开启）
    -- 注：import 只能在脚本域（回调函数内）使用，不能在 xmake.lua 顶层描述域调用
    after_build(function (target)
        local config = import("core.project.config")
        if config.get("enable_pack") then
            local pack = import("scripts.pack")
            pack.run(os.scriptdir())
        end
    end)

-- =============================================================
-- 单元测试：内容解析器
-- =============================================================
target("test_content_parser")
    set_kind("binary")
    add_rules("qt.console")
    add_defines("QT_TESTLIB_LIB")
    -- 使用 qt.moc 规则处理 .moc 文件（测试类定义在 .cpp 中，需 #include "xxx.moc"）
    add_files("test/test_content_parser.cpp", {rules = "qt.moc"})
    add_files("src/core/content_parser.cpp")
    add_files("src/utils/config_manager.cpp")
    add_links("Qt6Test")
