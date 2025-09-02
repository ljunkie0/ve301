/*
 * Copyright 2022 LJunkie
 * https://github.com/ljunkie0/ve301
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
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>

#define INTERNET_CHECK_HOST "www.google.com"

static pthread_t __internet_thread = 0;
static int __internet_thread_run = 0;
static int __internet_available = 0;

FILE *log_file;
char *user_home;
const char *config_path = 0;
static char *__app;
static char *__log_prefix;

static int _log_levels[NUM_CTX];

static char *__log_level_colors[] = {
	"\x1b[0m", 	// NO LEVEL
	"\x1b[31m", 	// ERROR
	"\x1b[33m",	// WARNING
	"\x1b[35m",	// INFO
	"\x1b[0m",	// CONFIG
	"\x1b[0m",	// DEBUG
	"\x1b[0m"	// TRACE
};

typedef struct config_entry {
    char *key;
    char *value;
    char *group;
    char *config_file_name;
    int id;
} config_entry;

typedef struct __config_rec {
    char *path;
    config_entry **entries;
    int no_of_entries;
} __config_rec;

__config_rec *__config = 0;

char *my_copystr (const char *str) {
    return strdup(str);
}

char *my_copynstr (const char *str, unsigned int max_length) {
    char *res = calloc (max_length+1,sizeof (char));
    strncpy (res, str, max_length);
    return res;
}

char *my_catstr (const char *str1, const char *str2) {
    char *res = malloc ((strlen (str1) + strlen (str2) + 1) * sizeof (char));
    strcpy (res, str1);
    strcpy (res + strlen (str1), str2);
    return res;
}

char *my_cat3str (const char *str1, const char *str2, const char *str3) {
    char *res = malloc ((strlen (str1) + strlen (str2) + strlen(str3) + 1) * sizeof (char));
    strcpy (res, str1);
    strcpy (res + strlen (str1), str2);
    strcpy (res + strlen (str1) + strlen(str2), str3);
    return res;
}

int my_strcmp(const char *str1, const char *str2) {
	if (!str1) {
		if (str2) {
			return -1;
		}
		return 0;
	}

	if (!str2) {
		return 1;
	}

	return strcmp(str1,str2);

}

void free_time_check_interval(time_check_interval *i) {
    free(i);
}

time_check_interval *time_check_interval_new(int check_seconds) {
    time_check_interval *i = malloc(sizeof(time_check_interval));
    i->last_checked = 0;
    i->check_seconds = check_seconds;
    return i;
}

int check_time_interval(time_check_interval *i) {
    time_t timer;
    time(&timer);
    if (timer - i->last_checked > i->check_seconds) {
        i->last_checked = timer;
        return 1;
    }
    return 0;
}

void time_check_interval_set_check_seconds(time_check_interval *i, int check_seconds) {
    i->check_seconds = check_seconds;
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

network_interfaces *get_network_interfaces() {

    struct ifaddrs* ptr_ifaddrs = NULL;
    int r = getifaddrs(&ptr_ifaddrs);

    if (r) {
        return 0;
    }

    network_interfaces *interfaces = malloc(sizeof(network_interfaces));
    interfaces->n = 0;
    interfaces->interfaces = NULL;

    for (struct ifaddrs* ptr_entry = ptr_ifaddrs; ptr_entry; ptr_entry = ptr_entry->ifa_next) {
        sa_family_t address_family = ptr_entry->ifa_addr->sa_family;
        if( address_family == AF_INET ) {
            interfaces->n++;
            interfaces->interfaces = realloc(interfaces->interfaces,interfaces->n*sizeof(network_interface));
            network_interface *interface = malloc(sizeof(network_interface));
            interfaces->interfaces[interfaces->n-1] = interface;

            interface->ifname = my_copystr(ptr_entry->ifa_name);
            if( ptr_entry->ifa_addr){
                char buffer[INET_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in*)(ptr_entry->ifa_addr))->sin_addr,
                    buffer,
                    INET_ADDRSTRLEN
                    );
                interface->ipaddress = my_copystr(buffer);

                log_info(BASE_CTX,"%s: %s\n", interface->ifname, interface->ipaddress);

            }
        }

    }

    freeifaddrs(ptr_ifaddrs);

    return interfaces;
}

void free_network_interface(network_interface *interface) {
    if (interface) {
        if (interface->ifname) {
            free(interface->ifname);
        }
        if (interface->ipaddress) {
            free(interface->ipaddress);
        }
        free(interface);
    }
}

int __check_internet() {

    log_debug(BASE_CTX, "Checking internet...\n");
    const char *hostname = INTERNET_CHECK_HOST;
    log_debug(BASE_CTX, "1\n");
    struct addrinfo *result;
    log_debug(BASE_CTX, "2\n");
    int s = getaddrinfo(hostname,0,0,&result);
    log_debug(BASE_CTX, "Reply: %d\n", s);

    int reply = 0;
    if (s) {
        log_error(BASE_CTX, "Error checking internet: %s\n", gai_strerror(s));
    } else {
        freeaddrinfo(result);
        reply = 1;
    }

    log_config(BASE_CTX, "Internet available: %d\n", reply);

    return reply;
}

void *__internet_thread_start(void *arg) {
    log_info(BASE_CTX, "Internet thread successfully started\n");

    const struct timespec duration = {
        0,
        50000000
    };

    time_t timer;
    time(&timer);

    while (__internet_thread_run) {

        nanosleep(&duration, NULL);

        time_t timer2;
        time(&timer2);

        time_t diff = timer2 - timer;

        if (diff > 5) {
            timer = timer2;
            __internet_available = __check_internet();
        }

    }

    log_info(BASE_CTX, "Internet thread finished\n");
    return NULL;
}

void __start_internet_thread() {
    __internet_thread_run = 1;
    int r = pthread_create(&__internet_thread, NULL, __internet_thread_start, NULL);
    if (r) {
        __internet_thread_run = 0;
        log_error(BASE_CTX, "Could not start internet thread: %d\n", r);
    }
}

void __stop_internet_thread() {
    if (__internet_thread_run) {
        __internet_thread_run = 0;
        void *res;
        pthread_join(__internet_thread, &res);
    }
}

int check_internet() {
    if (!__internet_thread_run) {
        __internet_available = __check_internet();
        __start_internet_thread();
    }
    return __internet_available;
}

void init_log_file (const char *appname, FILE *dflt_log_file) {

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

int log_level_enabled(const int log_ctx, const int lvl) {
    return log_file && (_log_levels[log_ctx] >= lvl);
}

void
close_log_file () {
    if (log_file && (log_file != stderr) && (log_file != stdout)) {
        fclose (log_file);
    }
}

void log_variable_args (const int log_ctx, const int lvl, const char *__restrict __format,
                   va_list args) {

    if (!log_file) log_file = stderr;

    if (log_file && (_log_levels[log_ctx] >= lvl)) {
	  int fl = strlen(__format);
	  char *log_context_name = get_log_context_name(log_ctx);
	  char *log_level_color = __log_level_colors[lvl]; 
	  int cl = strlen(log_context_name);
	  int ll = strlen(log_level_color);
	  int rl = strlen(__log_level_colors[0]); 
	  char ctx_format[ll + cl + 2 + fl + rl + 1];
	  memcpy(ctx_format,log_level_color,ll);
	  memcpy(ctx_format+ll,log_context_name,cl);
	  ctx_format[ll+cl] = ':';
	  ctx_format[ll+cl+1] = ' ';
	  memcpy(ctx_format+ll+cl+2,__format,fl);
	  memcpy(ctx_format+ll+cl+2+fl,__log_level_colors[0],rl);
	  ctx_format[ll + cl + 2 + fl + rl] = 0;
          vfprintf (log_file, ctx_format, args);
    }
}

void __log_error (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_ERROR, __format, args);
    va_end (args);
}

void __log_warning (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_WARNING, __format, args);
    va_end (args);
}

void __log_info (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_INFO, __format, args);
    va_end (args);
}

void __log_config (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_CONFIG, __format, args);
    va_end (args);
}

void __log_debug (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_DEBUG, __format, args);
    va_end (args);
}

void __log_trace (const int log_ctx, const char *__restrict __format, ...) {
    va_list args;
    va_start (args, __format);
    log_variable_args (log_ctx, IR_LOG_LEVEL_TRACE, __format, args);
    va_end (args);
}

void free_config_entry(config_entry *entry) {

    log_config(BASE_CTX, "Freeing entry %s=%s\n", entry->key, entry->value);

    if (entry->config_file_name) {
        log_config(BASE_CTX, "entry->config_file_name = %p\n", entry->config_file_name);
        free(entry->config_file_name);
    }
    if (entry->group) {
        log_config(BASE_CTX, "entry->group = %p\n", entry->group);
        free(entry->group);
    }
    if (entry->key) {
        log_config(BASE_CTX, "entry->key = %p\n", entry->key);
        free(entry->key);
    }
    if (entry->value) {
        log_config(BASE_CTX, "entry->value = %p\n", entry->value);
        free(entry->value);
    }

    log_config(BASE_CTX, "...Done\n");

}

void free_config() {
    for (int e = 0; e < __config->no_of_entries; e++) {
        free_config_entry(__config->entries[e]);
    }
}

config_entry *
find_config_entry (const __config_rec *config, const char *key, const char *group) {
    config_entry *entry = 0;
    int e = 0;
    while (!entry && e < config->no_of_entries) {
        if (!strcmp (key, config->entries[e]->key)) {
            if ((!config->entries[e]->group && !group) || (config->entries[e]->group && group && !strcmp (group, config->entries[e]->group))) {
               entry = config->entries[e];
            }
        }
        e++;
    }
    return entry;
}

config_entry *add_config_entry (__config_rec *config, const char *config_file_name, const char *key, const char *value, const char *group) {
    config_entry *e = find_config_entry(config, key, group);
    if (e == NULL) {
        e = malloc (sizeof (config_entry));
        e->id =  config->no_of_entries++;
        config->entries = realloc (config->entries,
                                    config->no_of_entries * sizeof (config_entry *));
        config->entries[config->no_of_entries - 1] = e;
    } else {
        if (e->config_file_name) {
            free(e->config_file_name);
        }
        if (e->group) {
            free(e->group);
            e->group = NULL;
        }
        free(e->key);
        if (e->value) {
            free(e->value);
            e->value = NULL;
        }

    }

    e->config_file_name = my_copystr(config_file_name);

    e->key = my_copystr (key);
    if (value) {
        e->value = my_copystr (value);
    } else {
        e->value = NULL;
    }

    if (group) {
        e->group = my_copystr(group);
    } else {
        e->group = NULL;
    }

    return e;
}

char *get_config_value_group (char *key, const char *dflt, const char *group) {
    char *value = 0;
    if (__config) {
        config_entry *entry = find_config_entry (__config, key, group);
        if (entry) {
            value = entry->value;
        }
    }

    if (!value) {
        if (dflt) {
            value = my_copystr((char *) dflt);
        }
    } else {
        value = my_copystr(value);
    }

    return value;
}

char *get_directory(const char *path) {
    char *last_sep = strrchr(path,'/');
    char *s = (char *) path;
    char *directory = NULL;
    int pl = 0;
    while (s != last_sep) {
        pl = pl + 1;
        directory = (char *) realloc(directory, pl * sizeof(char));
        directory[pl-1] = *s;
        s = s + 1;
    }
    directory = (char *) realloc(directory, (pl+2) * sizeof(char));
    directory[pl] = '/';
    directory[pl+1] = 0;
    return directory;
}

char *get_absolute_path(char *directory, char *path) {
    char *absolute_path = NULL;
    if (path && path[0] != '/') {
        char *tmp = my_catstr("/", path);
        absolute_path = my_catstr(directory, tmp);
        free(tmp);
    } else {
        absolute_path = my_copystr(path);
    }

    if (path) {
        free(path);
    }

    return absolute_path;

}

char *get_config_value_path_group (char *key, const char *dflt, const char *group) {
    char *value = NULL;
    config_entry *entry = NULL;
    if (__config) {
        entry = find_config_entry (__config, key, group);
        if (entry) {
            value = entry->value;
        }
    }

    if (!value) {
        if (dflt) {
            value = my_copystr((char *) dflt);
        }
    } else {
        value = my_copystr(value);
    }

    if (value && entry) {
        char *directory = get_directory(entry->config_file_name);
        value = get_absolute_path(directory, value);
        free(directory);
    }

    return value;
}

char *get_config_value_path (char *key, const char *dflt) {
    return get_config_value_path_group(key,dflt,NULL);
}

char *get_config_value (char *key, const char *dflt) {
    return get_config_value_group(key,dflt,NULL);
}

int get_config_value_int_group(char *key, int dflt, const char *group) {
    char *value_string = get_config_value_group(key, NULL, group);
    int value_int = 0;
    if (value_string) {
        log_info(BASE_CTX, "%s: %s\n", key, value_string);
        value_int = atoi(value_string);
        free(value_string);
    } else {
        value_int = dflt;
    }

    return value_int;

}

int get_config_value_int(char *key, int dflt) {
    return get_config_value_int_group(key,dflt,NULL);
}

float get_config_value_float_group(char *key, float dflt, const char *group) {

    char *value_string = get_config_value_group(key, NULL, group);
    float value_float = 0;
    if (value_string) {
        log_info(BASE_CTX, "%s: %s\n", key, value_string);
        value_float = (float) atof(value_string);
        free(value_string);
    } else {
        value_float = dflt;
    }

    return value_float;

}

float get_config_value_float(char *key, float dflt) {
    return get_config_value_float_group(key,dflt,NULL);
}

double get_config_value_double_group(char *key, double dflt, const char *group) {

    char *value_string = get_config_value_group(key, NULL, group);
    double value_double = 0;
    if (value_string) {
        log_info(BASE_CTX, "%s: %s\n", key, value_string);
        value_double = atof(value_string);
        free(value_string);
    } else {
        value_double = dflt;
    }

    return value_double;

}

double get_config_value_double(char *key, double dflt) {
    return get_config_value_float_group(key,dflt,NULL);
}

void write_config () {
    if (__config) {
        FILE *f = fopen (__config->path, "w");
        int e = 0;
        char *current_group = NULL;
        for (e = 0; e < __config->no_of_entries; e++) {

            if (__config->entries[e]->group) {
                if (!current_group || strcmp(current_group,__config->entries[e]->group)) {
                    current_group = __config->entries[e]->group;
                    fprintf (f, "[%s]\n", current_group);
                }
            }

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

void read_config_file(const char *config_file_name) {
    log_info (BASE_CTX, "Config file: %s\n", config_file_name);
    FILE *config_file = fopen (config_file_name, "r");
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
        log_debug (BASE_CTX, "Config file: %p\n", config_file);
        char *config_path = NULL;

        char *config_line = NULL;
        size_t c = 0;
        char *current_group = NULL;

        while (getline (&config_line, &c, config_file) != -1) {
            if (c > 0) {
                log_info (BASE_CTX, "%s", config_line);
                if (config_line[0] == '[') {
                    char *group = strtok (config_line, "[]\n");
                    if (group) {
                        current_group = my_copystr(group);
                        log_info (BASE_CTX, "Group: %s\n", group);
                    }
                } else {
                    char *key = strtok (config_line, "=\n");
                    if (key) {
                        char *value = strtok (NULL, "=\n");

                        // value == null -> include?
                        if (value == NULL && !strncmp(key,"include",7)) {
                            strtok (key, " ");
                            char *p = strtok(NULL, " ");
                            if (p) {
                                char *path = NULL;
                                if (p[0] == '/') {
                                    path = p;
                                } else {
                                    if (config_path == NULL) {
                                        config_path = get_directory(config_file_name);
                                    }
                                    path = my_catstr(config_path,p);
                                }

                                read_config_file(path);
                                free(path);

                            } else {
                                log_error(BASE_CTX, "include without file");
                            }
                        } else {
                            config_entry *e = add_config_entry (__config, config_file_name, key, value, current_group);
                            if (e->value) {
                                log_info (BASE_CTX, "%s: %s\n", key, value);
                            }
                        }
                    }
                }
            }
            if (config_line) {
                free(config_line);
            }
            c = 0;
        }

        if (config_path) {
            free(config_path);
        }

        if (current_group) {
            free(current_group);
        }

        fclose (config_file);
    }
}

void init_config_file(const char *appname) {
    if (appname) {

        __config = malloc (sizeof (__config_rec));
        __config->entries = 0;
        __config->no_of_entries = 0;

        user_home = getenv ("HOME");
        log_info (BASE_CTX, "Home: %s\n", user_home);
        if (user_home && strlen(user_home) > 0) {
            config_path = my_catstr (user_home, my_catstr ("/.", appname));
        } else {
            config_path = my_catstr ("/etc/", appname);
        }
        log_info (BASE_CTX, "Config path: %s\n", config_path);
        char *config_file_name = my_catstr (config_path, "/config");
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
            read_config_file(config_file_name);
        }
        free((char *) config_path);

    }
}

void base_init (const char *appname, FILE *dflt_log_file, int log_level) {

    __app = my_copystr(appname);
    __log_prefix = NULL; 
    init_log_file (appname, dflt_log_file);

    init_config_file(appname);
    char *lls = get_config_value("log_levels", "11111111");
    int i = -1;
    while (lls[++i]) {
        char c[2];
        c[0] = lls[i];
        c[1] = 0;
        _log_levels[i] = atoi(c);
    }
    free(lls);

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
    __stop_internet_thread();
    free_config();
    close_log_file ();
    /*write_config();*/
}

