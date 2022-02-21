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

#include"menu_jni.h"

static char *menu_event_names[] = {
    "TURN_LEFT",
    "TURN_RIGHT",
    "ACTIVATE",
    "TURN_LEFT_1",
    "TURN_RIGHT_1",
    "ACTIVATE_1",
    "DISPOSE",
    "CLOSE"
};

int jni_menu_item_action(menu_event evt, menu *m, menu_item *item) {
    const menu_item_jni *item_jni = (const menu_item_jni *) item->object;
    if (item_jni) {
        menu *menu = item->menu;
        jobject listener = item_jni->item_listener;
        if (menu) {
            menu_ctrl *ctrl = menu->ctrl;
            if (ctrl) {
                menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) ctrl->object;
                if (ctrl_jni) {

                    if (!listener) {
                        listener = ctrl_jni->item_listener;
                    }

                    if (listener) {

                        JNIEnv *env;
                        (*ctrl_jni->g_vm)->AttachCurrentThread(ctrl_jni->g_vm, (void **)&env, NULL);

                        jclass item_listener_class = (*env)->FindClass(env,MENU_ITEM_LISTENER_CLASS);
                        if (item_listener_class) {
                            jmethodID method_id = (*env)->GetMethodID(env,item_listener_class,MENU_ITEM_LISTENER_METHOD,MENU_ITEM_LISTENER_SIGNATURE);
                            if (method_id) {

                                jobject j_menu_event = NULL;
                                jclass evt_class = (*env)->FindClass(env,MENU_EVENT_CLASS);
                                if (evt_class) {
                                    jmethodID evt_value_of_method_id = (*env)->GetStaticMethodID(env,evt_class,"valueOf",MENU_EVENT_VALUE_OF_SIGNATURE);
                                    if (evt_value_of_method_id) {
                                        const char *evt_nm = menu_event_names[evt];
                                        if (evt_nm) {
                                            jstring j_evt_nm = (*env)->NewStringUTF(env, evt_nm);
                                            j_menu_event = (*env)->CallStaticObjectMethod(env,evt_class,evt_value_of_method_id,j_evt_nm);
                                        }
                                    } else {
                                        log_warning(MENU_CTX, "Method valueOf%s in class %s not found\n",MENU_EVENT_VALUE_OF_SIGNATURE, MENU_EVENT_CLASS);
                                    }
                                } else {
                                    log_warning(MENU_CTX, "Event class %s not found\n",MENU_EVENT_CLASS);
                                }
                                (*env)->CallVoidMethod(env,listener,method_id,j_menu_event,item_jni->j_menu_item);
                            } else {
                                log_warning(MENU_CTX, "Method %s%s in class %s not found\n", MENU_ITEM_LISTENER_METHOD,MENU_ITEM_LISTENER_SIGNATURE, MENU_ITEM_LISTENER_CLASS);
                            }
                        } else {
                            log_warning(MENU_CTX, "Listener class %s not found\n", MENU_ITEM_LISTENER_CLASS);
                        }
                        (*ctrl_jni->g_vm)->DetachCurrentThread(ctrl_jni->g_vm);
                    } else {
                        log_warning(MENU_CTX, "no listener\n");
                    }

                } else {
                    log_warning(MENU_CTX, "ctrl has no corresponding jni object\n");
                }
            } else {
                log_warning(MENU_CTX, "menu has no ctrl\n");
            }
        } else {
            log_warning(MENU_CTX, "item has no menu\n");
        }
    } else {
        log_warning(MENU_CTX, "menu_item has no corresponding jni object\n");
    }
    return 0;
}

int jni_menu_ctrl_action(menu_event evt, menu *m, menu_item *item) {
    if (item) {
        return jni_menu_item_action(evt,m,item);
    } else if (m) {
        menu_ctrl *ctrl = m->ctrl;
        const menu_ctrl_jni *ctrl_jni = (const menu_ctrl_jni *) ctrl->object;
        if (ctrl_jni->item_listener) {

            JNIEnv *env;
            (*ctrl_jni->g_vm)->AttachCurrentThread(ctrl_jni->g_vm, (void **)&env, NULL);

            jclass item_listener_class = (*env)->FindClass(env,MENU_ITEM_LISTENER_CLASS);
            if (item_listener_class) {
                jmethodID method_id = (*env)->GetMethodID(env,item_listener_class,MENU_ITEM_LISTENER_METHOD,MENU_ITEM_LISTENER_SIGNATURE);
                if (method_id) {
                    jobject j_menu_event = NULL;
                    jclass evt_class = (*env)->FindClass(env,MENU_EVENT_CLASS);
                    if (evt_class) {
                        jmethodID evt_value_of_method_id = (*env)->GetStaticMethodID(env,evt_class,"valueOf",MENU_EVENT_VALUE_OF_SIGNATURE);
                        if (evt_value_of_method_id) {
                            const char *evt_nm = menu_event_names[evt];
                            if (evt_nm) {
                                jstring j_evt_nm = (*env)->NewStringUTF(env, evt_nm);
                                j_menu_event = (*env)->CallStaticObjectMethod(env,evt_class,evt_value_of_method_id,j_evt_nm);
                                log_info(MENU_CTX, "Menu event: %p\n", j_menu_event);
                                (*env)->CallVoidMethod(env,ctrl_jni->item_listener,method_id,j_menu_event,NULL);
                            } else {
                                log_warning(MENU_CTX, "Event with id %d not found\n", evt);
                            }
                        } else {
                            log_warning(MENU_CTX, "Method valueOf(%s) not found in class %s\n", MENU_EVENT_VALUE_OF_SIGNATURE, MENU_EVENT_CLASS);
                        }
                    } else {
                        log_warning(MENU_CTX, "Could not find event class %s\n", MENU_EVENT_CLASS);
                    }
                } else {
                    log_warning(MENU_CTX, "Method %s(%s) not found in class %s\n", MENU_ITEM_LISTENER_METHOD, MENU_ITEM_LISTENER_SIGNATURE,MENU_ITEM_LISTENER_CLASS);
                }
            } else {
                log_warning(MENU_CTX, "Class %s not found\n", MENU_ITEM_LISTENER_CLASS);
            }
            (*ctrl_jni->g_vm)->DetachCurrentThread(ctrl_jni->g_vm);
        } else {
            log_warning(MENU_CTX, "jni_menu_ctrl_action: No item listener\n");
        }
    }

    log_info(MENU_CTX, "<-- jni_menu_ctrl_action\n");
    return 0;

}
