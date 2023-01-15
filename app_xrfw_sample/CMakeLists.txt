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
    ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/ovr_openxr_mobile_sdk)
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
# native_app_glue as a static lib
#
set(TARGET_NAME android_native_app_glue)
set(ANDROID_NATIVE_APP_GLUE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
add_library(${TARGET_NAME} STATIC
            ${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c)
target_include_directories(${TARGET_NAME} PUBLIC ${ANDROID_NATIVE_APP_GLUE})
target_compile_options(${TARGET_NAME} PRIVATE -Wno-unused-parameter)
# Export ANativeActivity_onCreate(), Refer to:
# https://github.com/android-ndk/ndk/issues/381.

#
# xrfw
#
add_library(xrfw ../src/xrfw.cpp ../src/xrfw_android.cpp)
target_include_directories(xrfw PUBLIC ../include)
target_link_libraries(xrfw PUBLIC plog openxr_loader android_native_app_glue)
target_compile_definitions(xrfw PUBLIC XR_USE_GRAPHICS_API_OPENGL_ES
                                       XR_USE_PLATFORM_ANDROID)

#
# xrfw_sample
#
add_library(xrfw_sample SHARED main.cpp ../platform_android.cpp oglrenderer.cpp
                               ogldrawable.cpp)
target_link_libraries(
  xrfw_sample
  PRIVATE xrfw
          glm::glm
          android_native_app_glue
          EGL
          GLESv3
          android
          log)
target_link_options(xrfw_sample PUBLIC -u ANativeActivity_onCreate)
