#include "parameter_registry.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netsdr {

ParameterRegistry::ParameterRegistry(std::vector<ParameterDefinition> definitions)
    : definitions_(std::move(definitions)) {
    values_.reserve(definitions_.size());
    for (const auto& def : definitions_) {
        // Defaults are stored normalized in [0,1].
        const double norm = (def.max > def.min) ? (def.defaultValue - def.min) / (def.max - def.min) : 0.0;
        values_.push_back(std::clamp(norm, 0.0, 1.0));
    }
}

const ParameterDefinition* ParameterRegistry::definition(uint32_t id) const {
    for (const auto& def : definitions_) {
        if (def.id == id) {
            return &def;
        }
    }
    return nullptr;
}

double ParameterRegistry::value(uint32_t id) const {
    for (std::size_t i = 0; i < definitions_.size(); ++i) {
        if (definitions_[i].id == id) {
            return values_[i];
        }
    }
    return 0.0;
}

void ParameterRegistry::setValue(uint32_t id, double normalized) {
    for (std::size_t i = 0; i < definitions_.size(); ++i) {
        if (definitions_[i].id == id) {
            values_[i] = std::clamp(normalized, 0.0, 1.0);
            return;
        }
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
