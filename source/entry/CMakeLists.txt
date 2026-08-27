# VST3 plugin module. The VST3 SDK's `smtg_add_vst3plugin` builds a .vst3
# bundle and (on Windows) registers it in the standard VST3 install folder.

set(netsdrstation_sources
    ../vst/common/pluginids.h
    ../vst/common/paramids.h
    ../vst/common/paramdefinitions.h
    ../vst/common/parameter_registry.h
    ../vst/common/parameter_registry.cpp
    ../vst/common/bridge_protocol.h
    ../vst/common/bridge_protocol.cpp
    ../vst/common/processor_state.h
    ../vst/common/processor_state.cpp
    ../vst/processor/plugin_processor.h
    ../vst/processor/plugin_processor.cpp
    ../vst/controller/plugin_controller.h
    ../vst/controller/plugin_controller.cpp
    ../dsp/ima_adpcm.h
    ../dsp/ima_adpcm.cpp
    ../dsp/resampler.h
    ../dsp/resampler.cpp
    ../dsp/jitter_buffer.h
    ../dsp/jitter_buffer.cpp
    ../dsp/rate_limiter.h
    ../dsp/rate_limiter.cpp
    ../threading/audio_sample_queue.h
    ../threading/lock_free_spsc.h
    ../threading/worker_thread.h
    ../threading/worker_thread.cpp
    ../editor/plugin_editor.h
    ../editor/plugin_editor.cpp
    ../webview/webview_editor.h
    ../webview/webview_editor.cpp
    factory.cpp
    version.h
)

set(target netsdrstation)

smtg_add_vst3plugin(${target} PACKAGE_NAME "NetSDRStation" SOURCES_LIST ${netsdrstation_sources})
smtg_target_configure_version_file(${target})
message(STATUS "Created target: ${target}")

target_compile_features(${target} PRIVATE cxx_std_20)

target_include_directories(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}               # source/entry (version.h)
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}              # generated projectversion.h
        ${CMAKE_SOURCE_DIR}                       # third_party/moodycamel/...
        ${CMAKE_CURRENT_SOURCE_DIR}/../vst
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp
        ${CMAKE_CURRENT_SOURCE_DIR}/../threading
        ${CMAKE_CURRENT_SOURCE_DIR}/../editor
        ${CMAKE_CURRENT_SOURCE_DIR}/../webview
)

target_link_libraries(${target}
    PRIVATE
        sdk
        webview::core_static
        netsdr_network
        samplerate
)

# ---------------------------------------------------------------------------
# Bundle WebView2 Fixed Version Runtime into VST3 package
# This ensures the plugin works without any external WebView2 installation.
# WebView2Loader.dll and FixedRuntime/ are placed directly in the plugin directory
# (Contents/x86_64-win/) so WebView2Loader.dll can find the Fixed Runtime
# without any environment variables (DLL search order: same dir as loader).
# ---------------------------------------------------------------------------
if(WIN32 AND NS_ENABLE_UI)
    get_target_property(_BUNDLE_DIR netsdrstation SMTG_PLUGIN_PACKAGE_PATH)
    set(PLUGIN_DIR "${_BUNDLE_DIR}/Contents/x86_64-win")

    # Use local WebView2 SDK from workspace (set once in root CMakeLists.txt)
    set(WEBVIEW2_FIXED_RUNTIME_SRC "${WEBVIEW2_SDK_ROOT}/FixedRuntime/Microsoft.WebView2.FixedVersionRuntime.151.0.4129.93.x64")

    if(NOT EXISTS "${WEBVIEW2_FIXED_RUNTIME_SRC}")
        message(FATAL_ERROR "WebView2 FixedRuntime not found at: ${WEBVIEW2_FIXED_RUNTIME_SRC}. Please ensure the Fixed Runtime is extracted there.")
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WEBVIEW2_SDK_ROOT}/build/native/x64/WebView2Loader.dll"
            "${PLUGIN_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WEBVIEW2_FIXED_RUNTIME_SRC}"
            "${PLUGIN_DIR}/FixedRuntime"
        COMMENT "Bundling WebView2 Loader DLL and Fixed Runtime into VST3 package"
        VERBATIM
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui/dist"
            "${PLUGIN_DIR}/ui"
        COMMENT "Bundling Vue UI (ui/dist) into VST3 package"
        VERBATIM
    )

    add_dependencies(${target} netsdrstation_ui)
endif()

if(SMTG_MAC)
    smtg_target_set_bundle(${target}
        BUNDLE_IDENTIFIER "com.wogd.netsdrstation"
        COMPANY_NAME "WOGD"
    )
elseif(SMTG_WIN)
    target_sources(${target} PRIVATE ../vst/common/plugin.rc)
endif(SMTG_MAC)
