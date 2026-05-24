cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#
# 得到所有子文件夹目录的函数：
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)
function(collect_subdirectories OUTPUT_VAR ROOT_DIR)
    set(SUBDIRS "${ROOT_DIR}")  # 优先把根目录放进去
    function(_recurse dir)
        file(GLOB CHILDREN RELATIVE ${dir} ${dir}/*)
        foreach(CHILD ${CHILDREN})
            set(CHILD_PATH "${dir}/${CHILD}")
            if(IS_DIRECTORY ${CHILD_PATH})
                list(APPEND SUBDIRS ${CHILD_PATH})
                _recurse(${CHILD_PATH})
            endif()
        endforeach()
        set(SUBDIRS ${SUBDIRS} PARENT_SCOPE)
    endfunction()
    _recurse(${ROOT_DIR})
    list(REMOVE_DUPLICATES SUBDIRS)
    set(${OUTPUT_VAR} ${SUBDIRS} PARENT_SCOPE)
endfunction()

function(collect_all_c_files OUTPUT_VAR ROOT_DIR)
    file(GLOB_RECURSE C_FILES "${ROOT_DIR}/*.c")
    list(REMOVE_DUPLICATES C_FILES)
    set(${OUTPUT_VAR} ${C_FILES} PARENT_SCOPE)
endfunction()

# 整备lvgl文件夹
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)         # 设定根目录地址
collect_subdirectories(LVGL_SUBDIRS ${LVGL_DIR})      # 得到所有子文件夹
collect_all_c_files(LVGL_SOURCE_FILES ${LVGL_DIR})     # 得到所有文件名



# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME LCD_EX)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        ${LVGL_SOURCE_FILES}

)


# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# TFTLCD 显示驱动
file(GLOB_RECURSE TFTLCD_SOURCES "${CMAKE_SOURCE_DIR}/APP/tftlcd/*.c")
# 重定向/调试输出
file(GLOB_RECURSE RETARGET_SOURCES "${CMAKE_SOURCE_DIR}/APP/retarget/*.c")
# 触摸屏驱动
file(GLOB_RECURSE TP_SOURCES "${CMAKE_SOURCE_DIR}/APP/tp/*.c")
# 延时函数
file(GLOB_RECURSE DELAY_SOURCES "${CMAKE_SOURCE_DIR}/APP/delay/*.c")
#can通信
file(GLOB_RECURSE CAN_SOURCES "${CMAKE_SOURCE_DIR}/APP/BSP/*.c")
#ui界面
file(GLOB_RECURSE UI_SOURCES "${CMAKE_SOURCE_DIR}/APP/GUI/*.c")
#ui图标
file(GLOB_RECURSE IMG_SOURCES "${CMAKE_SOURCE_DIR}/APP/GUI/IMG/*.c")
# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${TFTLCD_SOURCES}
    ${RETARGET_SOURCES}
    ${TP_SOURCES}
    ${DELAY_SOURCES}
    ${CAN_SOURCES}
    ${UI_SOURCES}
    ${IMG_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_SOURCE_DIR}/APP/tftlcd
    ${CMAKE_SOURCE_DIR}/APP/retarget
    ${CMAKE_SOURCE_DIR}/APP/tp
    ${LVGL_SUBDIRS}
    ${CMAKE_SOURCE_DIR}/APP/delay
    ${CMAKE_SOURCE_DIR}/APP/BSP
    ${CMAKE_SOURCE_DIR}/APP/GUI

)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
