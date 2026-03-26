set_project("quantum-atom-simulation")
set_version("0.2.0")
set_languages("cxx20")
add_rules("mode.debug", "mode.release")

if is_plat("windows") then
    add_cxxflags("/utf-8", {force = true})
end

add_requires("glfw 3.4")
add_requires("glew 2.2.0")
add_requires("glm 1.0.1")
add_requires("nlohmann_json v3.11.3")
add_requires("imgui v1.92.6-docking", {configs = {glfw = true, opengl3 = true}})

target("quantum_atom_simulation")
    set_kind("binary")
    add_files("app/**.cpp", "core/**.cpp", "physics/**.cpp", "render/**.cpp", "ui/**.cpp")
    add_includedirs("include")
    add_packages("glfw", "glew", "glm", "nlohmann_json", "imgui")
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
    add_files("tests/**.cpp", "core/**.cpp", "physics/**.cpp")
    add_includedirs("include")
    add_packages("glm", "nlohmann_json")
