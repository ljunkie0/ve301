/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 */

#ifndef MENU_WEB_H
#define MENU_WEB_H

#include "../menu_ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct menu_web menu_web;

menu_web *menu_web_new(menu_ctrl *ctrl);
void menu_web_poll(menu_web *web, int timeout_ms);
void menu_web_free(menu_web *web);

#ifdef __cplusplus
}
#endif

#endif // MENU_WEB_H
