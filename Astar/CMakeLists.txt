cmake_minimum_required (VERSION 3.8)

add_executable(AstarExecutable "Astar.cpp")
target_include_directories(AstarExecutable PUBLIC 
    "tgengine/TGEngine"
	"tgengine/dependencies/Include"
	"tgengine/dependencies/Third-Party/Include"
)

target_link_libraries(AstarExecutable PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/tgengine/dependencies/Third-Party/Bin/SDL2.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/tgengine/dependencies/Lib/vulkan-1.lib"
	TGEngine
)
set_target_properties(TGEngine PROPERTIES COMPILE_DEFINITIONS "DEBUG;UNICODE")
set_target_properties(AstarExecutable PROPERTIES COMPILE_DEFINITIONS "DEBUG")