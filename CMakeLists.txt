# 禁用编译器自动测试
#set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#定义工具链
INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/tools/toolchain/cortex_m4.cmake")
# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(STM32F4_BOOT)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)

# 指定链接文件；
SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/platform/stm32f407vetx_flash.ld)

# 指定启动文件；
SET(STARTUP_ASM  ${CMAKE_SOURCE_DIR}/platform/cmsis/device/startup_stm32f407xx.s)

# 项目底层公共头文件；
# 步骤1：递归获取 cmsis 下所有子目录（含自身）
file(GLOB_RECURSE CMSIS_SUBDIRS RELATIVE "${CMAKE_SOURCE_DIR}/platform/cmsis" "${CMAKE_SOURCE_DIR}/platform/cmsis/*")

# 显式添加 cmsis 及其子目录（推荐）
INCLUDE_DIRECTORIES(
  "${CMAKE_SOURCE_DIR}/platform/cmsis/device"  #  device 子目录
  "${CMAKE_SOURCE_DIR}/platform/cmsis/core"    #  core 子目录
  "${CMAKE_SOURCE_DIR}/platform/driver/inc"   #  device/inc子目录
)
# 获取ST官方源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/platform/driver/src DRIVER_SOURCES)
# 平台相关宏定义
ADD_DEFINITIONS(
  -DUSE_STDPERIPH_DRIVER
  -DSTM32
  -DSTM32F429_439xx
  -DHSE_VALUE=8000000
  -DCMAKE_DEMO
  -DUSING_NON_RTOS=0 # 不使用 RTOS
  -DUSING_FREERTOS=1 # 使用 FreeRTOS
  -DUSING_THREADX=2 # 使用 Threadx
  -DUSING_RTOS=USING_NON_RTOS # 选择使用的 RTOS
)
OPTION(OPEN_LOG_OMN_DEBUG "Open log output for debug" OFF)

# 修改该变量的值，可以修改输出文件的名称；
SET(OUTPUT_EXE_NAME "demo")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  ADD_DEFINITIONS()
ELSEIF("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
  ADD_DEFINITIONS()
ELSEIF("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
  ADD_DEFINITIONS()
ELSE()
  IF(OPEN_LOG_OMN_DEBUG)
    ADD_DEFINITIONS(-DLOG_BACKEND=LOG_BACKEND_NONE)
  ELSE()
    ADD_DEFINITIONS(-DLOG_BACKEND=LOG_BACKEND_NONE)
  ENDIF()
ENDIF()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "选择构建类型" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()
MESSAGE(STATUS "**** Build for ${CMAKE_BUILD_TYPE} ****")

# 设置库输出路径
# 设置库输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib_obj)

SET(ELF_FILE ${PROJECT_BINARY_DIR}/${OUTPUT_EXE_NAME}.elf)
SET(HEX_FILE ${PROJECT_BINARY_DIR}/${OUTPUT_EXE_NAME}.hex)
SET(BIN_FILE ${PROJECT_BINARY_DIR}/${OUTPUT_EXE_NAME}.bin)

#主要添加位置
INCLUDE_DIRECTORIES(
  # 应用层头文件包含路径；
  ${CMAKE_SOURCE_DIR}/boot/app/inc 
  ${CMAKE_SOURCE_DIR}/boot/app/override
  # 硬件驱动头文件路径；
  ${CMAKE_SOURCE_DIR}/boot/driver/led)
  # 第三方库头文件路径；
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_lib/ringbuffer)

SET(USER_SOURCE
  ${CMAKE_SOURCE_DIR}/boot/app/main.c
  ${CMAKE_SOURCE_DIR}/boot/app/stm32f4xx_it.c
  ${CMAKE_SOURCE_DIR}/platform/cmsis/device/system_stm32f4xx.c)

  aux_source_directory(${CMAKE_SOURCE_DIR}/boot/app/override OVERRIDE_SOURCES)

  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${LINKER_SCRIPT} -Wl,-Map=${PROJECT_BINARY_DIR}/${OUTPUT_EXE_NAME}.map -Wl,--gc-sections,--print-memory-usage")

# 添加第三方库
# 定义第三方库根目录
set(THIRD_LIB_ROOT ${CMAKE_SOURCE_DIR}/third_lib)

# 1. 获取 third_lib 下的所有子目录（如 lvgl、ringbuffer）
file(GLOB THIRD_LIB_SUBDIRS RELATIVE ${THIRD_LIB_ROOT} ${THIRD_LIB_ROOT}/*)

# 2. 循环处理每个子目录：添加头文件路径 + 收集源文件
foreach(LIB_DIR ${THIRD_LIB_SUBDIRS})
  # 拼接完整路径
  set(LIB_FULL_PATH ${THIRD_LIB_ROOT}/${LIB_DIR})
  
  # 仅处理目录（跳过文件）
  if(IS_DIRECTORY ${LIB_FULL_PATH})
    # 2.1 添加该库的头文件路径（递归包含所有子目录，适应不同库的结构）
    include_directories(${LIB_FULL_PATH})
    
    # 2.2 递归获取该库下的所有源文件（.c/.cpp/.s等）
    file(GLOB_RECURSE LIB_SOURCES 
      ${LIB_FULL_PATH}/*.c
      ${LIB_FULL_PATH}/*.cpp
      ${LIB_FULL_PATH}/*.s  # 若有汇编文件
    )
    
    # 将当前库的源文件添加到全局第三方库源文件变量
    list(APPEND THIRD_LIB_SOURCES ${LIB_SOURCES})
  endif()
endforeach()

# 3. 将所有第三方库源文件添加到可执行目标
ADD_EXECUTABLE(
  ${PROJECT_NAME}  # 注意：这里必须用项目目标名称（如 STM32F4_BOOT），而非 USER_SOURCE
  ${USER_SOURCE}
  ${DRIVER_SOURCES}
  ${STARTUP_ASM}
  ${LINKER_SCRIPT}
  ${THIRD_LIB_SOURCES}  # 新增：所有第三方库源文件
  ${OVERRIDE_SOURCES}
)


# 添加依赖；
SET(PATH_COMPONENTS ${CMAKE_SOURCE_DIR}/boot/driver)

ADD_SUBDIRECTORY(${PATH_COMPONENTS}/led ${LIBRARY_OUTPUT_PATH}/led)

ADD_CUSTOM_COMMAND(
  TARGET "${PROJECT_NAME}"
  POST_BUILD
  # Build .hex and .bin files
  COMMAND ${OBJCOPY} -Obinary "${PROJECT_NAME}" "${OUTPUT_EXE_NAME}.bin"
  COMMAND ${OBJCOPY} -Oihex "${PROJECT_NAME}" "${OUTPUT_EXE_NAME}.hex"
  COMMENT "Building ${OUTPUT_EXE_NAME}.bin and ${OUTPUT_EXE_NAME}.hex"
  # Display sizes
  COMMAND ${SIZE} --format=berkeley ${PROJECT_NAME}
  COMMENT "Invoking: Cross ARM GNU Print Size")