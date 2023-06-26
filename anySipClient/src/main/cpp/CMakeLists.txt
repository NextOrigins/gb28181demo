cmake_minimum_required(VERSION 3.4.1)
project("anySipClient")
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(lib_base_DIR ${CMAKE_SOURCE_DIR}/thrid_lib/)
set(STATIC_LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,
${STATIC_LIB_DIR}/${ANDROID_ABI}/libexosip.so")

set(NATIVE_LIB "anySipClient")

file(GLOB mylib_SOURCES "dev_util/*.cpp")


add_library(exosip SHARED IMPORTED)
set_target_properties(exosip PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/${ANDROID_ABI}/libexosip.so)







add_library(
        ${NATIVE_LIB}
        SHARED
        gb_lib.cpp
        ctrl.cpp
        refresh.cpp
        threadsafe_queue.cpp
        anySipClient.cpp
        SipClientListener.cpp
        ${mylib_SOURCES}



)



target_link_libraries(
        ${NATIVE_LIB}
        exosip
        log)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/dev_util/)
