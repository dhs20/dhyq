#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string_view>

namespace quantum::model {

class AtomModel {
public:
    virtual ~AtomModel() = default;

    [[nodiscard]] virtual std::string_view id() const = 0;
    [[nodiscard]] virtual const quantum::meta::MethodStamp& methodStamp() const = 0;
    virtual void invalidate() = 0;
};

} // namespace quantum::model
