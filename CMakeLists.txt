cmake_minimum_required (VERSION 3.10)

project(AkaViewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Aka
add_subdirectory(lib/Aka)

add_executable(AkaViewer
	"src/main.cpp"

	"src/ModelViewer.cpp"

	"src/Model/Model.cpp"
	"src/Model/ModelLoader.cpp"

	"src/Model/GLTF/GLTFLoader.cpp"
)

target_link_libraries(AkaViewer Aka)

add_custom_command(
	TARGET AkaViewer POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${PROJECT_SOURCE_DIR}/lib/Aka/lib/glew/bin/glew32.dll"
	$<TARGET_FILE_DIR:AkaViewer>
)

add_custom_command(
	TARGET AkaViewer POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${PROJECT_SOURCE_DIR}/asset
	$<TARGET_FILE_DIR:AkaViewer>/asset
)
