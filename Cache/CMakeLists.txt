add_library(Cache AttrCacheTable.cpp AttrCacheTable.h OpenRelTable.cpp
                  OpenRelTable.h RelCacheTable.cpp RelCacheTable.h)

target_include_directories(Cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
