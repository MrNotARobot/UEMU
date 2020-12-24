/* Copyright (c) 2020 Gabriel Manoel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "x86-utils.h"

#include "cpu.h"

_Bool overflow8(uint8_t n1, uint8_t n2, uint8_t result)
{
    if (signbit8(n1) != signbit8(n2))
        return 0;

    if (signbit8(result) != signbit8(n1))
        return 1;

    return 0;
}

_Bool overflow16(uint16_t n1, uint16_t n2, uint16_t result)
{
    if (signbit16(n1) != signbit16(n2))
        return 0;

    if (signbit16(result) != signbit16(n1))
        return 1;

    return 0;
}

_Bool overflow32(uint32_t n1, uint32_t n2, uint32_t result)
{
    if (signbit32(n1) != signbit32(n2))
        return 0;

    if (signbit32(result) != signbit32(n1))
        return 1;

    return 0;
}

#define lownibble(byte)  (  (byte) & 0x0F  )
#define highnibble(byte) (  (byte) & 0xF0  )

_Bool auxcarry(uint8_t n1, uint8_t n2)
{
    if (highnibble(lownibble(n1) + lownibble(n2)))
        return 1;

    return 0;
}

_Bool auxborrow(uint8_t n1, uint8_t n2)
{
    if (lownibble(highnibble(n1) - highnibble(n2)))
        return 1;

    return 0;
}

inline _Bool parity_even(uint32_t bytes)
{
    uint8_t lsb = bytes & 0x000000ff;
    _Bool parity = 0;
    int parity_bits = 0;

    while (lsb) {
        if (lsb & 1)
            parity_bits++;
        lsb >>= 1;
    }

    if (parity_bits % 2 == 0)
        parity = 1;

    return parity;
}

uint8_t displacement16(uint8_t modrm)
{
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    uint8_t sz = 0;

    if (mod == 1)
        sz = 8;
    else if (mod == 2 || (mod == 0 && rm == 0b110))
        sz = 16;

    return sz;
}

uint8_t displacement32(uint8_t modrm)
{
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    uint8_t sz = 0;

    if (mod == 1)
        sz = 8;
    else if (mod == 2 || (mod == 0 && rm == 0b101))
        sz = 32;

    return sz;
}

inline _Bool reg8_islsb(uint8_t reg)
{
    switch (reg) {
        case AL: case CL: case DL: case BL:
            return 1;
    }

    return 0;
}

inline uint8_t reg8to32(uint8_t reg)
{
    int reg32 = 0;

    switch (reg) {
        case AL: case AH: reg32 = EAX; break;
        case CL: case CH: reg32 = ECX; break;
        case DL: case DH: reg32 = EDX; break;
        case BL: case BH: reg32 = EBX; break;
        default: break;
    }

    return reg32;
}

inline uint8_t reg32to8(uint8_t reg)
{
    switch (reg) {
        case EAX: return AL;
        case ECX: return CL;
        case EDX: return DL;
        case EBX:  return BL;
        case ESP: return AH;
        case EBP: return CH;
        case ESI: return DH;
        case EDI: return BH;
        default: break;
    }
    return 0;
}

inline uint8_t effctvregister(uint8_t modrm, uint8_t size)
{
    int reg = 0;
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);

    if (size == 8) {
        if (mod == 3) {
            switch (rm) {
                case 0b000: reg = AL; break;
                case 0b001: reg = CL; break;
                case 0b010: reg = DL; break;
                case 0b011: reg = BL; break;
                case 0b100: reg = AH; break;
                case 0b101: reg = CH; break;
                case 0b110: reg = DH; break;
                case 0b111: reg = BH; break;
            }
        }

    } else {
        if (mod == 3) {
            switch (rm) {
                case 0b000: reg = EAX; break;
                case 0b001: reg = ECX; break;
                case 0b010: reg = EDX; break;
                case 0b011: reg = EBX; break;
                case 0b100: reg = ESP; break;
                case 0b101: reg = EBP; break;
                case 0b110: reg = ESI; break;
                case 0b111: reg = EDI; break;
            }
        }

    }

    return reg;
}

inline const char *stringfyregister(uint8_t regstr, uint8_t size)
{
    if (size == 8) {
        switch (regstr) {
            case AL: return "al"; case CL: return "cl"; case DL: return "dl";
            case BL: return "bl"; case AH: return "ah"; case CH: return "ch";
            case DH: return "dh"; case BH: return "bh";
        }
    } else if (size == 16) {
        switch (regstr) {
            case AX: return "ax"; case CX: return "cx"; case DX: return "dx";
            case BX: return "bx"; case SP: return "sp"; case BP: return "bp";
            case SI: return "si"; case DI: return "di";
        }
    } else if (size == 32) {
        switch (regstr) {
            case EAX: return "eax"; case ECX: return "ecx"; case EDX: return "edx";
            case EBX: return "ebx"; case ESP: return "esp"; case EBP: return "ebp";
            case ESI: return "esi"; case EDI: return "edi"; case EIP: return "eip";
        }
    }

    return (void *)0;
}

// translate a Mod/RM byte + optional immediate into an effective address
moffset32_t x86_effectiveaddress16(void *cpu, uint8_t modrm, uint32_t imm)
{
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    moffset32_t vaddr = 0;

    if (!cpu)
        return 0;

    if (mod == 0) {
        switch (rm) {
            case 0b000: vaddr = x86_readR16(cpu, EBX) + x86_readR16(cpu, ESI); break;
            case 0b001: vaddr = x86_readR16(cpu, EBX) + x86_readR16(cpu, EDI); break;
            case 0b010: vaddr = x86_readR16(cpu, EBP) + x86_readR16(cpu, ESI); break;
            case 0b011: vaddr = x86_readR16(cpu, EBP) + x86_readR16(cpu, EDI); break;
            case 0b100: vaddr = x86_readR16(cpu, ESI); break;
            case 0b101: vaddr = x86_readR16(cpu, EDI); break;
            case 0b110: vaddr = imm; break;
            case 0b111: vaddr = x86_readR16(cpu, EBX); break;
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: vaddr = x86_readR16(cpu, EBX) + imm; break;
            case 0b001: vaddr = x86_readR16(cpu, EBX) + imm; break;
            case 0b010: vaddr = x86_readR16(cpu, EBP) + imm; break;
            case 0b011: vaddr = x86_readR16(cpu, EBP) + imm; break;
            case 0b100: vaddr = x86_readR16(cpu, ESI) + imm; break;
            case 0b101: vaddr = x86_readR16(cpu, EDI) + imm; break;
            case 0b110: vaddr = x86_readR16(cpu, EBP) + imm; break;
            case 0b111: vaddr = x86_readR16(cpu, EBX) + imm; break;
        }
    } // mod == 3 are registers, so we return zero

    return vaddr;
}

// translate a Mod/RM byte + optional immediate/SIB into an effective address
moffset32_t x86_effectiveaddress32(void *cpu, uint8_t modrm, uint8_t sib, uint32_t imm)
{
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    moffset32_t vaddr = 0;
    _Bool use_sib = 0;
    uint8_t ss_factor = sibss(sib);
    uint8_t index = sibindex(sib);
    uint8_t base = sibbase(sib);

    if (!cpu)
        return 0;

    if (mod == 0) {
        switch (rm) {
            case 0b000: vaddr = x86_readR32(cpu, EAX); break;
            case 0b001: vaddr = x86_readR32(cpu, ECX); break;
            case 0b010: vaddr = x86_readR32(cpu, EDX); break;
            case 0b011: vaddr = x86_readR32(cpu, EBX); break;
            case 0b100: use_sib = 1; break;
            case 0b101: vaddr = x86_readR32(cpu, EIP) + imm; break;
            case 0b110: vaddr = x86_readR32(cpu, ESI); break;
            case 0b111: vaddr = x86_readR32(cpu, EDI); break;
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: vaddr = x86_readR32(cpu, EAX) + imm; break;
            case 0b001: vaddr = x86_readR32(cpu, ECX) + imm; break;
            case 0b010: vaddr = x86_readR32(cpu, EDX) + imm; break;
            case 0b011: vaddr = x86_readR32(cpu, EBX) + imm; break;
            case 0b100: use_sib = 1; break;
            case 0b101: vaddr = x86_readR32(cpu, EBP) + imm; break;
            case 0b110: vaddr = x86_readR32(cpu, ESI) + imm; break;
            case 0b111: vaddr = x86_readR32(cpu, EDI) + imm; break;
        }
    } // mod == 3 are registers, so we return zero

    if (use_sib) {
        if (ss_factor == 0b00)
            ss_factor = 1;
        else if (ss_factor == 0b01)
            ss_factor = 2;
        else if (ss_factor == 0b10)
            ss_factor = 4;
        else if (ss_factor == 0b10)
            ss_factor = 4;

        if (base == EBP) {
            if (index != 0b100)
                vaddr = x86_readM32(cpu, x86_readR32(cpu, index) * ss_factor);

            if (mod == 1)
                vaddr += lsb(imm);
            else
                vaddr += imm;

            if (mod)
                vaddr += x86_readR32(cpu, EBP);
        } else {
            vaddr = x86_readR32(cpu, base);
            if (mod == 1)
                vaddr += lsb(imm);
            else if (mod == 2)
                vaddr += imm;

            if (index != 0b100)
                vaddr = vaddr + x86_readR32(cpu, index) * ss_factor;

        }

    }

    return vaddr;
}
