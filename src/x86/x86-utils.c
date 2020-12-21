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

#include "../system.h"

#include "x86-utils.h"

#include "cpu.h"

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
        case EAX: return AL; break;
        case ECX: return CL; break;
        case EDX: return DL; break;
        case EBX:  return BL; break;
        default: break;
    }
    return 0;
}

inline uint8_t effctvregister(uint8_t modrm)
{
    int reg = 0;
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);

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

    return reg;
}

inline const char *stringfyregister(uint8_t regstr, uint8_t size)
{
    if (size == 8) {
        switch (regstr) {
            case AL: return "AL"; case CL: return "CL"; case DL: return "DL";
            case BL: return "BL"; case AH: return "AH"; case CH: return "CH";
            case DH: return "DH"; case BH: return "BH";
        }
    } else if (size == 16) {
        switch (regstr) {
            case AX: return "AX"; case CX: return "CX"; case DX: return "DX";
            case BX: return "BX"; case SP: return "SP"; case BP: return "BP";
            case SI: return "SI"; case DI: return "DI";
        }
    } else if (size == 32) {
        switch (regstr) {
            case EAX: return "EAX"; case ECX: return "ECX"; case EDX: return "EDX";
            case EBX: return "EBX"; case ESP: return "ESP"; case EBP: return "EBP";
            case ESI: return "ESI"; case EDI: return "EDI"; case EIP: return "EIP";
        }
    }

    return (void *)0;
}

// translate a Mod/RM byte + optional immediate into an effective address
moffset32_t x86_effctvaddr16(void *cpu, uint8_t modrm, uint32_t imm)
{
    ASSERT(cpu != NULL);
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    moffset32_t effctvaddr = 0;

    if (mod == 0) {
        switch (rm) {
            case 0b000: effctvaddr = x86_rdreg16(cpu, EBX) + x86_rdreg16(cpu, ESI); break;
            case 0b001: effctvaddr = x86_rdreg16(cpu, EBX) + x86_rdreg16(cpu, EDI); break;
            case 0b010: effctvaddr = x86_rdreg16(cpu, EBP) + x86_rdreg16(cpu, ESI); break;
            case 0b011: effctvaddr = x86_rdreg16(cpu, EBP) + x86_rdreg16(cpu, EDI); break;
            case 0b100: effctvaddr = x86_rdreg16(cpu, ESI); break;
            case 0b101: effctvaddr = x86_rdreg16(cpu, EDI); break;
            case 0b110: effctvaddr = imm; break;
            case 0b111: effctvaddr = x86_rdreg16(cpu, EBX); break;
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: effctvaddr = x86_rdreg16(cpu, EBX) + imm; break;
            case 0b001: effctvaddr = x86_rdreg16(cpu, EBX) + imm; break;
            case 0b010: effctvaddr = x86_rdreg16(cpu, EBP) + imm; break;
            case 0b011: effctvaddr = x86_rdreg16(cpu, EBP) + imm; break;
            case 0b100: effctvaddr = x86_rdreg16(cpu, ESI) + imm; break;
            case 0b101: effctvaddr = x86_rdreg16(cpu, EDI) + imm; break;
            case 0b110: effctvaddr = x86_rdreg16(cpu, EBP) + imm; break;
            case 0b111: effctvaddr = x86_rdreg16(cpu, EBX) + imm; break;
        }
    } // mod == 3 are registers, so we return zero

    return effctvaddr;
}

// translate a Mod/RM byte + optional immediate/SIB into an effective address
moffset32_t x86_effctvaddr32(void *cpu, uint8_t modrm, uint8_t sib, uint32_t imm)
{
    ASSERT(cpu != NULL);
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    moffset32_t effctvaddr = 0;
    _Bool use_sib = 0;
    uint8_t ss_factor = sibss(sib);
    uint8_t index = sibindex(sib);
    uint8_t base = sibbase(sib);

    if (mod == 0) {
        switch (rm) {
            case 0b000: effctvaddr = x86_rdreg32(cpu, EAX); break;
            case 0b001: effctvaddr = x86_rdreg32(cpu, ECX); break;
            case 0b010: effctvaddr = x86_rdreg32(cpu, EDX); break;
            case 0b011: effctvaddr = x86_rdreg32(cpu, EBX); break;
            case 0b100: use_sib = 1; break;
            case 0b101: effctvaddr = x86_rdreg32(cpu, EIP) + imm; break;
            case 0b110: effctvaddr = x86_rdreg32(cpu, ESI); break;
            case 0b111: effctvaddr = x86_rdreg32(cpu, EDI); break;
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: effctvaddr = x86_rdreg32(cpu, EAX) + imm; break;
            case 0b001: effctvaddr = x86_rdreg32(cpu, ECX) + imm; break;
            case 0b010: effctvaddr = x86_rdreg32(cpu, EDX) + imm; break;
            case 0b011: effctvaddr = x86_rdreg32(cpu, EBX) + imm; break;
            case 0b100: use_sib = 1; break;
            case 0b101: effctvaddr = x86_rdreg32(cpu, EBP) + imm; break;
            case 0b110: effctvaddr = x86_rdreg32(cpu, ESI) + imm; break;
            case 0b111: effctvaddr = x86_rdreg32(cpu, EDI) + imm; break;
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
                effctvaddr = x86_rdmem32(cpu, x86_rdreg32(cpu, index) * ss_factor);

            effctvaddr += imm;

                if (mod)
                    effctvaddr += x86_rdreg32(cpu, EBP);
        } else {
            if (index != 0b100)
                effctvaddr = x86_rdmem32(cpu, x86_rdreg32(cpu, index) * ss_factor);
            effctvaddr += x86_rdreg32(cpu, base);
        }

    }

    return effctvaddr;
}
