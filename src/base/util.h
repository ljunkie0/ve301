#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>
#include <stddef.h>

void strshift(char *str, int shift, int offset);
void inschar(char *str, char c, int pos);
char *my_copystr(const char *str);
char *my_copynstr(const char *str, size_t max_length);

uint16_t *to_unicode(const char *txt, uint32_t *length, uint16_t **second_line, uint32_t *length2);
char *to_utf8(uint32_t utf, uint32_t *length);

char *my_catstr(const char *str1, const char *str2);
char *my_cat3str(const char *str1, const char *str2, const char *str3);
int my_strcmp(const char *str1, const char *str2);
char *my_strdup(const char *str);

#endif // UTIL_H
