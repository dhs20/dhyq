#include "quantum/app/Application.h"
#include "quantum/physics/ElementDatabase.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path detectProjectRoot(const std::filesystem::path& seed) {
    std::error_code error;
    auto candidate = std::filesystem::weakly_canonical(seed, error);
    if (error) {
        candidate = seed;
    }

    if (std::filesystem::is_regular_file(candidate, error)) {
        candidate = candidate.parent_path();
    }

    for (auto current = candidate; !current.empty(); current = current.parent_path()) {
        const auto assetMarker = current / "assets" / "data" / "elements.json";
        if (std::filesystem::exists(assetMarker)) {
            return current;
        }
        const auto parent = current.parent_path();
        if (parent == current) {
            break;
        }
    }

    return std::filesystem::current_path();
}

bool hasArgument(int argc, char** argv, const std::string& flag) {
    for (int index = 1; index < argc; ++index) {
        if (argv[index] != nullptr && flag == argv[index]) {
            return true;
        }
    }
    return false;
}

int runHealthCheck(const std::filesystem::path& projectRoot) {
    const std::vector<std::filesystem::path> requiredFiles = {
        projectRoot / "assets" / "data" / "elements.json",
        projectRoot / "assets" / "data" / "reference_catalog.json",
        projectRoot / "assets" / "data" / "isotopes.json",
        projectRoot / "assets" / "data" / "nist_reference_lines.csv",
        projectRoot / "assets" / "scenarios" / "demo_scenes.json",
        projectRoot / "assets" / "scenarios" / "demo_script.json",
    };

    bool ok = true;
    for (const auto& file : requiredFiles) {
        if (!std::filesystem::exists(file)) {
            std::cerr << "[health] missing required asset: " << file.string() << '\n';
            ok = false;
        }
    }

    quantum::physics::ElementDatabase database;
    if (!database.loadFromJson(projectRoot / "assets" / "data" / "elements.json")) {
        std::cerr << "[health] failed to load elements.json\n";
        ok = false;
    }
    if (!database.loadIsotopes(projectRoot / "assets" / "data" / "isotopes.json")) {
        std::cerr << "[health] failed to load isotopes.json\n";
        ok = false;
    }
    if (!database.loadReferenceTransitions(projectRoot / "assets" / "data" / "reference_catalog.json") &&
        !database.loadReferenceTransitions(projectRoot / "assets" / "data" / "nist_reference_lines.csv")) {
        std::cerr << "[health] failed to load reference spectra\n";
        ok = false;
    }

    try {
        const auto& hydrogen = database.elementByAtomicNumber(1);
        if (hydrogen.symbol != "H" || database.referenceTransitions(1).empty()) {
            std::cerr << "[health] loaded data failed hydrogen sanity checks\n";
            ok = false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "[health] metadata sanity failed: " << exception.what() << '\n';
        ok = false;
    }

    if (ok) {
        std::cout << "[health] ok: assets and data catalogs loaded from " << projectRoot.string() << '\n';
        return 0;
    }
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path currentRoot = detectProjectRoot(std::filesystem::current_path());
    std::filesystem::path executableRoot = currentRoot;
    if (argc > 0 && argv != nullptr && argv[0] != nullptr) {
        executableRoot = detectProjectRoot(std::filesystem::absolute(argv[0]));
    }

    const std::filesystem::path projectRoot =
        std::filesystem::exists(executableRoot / "assets" / "data" / "elements.json") ? executableRoot : currentRoot;

    if (hasArgument(argc, argv, "--health-check")) {
        return runHealthCheck(projectRoot);
    }

    quantum::app::Application application(projectRoot);
    if (!application.initialize()) {
        return 1;
    }
    return application.run();
}
