cmake_minimum_required(VERSION 3.9)
project(pears_npcs VERSION 0.1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NPC_SYNC_PACKET_ID 218)
set(NPC_CONTROL_RPC_ID 184)

if (MSVC)
    # Enable exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif ()

add_compile_definitions(NPC_SYNC_PACKET_ID=${NPC_SYNC_PACKET_ID})
add_compile_definitions(NPC_CONTROL_RPC_ID=${NPC_CONTROL_RPC_ID})

if (BUILD_CLIENT)
    add_subdirectory(client)
endif ()
add_subdirectory(server)
