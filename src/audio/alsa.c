#include "alsa.h"
#include "../base.h"
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

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
    
    const char **mixers = malloc(count * sizeof(char *));
    count = 0;
    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
        if (snd_mixer_selem_is_active(elem)
        	&& !snd_mixer_selem_has_common_volume(elem)
        	&& !snd_mixer_selem_is_enumerated(elem)
        	&& !snd_mixer_selem_has_capture_volume(elem)
        	&& !snd_mixer_selem_has_capture_switch(elem)
        ) {
            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_get_id(elem, sid);
            mixers[count] = strdup(snd_mixer_selem_id_get_name(sid));
	    log_info(AUDIO_CTX, "Found mixer: %s\n", mixers[count]);
            count++;
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
        log_error(AUDIO_CTX, "Could not find  mixer %s\n", mixer);
        snd_mixer_close(handle);
        return -1;
    }
    
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    log_config(AUDIO_CTX, "Volume range for mixer %s: %d - %d\n", mixer, min, max);
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    
    snd_mixer_close(handle);
    return (int)((vol - min) * 100 / (max - min));

}
