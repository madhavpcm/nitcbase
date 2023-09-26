add_library( Defines SHARED 
	constants.h
	constants.cpp
	id.h
)

target_include_directories(Defines
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
