#include "quantum/app/Application.h"

#include <filesystem>

int main() {
    quantum::app::Application application(std::filesystem::current_path());
    if (!application.initialize()) {
        return 1;
    }
    return application.run();
}
