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
 *
 * DESCRIPTION:
 *      symbol cache resolver and resolver for your cpu.
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <string.h>

#include "sym-resolver.h"

#include "system.h"
#include "memory.h"

struct symtab_info {
    Elf32_Off offset;
    size_t length;
    uint16_t entsize;
    Elf32_Off strtab;
};

static int getsymtabfromfile(Elf32_Off, uint16_t, uint16_t, int, struct symtab_info *);
static int createregions(sym_resolver_t *, Elf32_Off, uint16_t, uint16_t, int);
static int fillsymboltable(sym_resolver_t *, const struct symtab_info *, int);

static void loadsymbol(sym_resolver_t *, moffset32_t, off_t, size_t);
static void add2zone(mem_region_t *, sym_map_t *);
static void add2region(sym_resolver_t *, sym_map_t *);

static void fetch_symbolname(sym_resolver_t *, sym_map_t *);
static mem_region_t *findregion(sym_resolver_t *, moffset32_t);
static sym_record_t *findprev_lastrecord(mem_region_t *, size_t);

// debug functions
static void dump_zone(sym_resolver_t *, mem_zone_t *, size_t);
static void dump_strtab(sym_resolver_t *);

static _Bool conf_resolver_debug_mode_dump_zone = 0;
static _Bool conf_resolver_debug_mode_dump_strtab = 0;

///////////////


static int getsymtabfromfile(Elf32_Off shoff, uint16_t shnum, uint16_t shentsize, int fd, struct symtab_info *symtabinfo_ptr)
{
    Elf32_Shdr *shdr;
    _Bool found_symtab = 0;
    _Bool found_dynsym = 0;
    uint16_t dynsym = 0;
    uint16_t strtab = 0;

    shdr = xcalloc(shnum, sizeof(*shdr));

    for (uint16_t i = 0; i < shnum; i++, shoff += shentsize) {
        if (pread(fd, &shdr[i], sizeof(*shdr), shoff) == -1)
            return 0;

        if (shdr[i].sh_type == SHT_SYMTAB) {
            symtabinfo_ptr->offset = shdr[i].sh_offset;
            symtabinfo_ptr->length = shdr[i].sh_size / shdr[i].sh_entsize;
            symtabinfo_ptr->entsize = shdr[i].sh_entsize;
            strtab = shdr[i].sh_link;
            found_symtab = 1;
        } else if (shdr[i].sh_type == SHT_DYNSYM) {
            dynsym = i;
            strtab = shdr[i].sh_link;
            found_dynsym = 1;
        }
    }

    if (!found_symtab && found_dynsym) {
        symtabinfo_ptr->offset = shdr[dynsym].sh_offset;
        symtabinfo_ptr->length = shdr[dynsym].sh_size / shdr[dynsym].sh_entsize;
        symtabinfo_ptr->entsize = shdr[dynsym].sh_entsize;
    }

    if (found_dynsym || found_symtab) {
        symtabinfo_ptr->strtab = shdr[strtab].sh_offset;
        xfree(shdr);
        return 1;
    } else {
        xfree(shdr);
        return 0;
    }

}


static int createregions(sym_resolver_t *resolver, Elf32_Off phoff, uint16_t phnum, uint16_t phentsize, int fd)
{
    size_t pagesize = sysconf(_SC_PAGESIZE);
    Elf32_Phdr phdr;

    resolver->sr_regions = NULL;
    resolver->sr_nregions = 0;

    for (uint16_t i = 0; i < phnum; i++, phoff += phentsize) {
        if (pread(fd, &phdr, sizeof(phdr), phoff) == -1)
            return 0;

        if (phdr.p_type == PT_LOAD) {
            size_t size = ((phdr.p_memsz + pagesize - 1) & ~(pagesize-1));
            resolver->sr_regions = xreallocarray(resolver->sr_regions, ++resolver->sr_nregions, sizeof(*resolver->sr_regions));
            resolver->sr_regions[resolver->sr_nregions-1].mr_zones = xcalloc(size / pagesize, sizeof(mem_zone_t));
            resolver->sr_regions[resolver->sr_nregions-1].mr_base = phdr.p_vaddr;
            resolver->sr_regions[resolver->sr_nregions-1].mr_end = phdr.p_vaddr + size;
            resolver->sr_regions[resolver->sr_nregions-1].mr_nzones = size / pagesize;
        }
    }

    return 1;

}

static int fillsymboltable(sym_resolver_t *resolver, const struct symtab_info *symtabinfo_ptr, int fd)
{
    Elf32_Sym sym;
    Elf32_Off symtaboff = symtabinfo_ptr->offset;

    resolver->sr_strtab = NULL;
    resolver->sr_strtaboff = symtabinfo_ptr->strtab;
    resolver->sr_strtabsz = 0;
    resolver->sr_symtab = NULL;
    resolver->sr_symtabsz = 0;

    for (uint32_t i = 0; i < symtabinfo_ptr->length; i++, symtaboff += symtabinfo_ptr->entsize) {
        if (pread(fd, &sym, sizeof(sym), symtaboff) == -1)
            return 0;

        if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC || ELF32_ST_TYPE(sym.st_info) == STT_OBJECT)
            loadsymbol(resolver, sym.st_value, symtabinfo_ptr->strtab + sym.st_name, sym.st_size);
    }

    return 1;
}

static void loadsymbol(sym_resolver_t *resolver, moffset32_t start, off_t strtaboff, size_t size)
{
    size_t idx = resolver->sr_symtabsz++;

    resolver->sr_symtab = xreallocarray(resolver->sr_symtab, idx+1, sizeof(*resolver->sr_symtab));
    resolver->sr_symtab[idx].fr_strtaboff = strtaboff;
    resolver->sr_symtab[idx].fr_sym = NULL;
    resolver->sr_symtab[idx].fr_size = size;
    resolver->sr_symtab[idx].fr_start = start;
}


static void add2zone(mem_region_t *region, sym_map_t *symbol)
{
    size_t zone_index;
    mem_zone_t *zone;
    moffset16_t off_in_zone;
    size_t current_symbol_index = 0;

    zone_index = (symbol->fr_start - region->mr_base) / sysconf(_SC_PAGESIZE);

    // we are down to one page here, got 4096 indexes. if you map a symbol at every byte it might get
    // a little slow to go through every one but I want to guess that people don't do that very often
    // this is as far as my little brain can go to try to speed things up, maybe things will work out
    zone = &region->mr_zones[zone_index];
    off_in_zone = moffset16(symbol->fr_start) - moffset16(zone->mz_base);

    if (zone->mz_nrecords == 0) {
        zone->mz_nrecords++;

        zone->mz_low = xcalloc(1, sizeof(*zone->mz_low));

        zone->mz_low[0].r_offset = off_in_zone;
        zone->mz_low[0].r_end = symbol->fr_start + symbol->fr_size;
        zone->mz_low[0].r_sym = symbol;
        zone->mz_high = zone->mz_low;
        zone->mz_divline = off_in_zone;
        zone->mz_mop = 0;
        return;
    }

    zone->mz_low = xreallocarray(zone->mz_low, ++zone->mz_nrecords,  sizeof(*zone->mz_low));
    memset(zone->mz_low+(zone->mz_nrecords-1), 0, sizeof(*zone->mz_low));

    // WAIT! I got an idea! If we sort the symbols and we have a dividing line then we only have to worry
    // about one side of the records! that means 2048 indexes to go, not 4096!
    for (size_t i = 0; i < zone->mz_nrecords; i++) {

        if (off_in_zone < zone->mz_low[i].r_offset || i == (zone->mz_nrecords-1)) {

            current_symbol_index = i;
            break;
        }

    }

    sym_record_t temp;
    temp.r_offset = moffset16(symbol->fr_start) - moffset16(zone->mz_base);
    temp.r_sym = symbol;
    temp.r_end = symbol->fr_start + symbol->fr_size;

    // move all from current_symbol_index forward
    for (size_t i = current_symbol_index; i < zone->mz_nrecords; i++) {
        sym_record_t old = zone->mz_low[i];
        zone->mz_low[i] = temp;
        temp = old;
    }

    zone->mz_mop = (zone->mz_nrecords % 2) == 0 ? zone->mz_nrecords / 2 : zone->mz_mop;
    zone->mz_divline = zone->mz_low[zone->mz_mop].r_offset;
    zone->mz_high = &zone->mz_low[zone->mz_mop];
    // set the last record's end to zero as we do not know where it ends for now
}

static void add2region(sym_resolver_t *resolver, sym_map_t *symbol)
{
    for (size_t k = 0; k < resolver->sr_nregions; k++) {
        mem_region_t *region = &resolver->sr_regions[k];

        if (symbol->fr_start >= region->mr_base && symbol->fr_start < region->mr_end) {
                add2zone(region, symbol);
        }
    }
}

//
// initialization/destruction
//

int sr_loadcache(sym_resolver_t *resolver, const char *executable)
{
    Elf32_Ehdr ehdr;
    struct symtab_info symtab = {0, 0, 0, 0};
    int fd;

    if (!resolver || !executable)
        return 0;


    fd = open(executable, O_RDONLY);
    if (fd == - 1)
        return 0;

    if (pread(fd, &ehdr, sizeof(ehdr), 0) == -1)
        goto read_fail;

    resolver->sr_fd = fd;

    if (!getsymtabfromfile(ehdr.e_shoff, ehdr.e_shnum, ehdr.e_shentsize, fd, &symtab))
        goto read_fail;

    if (!fillsymboltable(resolver, &symtab, fd))
        goto read_fail;

    if (!createregions(resolver, ehdr.e_phoff, ehdr.e_phnum, ehdr.e_phentsize, fd))
        goto read_fail;

    for (size_t i = 0; i < resolver->sr_symtabsz; i++)
        add2region(resolver, &resolver->sr_symtab[i]);

    resolver->sr_strtab = NULL;
    resolver->sr_strtabsz = 0;
    return 1;
read_fail:
    close(fd);
    return 0;
}

void sr_closecache(sym_resolver_t *resolver)
{
    if (!resolver)
        return;

    for (size_t i = 0; i < resolver->sr_nregions; i++) {
        for (size_t k = 0; k < resolver->sr_regions[i].mr_nzones; k++) {
            xfree(resolver->sr_regions[i].mr_zones[k].mz_low);
        }
        xfree(resolver->sr_regions[i].mr_zones);
    }

    for (size_t i = 0; i < resolver->sr_strtabsz; i++)
        xfree(resolver->sr_strtab[i]);

    close(resolver->sr_fd);
    xfree(resolver->sr_regions);
    xfree(resolver->sr_strtab);
    xfree(resolver->sr_symtab);
}

//
//  Lookup
//

static void fetch_symbolname(sym_resolver_t *resolver, sym_map_t *symbol)
{
    size_t index = 0;
    off_t offset = symbol->fr_strtaboff;
    _Bool found_null = 0;
    uint8_t byte = 0;
    size_t size = 0;

    while (!found_null) {
        if (pread(resolver->sr_fd, &byte, 1, offset++) == -1)
            return;

        if (!byte)
            found_null = 1;

        symbol->fr_sym = xreallocarray(symbol->fr_sym, ++size, sizeof(*symbol->fr_sym));

        symbol->fr_sym[index++] = byte;
    }

    resolver->sr_strtab = xreallocarray(resolver->sr_strtab, ++resolver->sr_strtabsz, sizeof(*resolver->sr_strtab));
    resolver->sr_strtab[resolver->sr_strtabsz-1] = symbol->fr_sym;

}

static mem_region_t *findregion(sym_resolver_t *resolver, moffset32_t vaddr)
{
    for (size_t i = 0; i < resolver->sr_nregions; i++) {

        if (vaddr >= resolver->sr_regions[i].mr_base && vaddr < resolver->sr_regions[i].mr_end)
            return &resolver->sr_regions[i];
    }
    return NULL;
}

static sym_record_t *findprev_lastrecord(mem_region_t *region, size_t starting_zone)
{
    for (int i = starting_zone; i >= 0; i--) {
        mem_zone_t *zone = &region->mr_zones[i];

        if (zone->mz_nrecords)
            return &zone->mz_low[zone->mz_nrecords-1];
    }

    return NULL;
}

struct symbol_lookup_record sr_lookup(sym_resolver_t *resolver, moffset32_t vaddr)
{
    mem_region_t *region = NULL;
    mem_zone_t *zone;
    sym_record_t *records;
    sym_map_t *symbol = NULL;
    moffset16_t off_in_zone;
    size_t zone_idx;
    size_t max;

    if (!resolver)
        return (struct symbol_lookup_record){NULL, 0, 0};

    region = findregion(resolver, vaddr);
    if (!region)
        return (struct symbol_lookup_record){NULL, 0, 0};

    zone_idx = (vaddr - region->mr_base) / sysconf(_SC_PAGESIZE);
    zone = &region->mr_zones[zone_idx];

    off_in_zone = moffset16(vaddr) - moffset16(zone->mz_base);

    if (conf_resolver_debug_mode_dump_zone)
        dump_zone(resolver, zone, zone_idx);

    // there might not be a single record in the zone
    // i.e. we have a symbol that its size is so big that span across pages
    // we loop back till we find a zone with symbols and get the last one
    if (!zone->mz_nrecords) {

        sym_record_t *rec = findprev_lastrecord(region, zone_idx);

        if (rec) {
            symbol = rec->r_sym;

            if (!symbol->fr_sym)
                fetch_symbolname(resolver, symbol);

            return (struct symbol_lookup_record){symbol->fr_sym, symbol->fr_start, rec->r_end};
        }


        return (struct symbol_lookup_record){NULL, 0, 0};
    }

    if (off_in_zone >= zone->mz_divline) {
        records = zone->mz_high;
        max = zone->mz_nrecords - zone->mz_mop;
    } else {
        records = zone->mz_low;
        max = zone->mz_nrecords;
    }

    for (size_t i = 0; i < max; i++) {

        if (vaddr == 0x0804a7f7 || vaddr == 0x0804a7fa)
            s_info("%08x %08x", records[i].r_offset, records[i].r_end);

        if (symbol && off_in_zone < records[i].r_offset)
            break;

        if (off_in_zone >= records[i].r_offset) {
            symbol = records[i].r_sym;
            continue;
        }

    }
    // if this is the last record then the address is between this symbol and the symbol in the next zone
    if (!symbol)
        symbol = records[zone->mz_nrecords-1].r_sym;

    if (!symbol->fr_sym)
        fetch_symbolname(resolver, symbol);

    if (conf_resolver_debug_mode_dump_strtab)
        dump_strtab(resolver);

    return (struct symbol_lookup_record){symbol->fr_sym, symbol->fr_start, symbol->fr_start + symbol->fr_size};

}

//
// Debug
//

static void dump_zone(sym_resolver_t *resolver, mem_zone_t *zone, size_t zone_index)
{
    s_info("==== [%02ld:zone START] ====  records: %02ld base: 0x%08x divline: 0x%04x mop: %ld",
            zone_index, zone->mz_nrecords, zone->mz_base, zone->mz_divline, zone->mz_mop);
    for (size_t i = 0; i < zone->mz_nrecords; i++) {
        sym_record_t *record = &zone->mz_low[i];
        if (!record->r_sym->fr_sym)
            fetch_symbolname(resolver, record->r_sym);

        s_info("[%02ld:zone] record: %02ld offset: 0x%04x end: 0x%04x symbol: %s size: %ld", zone_index, i,
                record->r_offset, record->r_end, record->r_sym->fr_sym, record->r_sym->fr_size);
    }

    s_info("==== [%ld:zone END] ====", zone_index);
}

static void dump_strtab(sym_resolver_t *resolver)
{
    s_info("==== [string table BEGIN] ====");

    if (!resolver->sr_strtab) {
        s_info("==== [string table END] ====");
        return;
    }

    for (size_t i = 0; i < resolver->sr_strtabsz; i++) {
        s_info("%s\\0", resolver->sr_strtab[i]);
    }

    s_info("==== [string table END] ====");
}
