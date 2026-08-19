# C++ unit tests (Catch2 v2.13.6 single-header, BSL-1.0, vendored in
# third_party/catch2). Tests cover the DSP core, threading primitives and the
# parameter registry. Pure test-first (CCD blue/yellow), no audio hardware.

add_executable(netsdrstation_tests
    test_main.cpp
    dsp/sine_oscillator_tests.cpp
    threading/lock_free_spsc_tests.cpp
    threading/worker_thread_tests.cpp
    vst/parameter_registry_tests.cpp
    # Implementation units under test (compiled directly into the test binary).
    ${CMAKE_SOURCE_DIR}/source/dsp/sine_oscillator.cpp
    ${CMAKE_SOURCE_DIR}/source/threading/worker_thread.cpp
    ${CMAKE_SOURCE_DIR}/source/vst/common/parameter_registry.cpp
)

target_compile_features(netsdrstation_tests PRIVATE cxx_std_20)

target_include_directories(netsdrstation_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_SOURCE_DIR}/source/vst
        ${CMAKE_SOURCE_DIR}/source/dsp
        ${CMAKE_SOURCE_DIR}/source/threading
        ${CMAKE_SOURCE_DIR}/third_party/catch2
)

include(CTest)
add_test(NAME netsdrstation_tests COMMAND netsdrstation_tests)
