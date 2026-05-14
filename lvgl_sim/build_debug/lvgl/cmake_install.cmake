# Install script for directory: E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/garden_lvgl_sim")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE DIRECTORY FILES "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE FILE FILES
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lv_version.h"
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lvgl.h"
    "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/lv_conf.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE DIRECTORY FILES "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/examples" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE DIRECTORY FILES "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/demos" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" TYPE FILE FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lvgl.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Debug/lvgl.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Release/lvgl.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/MinSizeRel/lvgl.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/RelWithDebInfo/lvgl.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE FILE FILES
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lv_version.h"
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lvgl.h"
    "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/lv_conf.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Debug/lvgl_thorvg.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Release/lvgl_thorvg.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/MinSizeRel/lvgl_thorvg.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/RelWithDebInfo/lvgl_thorvg.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE FILE FILES
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lv_version.h"
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lvgl.h"
    "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/lv_conf.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Debug/lvgl_demos.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Release/lvgl_demos.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/MinSizeRel/lvgl_demos.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/RelWithDebInfo/lvgl_demos.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE FILE FILES
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lv_version.h"
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lvgl.h"
    "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/lv_conf.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Debug/lvgl_examples.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/Release/lvgl_examples.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/MinSizeRel/lvgl_examples.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/build_debug/lvgl/lib/RelWithDebInfo/lvgl_examples.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lvgl" TYPE FILE FILES
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lv_version.h"
    "E:/MCU/lvgl_tools/lv_port_pc_vscode/lv_port_pc_vscode/lvgl/lvgl.h"
    "E:/MCU/esp32/xaiozhi1111111111111111111111111/garden/lvgl_sim/lv_conf.h"
    )
endif()

