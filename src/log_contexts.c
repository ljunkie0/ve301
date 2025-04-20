#include "log_contexts.h"

static char *__log_context_names[] = {
	"BASE",
	"MAIN",
	"MENU",
	"AUDIO",
	"ENCODER",
	"SDL",
	"WHEATHER",
	"BLUETOOTH",
	"SPOTIFY"
};

char *get_log_context_name(enum log_context context) {
	return __log_context_names[context];
}
