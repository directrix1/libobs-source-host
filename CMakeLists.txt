cmake_minimum_required(VERSION 3.12)
project(libobs-source-host C)

find_package(PkgConfig)
pkg_check_modules(OBS REQUIRED obs)

add_library(libsobs-source-host PRIVATE src/libobs-source-host.c)
target_include_directories(libobs-source-host PUBLIC
    $(OBS_INCLUDE_DIRS)
)
target_compile_options(libobs-source-host PUBLIC
    $(OBS_CFLAGS)
)
target_link_libraries(libobs-source-host PUBLIC
    $(OBS_LINK_LIBRARIES)
)
target_link_options(libobs-source-host PUBLIC
    $(OBS_LDFLAGS)
)
