-- =============================================================
-- 多元剪贴板 - 打包模块（xmake Lua 实现）
--
-- 功能：将 release 构建产物（主程序 exe、实际依赖的 Qt DLL、
--       Qt 插件目录）收集到打包目录 dist。
--
-- 用法：在 xmake.lua 脚本域（after_build 回调 / pack 任务）中：
--       local pack = import("scripts.pack")
--       pack.run(root)
--
-- 说明：
--   - 仅打包 release 构建产物，输出目录固定为工程根目录下的 dist
--   - 依赖收集基于 PE 导入表解析（BFS 遍历），只拷贝真正用到的 DLL
--   - Qt 插件目录整体拷贝（platforms / styles / imageformats 等）
--   - 生成 qt.conf，使 Qt 以打包目录为前缀查找插件，脱离 vcpkg 环境可运行
--   - 若检测到 VS 的 VC++ 运行库目录，一并拷贝运行库（vcruntime140 等）
--   - 打包完成后自动做完整性校验，报告缺失的依赖
-- =============================================================

-- 读取小端 16 位整数（offset 为 1 起始索引，对应文件偏移 offset-1）
local function read_le16(data, offset)
    local b0, b1 = string.byte(data, offset, offset + 1)
    return (b0 or 0) + (b1 or 0) * 256
end

-- 读取小端 32 位整数（offset 为 1 起始索引，对应文件偏移 offset-1）
local function read_le32(data, offset)
    local b0, b1, b2, b3 = string.byte(data, offset, offset + 3)
    return (b0 or 0) + (b1 or 0) * 256 + (b2 or 0) * 65536 + (b3 or 0) * 16777216
end

-- 读取以 '\0' 结尾的字符串（offset 为 1 起始索引）
local function read_cstring(data, offset)
    local chars = {}
    while true do
        local c = string.byte(data, offset, offset)
        if not c or c == 0 then
            break
        end
        chars[#chars + 1] = string.char(c)
        offset = offset + 1
    end
    return table.concat(chars)
end

-- 解析 PE 文件导入表，返回依赖的 DLL 名称列表
local function get_imports(file_path)
    local imports = {}
    local f = io.open(file_path, "rb")
    if not f then
        return imports
    end
    local data = f:read("*a")
    f:close()
    local len = #data
    if len < 0x40 then
        return imports
    end

    -- DOS 头 -> e_lfanew -> PE 签名
    local pe_offset = read_le32(data, 0x3D)                          -- e_lfanew @0x3C
    if pe_offset < 0x40 or pe_offset > len - 4 then return imports end
    if read_le32(data, pe_offset + 1) ~= 0x00004550 then return imports end   -- "PE\0\0"

    -- COFF 头：节数量、可选头大小
    local num_sections = read_le16(data, pe_offset + 7)              -- @COFF+2
    local size_optional = read_le16(data, pe_offset + 21)            -- @COFF+16

    -- 可选头 Magic：0x10B = PE32，0x20B = PE32+
    local magic = read_le16(data, pe_offset + 25)
    if magic ~= 0x10B and magic ~= 0x20B then return imports end

    -- 数据目录起始偏移（Import Directory 是第 2 个条目，index 1）
    local opt_start = pe_offset + 24
    local dd_offset = opt_start + (magic == 0x10B and 96 or 112)
    local import_rva = read_le32(data, dd_offset + 9)                -- @dd+8

    -- 读取节表，用于 RVA -> 文件偏移转换
    local section_start = opt_start + size_optional
    local sections = {}
    for i = 0, num_sections - 1 do
        local base = section_start + i * 40
        table.insert(sections, {
            vsize    = read_le32(data, base + 9),                    -- @+8  VirtualSize
            va       = read_le32(data, base + 13),                   -- @+12 VirtualAddress
            raw_size = read_le32(data, base + 17),                   -- @+16 SizeOfRawData
            raw      = read_le32(data, base + 21),                   -- @+20 PointerToRawData
        })
    end

    -- RVA 转文件偏移（找不到返回 -1）
    local function rva_to_offset(rva)
        for _, s in ipairs(sections) do
            local span = math.max(s.vsize, s.raw_size)
            if rva >= s.va and rva < s.va + span then
                return s.raw + (rva - s.va)
            end
        end
        return -1
    end

    -- 遍历导入描述符（每条 20 字节，Name RVA 为 0 表示结束）
    local import_off = rva_to_offset(import_rva)
    if import_off >= 0 then
        while true do
            local oft = read_le32(data, import_off + 1)              -- @+0 OriginalFirstThunk
            local name_rva = read_le32(data, import_off + 13)        -- @+12 Name（指向 DLL 名）
            if oft == 0 and name_rva == 0 then
                break
            end
            if name_rva ~= 0 then
                local name_off = rva_to_offset(name_rva)
                if name_off >= 0 then
                    local name = read_cstring(data, name_off + 1)
                    if name ~= "" then
                        imports[#imports + 1] = name
                    end
                end
            end
            import_off = import_off + 20
        end
    end
    return imports
end

-- 打包入口（公共接口）：仅打包 release 构建产物，root 为工程根目录
function run(root)
    root = root or os.curdir()

    -- 路径定义（固定 release 构建产物）
    local exe_path = path.join(root, "build/windows/x64/release/multiclipboard.exe")
    local vcpkg_root = os.getenv("VCPKG_ROOT")
    local vcpkg_bin = path.join(vcpkg_root, "installed/x64-windows/bin")
    -- 注意：vcpkg 的 qtbase 插件目录位于 installed/x64-windows/Qt6/plugins
    local plugins_dir = path.join(vcpkg_root, "installed/x64-windows/Qt6/plugins")
    -- 打包目录：工程根目录下 dist/windows（与 dist/linux 分开）
    local pack_dir = path.join(root, "dist/windows")

    -- 前置校验
    if not os.isfile(exe_path) then
        print("[pack] 错误：未找到构建产物 " .. exe_path .. "（请先执行 xmake 编译）")
        return
    end
    if not vcpkg_root or not os.isdir(vcpkg_bin) then
        print("[pack] 错误：未找到 vcpkg Qt 运行库目录（请确认环境变量 VCPKG_ROOT）")
        return
    end

    -- 清空并重建打包目录
    if os.isdir(pack_dir) then
        os.rm(pack_dir)
    end
    os.mkdir(pack_dir)
    print("[pack] 打包目录：" .. pack_dir)

    -- 拷贝主程序
    os.cp(exe_path, path.join(pack_dir, "multiclipboard.exe"))
    print("[pack] 已拷贝：multiclipboard.exe")

    -- 生成 qt.conf：以打包目录为 Qt 前缀，保证插件可被发现
    io.writefile(path.join(pack_dir, "qt.conf"), "[Paths]\nPrefix = .\n")
    print("[pack] 已生成：qt.conf")

    -- 收集依赖 DLL（BFS 遍历：exe -> 依赖 DLL -> 依赖 DLL 的依赖）
    local copied = {}
    local queue = { exe_path }
    local count = 0
    while #queue > 0 do
        local file = table.remove(queue, 1)
        for _, dep in ipairs(get_imports(file)) do
            if not copied[dep] then
                -- 只处理 vcpkg 运行库目录中存在的 DLL（系统 DLL 自动跳过）
                local src = path.join(vcpkg_bin, dep)
                if os.isfile(src) then
                    os.cp(src, path.join(pack_dir, dep))
                    copied[dep] = true
                    count = count + 1
                    queue[#queue + 1] = src
                end
            end
        end
    end
    print("[pack] 共收集依赖 DLL " .. count .. " 个")

    -- 拷贝 Qt 插件目录（platforms / styles / imageformats 等）
    if os.isdir(plugins_dir) then
        os.cp(plugins_dir, pack_dir)
        print("[pack] 已拷贝 Qt 插件目录")
    else
        print("[pack] 警告：未找到 Qt 插件目录 " .. plugins_dir)
    end

    -- 移除调试符号文件（.pdb），发行版不携带
    local pdbs = os.files(path.join(pack_dir, "**/*.pdb"))
    for _, pdb in ipairs(pdbs) do
        os.rm(pdb)
    end
    if #pdbs > 0 then
        print("[pack] 已移除调试符号文件 " .. #pdbs .. " 个")
    end

    -- 查找 VC++ 运行库目录：优先环境变量 VCToolsRedistDir，其次通过 vswhere 定位 VS
    local crt_root = nil
    local vctools = os.getenv("VCToolsRedistDir")
    if vctools and os.isdir(vctools) then
        local crt_dirs = os.dirs(path.join(vctools, "x64", "Microsoft.VC*.CRT"))
        if #crt_dirs > 0 then crt_root = crt_dirs[#crt_dirs] end
    end
    if not crt_root then
        local pf86 = os.getenv("ProgramFiles(x86)") or "C:/Program Files (x86)"
        local vswhere = path.join(pf86, "Microsoft Visual Studio/Installer/vswhere.exe")
        if os.isfile(vswhere) then
            local vsdir = os.iorun("\"" .. vswhere .. "\" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath")
            vsdir = vsdir:gsub("%s+$", "")
            if vsdir ~= "" then
                local crt_dirs = os.dirs(path.join(vsdir, "VC/Redist/MSVC/*/x64/Microsoft.VC*.CRT"))
                if #crt_dirs > 0 then crt_root = crt_dirs[#crt_dirs] end
            end
        end
    end
    if crt_root then
        for _, f in ipairs({"vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll", "concrt140.dll"}) do
            local src = path.join(crt_root, f)
            if os.isfile(src) then
                os.cp(src, path.join(pack_dir, f))
                print("[pack] 已拷贝 VC 运行库：" .. f)
            end
        end
    else
        print("[pack] 警告：未找到 VS VC++ 运行库目录，目标机器需安装 VC++ 2015-2022 运行库")
    end

    -- 完整性校验：检查打包后 exe 的依赖是否全部覆盖
    local missing = {}
    for _, dep in ipairs(get_imports(path.join(pack_dir, "multiclipboard.exe"))) do
        -- UCRT / 系统 API set（Windows 10+ 内置），无需分发
        if dep:find("^api%-ms%-") or dep:find("^ext%-ms%-") then
            -- 跳过
        elseif not os.isfile(path.join(pack_dir, dep)) then
            -- 未打包到目录时，若系统目录已提供则放行（视为系统 DLL）
            local sys_dll = path.join(os.getenv("WINDIR") or "C:/Windows", "System32", dep)
            if not os.isfile(sys_dll) then
                missing[#missing + 1] = dep
            end
        end
    end
    if #missing > 0 then
        print("[pack] 警告：以下依赖未覆盖（可能缺失）：" .. table.concat(missing, ", "))
    else
        print("[pack] 完整性校验通过：主程序依赖已全部覆盖")
    end

    print("[pack] 打包完成：" .. pack_dir)
end
