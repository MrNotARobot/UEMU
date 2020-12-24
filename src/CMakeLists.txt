set(SRCS
    memory.c
    system.c
    string-utils.c

    generic-elf.c
    sym-resolver.c
    tracer.c
    conf.c

    x86/x86-mmu.c
    x86/cpu.c
    x86/dbg.c
    x86/x86-utils.c
    x86/disassembler.c
    x86/general-purpose.c
    x86/instructions.c
    x86/opcodes.c
    uemu.c
)

add_executable(uemu ${SRCS})

target_compile_options(uemu PRIVATE -O3 -g -Wall -Wextra -Werror)
