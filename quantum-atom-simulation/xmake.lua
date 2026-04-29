set_project("quantum-atom-simulation")
set_version("0.3.1")
set_languages("cxx20")
add_rules("mode.debug", "mode.release")

option("local_pkg_root")
    set_default("")
    set_showmenu(true)
    set_description("Optional xmake local package install root for offline/reproducible builds")
option_end()

if is_plat("windows") then
    add_cxxflags("/utf-8", {force = true})
end

add_requires("glfw 3.4")
add_requires("glew 2.2.0")
add_requires("glm 1.0.1")
add_requires("nlohmann_json v3.11.3")
add_requires("imgui v1.92.6-docking", {configs = {glfw = true, opengl3 = true}})

local function add_local_cached_package(name, version, links)
    local local_pkg_root = get_config("local_pkg_root")
    if not local_pkg_root or local_pkg_root == "" then
        return
    end
    local pkgroot = path.join(local_pkg_root, name:sub(1, 1), name, version)
    for _, dir in ipairs(os.dirs(path.join(pkgroot, "*"))) do
        local includedir = path.join(dir, "include")
        local libdir = path.join(dir, "lib")
        if os.isdir(includedir) then
            add_includedirs(includedir)
        end
        if os.isdir(libdir) then
            add_linkdirs(libdir)
        end
    end
    if links then
        add_links(table.unpack(links))
    end
end

target("quantum_atom_simulation")
    set_kind("binary")
    add_files("app/**.cpp", "basis/**.cpp", "core/**.cpp", "data/**.cpp", "dynamics/**.cpp", "physics/**.cpp", "render/**.cpp", "solvers/**.cpp", "spectroscopy/**.cpp", "ui/**.cpp", "validation/**.cpp")
    add_includedirs("include")
    add_packages("glfw", "glew", "glm", "nlohmann_json", "imgui")
    add_local_cached_package("glfw", "3.4", {"glfw3"})
    add_local_cached_package("glew", "2.2.0", {"glew32s"})
    add_local_cached_package("glm", "1.0.1")
    add_local_cached_package("nlohmann_json", "v3.11.3")
    add_local_cached_package("imgui", "v1.92.6-docking", {"imgui"})
    add_defines("GLEW_STATIC")
    if is_plat("windows") then
        add_syslinks("opengl32", "gdi32", "shell32", "user32")
    elseif is_plat("linux") then
        add_syslinks("dl", "pthread")
    end
    set_rundir("$(projectdir)")

target("quantum_atom_tests")
    set_kind("binary")
    set_group("tests")
    add_files("tests/**.cpp", "basis/**.cpp", "core/**.cpp", "data/**.cpp", "dynamics/**.cpp", "physics/**.cpp", "solvers/**.cpp", "spectroscopy/**.cpp", "validation/**.cpp")
    add_includedirs("include")
    add_packages("glm", "nlohmann_json")
    add_local_cached_package("glm", "1.0.1")
    add_local_cached_package("nlohmann_json", "v3.11.3")
