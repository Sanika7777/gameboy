cmake_minimum_required(VERSION 3.24)
project(neobrick LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

if(EMSCRIPTEN)
    add_compile_options(-sUSE_SDL=2 -fwasm-exceptions $<$<CONFIG:Release>:-O2>)
endif()

# Everything except the platform layer and the composition root (main, App): SDL-free, so tests
# build without SDL.
file(GLOB_RECURSE NB_CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(FILTER NB_CORE_SOURCES EXCLUDE REGEX "src/(main\\.cpp|platform/|core/App\\.cpp)")
add_library(nbcore STATIC ${NB_CORE_SOURCES})
target_include_directories(nbcore PUBLIC src)
target_compile_options(nbcore PUBLIC
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual)

file(GLOB NB_PLATFORM_SOURCES CONFIGURE_DEPENDS src/platform/*.cpp)

if(EMSCRIPTEN)
    add_executable(neobrick src/main.cpp src/core/App.cpp ${NB_PLATFORM_SOURCES})
    target_link_libraries(neobrick PRIVATE nbcore)
    target_link_options(neobrick PRIVATE
        -sUSE_SDL=2 -fwasm-exceptions -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web -lidbstore.js
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
        --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html
        $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-sASSERTIONS=1> $<$<CONFIG:Debug>:-gsource-map>
        $<$<CONFIG:Release>:-O2> $<$<CONFIG:MinSizeRel>:-Os>)
    set_target_properties(neobrick PROPERTIES OUTPUT_NAME index SUFFIX .html
        LINK_DEPENDS ${CMAKE_SOURCE_DIR}/web/shell.html)
    # The asset pack is regenerated on link; relink when any asset changes.
    file(GLOB_RECURSE NB_ASSETS CONFIGURE_DEPENDS assets/*)
    set_property(TARGET neobrick APPEND PROPERTY LINK_DEPENDS ${NB_ASSETS})
else()
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        add_executable(neobrick src/main.cpp src/core/App.cpp ${NB_PLATFORM_SOURCES})
        target_link_libraries(neobrick PRIVATE nbcore SDL2::SDL2)
        add_custom_command(TARGET neobrick POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:neobrick>/assets)
    else()
        message(STATUS "SDL2 not found: building library and tests only (install libsdl2-dev for the app)")
    endif()

    file(GLOB NB_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(nbtests ${NB_TEST_SOURCES})
    target_link_libraries(nbtests PRIVATE nbcore)
    target_compile_definitions(nbtests PRIVATE NB_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    add_test(NAME nbtests COMMAND nbtests)
endif()
