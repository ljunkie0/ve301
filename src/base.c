/*
 * Copyright 2022 LJunkie
 * http://www.???.??
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "base.h"
#include <stdlib.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#define INTERNET_CHECK_HOST "www.google.com"

FILE *log_file;
char *user_home;
static FILE *config_file;
static char *__app;
static char *__log_prefix;

static int _log_levels[NUM_CTX];

typedef struct config_entry {
    char *key;
    char *value;
    int id;
} config_entry;

typedef struct __config_rec {
    char *path;
    config_entry **entries;
    int no_of_entries;
} __config_rec;

__config_rec *__config = 0;

char *
my_copystr (const char *str) {
    char *res = malloc ((strlen (str) + 1) * sizeof (char));
    strcpy (res, str);
    return res;
}

char *
my_copynstr (const char *str, unsigned int max_length) {
    char *res = calloc (max_length+1,sizeof (char));
    strncpy (res, str, max_length);
    return res;
}

char *
my_catstr (const char *str1, const char *str2) {
    char *res = malloc ((strlen (str1) + strlen (str2) + 1) * sizeof (char));
    strcpy (res, str1);
    strcpy (res + strlen (str1), str2);
    return res;
}
/**
 * Encode a code point using UTF-8
 *
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @license MIT
 *
 * @param out - output buffer (min 5 characters), will be 0-terminated
 * @param utf - code point 0-0x10FFFF
 * @return number of bytes on success, 0 on failure (also produces U+FFFD, which uses 3 bytes)
 */
char *to_utf8(uint32_t utf, uint32_t *length)
{
    char *out = NULL;
    if (utf <= 0x7F) {
        // Plain ASCII
        out = malloc(2*sizeof(char));
        out[0] = (char) utf;
        out[1] = 0;
        if (length) *length = 1;
    }
    else if (utf <= 0x07FF) {
        // 2-byte unicode
        out = malloc(3*sizeof(char));
        out[0] = (char) (((utf >> 6) & 0x1F) | 0xC0);
        out[1] = (char) (((utf >> 0) & 0x3F) | 0x80);
        out[2] = 0;
        if (length) *length = 2;
    }
    else if (utf <= 0xFFFF) {
        // 3-byte unicode
        out = malloc(4*sizeof(char));
        out[0] = (char) (((utf >> 12) & 0x0F) | 0xE0);
        out[1] = (char) (((utf >>  6) & 0x3F) | 0x80);
        out[2] = (char) (((utf >>  0) & 0x3F) | 0x80);
        out[3] = 0;
        if (length) *length = 3;
    }
    else if (utf <= 0x10FFFF) {
        // 4-byte unicode
        out = malloc(5*sizeof(char));
        out[0] = (char) (((utf >> 18) & 0x07) | 0xF0);
        out[1] = (char) (((utf >> 12) & 0x3F) | 0x80);
        out[2] = (char) (((utf >>  6) & 0x3F) | 0x80);
        out[3] = (char) (((utf >>  0) & 0x3F) | 0x80);
        out[4] = 0;
        if (length) *length = 4;
    }

    return out;

}
uint16_t *to_unicode(const char *txt, uint32_t *length, uint16_t **second_line, uint32_t *length2) {

    uint32_t max_length = 0;

    if (length) {
        max_length = *length;
    }

    int i;

    uint32_t s = 0;
    uint32_t s1 = 0;
    uint32_t s2 = 0;

    uint16_t *u_txt = NULL;

    if (second_line) {
        *second_line = NULL;
    }

    int line = 0;
    if (txt && txt[0]) {

        i = 0;

        while (txt[i]) {

            unsigned char c = (unsigned char) txt[i++];

            uint16_t u = (uint16_t) c;

            int l = 0;

            if (c > 252) {
                l = 5;
            } else if (c > 248) {
                l = 4;
            } else if (c > 240) {
                l = 3;
            } else if (c > 224) {
                l = 2;
            } else if (c > 192) {
                l = 1;
            }

            if (l > 0) {

                u = c & 0x3f;
                u = (uint16_t) (u << 6*l);

                int b = 0;
                for (b = 1 ; b <= l; b++) {
                    uint16_t n = (uint16_t) (unsigned char) txt[i++];
                    n = n & 0x3f;
                    n = (uint16_t) (n << 6*(l-b));
                    u = u | n;
                }
            }

            if (u == 10) {
                line = 1;
                s = 0;
            } else {
                if (line == 0) {
                    if (max_length == 0 || s < max_length) {
                        u_txt = realloc(u_txt,(++s)*sizeof(uint16_t));
                        u_txt[s-1] = u;
                        s1 = s;
                    } else if (!second_line) {
                        break;
                    }
                } else if (second_line) {
                    if (max_length > 0 && *length2 && s >= *length2) {
                        break;
                    }
                    *second_line = realloc(*second_line,(++s)*sizeof(uint16_t));
                    (*second_line)[s-1] = u;
                    s2 = s;
                }
            }


        }

        u_txt = realloc(u_txt,(s1+1)*sizeof(uint16_t));
        u_txt[s1] = 0;

        if (length2 && s2 > 0) {
            *second_line = realloc(*second_line,(s2+1)*sizeof(uint16_t));
            (*second_line)[s2] = 0;
        }

    }

    if (length) {
        *length = s1;
    }

    if (length2) {
        if (second_line && *second_line) {
            *length2 = s2;
        } else {
            *length2 = 0;
        }
    }

    return u_txt;

}

unsigned int unicode_len(const unsigned short *txt) {
    if (!txt) {
        return 0;
    }
    unsigned int len = 0;
    while (txt[len]) len++;
    return len;
}

unsigned short *unicode_copy(const unsigned short *txt) {
    unsigned short *new_txt = malloc((unicode_len(txt)+1)*sizeof(unsigned short));
    unsigned int len = 0;
    while (txt[len]) {
        new_txt[len] = txt[len];
        len++;
    }
    new_txt[len] = 0;
    return new_txt;
}

char *get_name_from_path(const char *path) {
    char *n = strrchr(path,'/');
    if (!n) {
        n = my_copystr(path);
    } else {
        n = my_copystr(n+1);
    }
    return n;
}

int index_of(const char *str, const char chr) {
    char *substr = strchr(str,chr);
    if (substr) {
        return substr - str - 1;
    }
    return -1;
}

/*char *complete_entry(char *entry) {
    regex_t regex;
    int comp_result = regcomp(&regex, "\$([a-zA-Z]\w.*)"
    if (!comp_result) {
    regmatch_t matches[2];
    int match_result = regexec(regex, entry, 1, &matches, 0);
    if (!match_result) {
        int l = matches[1].rm_eo - matches[1].rm_so;
        char *matching_string = malloc((l+1)*sizeof(char));
        int i = 0;
        for (i = 0; i < l; i++) {
        matching_string[i] = entry[i+matches[1].rm_eo];
        }
    }
    }
    regfree(&regex);
    return
}
*/

int check_internet() {
    const char *hostname = INTERNET_CHECK_HOST;
    struct addrinfo *result;
    int s = getaddrinfo(hostname,0,0,&result);
    if (!s) {
        return 1;
    }
    return 0;
}

void
init_log_file (const char *appname, FILE *dflt_log_file) {

    if (!dflt_log_file) {
        char *logfile_path = my_catstr ("/tmp/", my_catstr (appname, ".log"));
        fprintf (stderr, "Logfile: %s\n", logfile_path);
        log_file = fopen (logfile_path, "w");

        log_error (BASE_CTX, "log file created\n");
        if (!log_file) {
            fprintf (stderr, "Could not create file %s\n", logfile_path);
        } else {
            setlinebuf (log_file);
        }
        free (logfile_path);
    }
}

void
close_log_file () {
    if (log_file && (log_file != stderr) && (log_file != stdout)) {
        fclose (log_file);
    }
}

void
log_variable_args (const int log_ctx, const int lvl, const char *__restrict __format,
                   va_list args) {

    if (!log_file) log_file = stderr;

    if (log_file && (_log_levels[log_ctx] >= lvl)) {
          vfprintf (log_file, __format, args);
    }
}

void
__log_error (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_ERROR, __format, args);
    va_end (args);
}

void
__log_warning (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_WARNING, __format, args);
    va_end (args);
}

void
__log_info (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_INFO, __format, args);
    va_end (args);
}

void
__log_config (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_CONFIG, __format, args);
    va_end (args);
}

void
__log_debug (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_DEBUG, __format, args);
    va_end (args);
}

void
__log_trace (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_TRACE, __format, args);
    va_end (args);
}

config_entry *
find_config_entry (char *key) {
    config_entry *entry = 0;
    int e = 0;
    while (!entry && e < __config->no_of_entries) {
        if (!strcmp (key, __config->entries[e]->key)) {
            entry = __config->entries[e];
        }
        e++;
    }
    return entry;
}

config_entry *
add_config_entry (const char *key, const char *value) {
    config_entry *e = malloc (sizeof (config_entry));
    e->key = my_copystr (key);
    if (value) {
        e->value = my_copystr (value);
    } else {
        e->value = NULL;
    }
    __config->no_of_entries++;
    __config->entries = realloc (__config->entries,
                                 __config->no_of_entries * sizeof (config_entry *));
    __config->entries[__config->no_of_entries - 1] = e;
    return e;
}

void set_config_value (char *key, char *value) {
    if (__config) {
        config_entry *config = find_config_entry (key);
        if (!config) {
            add_config_entry (key, value);
        } else {
            config->value = my_copystr(value);
        }
    }
}

void set_config_value_int (char *key, int value) {
    if (__config) {
        char *value_chr = malloc(12*sizeof(char));
        sprintf(value_chr,"%d",value);
        config_entry *config = find_config_entry (key);
        if (!config) {
            add_config_entry (key, value_chr);
        } else {
            config->value = value_chr;
        }
    }
}

void set_config_value_double (char *key, double value) {
    if (__config) {
        char *value_chr = malloc(12*sizeof(char));
        sprintf(value_chr,"%.3f",value);
        config_entry *config = find_config_entry (key);
        if (!config) {
            add_config_entry (key, value_chr);
        } else {
            config->value = value_chr;
        }
    }
}

char *get_config_value (char *key, const char *dflt) {
    char *value = 0;
    if (__config) {
        config_entry *entry = find_config_entry (key);
        if (entry) {
            value = entry->value;
        }
    }

    if (!value) {
        value = (char *) dflt;
    } else {
        value = my_copystr(value);
    }

    return value;
}

int get_config_value_int(char *key, int dflt) {

    char *value_string = get_config_value(key, NULL);
    int value_int = 0;
    if (value_string) {
        log_info(BASE_CTX, "%s: %s\n", key, value_string);
        value_int = atoi(value_string);
    } else {
        value_int = dflt;
    }

    return value_int;

}

float get_config_value_float(char *key, float dflt) {

    char *value_string = get_config_value(key, NULL);
    float value_float = 0;
    if (value_string) {
        log_info(BASE_CTX, "%s: %s\n", key, value_string);
        value_float = (float) atof(value_string);
    } else {
        value_float = dflt;
    }

    return value_float;

}

double get_config_value_double(char *key, double dflt) {

    char *value_string = get_config_value(key, NULL);
    double value_double = 0;
    if (value_string) {
        log_info(BASE_CTX, "%s: %s\n", key, value_string);
        value_double = atof(value_string);
    } else {
        value_double = dflt;
    }

    return value_double;

}

void write_config () {
    if (__config) {
        FILE *f = fopen (__config->path, "w");
        int e = 0;
        for (e = 0; e < __config->no_of_entries; e++) {
            if (__config->entries[e]->value) {
                fprintf (f, "%s=%s\n", __config->entries[e]->key,
                         __config->entries[e]->value);
            } else {
                fprintf (f, "%s\n", __config->entries[e]->key);
            }
        }
        fclose (f);
    }
}

void
init_config_file(const char *appname) {
    if (appname) {

        __config = malloc (sizeof (__config_rec));
        __config->entries = 0;
        __config->no_of_entries = 0;

        user_home = getenv ("HOME");
        log_info (BASE_CTX, "Home: %s\n", user_home);
        const char *config_path = 0;
        if (user_home) {
            config_path = my_catstr (user_home, my_catstr ("/.", appname));
        } else {
            config_path = my_catstr ("/etc/", appname);
        }
        log_info (BASE_CTX, "Config path: %s\n", config_path);
        char *config_file_name = my_catstr (config_path, "/config");
        log_info (BASE_CTX, "Config file: %s\n", config_file_name);
        __config->path = config_file_name;

        int mkdir_res = mkdir (config_path, S_IRWXU | S_IRGRP | S_IROTH);
        if (mkdir_res == -1 && !(errno == EEXIST)) {
            char *error_str = strerror (errno);
            if (!error_str) {
                log_error (BASE_CTX, "Mkdir failed: %d\n", errno);
            } else {
                log_error (BASE_CTX, "Mkdir failed: %s\n", error_str);
            }
        } else {
            config_file = fopen (config_file_name, "r");
            if (!config_file) {
                char *error_str = strerror (errno);
                if (!error_str) {
                    log_error (BASE_CTX, "Could not open config file %s: %d\n",
                               config_file_name, errno);
                } else {
                    log_error (BASE_CTX, "Could not open config file %s: %s\n",
                               config_file_name, error_str);
                }
            } else {
                log_info (BASE_CTX, "Config file: %p\n", config_file);
                char *config_line = 0;
                size_t c = 0;

                while (getline (&config_line, &c, config_file) != -1) {
                    log_info (BASE_CTX, "%s", config_line);
                    char *key = strtok (config_line, "=\n");
                    if (key) {
                        char *value = strtok (0, "=\n");
//                        if (value) {
                            add_config_entry (key, value);
                            log_info (BASE_CTX, "%s: %s\n", key, value);
//                        }
                    }
                    free (config_line);
                    config_line = 0;
                    c = 0;
                }
                fclose (config_file);
            }
        }

    }
}

void
base_init (const char *appname, FILE *dflt_log_file, int log_level) {

    __app = my_copystr(appname);
    __log_prefix = NULL; //my_catstr(__app,": ");
    init_log_file (appname, dflt_log_file);

    _log_levels[0] = 2;
    init_config_file(appname);
    char *lls = get_config_value("log_levels", "1111111");
    int i = -1;
    while (lls[++i]) {
        char c[2];
        c[0] = lls[i];
        c[1] = 0;
        _log_levels[i] = atoi(c);
    }

    while (i < NUM_CTX) {
        _log_levels[i++] = 1;
    }

    log_info(BASE_CTX, "Log levels:\n");
    log_info(BASE_CTX, "\t%d", _log_levels[0]);
    for (i = 1; i < NUM_CTX; i++) {
        log_info(BASE_CTX, " %d", _log_levels[i]);
    }
    log_info(BASE_CTX, "\n");
    log_info (BASE_CTX, "Base init done\n");

}

void
base_close () {
    close_log_file ();
    /*write_config();*/
}

