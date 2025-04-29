
cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdeclspec --bind -s WASM=1 -lworkerfs.js -flto -O3 -lembind")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
-s USE_LIBPNG=1 \
-s FORCE_FILESYSTEM=1 \
-s USE_ZLIB=1 \
-s MODULARIZE=1 \
-s EXPORT_ES6=1 \
-s ASSERTIONS=1 \
-s EXPORTED_RUNTIME_METHODS=['FS','WORKERFS'] \
-s DISABLE_EXCEPTION_CATCHING=0 \
-s ALLOW_MEMORY_GROWTH=1 \
-s ENVIRONMENT=worker --bind -s WASM=1 -lworkerfs.js -flto -lembind")

project(dng_sdk_wasm)

add_definitions(-D_FILE_OFFSET_BITS=64)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake)


if(MINGW)
    # Disable specific flags for MinGW
    add_definitions(-D__NO_ANSI_CPP)
endif()

# -------------------------------
# Third-party libraries
# -------------------------------
#libjpeg
include_directories(${CMAKE_SOURCE_DIR}/dng_sdk_1_7/libjpeg)
file(GLOB JPEG_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/libjpeg/*.c"
)
list(FILTER JPEG_SOURCES EXCLUDE REGEX ".*jmemdos.c$")
list(FILTER JPEG_SOURCES EXCLUDE REGEX ".*jmemmac.c$")
list(FILTER JPEG_SOURCES EXCLUDE REGEX ".*example.c$")
list(FILTER JPEG_SOURCES EXCLUDE REGEX ".*jmemname.c$")

add_library(dng_jpeg STATIC ${JPEG_SOURCES})

target_include_directories(dng_jpeg PUBLIC
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/libjpeg
)

#expat
file(GLOB EXPAT_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore/third-party/expat/public/lib/*.c"
)

add_library(expat STATIC ${EXPAT_SOURCES})
target_include_directories(expat PUBLIC
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore/third-party/expat/public/include
)

#jpex xl precompiled
set(JPEGXL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/jpeg-xl/include")
set(JPEGXL_LIB_DIR "${CMAKE_SOURCE_DIR}/jpeg-xl/lib")


# -------------------------------
# Internal SDK libraries
# -------------------------------

# xmp_core
file(GLOB_RECURSE XMP_CORE_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore/source/*.cpp"
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/source/*.cpp"
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid/sources/*.cpp"
)
list(FILTER XMP_CORE_SOURCES EXCLUDE REGEX ".*Host_IO-Win.cpp$")

add_library(xmp_core STATIC ${XMP_CORE_SOURCES})

target_include_directories(xmp_core PUBLIC
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid/interfaces
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zuid
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/public/include
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPCore
)

target_compile_definitions(xmp_core PUBLIC
        SUPPORT_SHARED_POINTERS_IN_STD=1
        SUPPORT_SHARED_POINTERS_IN_TR1=0
        expat
        qDNGUseStdInt=1
        WEB_ENV=1
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        BUILDING_XMPCORE_LIB=1
        ENABLE_CPP_DOM_MODEL=0
        XMP_COMPONENT_INT_NAMESPACE=AdobeXMPCore_Int
        qDNGUseLibJPEG=1
        BIB_MULTI_THREAD=1
        UNICODE
        _UNICODE
        qDNGValidate=0
        qDNGValidateTarget=0
        _CRT_SECURE_NO_WARNINGS=1
        AdobePrivate=1
        qDNGXMPDocOps=0
        HAVE_EXPAT_CONFIG_H=1
        XMP_64=1
        XML_STATIC=1
        qDNGThreadSafe=1
)

# xmp_files
file(GLOB_RECURSE XMP_FILES_SOURCES
        "${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/XMPFiles/source/*.cpp"
)
# Remove Mac & Linux versions
list(FILTER XMP_FILES_SOURCES EXCLUDE REGEX ".*OS_Utils_Mac.cpp$")
list(FILTER XMP_FILES_SOURCES EXCLUDE REGEX ".*OS_Utils_WIN.cpp$")
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
        qWeb=1
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        WEB_ENV=1
        qDNGUseLibJPEG=1
        BIB_MULTI_THREAD=1
        UNICODE
        _UNICODE
        qDNGValidate=0
        qDNGValidateTarget=0
        _CRT_SECURE_NO_WARNINGS=1
        AdobePrivate=1
        qDNGXMPDocOps=0
        HAVE_EXPAT_CONFIG_H=1
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
        ${CMAKE_SOURCE_DIR}/dng_sdk_1_7/xmp/toolkit/third-party/zlib
        ${JPEGXL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(dng_sdk PUBLIC
        SUPPORT_SHARED_POINTERS_IN_STD=1
        SUPPORT_SHARED_POINTERS_IN_TR1=0
        qDNGUseStdInt=1
        WEB_ENV=1
        qDNGThreadSafe=1
        qDNGUseXMP=1
        qDNGXMPDocOps=0
        JXL_EXPORT=__declspec\(dllimport\)
        JXL_THREADS_EXPORT=__declspec\(dllimport\)
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
        qDNGUseLibJPEG=1
        BIB_MULTI_THREAD=1
        qDNGValidate=0
        qDNGValidateTarget=0
        _CRT_SECURE_NO_WARNINGS=1
        AdobePrivate=1
        qDNGXMPDocOps=0
        HAVE_EXPAT_CONFIG_H=1
        XMP_StaticBuild=1
        XMP_64=1
        XML_STATIC=1
        qDNGUseStdInt=1
        WEB_ENV=1
)

target_link_directories(dng_sdk PUBLIC ${JPEGXL_LIB_DIR})

target_link_libraries(dng_sdk PUBLIC
        ${CMAKE_SOURCE_DIR}/libbrotli/libbrotlienc.a
        ${CMAKE_SOURCE_DIR}/libbrotli/libbrotlidec.a
        ${CMAKE_SOURCE_DIR}/libbrotli/libbrotlicommon.a
        expat
        xmp_core
        xmp_files
        dng_jpeg
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
