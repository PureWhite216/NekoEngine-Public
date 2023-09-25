add_deps("ImGui")

add_requires("glfw", "glad", "glm", "vulkansdk", "stb", "cereal", "entt", "volk", "vulkan-memory-allocator-hpp", "sol2", "spdlog", "meshoptimizer")
add_packages("glfw", "glad", "glm", "vulkansdk", "stb", "cereal", "entt", "volk", "vulkan-memory-allocator-hpp", "sol2", "spdlog", "meshoptimizer")

add_syslinks("dbghelp", "dwmapi")

add_includedirs("/Runtime/Function")
add_includedirs("/Runtime/Function/Graphics")
add_includedirs("/Runtime/Function/Engine")
add_includedirs("/Runtime/Function/FrameWork")
add_includedirs("/Runtime/Function/Physics")
add_includedirs("/Runtime/Core")
add_includedirs("/Runtime/Core/Math")
add_includedirs("/Runtime/Platform")
add_includedirs("/Editor")

includes("Editor", "Runtime")

