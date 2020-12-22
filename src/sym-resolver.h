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

#ifndef SYM_RESOLVER_H
#define SYM_RESOLVER_H

#include "types.h"

typedef struct {
    moffset32_t fr_start;
    off_t fr_strtaboff; // this is added to the offset of the string table when reading from the file
    char *fr_sym;
    size_t fr_size;
} sym_map_t;

typedef struct {
    moffset16_t r_offset;
    moffset32_t r_end;
    sym_map_t *r_sym;
} sym_record_t;

typedef struct {
    moffset32_t mz_base;
    moffset16_t mz_divline;     // an offset that divides records in half
    sym_record_t *mz_high;      // an ptr to the first record with offset >= mz_divline
    sym_record_t *mz_low;       // the start of the records
    size_t mz_mop;     // number of high records. Don't know why the name. Was struggling
                        // for a few minutes thinking up one. So I used whatever word came up
                        // first. Spoiler alert: mop.
    size_t mz_nrecords;
} mem_zone_t;

typedef struct {
    moffset32_t mr_base;
    moffset32_t mr_end;
    mem_zone_t *mr_zones;
    size_t  mr_nzones;
} mem_region_t;

typedef struct {
    _Bool sr_filled;    // set if there are symbols. Zero if the binary is static + stripped
    size_t sr_nregions;
    mem_region_t *sr_regions;     // table for the mapped regions
    sym_map_t *sr_symtab;
    size_t sr_symtabsz;
    int sr_fd;          // file descriptor that points to the file used to load the cache
                        // its not the same as the one hold by the GenericELF struct but a new one
    off_t sr_strtaboff; // offset of the string table from the file pointed to by sr_fd
    char **sr_strtab;  // table for already resolved symbols
    size_t sr_strtabsz;

} sym_resolver_t;

struct symbol_lookup_record {
    const char *sl_name;
    moffset32_t sl_start;
    size_t sl_size;
};

int sr_loadcache(sym_resolver_t *, const char *executable);
void sr_closecache(sym_resolver_t *);
struct symbol_lookup_record sr_lookup(sym_resolver_t *, moffset32_t);

#endif /* SYM_RESOLVER_H */
