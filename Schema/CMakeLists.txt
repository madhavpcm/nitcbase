add_library(Schema SHARED Schema.cpp Schema.h)

target_include_directories(Schema PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
