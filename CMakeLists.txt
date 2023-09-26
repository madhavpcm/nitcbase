cmake_minimum_required(VERSION 3.10)

set(cmake_c_link_executable ${cmake_cxx_link_executable})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 23)

project(nitcbase LANGUAGES CXX)

# Subdirectories of each component
add_subdirectory(Algebra)
add_subdirectory(BlockAccess)
add_subdirectory(BPlusTree)
add_subdirectory(Buffer)
add_subdirectory(Cache)
add_subdirectory(Disk_Class)
add_subdirectory(Frontend)
add_subdirectory(FrontendInterface)
add_subdirectory(Schema)
add_subdirectory(define)

# Executable
add_executable(nitcbase main.cpp)

target_link_libraries(
  nitcbase
  Algebra
  BlockAccess
  BPlusTree
  StaticBuffer
  BlockBuffer 
  Cache
  Defines
  Disk_Class
  Frontend
  FrontendInterface
  Schema)
