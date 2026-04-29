#pragma once

#include "quantum/spectroscopy/SpectralEngine.h"

namespace quantum::spectroscopy {

class HydrogenicSpectralEngine final : public SpectralEngine {
public:
    [[nodiscard]] std::string_view name() const override;
    [[nodiscard]] SpectralResult build(const SpectralRequest& request) const override;
};

} // namespace quantum::spectroscopy
