set(SRCS
    memory.c
    system.c
    generic-elf.c
    basicmmu.c
    uemu.c
)

add_executable(uemu ${SRCS})

target_compile_options(uemu PRIVATE -g -Wall -Wextra -Werror)
target_compile_options(uemu PRIVATE -Os)
