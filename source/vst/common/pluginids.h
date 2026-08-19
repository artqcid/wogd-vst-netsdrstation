#pragma once
// VST3 component and class IDs for the NetSDRStation plugin.
//
// These 16-byte GUIDs identify the processor (component), the controller and
// the plugin factory. They are fixed for the lifetime of the plugin so that
// DAW sessions keep resolving to the correct classes.

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

namespace netsdr {

// Processor component ID.
static const Steinberg::FUID kProcessorUID(0x3F1A9B2C, 0x5D4E4A86, 0x9C7B2F1A, 0x8D6E5C4B);

// Edit controller ID.
static const Steinberg::FUID kControllerUID(0x7A4B8C2D, 0x1E3F5A6B, 0x0D9C8B7A, 0x2F3E4D5C);

// Factory class IDs (processor and controller) used by the module entry point.
static const Steinberg::FUID kPluginProcessorCID(kProcessorUID);
static const Steinberg::FUID kPluginControllerCID(kControllerUID);

} // namespace netsdr
