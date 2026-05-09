cmake_minimum_required(VERSION 3.20)

project(VoidLayer VERSION 0.1.0 LANGUAGES CXX)

add_executable(VoidLayer WIN32
    src/AppHost.cpp
    src/HotkeyManager.cpp
    src/Localization.cpp
    src/ReapplyService.cpp
    src/RuleStore.cpp
    src/SettingsDialog.cpp
    src/TransparencyEngine.cpp
    src/Types.cpp
    src/WindowResolver.cpp
    src/main.cpp
)

target_compile_features(VoidLayer PRIVATE cxx_std_17)

target_compile_definitions(VoidLayer PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_include_directories(VoidLayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(VoidLayer PRIVATE
    comctl32
    dwmapi
    shell32
    shlwapi
    user32
    gdi32
    advapi32
    ole32
    uxtheme
)

if(MSVC)
    target_compile_options(VoidLayer PRIVATE /W4 /permissive- /utf-8)
    target_link_options(VoidLayer PRIVATE /OPT:REF /OPT:ICF)
endif()
