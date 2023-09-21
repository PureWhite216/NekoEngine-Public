target("Editor")
    set_kind("static")
    add_files("/*.cpp", "/Panels./*.cpp")
    add_deps("Function")