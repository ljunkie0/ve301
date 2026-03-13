/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE

#include "config.h"
#include "log_contexts.h"
#include "logging.h"
#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *config_path = NULL;

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

static __config_rec *__config = NULL;

void free_config_entry(config_entry *entry) {
    free_and_set_null((void **) &entry->config_file_name);
    free_and_set_null((void **) &entry->group);
    free_and_set_null((void **) &entry->key);
    free_and_set_null((void **) &entry->value);
    free_and_set_null((void **) &entry);
}

void free_config() {
    for (int e = 0; e < __config->no_of_entries; e++) {
        free_config_entry(__config->entries[e]);
    }
    free_and_set_null((void **) &__config->entries);
    free_and_set_null((void **) &__config->path);
    free_and_set_null((void **) &__config);
}

config_entry *find_config_entry(const __config_rec *config, const char *key, const char *group) {
    config_entry *entry = 0;
    int e = 0;
    while (!entry && e < config->no_of_entries) {
        if (!strcmp(key, config->entries[e]->key)) {
            if ((!config->entries[e]->group && !group)
                || (config->entries[e]->group && group
                    && !strcmp(group, config->entries[e]->group))) {
                entry = config->entries[e];
            }
        }
        e++;
    }
    return entry;
}

config_entry *add_config_entry(__config_rec *config,
                               const char *config_file_name,
                               const char *key,
                               const char *value,
                               const char *group) {
    config_entry *e = find_config_entry(config, key, group);
    if (!e) {
        e = malloc(sizeof(config_entry));
        e->id = config->no_of_entries++;
        e->key = my_copystr(key);
        config->entries = realloc(config->entries, config->no_of_entries * sizeof(config_entry *));
        config->entries[config->no_of_entries - 1] = e;
    } else {
        free_and_set_null((void **) &e->config_file_name);
        free_and_set_null((void **) &e->group);
        free_and_set_null((void **) &e->value);
    }

    e->config_file_name = my_copystr(config_file_name);

    if (value) {
        e->value = my_copystr(value);
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

char *get_config_value_group(char *key, const char *dflt, const char *group) {
    char *value = NULL;
    if (__config) {
        config_entry *entry = find_config_entry(__config, key, group);
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

void config_value_group(char *buffer, char *key, const char *dflt, const char *group) {
    char *value = NULL;
    if (__config) {
        config_entry *entry = find_config_entry(__config, key, group);
        if (entry) {
            value = entry->value;
        }
    }

    if (!value) {
        if (dflt) {
            value = (char *) dflt;
        }
    }

    if (value) {
        strncpy(buffer, value, MAX_CONFIG_LINE_LENGTH - 1);
    } else {
        buffer[0] = 0;
    }
}

char *get_directory(const char *path) {
    char *last_sep = strrchr(path, '/');
    char *s = (char *) path;
    char *directory = NULL;
    int pl = 0;
    while (s != last_sep) {
        pl = pl + 1;
        directory = (char *) realloc(directory, pl * sizeof(char));
        directory[pl - 1] = *s;
        s = s + 1;
    }
    directory = (char *) realloc(directory, (pl + 2) * sizeof(char));
    directory[pl] = '/';
    directory[pl + 1] = 0;
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

    return absolute_path;
}

char *get_config_value_path_group(char *key, const char *dflt, const char *group) {
    char *value = NULL;
    config_entry *entry = NULL;
    if (__config) {
        entry = find_config_entry(__config, key, group);
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
        char *v = value;
        value = get_absolute_path(directory, value);
        free(v);
        free(directory);
    }

    return value;
}

void config_value_path_group(char *buffer, char *key, const char *dflt, const char *group) {
    char *value = NULL;
    config_entry *entry = NULL;
    if (__config) {
        entry = find_config_entry(__config, key, group);
        if (entry) {
            value = entry->value;
        }
    }

    if (!value) {
        if (dflt) {
            value = (char *) dflt;
        }
    }

    if (value) {
        if (entry) {
            char *directory = get_directory(entry->config_file_name);
            char *abs_path = get_absolute_path(directory, value);
            strncpy(buffer, abs_path, MAX_CONFIG_LINE_LENGTH);
            free(abs_path);
            free(directory);
        } else {
            strncpy(buffer, value, MAX_CONFIG_LINE_LENGTH);
        }
    } else {
        buffer[0] = 0;
    }
}

char *get_config_value_path(char *key, const char *dflt) {
    return get_config_value_path_group(key, dflt, NULL);
}

void config_value_path(char *buffer, char *key, const char *dflt) {
    config_value_path_group(buffer, key, dflt, NULL);
}

char *get_config_value(char *key, const char *dflt) {
    return get_config_value_group(key, dflt, NULL);
}

void config_value(char *buffer, char *key, const char *dflt) {
    config_value_group(buffer, key, dflt, NULL);
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
    return get_config_value_int_group(key, dflt, NULL);
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
    return get_config_value_double_group(key, dflt, NULL);
}

void write_config() {
    if (__config) {
        FILE *f = fopen(__config->path, "w");
        int e = 0;
        char *current_group = NULL;
        for (e = 0; e < __config->no_of_entries; e++) {
            if (__config->entries[e]->group) {
                if (!current_group || strcmp(current_group, __config->entries[e]->group)) {
                    current_group = __config->entries[e]->group;
                    fprintf(f, "[%s]\n", current_group);
                }
            }

            if (__config->entries[e]->value) {
                fprintf(f, "%s=%s\n", __config->entries[e]->key, __config->entries[e]->value);
            } else {
                fprintf(f, "%s\n", __config->entries[e]->key);
            }
        }
        fclose(f);
    }
}

void read_config_file(const char *config_file_name) {
    log_info(BASE_CTX, "Config file: %s\n", config_file_name);
    FILE *config_file = fopen(config_file_name, "r");
    if (!config_file) {
        char *error_str = strerror(errno);
        if (!error_str) {
            log_error(BASE_CTX, "Could not open config file %s: %d\n", config_file_name, errno);
        } else {
            log_error(BASE_CTX, "Could not open config file %s: %s\n", config_file_name, error_str);
        }
    } else {
        log_debug(BASE_CTX, "Config file: %p\n", config_file);
        char *config_path = NULL;

        char *config_line = NULL;
        size_t c = 0;
        char *current_group = NULL;

        while (getline(&config_line, &c, config_file) != -1) {
            if (c > 0) {
                log_info(BASE_CTX, "%s", config_line);
                if (config_line[0] == '[') {
                    char *group = strtok(config_line, "[]\n");
                    if (group) {
                        if (current_group) {
                            free(current_group);
                        }
                        current_group = my_copystr(group);
                        log_info(BASE_CTX, "Group: %s\n", group);
                    }
                } else {
                    char *key = strtok(config_line, "=\n");
                    if (key) {
                        char *value = strtok(NULL, "=\n");

                        // value == null -> include?
                        if (value == NULL && !strncmp(key, "include", 7)) {
                            strtok(key, " ");
                            char *p = strtok(NULL, " ");
                            if (p) {
                                char *path = NULL;
                                if (p[0] == '/') {
                                    path = p;
                                } else {
                                    if (config_path == NULL) {
                                        config_path = get_directory(config_file_name);
                                    }
                                    path = my_catstr(config_path, p);
                                }

                                read_config_file(path);
                                free(path);

                            } else {
                                log_error(BASE_CTX, "include without file");
                            }
                        } else {
                            config_entry *e = add_config_entry(__config,
                                                               config_file_name,
                                                               key,
                                                               value,
                                                               current_group);
                            if (e->value) {
                                log_info(BASE_CTX, "%s: %s\n", key, value);
                            }
                        }
                    }
                }
            }
        }

        free(config_line);

        if (config_path) {
            free(config_path);
        }

        if (current_group) {
            free(current_group);
        }

        fclose(config_file);
    }
}

void init_config_file(const char *appname) {
    if (appname) {
        __config = malloc(sizeof(__config_rec));
        __config->entries = NULL;
        __config->no_of_entries = 0;

        const char *user_home = getenv("HOME");
        log_info(BASE_CTX, "Home: %s\n", user_home);
        if (user_home && strlen(user_home) > 0) {
            char *config_path1 = my_catstr("/.", appname);
            config_path = my_catstr(user_home, config_path1);
            free(config_path1);
        } else {
            config_path = my_catstr("/etc/", appname);
        }
        log_info(BASE_CTX, "Config path: %s\n", config_path);
        char *config_file_name = my_catstr(config_path, "/config");
        __config->path = config_file_name;

        int mkdir_res = mkdir(config_path, S_IRWXU | S_IRGRP | S_IROTH);
        if (mkdir_res == -1 && !(errno == EEXIST)) {
            char *error_str = strerror(errno);
            if (!error_str) {
                log_error(BASE_CTX, "Mkdir failed: %d\n", errno);
            } else {
                log_error(BASE_CTX, "Mkdir failed: %s\n", error_str);
            }
        } else {
            read_config_file(config_file_name);
        }
        free((char *) config_path);
    }
}
