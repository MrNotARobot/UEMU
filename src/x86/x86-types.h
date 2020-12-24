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

enum EFlagsRegisterFlags {
    ID, VIP, VIF, AC, VM, RF, NT, IOPL, OF, DF, IF, TF, SF, ZF, AF, PF, CF
};

struct EFlags {
    unsigned int ID : 1;
    unsigned int VIP : 1;
    unsigned int VIF : 1;
    unsigned int AC : 1;
    unsigned int VM : 1;
    unsigned int RF : 1;
    unsigned int NT : 1;
    unsigned int IOPL : 2;
    unsigned int OF : 1;
    unsigned int DF : 1;
    unsigned int IF : 1;
    unsigned int TF : 1;
    unsigned int SF : 1;
    unsigned int ZF : 1;
    unsigned int AF : 1;
    unsigned int PF : 1;
    unsigned int CF : 1;
};



#endif /* X86_TYPES_H */
