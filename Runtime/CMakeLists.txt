set(project Runtime)

file(GLOB_RECURSE CORE_SOURCES Core/*.cpp Core/*.h)
file(GLOB RHI_SOURCES RHI/*.cpp RHI/*.h)
file(GLOB RHI_D3D12_SOURCES RHI/D3D12/*.cpp RHI/D3D12/*.h)
if(vulkan_lib)
    file(GLOB RHI_VULKAN_SOURCES RHI/Vulkan/*.cpp RHI/Vulkan/*.h)
else()
    set(RHI_VULKAN_SOURCES "")
endif()
file(GLOB_RECURSE APP_SOURCES WinApp/*.cpp WinApp/*.h)
file(GLOB SOURCES *.cpp *.h)

add_executable(${project} WIN32 
    ${CORE_SOURCES}
    ${RHI_SOURCES}
    ${RHI_D3D12_SOURCES}
    ${RHI_VULKAN_SOURCES}
    ${APP_SOURCES}
    ${SOURCES}
)

add_dependencies(${project} Shaders DirectXMesh DirectXTex imgui)
set_target_properties(${project} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")

target_link_libraries(${project} PUBLIC
    imgui
    d3d
    glm
    stb
    assimp
    DirectXMesh
    DirectXTex
    d3dcompiler.lib
    d3d12.lib
    dxgi.lib
    dxguid.lib
    assimp-vc143-mt.lib)

if(vulkan_lib)
    target_link_libraries(${project} PUBLIC vulkan ${vulkan_lib})
endif()