#include "navigation.h"
#include "../radio_browser/menu.h"
#include "../podcast/menu.h"

#include <stddef.h>

static radio_app_navigation_hook *navigation_hooks[] = {
    &radio_browser_attach_navigation_menu,
    &podcast_attach_navigation_menu,
};

void radio_app_attach_navigation_hooks(const radio_app_navigation_context *ctx) {
    for (size_t i = 0; i < sizeof(navigation_hooks) / sizeof(navigation_hooks[0]); i++) {
        navigation_hooks[i](ctx);
    }
}
