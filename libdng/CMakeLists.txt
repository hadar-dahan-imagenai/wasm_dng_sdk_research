#ADD_SUBDIRECTORY(md5)
#ADD_SUBDIRECTORY(xmp-sdk)
#ADD_SUBDIRECTORY(dng-sdk)

FIND_PACKAGE(Threads)

# =======================================================
# libdng source code

ADD_LIBRARY( dng_host STATIC ${CMAKE_CURRENT_SOURCE_DIR}/dnghost.cpp )

TARGET_INCLUDE_DIRECTORIES( dng_host INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
TARGET_COMPILE_DEFINITIONS( dng_host PRIVATE -DkLocalUseThreads=1 )
TARGET_COMPILE_OPTIONS( dng_host PRIVATE -fexceptions -std=c++11 )
target_compile_definitions(dng_host PRIVATE qWeb=1)

TARGET_LINK_LIBRARIES( dng_host ${CMAKE_THREAD_LIBS_INIT} )
#TARGET_LINK_LIBRARIES( dng_host dng-sdk ${CMAKE_THREAD_LIBS_INIT} )
