#define _GNU_SOURCE

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *my_copynstr(const char *str, size_t max_length) {
    char *res = NULL;
    if (str) {
        res = malloc((max_length + 1) * sizeof(char));
        strncpy(res, str, max_length);
        res[max_length] = '\0';
    }
    return res;
}

char *my_catstr(const char *str1, const char *str2) {
    int l1 = strlen(str1);
    int l2 = strlen(str2);
    int l = l1 + l2;
    char *res = malloc((l + 1) * sizeof(char));
    strncpy(res, str1, l1);
    strncpy(res + l1, str2, l2);
    res[l] = 0;
    return res;
}

char *my_cat3str(const char *str1, const char *str2, const char *str3) {
    int l1 = strlen(str1);
    int l2 = strlen(str2);
    int l3 = strlen(str3);
    int l = l1 + l2 + l3;
    char *res = malloc((l + 1) * sizeof(char));
    strncpy(res, str1, l1);
    strncpy(res + l1, str2, l2);
    strncpy(res + l1 + l2, str3, l3);
    res[l] = 0;
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

    return strcmp(str1, str2);
}

char *my_strdup(const char *str) {
    if (str) {
        return strdup(str);
    }
    return NULL;
}

/**
 * Encode a code point using UTF-8
 *
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @license MIT
 *
 * @param out - output buffer (min 5 characters), will be 0-terminated
 * @param utf - code point 0-0x10FFFF
 * @return number of bytes on success, 0 on failure (also produces U+FFFD, which
 * uses 3 bytes)
 */
char *to_utf8(uint32_t utf, uint32_t *length) {
    char *out = NULL;
    if (utf <= 0x7F) {
        // Plain ASCII
        out = malloc(2 * sizeof(char));
        out[0] = (char) utf;
        out[1] = 0;
        if (length)
            *length = 1;
    } else if (utf <= 0x07FF) {
        // 2-byte unicode
        out = malloc(3 * sizeof(char));
        out[0] = (char) (((utf >> 6) & 0x1F) | 0xC0);
        out[1] = (char) (((utf >> 0) & 0x3F) | 0x80);
        out[2] = 0;
        if (length)
            *length = 2;
    } else if (utf <= 0xFFFF) {
        // 3-byte unicode
        out = malloc(4 * sizeof(char));
        out[0] = (char) (((utf >> 12) & 0x0F) | 0xE0);
        out[1] = (char) (((utf >> 6) & 0x3F) | 0x80);
        out[2] = (char) (((utf >> 0) & 0x3F) | 0x80);
        out[3] = 0;
        if (length)
            *length = 3;
    } else if (utf <= 0x10FFFF) {
        // 4-byte unicode
        out = malloc(5 * sizeof(char));
        out[0] = (char) (((utf >> 18) & 0x07) | 0xF0);
        out[1] = (char) (((utf >> 12) & 0x3F) | 0x80);
        out[2] = (char) (((utf >> 6) & 0x3F) | 0x80);
        out[3] = (char) (((utf >> 0) & 0x3F) | 0x80);
        out[4] = 0;
        if (length)
            *length = 4;
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
                u = (uint16_t) (u << 6 * l);

                int b = 0;
                for (b = 1; b <= l; b++) {
                    uint16_t n = (uint16_t) (unsigned char) txt[i++];
                    n = n & 0x3f;
                    n = (uint16_t) (n << 6 * (l - b));
                    u = u | n;
                }
            }

            if (u == 10) {
                line = 1;
                s = 0;
            } else {
                if (line == 0) {
                    if (max_length == 0 || s < max_length) {
                        u_txt = realloc(u_txt, (++s) * sizeof(uint16_t));
                        u_txt[s - 1] = u;
                        s1 = s;
                    } else if (!second_line) {
                        break;
                    }
                } else if (second_line) {
                    if (max_length > 0 && *length2 && s >= *length2) {
                        break;
                    }
                    *second_line = realloc(*second_line, (++s) * sizeof(uint16_t));
                    (*second_line)[s - 1] = u;
                    s2 = s;
                }
            }
        }

        u_txt = realloc(u_txt, (s1 + 1) * sizeof(uint16_t));
        u_txt[s1] = 0;

        if (length2 && s2 > 0) {
            *second_line = realloc(*second_line, (s2 + 1) * sizeof(uint16_t));
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

void strshift(char *str, int shift, int offset) {
    for (int s = strlen(str); s > offset; s--) {
        str[s + shift] = str[s];
    }
}

void inschar(char *str, char c, int pos) {
    strshift(str, 1, pos);
    str[pos] = c;
}

unsigned int unicode_len(const unsigned short *txt) {
    if (!txt) {
        return 0;
    }
    unsigned int len = 0;
    while (txt[len])
        len++;
    return len;
}

unsigned short *unicode_copy(const unsigned short *txt) {
    unsigned short *new_txt = malloc((unicode_len(txt) + 1) * sizeof(unsigned short));
    unsigned int len = 0;
    while (txt[len]) {
        new_txt[len] = txt[len];
        len++;
    }
    new_txt[len] = 0;
    return new_txt;
}
