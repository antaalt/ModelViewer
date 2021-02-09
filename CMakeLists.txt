cmake_minimum_required (VERSION 3.8)

project(AkaViewer)

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
