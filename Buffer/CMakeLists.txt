add_library(StaticBuffer SHARED StaticBuffer.h StaticBuffer.cpp )
add_library(BlockBuffer SHARED BlockBuffer.h BlockBuffer.cpp )

set(cmake_c_link_executable ${cmake_cxx_link_executable})
target_include_directories(StaticBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(BlockBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
