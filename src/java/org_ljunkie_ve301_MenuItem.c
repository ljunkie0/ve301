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

#include<pthread.h>
#include "menu_jni.h"
#include"org_ljunkie_ve301_MenuItem.h"
/* Header for class org_ljunkie_ve301_MenuItem */

#undef org_ljunkie_ve301_MenuItem_UNKNOWN_OBJECT_TYPE
#define org_ljunkie_ve301_MenuItem_UNKNOWN_OBJECT_TYPE -1L
#undef org_ljunkie_ve301_MenuItem_OBJECT_TYPE_ACTION
#define org_ljunkie_ve301_MenuItem_OBJECT_TYPE_ACTION -2L

//#pragma GCC diagnostic ignored "-Wunused-parameter"
//int jni_menu_item_action(menu_event evt, menu *m, menu_item *item) {

//    if (item) {
//        log_info("jni_menu_item_action: %s\n", item->utf8_label);
//        const menu_item_jni *item_jni = (const menu_item_jni *) item->object;
//        if (item_jni) {
//            log_info("jni_menu_item_action(%s): item_jni->item = %p->%p item_jni->j_menu_item = %p->%p\n", item->utf8_label, item_jni, item_jni->item, item_jni, item_jni->j_menu_item);
//            if (item_jni->item_listener) {
//                menu_ctrl *ctrl = (menu_ctrl *) m->ctrl;
//                menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) ctrl->object;
//                JNIEnv *env;
//                (*ctrl_jni->g_vm)->AttachCurrentThread(ctrl_jni->g_vm, (void **)&env, NULL);

//                jclass item_listener_class = (*env)->GetObjectClass(env,item_jni->item_listener);
//                if (item_listener_class) {
//                    jmethodID method_id = (*env)->GetMethodID(env,item_listener_class,MENU_ITEM_LISTENER_METHOD,MENU_ITEM_LISTENER_SIGNATURE);
//                    if (method_id) {
//                        if (item_jni->j_menu_item) {
//                            log_info("jni_menu_item_action: calling method (*env)->CallVoidMethod(env -> %p, item_jni->item_listener -> %p, method_id -> %p, item_jni->j_menu_item -> %p\n", env, item_jni->item_listener, method_id, item_jni->j_menu_item);
//                            (*env)->CallVoidMethod(env,item_jni->item_listener,method_id,item_jni->j_menu_item);
//                        } else {
//                            log_warning("item_jni->j_menu_item is null\n");
//                        }
//                    }
//                }

//                (*ctrl_jni->g_vm)->DetachCurrentThread(ctrl_jni->g_vm);
//            } else {
//                log_warning("jni_menu_item_action: no listener\n");
//            }
//        } else {
//            log_warning("jni_menu_item_action: item has no corresponding jni object\n");
//        }
//    } else {
//        log_warning("jni_menu_item_action: item is null\n");
//    }

//    return 0;

//}

/*
 * Class:     org_ljunkie_ve301_MenuItem
 * Method:    menu_item_new
 * Signature: (JLjava/lang/String;JILjava/lang/String;I)J
 */
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jlong JNICALL Java_org_ljunkie_ve301_MenuItem_menu_1item_1new
(JNIEnv *env, jobject obj, jlong menu_handle, jstring label, jlong object_handle, jint object_type, jstring font, jint font_size) {

    const char *label_chr = (*env)->GetStringUTFChars(env,label,NULL);
    const char *font_chr = (*env)->GetStringUTFChars(env,font,NULL);
    menu_jni *m_jni = (menu_jni *) menu_handle;
    menu_item *item = menu_item_new(m_jni->m, label_chr, (void *)object_handle, object_type, font_chr, font_size, NULL, NULL, -1);
    menu_item_jni *item_jni = malloc(sizeof(menu_item_jni));
    item_jni->item = item;
    item_jni->j_menu_item = (*env)->NewGlobalRef(env,obj);
    item_jni->item_listener = NULL;
    item->object = item_jni;
    log_trace(MENU_CTX, "Java_org_ljunkie_ve301_MenuItem_menu_1item_1new(%s): item_jni->item = %p->%p item_jni->j_menu_item = %p->%p\n", label_chr, item_jni, item_jni->item, item_jni, item_jni->j_menu_item);
    return (long) item_jni;

}

JNIEXPORT void JNICALL Java_org_ljunkie_ve301_MenuItem_menu_1item_1update_1label
(JNIEnv *env, jobject obj, jlong handle, jstring label) {

    const char *label_chr = (*env)->GetStringUTFChars(env, label, NULL);
    menu_item_jni *item_jni = (menu_item_jni *) handle;
    menu_item_update_label(item_jni->item,label_chr);

}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_MenuItem_menu_1item_1set_1listener
(JNIEnv *env, jobject obj, jlong menu_item_handle, jobject item_listener) {
    menu_item_jni *item_jni = (menu_item_jni *) menu_item_handle;
    if (item_jni->item_listener) {
        (*env)->DeleteGlobalRef(env,item_jni->item_listener);
    }
    item_jni->item->object_type = OBJECT_TYPE_ACTION;
    item_jni->item_listener = (*env)->NewGlobalRef(env,item_listener);
    item_jni->item->action = &jni_menu_item_action;

}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_MenuItem_menu_1item_1warp_1to
(JNIEnv *env, jobject obj, jlong menu_item_handle) {
    menu_item_jni *item_jni = (menu_item_jni *) menu_item_handle;
    menu_item *item = item_jni->item;
    menu_item_warp_to(item);
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_MenuItem_menu_1item_1show
(JNIEnv *env, jobject obj, jlong menu_item_handle) {
    menu_item_jni *item_jni = (menu_item_jni *) menu_item_handle;
    menu_item *item = item_jni->item;
    menu_item_show(item);
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jint JNICALL Java_org_ljunkie_ve301_MenuItem_is_1sub_1menu
(JNIEnv *env, jobject obj, jlong menu_item_handle) {
    menu_item_jni *item_jni = (menu_item_jni *) menu_item_handle;
    menu_item *item = item_jni->item;
    return item->is_sub_menu;
}

