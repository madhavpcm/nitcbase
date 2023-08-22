add_library(BPlusTree BPlusTree.cpp BPlusTree.h)

target_include_directories(BPlusTree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
