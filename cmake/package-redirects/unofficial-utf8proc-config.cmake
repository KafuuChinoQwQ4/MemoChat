include(CMakeFindDependencyMacro)

find_dependency(utf8proc CONFIG)

if(NOT TARGET utf8proc AND TARGET utf8proc::utf8proc)
    add_library(utf8proc ALIAS utf8proc::utf8proc)
endif()
