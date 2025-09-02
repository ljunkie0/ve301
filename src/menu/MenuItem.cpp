#include "MenuItem.h"

MenuItem::MenuItem(
        Menu *m,
        const char *label,
        const char *iconPath,
        const void *object,
        int objectType,
        const char *font,
        int fontSize,
        item_action *action,
        char *font2ndLine,
        int fontSize2ndLine) {

    init(m,
            label,
            iconPath,
            object,
            objectType,
            font,
            fontSize,
            action,
            font2ndLine,
            fontSize2ndLine);

}

MenuItem::MenuItem(Menu *m,
        const char *label) {
    init( /* m => */ m,
            /* label => */ label,
            /* iconPath => */ (char *) 0,
            /* object => */ (void *) 0,
            /* objectType => */ UNKNOWN_OBJECT_TYPE,
            /* font => */ (const char *) 0,
            /* fontSize => */ -1,
            /* action => */ (item_action *) 0,
            /* font2ndLine => */ (char *) 0,
            /* fontSize2ndLine => */ -1);
}

MenuItem::MenuItem(Menu *m,
        const char *label,
        const char *iconPath) {
    init( /* m => */ m,
            /* label => */ label,
            /* iconPath => */ iconPath,
            /* object => */ (void *) 0,
            /* objectType => */ UNKNOWN_OBJECT_TYPE,
            /* font => */ (const char *) 0,
            /* fontSize => */ -1,
            /* action => */ (item_action *) 0,
            /* font2ndLine => */ (char *) 0,
            /* fontSize2ndLine => */ -1);
}

void MenuItem::init(
        Menu *m,
        const char *label,
        const char *iconPath,
        const void *object,
        int objectType,
        const char *font,
        int fontSize,
        item_action *action,
        char *font2ndLine,
        int fontSize2ndLine) {
    this->m = m;
    this->handle = menu_item_new(m->handle,
            label,
            iconPath,
            object,
            objectType,
            font,
            fontSize,
            action,
            font2ndLine,
            fontSize2ndLine);
}

MenuItem::~MenuItem() {
    menu_item_free(this->handle);
}

void MenuItem::activate() {
    menu_item_activate(this->handle);
}

void MenuItem::warpTo() {
    menu_item_warp_to(this->handle);
}

void MenuItem::show() {
    menu_item_show(this->handle);
}

void MenuItem::updateLabel(const char *label) {
    menu_item_update_label(this->handle, label);
}

void MenuItem::updateIcon(const char *icon) {
    menu_item_update_icon(this->handle, icon);
}

void MenuItem::setVisible(const int visible) {
    menu_item_set_visible(this->handle, visible);
}

int MenuItem::isVisible() {
    menu_item_get_visible(this->handle);
}
