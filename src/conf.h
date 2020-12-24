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

#ifndef CONF_H
#define CONF_H

#include "types.h"

enum ConfigTypes {
    CONF_TP_STRING,
    CONF_TP_BOOL,
    CONF_TP_NUMBER,
    CONF_TP_HEX,
};

enum {
    CONF_OPTIONAL,
    CONF_REQUIRED
};

enum {
    CONF_NO_ARG,
    CONF_ARG_OPTIONAL,
    CONF_ARG_REQUIRED
};

typedef struct {
    const char *o_name;
    const char *o_longopt;
    char o_opt;
    uint8_t o_type;
    uint8_t o_required;
    uint8_t o_arg;

    union {
        void *ptr;
        uint64_t value;
    } o_default;

    union {
        void *ptr;
        uint64_t value;
    } o_current_value;
} conf_opt_t;

typedef struct {
    conf_opt_t *cf_table;
    size_t cf_noptions;
    uint32_t *cf_required;
    size_t cf_nrequired;

    uint32_t *cf_bucket;
    uint32_t *cf_chain;
} config_t;

// initializes and prepare the struct
// should be called before any conf_add()
void conf_start(config_t *);
// compute the hashes for the names for faster lookup
// should be called after the last conf_add()
void conf_end(config_t *);
void conf_freetables(config_t *);

void conf_add(config_t *, const char *, const char *, char, uint8_t, _Bool, int, void *, uint64_t);

int conf_parse_argv(config_t *, char **);

// these function will return NULL or 0 if the type does not match
void *conf_getptr(config_t *, const char *);
uint64_t conf_getval(config_t *, const char *);

#endif /* CONF_H */
