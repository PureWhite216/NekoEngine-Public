-- add_deps("Core")
-- includes("Graphics",  "FrameWork", "Asset", "GUI", "Physics", "Script")
target("Function")
    add_deps("Core", "File", "OS", "ImGui", "OpenFBX")
    set_kind("static")
    add_files("/**.cpp")