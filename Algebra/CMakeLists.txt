add_library(Algebra SHARED Algebra.cpp Algebra.h)

target_include_directories(Algebra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
