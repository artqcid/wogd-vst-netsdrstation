#include "parameter_registry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace netsdr {

ParameterRegistry::ParameterRegistry(std::vector<ParameterDefinition> definitions)
    : definitions_(std::move(definitions)) {
    values_.reserve(definitions_.size());
    index_.reserve(definitions_.size());
    for (std::size_t i = 0; i < definitions_.size(); ++i) {
        const auto& def = definitions_[i];
        // Defaults are stored normalized in [0,1].
        const double norm = (def.max > def.min) ? (def.defaultValue - def.min) / (def.max - def.min) : 0.0;
        values_.push_back(std::clamp(norm, 0.0, 1.0));
        // O(1) id -> index map (FIX-35). Duplicate ids keep the first entry.
        if (index_.find(def.id) == index_.end()) {
            index_.emplace(def.id, i);
        }
    }
}

std::size_t ParameterRegistry::indexOf(uint32_t id) const {
    const auto it = index_.find(id);
    return it == index_.end() ? definitions_.size() : it->second;
}

const ParameterDefinition* ParameterRegistry::definition(uint32_t id) const {
    const std::size_t i = indexOf(id);
    return i < definitions_.size() ? &definitions_[i] : nullptr;
}

double ParameterRegistry::value(uint32_t id) const {
    const std::size_t i = indexOf(id);
    return i < definitions_.size() ? values_[i] : 0.0;
}

void ParameterRegistry::setValue(uint32_t id, double normalized) {
    const std::size_t i = indexOf(id);
    if (i < definitions_.size()) {
        values_[i] = std::clamp(normalized, 0.0, 1.0);
    }
}

double ParameterRegistry::toPlain(uint32_t id, double normalized) const {
    const ParameterDefinition* def = definition(id);
    if (def == nullptr) {
        return 0.0;
    }
    return def->min + normalized * (def->max - def->min);
}

double ParameterRegistry::toNormalized(uint32_t id, double plain) const {
    const ParameterDefinition* def = definition(id);
    if (def == nullptr || def->max <= def->min) {
        return 0.0;
    }
    return std::clamp((plain - def->min) / (def->max - def->min), 0.0, 1.0);
}

} // namespace netsdr