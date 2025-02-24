# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(myNpmLib)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)

find_package(Iconv REQUIRED)

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

file(GLOB STREAM_FILE "stream/*.cpp")
file(GLOB DEFLATE_FILE "deflate/*.cpp")
file(GLOB ZIP_FILE "zip/*.cpp")
file(GLOB BUFFER_POOL "bufferpool/*.cpp")
file(GLOB BROTLI "brotli/*.cpp")
file(GLOB READER_FILE "reader/*.cpp")
file(GLOB BROTLI_SOURCE "third_party/brotli/*/*.c")

add_library(libbrotli STATIC ${BROTLI_SOURCE})

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include
                    ${NATIVERENDER_ROOT_PATH}/third_party/zlib-ng/include
                    ${NATIVERENDER_ROOT_PATH}/third_party/brotli/include)

add_library(zlib-ng STATIC IMPORTED)
set_target_properties(zlib-ng PROPERTIES 
    IMPORTED_LOCATION ${NATIVERENDER_ROOT_PATH}/third_party/zlib-ng/libs/${OHOS_ARCH}/libz-ng.a
    INTERFACE_INCLUDE_DIRECTORIES ${NATIVERENDER_ROOT_PATH}/third_party/zlib-ng/include)


add_library(jemoc_stream SHARED napi_init.cpp ${STREAM_FILE} ${DEFLATE_FILE} ${ZIP_FILE} ${BUFFER_POOL} ${BROTLI} ${READER_FILE})
target_link_libraries(jemoc_stream PUBLIC libace_napi.z.so zlib-ng libhilog_ndk.z.so libbrotli Iconv::Iconv)
