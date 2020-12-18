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

#ifndef BIN_UTILS_H
#define BIN_UTILS_H

#include "../types.h"

#define sign_bit(bytes) (   (bytes) >> (sizeof((bytes))-1)    )
#define lb(bytes) (     (bytes) & 0x000000ff     )  // low 8 bits
#define l16(bytes) (     (bytes) & 0x0000ffff     )  // low 16 bits
#define h16(bytes) (     (bytes) & 0xffff0000 >> 16  )  // low 16 bits
#define signxtnd8(bytes) (  bytes |   ((bytes & 0x000000ff) | (bytes & 0x00000080) ? 0xffffff00 : 0)  )     // sign extend to 32 bits
#define signxtnd8to16(bytes) (  bytes |((bytes & 0x00ff) | (bytes & 0x0080) ? 0xff00 : 0)  )     // sign extend to 32 bits
#define signxtnd16(bytes) (  bytes | ((bytes & 0x0000ffff) | (bytes & 0x00008000) ? 0xffffff00 : 0)  )     // sign extend to 32 bits
#define zeroxtnd8to16(bytes) (   (bytes & 0x00ff)  )     // zero-extend to 16 bits
#define zeroxtnd16(bytes) (    bytes & 0x0000ffff     )
#define zeroxtnd8(bytes) (    bytes & 0x000000ff     )

_Bool parity_even(uint32_t);

#endif /* BIN_UTILS_H */
