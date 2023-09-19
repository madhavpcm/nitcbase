add_library(BlockAccess SHARED BlockAccess.cpp BlockAccess.h)

target_include_directories(BlockAccess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
