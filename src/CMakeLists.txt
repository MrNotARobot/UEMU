set(SRCS
    memory.c
    system.c
    generic-elf.c
    sym-resolver.c
    x86/x86-mmu.c
    x86/x86-utils.c
    x86/general.c
    x86/opcodes.c
    x86/disassembler.c
    x86/instructions.c
    x86/cpustat.c
    x86/cpu.c
    uemu.c
)

add_executable(uemu ${SRCS})

target_compile_options(uemu PRIVATE -O3 -g -Wall -Wextra -Werror)
