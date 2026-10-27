cmake_minimum_required(VERSION 3.21)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(sdl3_image_webp_opengl_test CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
if(WIN32)
    set(GLAD_DIR         "C:/libs/glad-0.1.36/opengl-3.3")
    set(SDL3_DIR         "C:/libs/sdl3-3.4.18-mingw/lib/cmake/SDL3")
    set(SDL3_image_DIR   "C:/libs/sdl3_image-3.4.8-webp-mingw/lib/cmake/SDL3_image")
    set(webp_ROOT        "C:/libs/webp-1.6.0-mingw")
    set(webp_INCLUDE_DIR "C:/libs/webp-1.6.0-mingw/include")
    set(webp_LIBRARY     "C:/libs/webp-1.6.0-mingw/lib/libwebp.a")
else()
    set(GLAD_DIR         "$ENV{HOME}/libs/glad-0.1.36/opengl-3.3")
    set(SDL3_DIR         "$ENV{HOME}/libs/sdl3-3.4.18-ubuntu/lib/cmake/SDL3")
    set(SDL3_image_DIR   "$ENV{HOME}/libs/sdl3_image-3.4.8-webp-ubuntu/lib/cmake/SDL3_image")
    set(webp_ROOT        "$ENV{HOME}/libs/webp-1.6.0-ubuntu")
    set(webp_INCLUDE_DIR "$ENV{HOME}/libs/webp-1.6.0-ubuntu/include")
    set(webp_LIBRARY     "$ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebp.a")
endif()

# -----------------------------------------------------------------------------
# Locate Official SDL3 and SDL3_image Packages
# -----------------------------------------------------------------------------
find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)

# -----------------------------------------------------------------------------
# Executable Target
# -----------------------------------------------------------------------------
add_executable(app
    src/main.cpp
    ${GLAD_DIR}/src/glad.c
)

# -----------------------------------------------------------------------------
# Include Directories
# -----------------------------------------------------------------------------
target_include_directories(app PRIVATE
    ${GLAD_DIR}/include
)

# -----------------------------------------------------------------------------
# Copy Assets to Build Directory
# -----------------------------------------------------------------------------
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:app>/assets
)

# -----------------------------------------------------------------------------
# Static C++ Runtime (Applied to BOTH Windows & Ubuntu)
# -----------------------------------------------------------------------------
target_link_options(app PRIVATE -static-libgcc -static-libstdc++)

# -----------------------------------------------------------------------------
# Optimization & Symbol Stripping (Release Builds Only)
# -----------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    target_compile_options(app PRIVATE -Os)
    target_link_options(app PRIVATE -s)
endif()

# -----------------------------------------------------------------------------
# Platform Linking & Dependencies
# -----------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(app PRIVATE
        SDL3_image::SDL3_image-static
        SDL3::SDL3-static
        opengl32
        C:/libs/webp-1.6.0-mingw/lib/libwebpdemux.a
        C:/libs/webp-1.6.0-mingw/lib/libwebpmux.a
        C:/libs/webp-1.6.0-mingw/lib/libwebp.a
        C:/libs/webp-1.6.0-mingw/lib/libsharpyuv.a
    )

    # Windows static linking & subsystem selection
    target_link_options(app PRIVATE -static)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(app PRIVATE -mconsole)
    else()
        target_link_options(app PRIVATE -mwindows)
    endif()
else()
    target_link_libraries(app PRIVATE
        SDL3_image::SDL3_image-static
        SDL3::SDL3-static
        GL
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebpdemux.a
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebpmux.a
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebp.a
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libsharpyuv.a
    )
endif()
