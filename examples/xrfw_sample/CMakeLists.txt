# for android build
cmake_minimum_required(VERSION 3.22.1)
project(xrfw)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
  plog
  GIT_REPOSITORY https://github.com/SergiusTheBest/plog
  GIT_TAG 1.1.9)
FetchContent_MakeAvailable(plog)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 0.9.9.8)
FetchContent_MakeAvailable(plog glm)

FetchContent_Populate(
  openxrsdk
  URL https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.0.26.tar.gz
)

#
# openxr_loader
#
set(MOBILE_SDK_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/ovr_openxr_mobile_sdk)
add_library(openxr_loader SHARED IMPORTED)
set_target_properties(
  openxr_loader
  PROPERTIES
    IMPORTED_LOCATION
    ${MOBILE_SDK_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/Release/libopenxr_loader.so
)
set_property(
  TARGET openxr_loader
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${openxrsdk_SOURCE_DIR}/include
           ${MOBILE_SDK_DIR}/OpenXR/include)

#
# xrfw
#
add_library(xrfw ../../src/xrfw.cpp)
target_include_directories(xrfw PUBLIC ../../include)
target_link_libraries(xrfw PUBLIC plog openxr_loader)
target_compile_definitions(xrfw PUBLIC XR_USE_GRAPHICS_API_OPENGL_ES
                                       XR_USE_PLATFORM_ANDROID)

#
# xrfw_sample
#
add_library(xrfw_sample SHARED main.cpp)
target_link_libraries(xrfw_sample PRIVATE xrfw glm::glm)
