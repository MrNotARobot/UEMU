cmake_minimum_required(VERSION 3.17)

project(UEMU
    DESCRIPTION "Usersapace Emulator"
    LANGUAGES C
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(EXECUTABLE_PATH /usr/bin)

set(CMAKE_C_COMPILER ${EXECUTABLE_PATH}/gcc)

find_program(NASM_COMPILER nasm)
if(NASM_COMPILER)
    set(CMAKE_ASM_COMPILER ${NASM_COMPILER})
endif()

add_subdirectory(src)
