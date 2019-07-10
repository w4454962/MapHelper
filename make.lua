local lm = require "luamake"

lm:source_set "detours" {
    rootdir = "mapHelper/3rd/detours/src",
    permissive = true,
    sources = {
        "*.cpp",
        "!uimports.cpp"
    }
}

lm:shared_library "MapHelper" {
    rootdir = "mapHelper/mapHelper",
    deps = "detours",
    defines = {
        "_CRT_SECURE_NO_WARNINGS",
        "EMBED_YDWE",
    },
    sources = {
        "*.cpp",
    },
    links = {
        "user32"
    }
}
