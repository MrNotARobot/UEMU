set(SRCS
    memory.c
    system.c
    generic-elf.c
    basicmmu.c
    x86/bin-utils.c
    x86/general.c
    x86/opcodes.c
    x86/disassembler.c
    x86/instructions.c
    x86/cpu.c
    uemu.c
)

add_executable(uemu ${SRCS})

target_compile_options(uemu PRIVATE -g -Wall -Wextra -Werror)
target_compile_options(uemu PRIVATE -Os)
