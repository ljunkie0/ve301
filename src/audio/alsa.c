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

#include "alsa.h"
#include "../base/log_contexts.h"
#include "../base/logging.h"
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int alsa_set_volume(const char *mixer,const int value) {
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    long min, max;
   
    int e = snd_mixer_open(&handle, 0); 
    if (e < 0) {
	log_error(AUDIO_CTX, "Could not get mixer handle: %s\n", snd_strerror(e));
        return -1;
    }
    e = snd_mixer_attach(handle, "default");
    if (e < 0) {
	log_error(AUDIO_CTX, "Could attach to mixer: %s\n", snd_strerror(e));
        snd_mixer_close(handle);
        return -1;
    }
    e = snd_mixer_selem_register(handle, NULL, NULL);
    if (e < 0) {
	log_error(AUDIO_CTX, "Could register mixer: %s\n", snd_strerror(e));
        snd_mixer_close(handle);
        return -1;
    }
    e = snd_mixer_load(handle);
    if (e < 0) {
	log_error(AUDIO_CTX, "Could load mixer: %s\n", snd_strerror(e));
        snd_mixer_close(handle);
        return -1;
    }
    
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, mixer);
    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        snd_mixer_close(handle);
        return -1;
    }
    
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    long set_value = (value * (max - min) / 100) + min;
    snd_mixer_selem_set_playback_volume_all(elem, set_value);
    
    snd_mixer_close(handle);
    return 0;
}

const char **alsa_get_mixers(const char *mixer_device, int *n) {
    log_config(AUDIO_CTX, "alsa_get_mixers(%s)\n", mixer_device);
    snd_mixer_t *handle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    int count = 0;
    
    if (snd_mixer_open(&handle, 0) < 0) {
        return NULL;
    }
    if (snd_mixer_attach(handle, mixer_device) < 0) {
        snd_mixer_close(handle);
        return NULL;
    }
    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
        snd_mixer_close(handle);
        return NULL;
    }
    if (snd_mixer_load(handle) < 0) {
        snd_mixer_close(handle);
        return NULL;
    }
    
    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_selem_is_active(elem)
        	&& !snd_mixer_selem_has_common_volume(elem)
        	&& !snd_mixer_selem_is_enumerated(elem)
        	&& !snd_mixer_selem_has_capture_volume(elem)
        	&& !snd_mixer_selem_has_capture_switch(elem)
        ) {
            count++;
        }
    }

    const char **mixers = count > 0 ? malloc(count * sizeof(char *)) : NULL;
    if (mixers) {
        count = 0;
        for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
            if (snd_mixer_selem_is_active(elem) && !snd_mixer_selem_has_common_volume(elem)
                && !snd_mixer_selem_is_enumerated(elem) && !snd_mixer_selem_has_capture_volume(elem)
                && !snd_mixer_selem_has_capture_switch(elem)) {
                snd_mixer_selem_id_alloca(&sid);
                snd_mixer_selem_get_id(elem, sid);
                mixers[count] = strdup(snd_mixer_selem_id_get_name(sid));
                log_info(AUDIO_CTX, "Found mixer: %s\n", mixers[count]);
                count++;
            }
        }
    }

    *n = count;
    snd_mixer_close(handle);
    return mixers;
}

const int alsa_get_volume(const char *mixer) {
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    long min, max, vol;

   int e = snd_mixer_open(&handle, 0); 
    if (e < 0) {
        log_error(AUDIO_CTX, "Could not get mixer handle: %s\n", snd_strerror(e));
        return -1;
    }
    e = snd_mixer_attach(handle, "default");
    if (e < 0) {
        log_error(AUDIO_CTX, "Could attach to mixer: %s\n", snd_strerror(e));
        snd_mixer_close(handle);
        return -1;
    }
    e = snd_mixer_selem_register(handle, NULL, NULL);
    if (e < 0) {
        log_error(AUDIO_CTX, "Could register mixer: %s\n", snd_strerror(e));
        snd_mixer_close(handle);
        return -1;
    }
    e = snd_mixer_load(handle);
    if (e < 0) {
        log_error(AUDIO_CTX, "Could load mixer: %s\n", snd_strerror(e));
        snd_mixer_close(handle);
        return -1;
    }   
    
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, mixer);
    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        log_error(AUDIO_CTX, "Could not find mixer %s\n", mixer);
        snd_mixer_close(handle);
        return -1;
    }
    
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    log_config(AUDIO_CTX, "Volume range for mixer %s: %d - %d\n", mixer, min, max);
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    
    snd_mixer_close(handle);

    if (max-min == 0) {
        return 0;
    }

    return (int)((vol - min) * 100 / (max - min));

}
