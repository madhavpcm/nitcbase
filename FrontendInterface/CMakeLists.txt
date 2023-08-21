add_library( FrontendInterface
	FrontendInterface.cpp
	FrontendInterface.h
)
add_library( RegexHandler
	RegexHandler.h
)

set_target_properties(RegexHandler
    PROPERTIES LINKER_LANGUAGE CXX
)
target_link_libraries(FrontendInterface
	RegexHandler
	Frontend
	readline
)
target_include_directories(FrontendInterface
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
