// VST3 module entry point / factory for the NetSDRStation plugin.
//
// Registers the audio effect (processor) and its edit controller with the
// VST3 factory so a DAW can instantiate and control the plugin.

#include "vst/common/pluginids.h"
#include "vst/processor/plugin_processor.h"
#include "vst/controller/plugin_controller.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "NetSDRStation"

using namespace Steinberg::Vst;
using namespace netsdr;

//------------------------------------------------------------------------
// VST Plug-in Entry
//------------------------------------------------------------------------
// Windows: the plugin factory is exported by the .def file / moduleinfo.
//------------------------------------------------------------------------

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail)

    // Audio effect (processor) component.
    DEF_CLASS2(INLINE_UID_FROM_FUID(kProcessorUID),
               PClassInfo::kManyInstances, // cardinality
               kVstAudioEffectClass,       // component category (do not change)
               stringPluginName,           // plug-in name
               Vst::kDistributable,        // component + controller can be distributed
               "Instrument",               // subcategory (synth)
               FULL_VERSION_STR,           // plug-in version
               kVstVersionString,          // VST 3 SDK version (do not change)
               PluginProcessor::createInstance) // instantiation function

    // Edit controller component.
    DEF_CLASS2(INLINE_UID_FROM_FUID(kControllerUID),
               PClassInfo::kManyInstances, // cardinality
               kVstComponentControllerClass, // controller category (do not change)
               stringPluginName " Controller", // controller name
               0,                         // not used here
               "",                        // not used here
               FULL_VERSION_STR,          // plug-in version
               kVstVersionString,         // VST 3 SDK version (do not change)
               PluginController::createInstance) // instantiation function

END_FACTORY
