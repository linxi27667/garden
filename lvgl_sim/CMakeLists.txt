cmake_minimum_required(VERSION 3.12)
project(garden_lvgl_sim C)

cmake_policy(SET CMP0079 NEW)

set(LVGL_DIR "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl")
set(GAOCHANG_DIR "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode")
set(MINGW_DIR "E:/MCU/lvgl_tools/mingw64/mingw64")

add_subdirectory(${LVGL_DIR} lvgl)

get_target_property(LVGL_SOURCES lvgl SOURCES)
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*neon.*\\.S$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*helium.*\\.S$")
set_target_properties(lvgl PROPERTIES SOURCES "${LVGL_SOURCES}")

target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR} ${GAOCHANG_DIR} "${MINGW_DIR}/include" "${MINGW_DIR}/include/SDL2")
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

set(SDL2_DIR "E:/MCU/lvgl_tools/mingw64/mingw64")
find_package(SDL2 QUIET CONFIG)

if(SDL2_FOUND)
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
else()
    target_link_directories(lvgl PUBLIC "${SDL2_DIR}/lib")
    target_link_libraries(lvgl PUBLIC SDL2)
endif()

set(UI_SRC
    main/UI/core/ui_events.c
    main/UI/core/ui_styles.c
    main/UI/core/ui_app.c
    main/UI/model/garden_model.c
    main/UI/services/ui_i18n.c
    main/UI/services/ui_export.c
    main/UI/fonts/ui_font.c
    main/UI/utils/ui_theme.c
    main/UI/widgets/ui_header.c
    main/UI/widgets/ui_tabbar.c
    main/UI/pages/ui_page_data.c
    main/UI/pages/ui_page_ctrl.c
    main/UI/pages/ui_page_set.c
    main/UI/pages/ui_page_garden.c
)

add_executable(garden_sim
    main/src/SDL_main.c
    ${UI_SRC}
)

target_include_directories(garden_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/main
    ${CMAKE_SOURCE_DIR}/main/UI
    ${GAOCHANG_DIR}
    ${LVGL_DIR}
    ${LVGL_DIR}/src
    ${MINGW_DIR}/include
    ${MINGW_DIR}/include/SDL2
)

if(SDL2_FOUND)
    target_link_libraries(garden_sim PRIVATE lvgl lvgl::thorvg SDL2::SDL2 SDL2::SDL2main)
else()
    target_link_directories(garden_sim PRIVATE "${SDL2_DIR}/lib")
    target_link_libraries(garden_sim PRIVATE lvgl lvgl::thorvg SDL2 SDL2main)
endif()

target_compile_definitions(garden_sim PRIVATE LV_CONF_INCLUDE_SIMPLE)

# Ensure console output on Windows
if(WIN32)
    target_link_options(garden_sim PRIVATE "-mconsole")
endif()

add_custom_command(TARGET garden_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/main/UI/fonts/NotoSansSC-Regular.ttf"
        "${CMAKE_BINARY_DIR}/NotoSansSC.ttf"
    COMMENT "Copy Chinese TTF font to build directory")

add_custom_command(TARGET garden_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/main/UI/fonts/fa-solid-900.ttf"
        "${CMAKE_BINARY_DIR}/fa-solid-900.ttf"
    COMMENT "Copy FontAwesome Solid font to build directory")

add_custom_command(TARGET garden_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/main/UI/fonts/fa-regular-400.ttf"
        "${CMAKE_BINARY_DIR}/fa-regular-400.ttf"
    COMMENT "Copy FontAwesome Regular font to build directory")
