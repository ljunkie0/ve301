#ifndef ALSA_H
#define ALSA_H
const int alsa_set_volume(const char *mixer, const int value);
const char **alsa_get_mixers(const char *mixer_device, int *n);
const int alsa_get_volume(const char *mixer);
#endif
