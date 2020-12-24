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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "conf.h"

#include "system.h"
#include "memory.h"

_Bool isnumber(const char *);
_Bool ishex(const char *);
unsigned long hash(const char *);
conf_opt_t *get_confopt(config_t *, const char *);

_Bool isnumber(const char *string)
{
    for (size_t i = 0; i < strlen(string); i++) {
        if (!isdigit(string[i]))
            return 0;
    }

    return 1;
}

_Bool ishex(const char *string)
{
    for (size_t i = 0; i < strlen(string); i++) {
        if (!isxdigit(string[i]))
            return 0;
    }

    return 1;
}

// Yes, I'm using the same hashing function that the elf hash tables uses
unsigned long hash(const char *name)
{
    unsigned long h = 0, g;

    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000))
            h ^= g >> 24;
        h &= ~g;
    }

    return h;
}

conf_opt_t *get_confopt(config_t *conf, const char *name)
{
    unsigned long h = hash(name);
    uint32_t optidx = conf->cf_bucket[h % conf->cf_noptions];

    while (strcmp(name, conf->cf_table[optidx].o_name) != 0) {
        optidx = conf->cf_chain[h % conf->cf_noptions];
        h = conf->cf_chain[optidx];
    }

    return &conf->cf_table[optidx];
}

//
// Initialization/Deinitialization
//

void conf_start(config_t *conf)
{
    if (!conf)
        return;

    conf->cf_bucket = conf->cf_chain = NULL;

    conf->cf_table = NULL;
    conf->cf_required = NULL;
    conf->cf_noptions = conf->cf_nrequired = 0;
}

void conf_end(config_t *conf)
{
    unsigned long h;
    if (!conf)
        return;

    conf->cf_bucket = xcalloc(conf->cf_noptions, sizeof(*conf->cf_bucket));
    conf->cf_chain = xcalloc(conf->cf_noptions, sizeof(*conf->cf_chain));

    for (size_t i = 0; i < conf->cf_noptions; i++) {
        h = hash(conf->cf_table[i].o_name);

        if (conf->cf_chain[conf->cf_bucket[h % conf->cf_noptions]] != i) {
            conf->cf_chain[conf->cf_bucket[h % conf->cf_noptions]] = i;
            conf->cf_chain[i] = i;
            continue;
        }

        conf->cf_bucket[h % conf->cf_noptions] = i;
        conf->cf_chain[i] = i;
    }
}

void conf_freetables(config_t *conf)
{
    if (!conf)
        return;

    xfree(conf->cf_table);
    xfree(conf->cf_required);
    xfree(conf->cf_chain);
    xfree(conf->cf_bucket);
}

void conf_add(config_t *conf, const char *name, const char *longopt, char opt,
        uint8_t type, _Bool required, _Bool arg_required, void *default_ptr, uint64_t default_value)
{
    uint32_t index;

    if (!conf)
        return;

    if (!name)
        s_error(1, "conf: configuration option with no name detected");

    if (type != CONF_TP_STRING && type != CONF_TP_NUMBER && type != CONF_TP_HEX &&
            type != CONF_TP_BOOL && (type == CONF_TP_BOOL && arg_required != CONF_NO_ARG) &&
            (required == CONF_REQUIRED && arg_required != CONF_NO_ARG))
        s_error(1, "conf: configuration type invalid: %s", name);


    index = conf->cf_noptions;

    conf->cf_table = xreallocarray(conf->cf_table, ++conf->cf_noptions, sizeof(*conf->cf_table));

    conf->cf_table[index].o_name = name;
    conf->cf_table[index].o_longopt = longopt;
    conf->cf_table[index].o_opt = opt;
    conf->cf_table[index].o_type = type;
    conf->cf_table[index].o_required = required;
    conf->cf_table[index].o_arg = arg_required;

    if (type == CONF_TP_STRING) {
        conf->cf_table[index].o_default.ptr = default_ptr;
        conf->cf_table[index].o_current_value.ptr = default_ptr;
    } else {
        conf->cf_table[index].o_default.value = default_value;
        conf->cf_table[index].o_current_value.value = default_value;
    }

    if (required == CONF_REQUIRED) {
        conf->cf_nrequired++;
        conf->cf_required = xreallocarray(conf->cf_required, conf->cf_nrequired, sizeof(*conf->cf_required));
        conf->cf_required[conf->cf_nrequired-1] = &conf->cf_table[index];
    }
}

int conf_parse_argv(config_t *conf, char **argv)
{
    conf_opt_t *opt = NULL;
    const char *arg = NULL;
    int non_option = 0;

    // skip argv[0]
    argv++;

    while (*argv) {
        if (strncmp(*argv, "--", 2) == 0) {

            for (size_t i = 0; i < conf->cf_noptions; i++) {
                if (strncmp(*argv, conf->cf_table[i].o_longopt, strlen(conf->cf_table[i].o_longopt)) == 0)
                    opt = &conf->cf_table[i];
            }

        } else if ((*argv)[0] == '-') {

            for (size_t i = 0; i < conf->cf_noptions; i++) {
                if (conf->cf_table[i].o_opt == (*argv)[1])
                    opt = &conf->cf_table[i];
            }

        } else {
            _Bool is_number = isnumber(*argv);
            _Bool is_hex = ishex(*argv);

            for (size_t i = 0; i < conf->cf_nrequired; i++) {
                conf_opt_t *required = conf->cf_required[i];
                if (is_number && required->o_type == CONF_TP_NUMBER && required->o_current_value.value != required->o_default.value) {
                    opt = required;
                    arg = *argv;
                    break;

                } else if (is_hex && required->o_type == CONF_TP_HEX && required->o_current_value.value != required->o_default.value) {
                    opt = required;
                    arg = *argv;
                    break;

                } else if (required->o_type == CONF_TP_STRING && required->o_current_value.ptr == required->o_default.ptr) {
                    opt = required;
                    arg = *argv;
                    break;

                }
            }

        }

        if (!opt)
            s_error(1, "unknown option: '%s'", *argv);

        if (opt->o_arg == CONF_NO_ARG && opt->o_required != CONF_REQUIRED) {
            if (index(*argv, '='))
                s_error(1, "option '%s' doesn't allow an argument", opt->o_longopt ? opt->o_longopt : &opt->o_opt);
            opt->o_current_value.value = ~opt->o_default.value;
        } else if (opt->o_arg == CONF_ARG_OPTIONAL) {
            arg = index(*argv, '=');
            if (arg)
                arg++;

        } else if (opt->o_arg == CONF_ARG_REQUIRED) {
            arg = index(*argv, '=');
            if (arg)
                arg++;
            else
                arg = *(argv++);

            if (!arg)
                s_error(1, "option '%s' requires an argument", opt->o_longopt ? opt->o_longopt : &opt->o_opt);
        }

        if (arg && opt->o_type == CONF_TP_STRING) {
            opt->o_current_value.ptr = (void *)arg;
        } else if (arg && (opt->o_type == CONF_TP_NUMBER || opt->o_type == CONF_TP_HEX)) {

            char *end = NULL;
            int base = opt->o_type == CONF_TP_NUMBER ? 10 : 16;
            opt->o_current_value.value = strtol(arg, &end, base);

            if (*end != '\0')
                s_error(1, "invalid argument '%s'",  arg);
        }

        argv++;
        non_option++;
        opt = NULL;
        arg = NULL;
    }

    for (size_t i = 0; i < conf->cf_nrequired; i++) {
        if (conf->cf_required[i]->o_type == CONF_TP_STRING) {
            if (conf->cf_required[i]->o_current_value.ptr != conf->cf_required[i]->o_default.ptr)
                s_error(1, "required argument '%s'",  conf->cf_required[i]->o_longopt);
        } else {
            if (conf->cf_required[i]->o_current_value.value != conf->cf_required[i]->o_default.value)
                s_error(1, "required argument '%s'",  conf->cf_required[i]->o_longopt);
        }
    }

    return non_option;
}


//
// Querying
//

void *conf_getptr(config_t *conf, const char *name)
{
    conf_opt_t *opt;

    if (!conf || !name)
        return NULL;

    opt = get_confopt(conf, name);

    if (!opt || opt->o_type != CONF_TP_STRING)
        return NULL;

    return opt->o_current_value.ptr;
}

uint64_t conf_getval(config_t *conf, const char *name)
{
    conf_opt_t *opt;

    if (!conf || !name)
        return 0;

    opt = get_confopt(conf, name);

    if (!opt || opt->o_type != CONF_TP_NUMBER)
        return 0;

    return opt->o_current_value.value;
}
