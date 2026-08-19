# VST3 plugin module. The VST3 SDK's `smtg_add_vst3plugin` builds a .vst3
# bundle and (on Windows) registers it in the standard VST3 install folder.

set(netsdrstation_sources
    ../vst/common/pluginids.h
    ../vst/common/paramids.h
    ../vst/common/paramdefinitions.h
    ../vst/common/parameter_registry.h
    ../vst/common/parameter_registry.cpp
    ../vst/processor/plugin_processor.h
    ../vst/processor/plugin_processor.cpp
    ../vst/controller/plugin_controller.h
    ../vst/controller/plugin_controller.cpp
    ../dsp/sine_oscillator.h
    ../dsp/sine_oscillator.cpp
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
)

if(SMTG_MAC)
    smtg_target_set_bundle(${target}
        BUNDLE_IDENTIFIER "com.wogd.netsdrstation"
        COMPANY_NAME "WOGD"
    )
elseif(SMTG_WIN)
    target_sources(${target} PRIVATE ../vst/common/plugin.rc)
endif(SMTG_MAC)
