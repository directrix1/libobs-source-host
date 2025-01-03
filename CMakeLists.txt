cmake_minimum_required(VERSION 3.12)
project(libobs-source-host CXX)

find_package(PkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(OBS REQUIRED libobs)
pkg_check_modules(OBS_FE REQUIRED obs-frontend-api)

string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_CFLAGS "${OBS_CFLAGS}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_FE_CFLAGS "${OBS_FE_CFLAGS}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_CFLAGS_OTHER "${OBS_CFLAGS_OTHER}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_FE_CFLAGS_OTHER "${OBS_FE_CFLAGS_OTHER}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_LINK_LIBRARIES "${OBS_LINK_LIBRARIES}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_FE_LINK_LIBRARIES "${OBS_FE_LINK_LIBRARIES}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_LDFLAGS "${OBS_LDFLAGS}")
string(REPLACE "-std=gnu17" "-std=gnu++17" OBS_FE_LDFLAGS "${OBS_FE_LDFLAGS}")

add_library(obs-source-host SHARED src/obs-source-host.cpp)
target_include_directories(obs-source-host PUBLIC
    /usr/include/obs
    ${GLIB_INCLUDE_DIRS}
    ${OBS_INCLUDE_DIRS}
    ${OBS_FE_INCLUDE_DIRS}
)
target_compile_options(obs-source-host PUBLIC
    ${GLIB_CFLAGS}
    ${OBS_CFLAGS}
    ${OBS_FE_CFLAGS}
    ${OBS_CFLAGS_OTHER}
    ${OBS_FE_CFLAGS_OTHER}
)
target_link_libraries(obs-source-host PUBLIC
    ${GLIB_LINK_LIBRARIES}
    ${OBS_LINK_LIBRARIES}
    ${OBS_FE_LINK_LIBRARIES}
)
target_link_options(obs-source-host PUBLIC
    ${GLIB_LDFLAGS}
    ${OBS_LDFLAGS}
    ${OBS_FE_LDFLAGS}
)

add_executable(obs-source-host-test src/obs-source-host.cpp src/obs-source-host-test.cpp)
target_include_directories(obs-source-host-test PUBLIC
    /usr/include/obs
    ${GLIB_INCLUDE_DIRS}
    ${OBS_INCLUDE_DIRS}
    ${OBS_FE_INCLUDE_DIRS}
)
target_compile_options(obs-source-host-test PUBLIC
    ${GLIB_CFLAGS}
    ${OBS_CFLAGS}
    ${OBS_FE_CFLAGS}
    ${OBS_CFLAGS_OTHER}
    ${OBS_FE_CFLAGS_OTHER}
)
target_link_libraries(obs-source-host-test PUBLIC
    ${GLIB_LINK_LIBRARIES}
    ${OBS_LINK_LIBRARIES}
    ${OBS_FE_LINK_LIBRARIES}
)
target_link_options(obs-source-host-test PUBLIC
    ${GLIB_LDFLAGS}
    ${OBS_LDFLAGS}
    ${OBS_FE_LDFLAGS}
)
