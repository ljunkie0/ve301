#ifndef CONFIG_H
#define CONFIG_H

#define MAX_CONFIG_LINE_LENGTH 255

void set_config_value(char *key, char *value);
void set_config_value_int(char *key, int value);

char *get_config_value(char *key, const char *dflt);
void config_value(char *buffer, char *key, const char *dflt);
char *get_config_value_group(char *key, const char *dflt, const char *group);
void config_value_group(char *buffer, char *key, const char *dflt, const char *group);
int get_config_value_int(char *key, int dflt);
int get_config_value_int_group(char *key, int dflt, const char *group);
double get_config_value_double(char *key, double dflt);
char *get_config_value_path_group(char *key, const char *dflt, const char *group);
void config_value_path_group(char *buffer, char *key, const char *dflt, const char *group);
char *get_config_value_path(char *key, const char *dflt);
void config_value_path(char *buffer, char *key, const char *dflt);
void init_config_file(const char *appname);
void write_config();
void free_config();

#endif // CONFIG_H
