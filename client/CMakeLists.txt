set(TARGET_NAME ${PROJECT_NAME}_client)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC-only flags
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif (MSVC)
    # Both MSVC and MSVC-like command-line interface compilers (Clang-cl)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 /EHsc")
endif ()

find_package(PluginSDK REQUIRED gtasa)

add_subdirectory(third-party)
add_library(${TARGET_NAME} SHARED
        pch.h
        main.cpp
        hooks.h
        hooks.cpp
        npcs_module/npcs_module.h
        npcs_module/npcs_module.cpp
        npcs_module/utils.cpp
        npcs_module/utils.h
        npcs_module/npc.cpp
        npcs_module/npc.h
)


set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".asi")
set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23) # required for plugin-sdk

target_link_libraries(${TARGET_NAME} PRIVATE kthook raknet PluginSDK::gtasa)

target_precompile_headers(${TARGET_NAME} PRIVATE pch.h)
