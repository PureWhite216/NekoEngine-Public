-- includes("RHI", "RenderManager", "OpenGL")

target("Graphics")
    set_kind("static")
    add_deps("Core")
    add_files("/Window/*.cpp", "/RHI/*.cpp", "/Vulkan/*.cpp", "/Renderable/*.cpp", "/Renderer/*.cpp")
