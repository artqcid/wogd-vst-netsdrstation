#pragma once
// Shared factory of the parameter definitions for the NetSDRStation plugin.
//
// Both the processor and the edit controller build their parameter registry
// from this single source of truth, so the parameter model (IDs, ranges,
// defaults) can never drift between the two sides (CCD Red: DRY).

#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"

#include <vector>

namespace netsdr {

inline std::vector<ParameterDefinition> createParameterDefinitions() {
    return {
        ParameterDefinition{kParamFreq,   "Frequency", "Hz", 20.0,  20000.0, 440.0, false, 0},
        ParameterDefinition{kParamVolume, "Volume",    "",   0.0,   1.0,     1.0,   false, 0},
        ParameterDefinition{kParamMute,   "Mute",      "",   0.0,   1.0,     0.0,   false, 1},
    };
}

} // namespace netsdr
