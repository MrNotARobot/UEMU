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


#ifndef X86_TYPES_H
#define X86_TYPES_H

struct EFlags {
    unsigned int f_ID : 1;
    unsigned int f_VIP : 1;
    unsigned int f_VIF : 1;
    unsigned int f_AC : 1;
    unsigned int f_VM : 1;
    unsigned int f_RF : 1;
    unsigned int f_NT : 1;
    unsigned int f_IOPL : 2;
    unsigned int f_OF : 1;
    unsigned int f_DF : 1;
    unsigned int f_IF : 1;
    unsigned int f_TF : 1;
    unsigned int f_SF : 1;
    unsigned int f_ZF : 1;
    unsigned int f_AF : 1;
    unsigned int f_PF : 1;
    unsigned int f_CF : 1;
};



#endif /* X86_TYPES_H */
