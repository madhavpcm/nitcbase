add_library(Buffer BlockBuffer.cpp BlockBuffer.h StaticBuffer.cpp
                   StaticBuffer.h)

target_include_directories(Buffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
