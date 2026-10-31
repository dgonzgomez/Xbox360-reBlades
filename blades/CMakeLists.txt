cmake_minimum_required(VERSION 3.25)
# The CPU renderer must not ship at the toolchain's default -O0.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
project(blades C)

add_executable(blades
	platform/xsdk/main.c
	platform/xsdk/ui_audio_xsdk.c
	src/ui_audio_data.c
	src/draw.c
	src/font.c
	src/app.c
	src/library.c
	src/guide.c
	src/guide_art_data.c
	src/asset_codec.c
	src/font_data.c
	src/reference_assets.c
	src/reference_asset_data.c)
target_include_directories(blades PRIVATE src third_party)
target_compile_options(blades PRIVATE -Wall -Wextra -Werror -Wno-unused-function)
xsdk_add_shaders(blades platform/xsdk/screen.vert platform/xsdk/screen.frag)
xsdk_add_xex(blades)

# Small hardware isolation test: no dashboard assets, shaders, audio or profile queries.
add_executable(blades_boot_probe platform/xsdk/boot_probe.c)
target_compile_options(blades_boot_probe PRIVATE -Wall -Wextra -Werror)
xsdk_add_xex(blades_boot_probe)
