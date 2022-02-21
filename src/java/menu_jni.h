/*
 * Copyright 2022 LJunkie
 * http://www.???.??
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../base.h"
#include "../menu.h"
#include <jni.h>

#ifndef MENU_JNI_H
#define MENU_JNI_H

#define CONSTRUCTOR_METHOD "<init>"
#define MENU_CLASS "org/ljunkie/ve301/Menu"
#define CONSTRUCTOR_WITH_HANDLE_SIGNATURE "(J)V"
#define MENU_CONSTRUCTOR_WITH_HANDLE_SIGNATURE "(Lorg/ljunkie/ve301/MenuControl;J)V"
#define MENU_EVENT_CLASS "org/ljunkie/ve301/MenuEvent"
#define MENU_EVENT_VALUE_OF_SIGNATURE "(Ljava/lang/String;)Lorg/ljunkie/ve301/MenuEvent;"
#define MENU_ITEM_CLASS "org/ljunkie/ve301/MenuItem"
#define MENU_ITEM_LISTENER_CLASS "org/ljunkie/ve301/MenuItemListener"
#define MENU_ITEM_LISTENER_METHOD "itemActivated"
#define MENU_ITEM_LISTENER_SIGNATURE "(Lorg/ljunkie/ve301/MenuEvent;Lorg/ljunkie/ve301/MenuItem;)V"
#define MENU_CONTROL_LISTENER_CLASS "org/ljunkie/ve301/MenuControlListener"
#define MENU_CONTROL_LISTENER_METHOD "callback"
#define MENU_CONTROL_LISTENER_SIGNATURE "(Lorg/ljunkie/ve301/MenuControl;)V"

typedef struct menu_ctrl_jni {
    menu_ctrl *ctrl;
    jobject j_menu_ctrl;
    jobject item_listener;
    jobject callback_listener;
    jobject aux_listener;
    JavaVM *g_vm;
} menu_ctrl_jni;

typedef struct menu_item_jni {
    menu_item *item;
    jobject item_listener;
    jobject j_menu_item;
} menu_item_jni;

typedef struct menu_jni {
    menu *m;
    jobject j_menu;
} menu_jni;

int jni_menu_ctrl_action(menu_event evt, menu *m, menu_item *item);
int jni_menu_item_action(menu_event evt, menu *m, menu_item *item);

#endif
