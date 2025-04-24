
cmake_minimum_required(VERSION 3.13)
project(dng_sdk_wasm)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_LIBPNG=1 -s FORCE_FILESYSTEM=1 -s USE_ZLIB=1 -s MODULARIZE=0 -s EXPORT_ES6=0 -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s ENVIRONMENT=web -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_LIBPNG=1 -s FORCE_FILESYSTEM=1 -s USE_ZLIB=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -sASSERTIONS=1 -s EXPORTED_RUNTIME_METHODS=['FS','WORKERFS'] -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s ENVIRONMENT=web -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_LIBPNG=1 -s FORCE_FILESYSTEM=1 -s USE_ZLIB=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -sASSERTIONS=1 -s EXPORTED_RUNTIME_METHODS=['FS','WORKERFS'] -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s ENVIRONMENT=worker -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_LIBPNG=1 -s FORCE_FILESYSTEM=1 -s USE_ZLIB=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s ENVIRONMENT=web -O3")

add_definitions(-D_FILE_OFFSET_BITS=64)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake)

#file(GLOB ZUID_SOURCES
#        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid/sources/*.cpp"
#
#)
#
#add_library(dng_zuid STATIC ${ZUID_SOURCES})
#
#target_include_directories(dng_zuid PUBLIC
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid/interfaces
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/public/include
#
#
#)

# Locate Boost
#include_directories("C:/msys64/mingw64/include")
#find_package(Boost REQUIRED)

#include_directories("C:/msys64/mingw64/include/boost/uuid")
#link_directories("C:/msys64/mingw64/lib") # Emscripten or MinGW lib path

if(MINGW)
    # Disable specific flags for MinGW
    add_definitions(-D__NO_ANSI_CPP)
endif()


#####
#
#include_directories(${CMAKE_SOURCE_DIR}/dng_sdk_1_7/libjpeg)
## libjpeg integration
#file(GLOB JPEG_SOURCES
#        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/libjpeg/*.c"
#)
#list(FILTER JPEG_SOURCES EXCLUDE REGEX ".*jmemdos.c$")
#list(FILTER JPEG_SOURCES EXCLUDE REGEX ".*jmemmac.c$")
#
#add_library(dng_jpeg STATIC ${JPEG_SOURCES})
#
#target_include_directories(dng_jpeg PUBLIC
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/libjpeg
#)

# -------------------------------------
# ZLIB from source (like WAF does)
## -------------------------------------
#file(GLOB ZLIB_SOURCES
#        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zlib/*.c"
#)
#
#add_library(dng_zlib STATIC ${ZLIB_SOURCES})
#
#target_include_directories(dng_zlib PUBLIC
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zlib
#)

file(GLOB EXPAT_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore/third-party/expat/public/lib/*.c"
)

add_library(expat STATIC ${EXPAT_SOURCES})
target_include_directories(expat PUBLIC
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore/third-party/expat/public/include
)


link_directories(${CMAKE_BINARY_DIR}/build)

# -------------------------------------
# JPEG XL (precompiled) integration
# -------------------------------------
set(JPEGXL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/jpeg-xl/include")
set(JPEGXL_LIB_DIR "${CMAKE_SOURCE_DIR}/jpeg-xl/lib")

# -------------------------------------
# XMP Toolkit (XMPCore)
# -------------------------------------
file(GLOB_RECURSE XMP_CORE_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore/source/*.cpp"
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/source/*.cpp"
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid/sources/*.cpp"
)
#list(FILTER XMP_CORE_SOURCES EXCLUDE REGEX ".*Host_IO-POSIX.cpp$")
list(FILTER XMP_CORE_SOURCES EXCLUDE REGEX ".*Host_IO-Win.cpp$")

add_library(xmp_core STATIC ${XMP_CORE_SOURCES})

target_include_directories(xmp_core PUBLIC
        #zuid
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid/interfaces
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/public/include

#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/public/include
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore
)

file(GLOB_RECURSE XMP_FILES_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPFiles/source/*.cpp"
)
# Remove Mac & Linux versions
list(FILTER XMP_FILES_SOURCES EXCLUDE REGEX ".*OS_Utils_Mac.cpp$")
list(FILTER XMP_FILES_SOURCES EXCLUDE REGEX ".*OS_Utils_WIN.cpp$")
#list(FILTER XMP_FILES_SOURCES EXCLUDE REGEX ".*OS_Utils_Linux.cpp$")
list(FILTER XMP_FILES_SOURCES EXCLUDE REGEX ".*OS_Utils_Android.cpp$")
add_library(xmp_files STATIC ${XMP_FILES_SOURCES})

target_include_directories(xmp_files PUBLIC
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/public/include
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPFiles/source
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPFilesPlugins/api/source

)

target_compile_definitions(xmp_files PUBLIC
        SUPPORT_SHARED_POINTERS_IN_STD=1
        SUPPORT_SHARED_POINTERS_IN_TR1=0
#        qLinux=1
        qWeb=1
        #        _WIN32=0
#        _WIN64=0
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
    #    WIN_ENV
#        UNIX_ENV=1
        WEB_ENV=1
  #      NOMINMAX

   #     qVisualC=1

        qDNGUseLibJPEG=0
        BIB_MULTI_THREAD=1
        UNICODE
        _UNICODE
#        qDNGPrintMessages=1
        qDNGValidate=0
        qDNGValidateTarget=0
        _CRT_SECURE_NO_WARNINGS=1
        AdobePrivate=1
        qDNGXMPDocOps=0
        HAVE_EXPAT_CONFIG_H=1
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        qDNGThreadSafe=1


)

target_compile_definitions(xmp_core PUBLIC
        SUPPORT_SHARED_POINTERS_IN_STD=1
        SUPPORT_SHARED_POINTERS_IN_TR1=0
#        _WIN32=0
#        _WIN64=0
        expat
#        qLinux=1
        qDNGUseStdInt=1
#        UNIX_ENV=1
        WEB_ENV=1

        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        BUILDING_XMPCORE_LIB=1
        ENABLE_CPP_DOM_MODEL=0
        XMP_COMPONENT_INT_NAMESPACE=AdobeXMPCore_Int
  #      WIN_ENV
    #    NOMINMAX
     #   qVisualC=1

        qDNGUseLibJPEG=0
        BIB_MULTI_THREAD=1
        UNICODE
        _UNICODE
#        qDNGPrintMessages=1
        qDNGValidate=0
        qDNGValidateTarget=0
        _CRT_SECURE_NO_WARNINGS=1
        AdobePrivate=1
        qDNGXMPDocOps=0
        HAVE_EXPAT_CONFIG_H=1
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        qDNGThreadSafe=1


)

# -------------------------------------
# DNG SDK sources
# -------------------------------------
file(GLOB_RECURSE DNG_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/dng_sdk/source/*.cpp"
)

# -------------------------------------
# DNG SDK static lib
# -------------------------------------
add_library(dng_sdk STATIC ${DNG_SOURCES})

target_include_directories(dng_sdk PUBLIC
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/dng_sdk/source
#        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/public/include
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zlib
        ${JPEGXL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(dng_sdk PUBLIC
      #  qWinOS=1
        SUPPORT_SHARED_POINTERS_IN_STD=1
        SUPPORT_SHARED_POINTERS_IN_TR1=0
#        qLinux=1
        qDNGUseStdInt=1
#        qMacOS=0
#        _CONSOLE
#        _CRT_SECURE_NO_DEPRECATE
#        WIN32
#        _WIN64=1
#        WIN64
#        _WINDOWS=1
#        WIN_ENV=1
#        UNIX_ENV=1
        WEB_ENV=1
  #      NOMINMAX

     #   qVisualC=1

        qDNGThreadSafe=1
        qDNGValidate=0
#        qDNGPrintMessages=0
        qDNGUseXMP=1
        qDNGXMPDocOps=0
        JXL_EXPORT=__declspec\(dllimport\)
        JXL_THREADS_EXPORT=__declspec\(dllimport\)
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE

        qDNGUseLibJPEG=0
        BIB_MULTI_THREAD=1
        UNICODE
        _UNICODE
#        qDNGPrintMessages=1
        qDNGValidate=0
        qDNGValidateTarget=0
        _CRT_SECURE_NO_WARNINGS=1
        AdobePrivate=1
        qDNGXMPDocOps=0
        HAVE_EXPAT_CONFIG_H=1
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        qDNGThreadSafe=1

        qDNGUseStdInt=1
#        UNIX_ENV=1
        WEB_ENV=1
)

target_link_directories(dng_sdk PUBLIC ${JPEGXL_LIB_DIR})

target_link_libraries(dng_sdk PUBLIC
#        dng_zuid
#        ${CMAKE_SOURCE_DIR}/build/libdng_zuid.a
        ${CMAKE_SOURCE_DIR}/build/libbrotlienc.a
        ${CMAKE_SOURCE_DIR}/build/libbrotlidec.a
        ${CMAKE_SOURCE_DIR}/build/libbrotlicommon.a
        expat
#        dng_zlib
        xmp_core
        xmp_files
#        dng_jpeg
        jxl
        jxl_threads
        jxl_cms
)

# -------------------------------------
# Main executable
# -------------------------------------
add_executable(dng_example main.cpp)
target_link_libraries(dng_example dng_sdk)
message(STATUS "This is a message from CMakeLists.txt for mini lightroom poc project")
