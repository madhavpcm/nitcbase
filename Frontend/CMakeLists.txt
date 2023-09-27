add_library(Frontend SHARED Frontend.cpp Frontend.h)

target_include_directories(Frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
