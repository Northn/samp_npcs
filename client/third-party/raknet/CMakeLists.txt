add_library(raknet)

target_include_directories(raknet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(raknet PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/BitStream.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BitStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RakClient.h
)
