cmake_minimum_required(VERSION 3.24)
project(Brawlink VERSION 0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Solo depende de raylib (se descarga sola). La física es propia; SQLite entra en el Sprint 4.
if(MSVC)
  add_compile_options(/utf-8)                  # textos con tildes
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 5.5 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

# Lógica del juego
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS src/game/*.cpp src/data/*.cpp)
add_library(brawlink_core STATIC ${CORE_SRC} src/core/Assets/Assets.cpp)
target_include_directories(brawlink_core PUBLIC src)
target_link_libraries(brawlink_core PUBLIC raylib)

# Ejecutable del juego
file(GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS src/core/Game/Game.cpp src/core/Input/Input.cpp src/scenes/*.cpp src/main.cpp)
add_executable(BRAWLINK WIN32 ${APP_SRC})
target_link_libraries(BRAWLINK PRIVATE brawlink_core)
if(MSVC)
  target_link_options(BRAWLINK PRIVATE /ENTRY:mainCRTStartup)
endif()
add_custom_command(TARGET BRAWLINK POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BRAWLINK>/assets)

# Pruebas (RNF-005)
# Solo se compilan si existe la carpeta tests/
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_logic.cpp)
  enable_testing()
  add_executable(brawlink_tests tests/test_logic.cpp)
  target_include_directories(brawlink_tests PRIVATE src)
  add_test(NAME brawlink_tests COMMAND brawlink_tests)
endif()
