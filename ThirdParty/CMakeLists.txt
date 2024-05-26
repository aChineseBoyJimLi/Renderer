add_library(d3d INTERFACE)
target_include_directories(d3d INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/d3d)

add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/Include)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

add_subdirectory(DirectXMesh)
add_subdirectory(DirectXTex)

add_library(assimp INTERFACE)
target_include_directories(assimp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include)
target_link_directories(assimp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/assimp/lib)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assimp/lib/assimp-vc143-mt.dll DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assimp/lib/assimp-vc143-mt.dll DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})


set(imgui_srcs
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp)

add_library(imgui STATIC ${imgui_srcs})
target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")