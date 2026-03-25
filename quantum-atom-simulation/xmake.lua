-- Basic build rules
add_rules("mode.debug", "mode.release")

-- Disable auto-generated C++ standard flags (fix duplicated std flags)
set_policy("build.langs.auto_flags", false)

-- Project name
set_project("QuantumAtomSimulation")

-- ==================== Platform Specific Config ====================
if is_plat("windows") then
    -- Use MSVC only on Windows
    set_toolchains("msvc")
    -- Fix Chinese garbled code: force MSVC to read UTF-8 files
    add_cxxflags("/utf-8", {force = true})
    -- Set C++11 standard (MSVC specific format)
    add_cxxflags("/std:c++11", {force = true})
else
    -- For Linux/macOS (GCC/Clang)
    set_languages("cxx11")
    add_cxxflags("-std=c++11", {force = true})
end

-- ==================== Dependency Management ====================
add_requires("freeglut", "opengl")

-- ==================== Target Configuration ====================
target("QuantumAtomSimulation")
    set_kind("binary")
    add_files("src/*.cpp")
    add_includedirs("include")
    add_packages("freeglut", "opengl")

    -- Link Windows system libraries
    if is_plat("windows") then
        add_links("user32", "gdi32", "opengl32", "glu32", "freeglut")
    end
