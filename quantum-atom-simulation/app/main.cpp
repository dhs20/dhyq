#include "quantum/app/Application.h"

#include <filesystem>
#include <string>

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

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path currentRoot = detectProjectRoot(std::filesystem::current_path());
    std::filesystem::path executableRoot = currentRoot;
    if (argc > 0 && argv != nullptr && argv[0] != nullptr) {
        executableRoot = detectProjectRoot(std::filesystem::absolute(argv[0]));
    }

    const std::filesystem::path projectRoot =
        std::filesystem::exists(executableRoot / "assets" / "data" / "elements.json") ? executableRoot : currentRoot;

    quantum::app::Application application(projectRoot);
    if (!application.initialize()) {
        return 1;
    }
    return application.run();
}
