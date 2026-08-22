# C++ unit tests (Catch2 v2.13.6 single-header, BSL-1.0, vendored in
# third_party/catch2). Tests cover the DSP core, threading primitives and the
# parameter registry. Pure test-first (CCD blue/yellow), no audio hardware.

add_executable(netsdrstation_tests
    test_main.cpp
    dsp/sine_oscillator_tests.cpp
    dsp/parameter_smoother_tests.cpp
    dsp/jitter_buffer_tests.cpp
    dsp/ima_adpcm_tests.cpp
    dsp/rate_limiter_tests.cpp
    dsp/resampler_tests.cpp
threading/lock_free_spsc_tests.cpp
    threading/worker_thread_tests.cpp
    threading/audio_sample_queue_tests.cpp
    vst/parameter_registry_tests.cpp
    vst/bridge_protocol_tests.cpp
    vst/processor_state_tests.cpp
    vst/plugin_processor_tests.cpp
    vst/plugin_controller_tests.cpp
    vst/plugin_editor_tests.cpp
    network/kiwi_connection_tests.cpp
    network/kiwi_commands_tests.cpp
    network/kiwi_client_tests.cpp
    network/kiwi_bridge_tests.cpp
    # Implementation units under test (compiled directly into the test binary).
    ${CMAKE_SOURCE_DIR}/source/dsp/sine_oscillator.cpp
    ${CMAKE_SOURCE_DIR}/source/dsp/parameter_smoother.cpp
    ${CMAKE_SOURCE_DIR}/source/dsp/jitter_buffer.cpp
    ${CMAKE_SOURCE_DIR}/source/dsp/rate_limiter.cpp
    ${CMAKE_SOURCE_DIR}/source/dsp/ima_adpcm.cpp
    ${CMAKE_SOURCE_DIR}/source/dsp/resampler.cpp
    ${CMAKE_SOURCE_DIR}/source/threading/worker_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/vst/common/parameter_registry.cpp
    ${CMAKE_SOURCE_DIR}/source/vst/common/bridge_protocol.cpp
    ${CMAKE_SOURCE_DIR}/source/vst/common/processor_state.cpp
    ${CMAKE_SOURCE_DIR}/source/vst/processor/plugin_processor.cpp
    ${CMAKE_SOURCE_DIR}/source/vst/controller/plugin_controller.cpp
    ${CMAKE_SOURCE_DIR}/source/editor/plugin_editor.cpp
    ${CMAKE_SOURCE_DIR}/source/webview/webview_editor.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/common/memorystream.cpp
)

target_compile_features(netsdrstation_tests PRIVATE cxx_std_20)

# VST3 SDK path (same as root CMakeLists.txt)
set(VST3_SDK_ROOT C:/Users/marku/Documents/GitHub/thirdParty/vst3sdk)

target_include_directories(netsdrstation_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_SOURCE_DIR}/source/vst
        ${CMAKE_SOURCE_DIR}/source/entry
        ${CMAKE_SOURCE_DIR}/source/dsp
        ${CMAKE_SOURCE_DIR}/source/threading
        ${CMAKE_SOURCE_DIR}/source/editor
        ${CMAKE_SOURCE_DIR}/source/webview
        ${CMAKE_SOURCE_DIR}/source/network
        ${CMAKE_SOURCE_DIR}/third_party/catch2
        ${CMAKE_BINARY_DIR}/source/entry
        # VST3 SDK include paths (needed for plugin_editor.h and IEditController)
        ${VST3_SDK_ROOT}/public.sdk
        ${VST3_SDK_ROOT}/public.sdk/source
        ${VST3_SDK_ROOT}/public.sdk/source/vst
        ${VST3_SDK_ROOT}/public.sdk/source/common
        ${VST3_SDK_ROOT}/public.sdk/source/vst/gui
)

include(CTest)
add_test(NAME netsdrstation_tests COMMAND netsdrstation_tests)

target_link_libraries(netsdrstation_tests PRIVATE sdk_common sdk webview::core_static netsdr_network samplerate)
