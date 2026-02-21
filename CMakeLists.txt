cmake_minimum_required(VERSION 3.16)
project(CaseOpener VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── SFML ──────────────────────────────────────────────────────────────────────
# Try system SFML first, otherwise fetch 2.6.2 (fixes GCC 15 xmmintrin errors)
find_package(SFML 2.5 COMPONENTS graphics window system audio QUIET)
if(NOT SFML_FOUND)
    include(FetchContent)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.2)
    FetchContent_MakeAvailable(SFML)
endif()

# ── nlohmann/json ─────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/_json)
FetchContent_MakeAvailable(nlohmann_json)

add_library(nlohmann_json_iface INTERFACE)
target_include_directories(nlohmann_json_iface INTERFACE
    ${CMAKE_BINARY_DIR}/_json)

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Game.cpp

    src/core/PlayerData.cpp
    src/core/AudioManager.cpp
    src/core/JsonSave.cpp
    src/core/FreeCaseReward.cpp

    src/ui/Button.cpp
    src/ui/InventoryUI.cpp
    src/ui/CaseInfoPanel.cpp
    src/ui/TradeUpUI.cpp
    src/ui/FreeCaseButton.cpp

    src/screens/MenuScreen.cpp
    src/screens/GameScreen.cpp

    src/items/Item.cpp
    src/items/ItemRegistry.cpp

    src/cases/Case.cpp
    src/cases/CaseRegistry.cpp

    src/gameplay/ReelAnimation.cpp
    src/gameplay/MultiReelManager.cpp
    src/gameplay/Inventory.cpp
    src/gameplay/TradeUp.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        sfml-graphics sfml-window sfml-system sfml-audio
        nlohmann_json_iface)

# ── Assets ────────────────────────────────────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets")

# ── Windows DLLs ──────────────────────────────────────────────────────────────
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS)
endif()
