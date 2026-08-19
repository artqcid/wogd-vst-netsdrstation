#pragma once
// Parameter registry: a small, self-contained model of the plugin parameters.
//
// It holds the numeric ranges, defaults and current values for every parameter
// and is shared (read-only definition) between the processor and the edit
// controller. The registry itself is pure C++ with no VST3 SDK dependency, so
// it can be unit-tested in isolation (CCD: SRP, DIP).

#include <cstdint>
#include <string>
#include <vector>

namespace netsdr {

// Definition of a single parameter: stable id, title, units, range, default.
struct ParameterDefinition {
    uint32_t id = 0;
    std::string title;
    std::string units;
    double min = 0.0;
    double max = 1.0;
    double defaultValue = 0.0;
    bool isBypass = false;   // whether this is the VST3 bypass parameter
    int32_t stepCount = 0;   // 0 = continuous; 1 = binary toggle, etc. (VST3 convention)
};

// In-memory, mutable value store for the parameters. Not thread-safe; the
// audio thread reads a snapshot produced elsewhere (see processor).
class ParameterRegistry {
public:
    explicit ParameterRegistry(std::vector<ParameterDefinition> definitions);

    // Access the immutable definition set.
    const ParameterDefinition* definition(uint32_t id) const;
    const std::vector<ParameterDefinition>& definitions() const { return definitions_; }

    // Read the current normalized value (0..1) of a parameter.
    double value(uint32_t id) const;

    // Set the current normalized value of a parameter, clamped to [0,1].
    void setValue(uint32_t id, double normalized);

    // Convert a normalized value (0..1) to the parameter's plain value.
    double toPlain(uint32_t id, double normalized) const;

    // Convert a plain value back to normalized (clamped).
    double toNormalized(uint32_t id, double plain) const;

private:
    std::vector<ParameterDefinition> definitions_;
    std::vector<double> values_;
};

} // namespace netsdr
