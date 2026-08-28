# Network module: WebSocket client for the KiwiSDR connection (Milestone M2).
# Pure C++ (no VST3 dependency) so it can be unit/integration-tested in
# isolation. Links IXWebSocket (BSD-3-Clause, fetched in the root CMakeLists).

add_library(netsdr_network STATIC
    kiwi_connection.h
    kiwi_connection.cpp
    kiwi_commands.h
    kiwi_commands.cpp
    kiwi_client.h
    kiwi_client.cpp
    kiwi_bridge.h
    kiwi_bridge.cpp
)

target_compile_features(netsdr_network PUBLIC cxx_std_20)

target_include_directories(netsdr_network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/source
)

target_link_libraries(netsdr_network PUBLIC ixwebsocket nlohmann_json::nlohmann_json)
