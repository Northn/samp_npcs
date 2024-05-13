set(TARGET_NAME ${PROJECT_NAME}_server)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC-only flags
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif (MSVC)
    # Both MSVC and MSVC-like command-line interface compilers (Clang-cl)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 /EHsc")
else ()
    # Linux
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif ()

add_subdirectory(third-party)
add_library(${TARGET_NAME} SHARED
        plugin.def
        NpcComponent.cpp
        NpcComponent.h
        Npc.cpp
        Npc.h
        natives.cpp
        NpcTask.hpp
        NpcNetwork.hpp
)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17)

add_compile_definitions(
        COMPONENT_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
        COMPONENT_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
        COMPONENT_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
        COMPONENT_VERSION_TWEAK=${CMAKE_PROJECT_VERSION_TWEAK}
        COMPONENT_NAME="${PROJECT_NAME}"
        HAVE_STDINT_H=1
)

target_include_directories(${TARGET_NAME} PRIVATE third-party third-party/amx/source third-party/amx/source/linux)
target_link_libraries(${TARGET_NAME} PRIVATE OMP-SDK OMP-Network)
