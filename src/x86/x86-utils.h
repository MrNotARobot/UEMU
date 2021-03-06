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

#ifndef X86_UTILS_H
#define X86_UTILS_H

#include "../types.h"

enum x86Registers {
    EAX = 0, AX = EAX,
    ECX = 1, CX = ECX,
    EDX = 2, DX = EDX,
    EBX = 3, BX = EBX,
    ESP = 4, SP = ESP,
    EBP = 5, BP = EBP,
    ESI = 6, SI = ESI,
    EDI = 7, DI = EDI,
    EIP = 8,
    AL, AH,
    CL, CH,
    DL, DH,
    BL, BH,
};

enum x86SegmentRegisters {
    CS, SS, DS, ES, FS, GS
};

_Bool overflow8(uint8_t, uint8_t, uint8_t);
_Bool overflow16(uint16_t, uint16_t, uint16_t);
_Bool overflow32(uint32_t, uint32_t, uint32_t);

_Bool auxcarry(uint8_t, uint8_t);
_Bool auxborrow(uint8_t, uint8_t);

#define MODRM_RM_BITMASK 0b00000111
#define MODRM_REG_BITMASK 0b00111000
#define MODRM_MOD_BITMASK 0b11000000

#define rm(modrm)  (    ((modrm) & MODRM_RM_BITMASK)           )
#define reg(modrm) (    ((modrm) & MODRM_REG_BITMASK) >> 3     )
#define mod(modrm) (    ((modrm) & MODRM_MOD_BITMASK) >> 6     )

#define SIB_BASE_BITMASK 0b00000111
#define SIB_INDEX_BITMASK 0b00111000
#define SIB_SS_BITMASK 0b11000000

#define sibbase(sib) (  (sib) & SIB_BASE_BITMASK  )
#define sibindex(sib) (  ((sib) & SIB_INDEX_BITMASK) >> 3  )
#define sibss(sib) (  ((sib) & SIB_SS_BITMASK) >> 6  )


// check the sign bit
#define signbit8(byte) (   ((byte) & 0x80) >> 7    )
#define signbit16(bytes) (   ((bytes) & 0x8000) >> 15    )
#define signbit32(bytes) (   (bytes & 0x80000000) >> 31    )

// mask the lsb byte of a 32-bit number
#define lsb(bytes) (     (bytes) & 0x000000ff     )

// mask the low 2 bytes of a 32-bit number
#define low16(bytes) (     (bytes) & 0x0000ffff     )
// mask the high 2 bytes of a 32-bit number
#define high16(bytes) (     (bytes) & 0xffff0000 >> 16  )

// sign-extend a 8-bit number to 32 bits
#define sign8to32(bytes) (  bytes | (signbit8(bytes) ? 0xffffff00 : 0) )
// sign-extend a 8-bit number to 16 bits
#define sign8to16(bytes) (  bytes |((bytes & 0x00ff) | (bytes & 0x0080) ? 0xff00 : 0)  )
// sign-extend a 16-bit number to 32 bits
#define sign16to32(bytes) (  bytes | ((bytes & 0x0000ffff) | (bytes & 0x00008000) ? 0xffffff00 : 0)  )

// zero-extend a 8-bit number to 16 bits
#define zeroxtnd8to16(bytes) (   (bytes & 0x00ff)  )
// zero-extend a 8-bit number to 32 bits
#define zeroxtnd8(bytes) (    bytes & 0x000000ff     )
// zero-extend a 16-bit number to 32 bits
#define zeroxtnd16(bytes) (    bytes & 0x0000ffff     )

#define sreg(modrm) reg(modrm)

// register is one of AL, DL, BL, CL
_Bool reg8_islsb(uint8_t);
// translate 8-bit registers into their 32-bit counterparts
uint8_t reg8to32(uint8_t);
// translate 32-bit registers into their 8-bit counterparts
uint8_t reg32to8(uint8_t);
// get the register index referenced by the Mod/RM when Mod == 3
uint8_t effctvregister(uint8_t, uint8_t);
// returns a read-only string representing the X-bit register
const char *stringfyregister(uint8_t, uint8_t);

// check if the number of set bits are even
_Bool parity_even(uint32_t);

// the size of the displacement added to the base register according to the encoding
// of the Mod/RM byte
uint8_t displacement16(uint8_t);
uint8_t displacement32(uint8_t);

// calculates the effective address
// example:
//      x86_effectiveaddressX(cpu, modrm, sib, imm1)
moffset32_t x86_effectiveaddress16(void *, uint8_t, uint32_t);
moffset32_t x86_effectiveaddress32(void *, uint8_t, uint8_t, uint32_t);

#endif /* X86_UTILS_H */
