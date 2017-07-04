cmake_minimum_required(VERSION 2.8.9)

if(NOT ITK_SOURCE_DIR)
  include(itk-module-init.cmake)
endif()

project(ITKBridgeArrayFire)

set(ITKBridgeArrayFire_LIBRARIES ITKBridgeArrayFire)

# When this module is loaded by an app, load ArrayFire and OpenCV too.
set(ITKBridgeArrayFire_EXPORT_CODE_INSTALL "
set(ArrayFire_DIR \"${ArrayFire_DIR}\")
find_package(ArrayFire REQUIRED)
set(OpenCV_DIR \"${OpenCV_DIR}\")
find_package(OpenCV REQUIRED)
")

set(ITKBridgeArrayFire_EXPORT_CODE_BUILD "
if(NOT ITK_BINARY_DIR)
  set(ArrayFire_DIR \"${ArrayFire_DIR}\")
  find_package(ArrayFire REQUIRED)
  set(OpenCV_DIR \"${OpenCV_DIR}\")
  find_package(OpenCV REQUIRED)
endif()
")

set(ITKBridgeArrayFire_SYSTEM_INCLUDE_DIRS ${ArrayFire_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
set(ITKBridgeArrayFire_SYSTEM_LIB_DIRS ${OpenCV_LIB_DIR})

if(NOT ITK_SOURCE_DIR)
  find_package(ITK REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  include(ITKModuleExternal)
else()
  itk_module_impl()
endif()
