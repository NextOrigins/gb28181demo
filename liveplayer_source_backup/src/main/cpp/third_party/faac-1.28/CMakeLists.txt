cmake_minimum_required(VERSION 3.10.2)

add_library(
        faac STATIC
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/aacquant.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/bitstream.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/fft.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/frame.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/midside.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/psychkni.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/util.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/backpred.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/channels.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/filtbank.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/huffman.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/ltp.c
        ${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/libfaac/tns.c
)
target_include_directories(faac PUBLIC ${CMAKE_SOURCE_DIR}/third_party/faac-1.28/include/)