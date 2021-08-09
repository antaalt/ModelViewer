cmake_minimum_required (VERSION 3.10)

project(AkaViewer)

# Include Aka
add_subdirectory(lib/Aka)

add_executable(AkaViewer
	"src/main.cpp"

	"src/ModelViewer.cpp"

	"src/Model/Model.cpp"
	"src/Model/ModelLoader.cpp"

	"src/Editor/SceneEditor.cpp"
	"src/Editor/InfoEditor.cpp" 
	"src/Editor/AssetEditor.cpp"
)

target_link_libraries(AkaViewer Aka)

# ASSIMP
set(ZLIB_LIBRARIES zlibstatic)
set(ENABLE_BOOST_WORKAROUND ON)
set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS OFF)
set(BUILD_ASSIMP_VIEW OFF)
set(ASSIMP_BUILD_STATIC_LIB ON)
set(ASSIMP_BUILD_TESTS OFF)
add_subdirectory(lib/assimp)
target_include_directories(AkaViewer PUBLIC lib/assimp lib/IconCppHeaders)
target_link_libraries(AkaViewer assimp)

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
