cmake_minimum_required(VERSION 3.16...3.26)

project(obs-game-audio-trigger VERSION 1.0.0)

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OBS Studioのパスを設定
set(OBS_STUDIO_DIR "" CACHE PATH "OBS Studio source directory")

# プラグイン情報
set(PLUGIN_NAME "obs-game-audio-trigger")
set(PLUGIN_DISPLAY_NAME "Game Audio Trigger")
set(PLUGIN_DESCRIPTION "Plays audio when specific game screen is detected")
set(PLUGIN_AUTHOR "Your Name")

# OBS Studio の検索
find_path(OBS_INCLUDE_DIR 
    NAMES obs-module.h
    PATHS ${OBS_STUDIO_DIR}/libobs
    PATH_SUFFIXES include)

find_library(OBS_LIB
    NAMES obs libobs
    PATHS ${OBS_STUDIO_DIR}/build
    PATH_SUFFIXES libobs libobs/RelWithDebInfo libobs/Release libobs/Debug)

# OpenCV の検索
find_package(OpenCV REQUIRED)

# ソースファイルの設定
set(PLUGIN_SOURCES
    src/plugin-main.cpp
    src/game-audio-trigger.cpp
    src/image-matcher.cpp
    src/audio-player.cpp
    src/process-detector.cpp
)

set(PLUGIN_HEADERS
    src/game-audio-trigger.h
    src/image-matcher.h
    src/audio-player.h
    src/process-detector.h
)

# プラグインライブラリの作成
add_library(${PLUGIN_NAME} MODULE
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# インクルードディレクトリの設定
target_include_directories(${PLUGIN_NAME} PRIVATE
    ${OBS_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    src/
)

# ライブラリのリンク
target_link_libraries(${PLUGIN_NAME}
    ${OBS_LIB}
    ${OpenCV_LIBS}
)

# Windows固有の設定
if(WIN32)
    target_link_libraries(${PLUGIN_NAME}
        winmm
        psapi
    )
    
    # DLLとして出力
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
endif()

# コンパイルフラグの設定
if(MSVC)
    target_compile_options(${PLUGIN_NAME} PRIVATE /W3 /MP)
else()
    target_compile_options(${PLUGIN_NAME} PRIVATE -Wall -Wextra)
endif()

# インストール設定
if(WIN32)
    set(OBS_PLUGIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/obs-plugins/64bit")
else()
    set(OBS_PLUGIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/obs-plugins")
endif()

install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION ${OBS_PLUGIN_DIR})

# データファイルのインストール
install(DIRECTORY data/
    DESTINATION ${OBS_PLUGIN_DIR}/../data/obs-plugins/${PLUGIN_NAME}/)
