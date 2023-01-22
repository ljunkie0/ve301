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
#include "org_ljunkie_ve301_MenuControl.h"
#include "menu_jni.h"

char *evt_name(menu_event evt) {
    if (evt == TURN_LEFT) {
        return "TURN_LEFT";
    } else if (evt ==TURN_RIGHT) {
        return "TURN_RIGHT";
    } else if (evt ==ACTIVATE) {
        return "ACTIVATE";
    } else if (evt ==TURN_LEFT_1) {
        return "TURN_LEFT_1";
    } else if (evt ==TURN_RIGHT_1) {
        return "TURN_RIGHT_1";
    } else if (evt ==ACTIVATE_1) {
        return "ACTIVATE_1";
    } else if (evt ==DISPOSE) {
        return "DISPOSE";
    } else if (evt ==CLOSE) {
        return "CLOSE";
    }
    return NULL;
}

int jni_menu_callback(menu_ctrl *ctrl) {
    if (ctrl) {

        menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) ctrl->object;
        if (!ctrl_jni) {
            log_error(MENU_CTX, "menu_ctrl has no ctrl_jni object\n");
            return 1;
        }

        if (ctrl_jni->callback_listener) {

            JNIEnv *env;
            (*ctrl_jni->g_vm)->AttachCurrentThread(ctrl_jni->g_vm, (void **)&env, NULL);
            jclass callback_class = (*env)->FindClass(env,MENU_CONTROL_LISTENER_CLASS);
            if (callback_class) {
                jmethodID method_id = (*env)->GetMethodID(env,callback_class,MENU_CONTROL_LISTENER_METHOD,MENU_CONTROL_LISTENER_SIGNATURE);
                (*env)->CallVoidMethod(env,ctrl_jni->callback_listener,method_id,ctrl_jni->j_menu_ctrl);
            } else {
                log_warning(MENU_CTX, "jni_menu_callback: callback class %s not found\n", MENU_CONTROL_LISTENER_CLASS);
            }

            (*ctrl_jni->g_vm)->DetachCurrentThread(ctrl_jni->g_vm);

        } else {
            log_warning(MENU_CTX, "jni_menu_callback: No callback listener\n");
        }
    } else {
        log_error(MENU_CTX, "jni_menu_callback: No ctrl\n");
    }

    return 0;

}

/*
 * Class:     org_ljunkie_ve301_MenuControl
 * Method:    menu_ctrl_new
 * Signature: (IIIIIIDLjava/lang/String;IJJ)I
 */
JNIEXPORT jlong JNICALL Java_org_ljunkie_ve301_MenuControl_menu_1ctrl_1new
(JNIEnv *env, jobject obj, jint w, jint x_offset, jint y_offset, jint radius_labels, jint radius_scales_start, jint radius_scales_end, jdouble angle_offset, jstring font, jint font_size, jobject action_listener, jobject call_back_listener) {

    menu_ctrl_jni *menu_ctrl_jni = malloc(sizeof(menu_ctrl_jni));
    menu_ctrl_jni->g_vm = malloc(sizeof(JavaVM));
    (*env)->GetJavaVM(env,&(menu_ctrl_jni->g_vm));

    const char *font_chr = (*env)->GetStringUTFChars(env,font,NULL);
    menu_ctrl *ctrl = menu_ctrl_new(w,x_offset,y_offset,radius_labels,1,radius_scales_start,radius_scales_end,angle_offset,font_chr,font_size,0,&jni_menu_ctrl_action,&jni_menu_callback);
    menu_ctrl_jni->ctrl = ctrl;
    ctrl->object = menu_ctrl_jni;

    menu_ctrl_jni->item_listener = NULL;
    menu_ctrl_jni->callback_listener = NULL;
    menu_ctrl_jni->j_menu_ctrl = (*env)->NewGlobalRef(env,obj);
    if (action_listener) {
        menu_ctrl_jni->item_listener = (*env)->NewGlobalRef(env,action_listener);
    }
    if (call_back_listener) {
        menu_ctrl_jni->callback_listener = (*env)->NewGlobalRef(env,call_back_listener);
    }

    menu *root = ctrl->root[0];
    menu_jni *root_jni = malloc(sizeof(menu_jni));
    root->object = root_jni;
    root_jni->m = root;
    jclass menu_class = (*env)->FindClass(env,MENU_CLASS);
    jmethodID constructor_id = (*env)->GetMethodID(env,menu_class,CONSTRUCTOR_METHOD,MENU_CONSTRUCTOR_WITH_HANDLE_SIGNATURE);
    jobject menu_object = (*env)->NewObject(env,menu_class,constructor_id,obj,(long)root_jni);
    root_jni->j_menu = (*env)->NewGlobalRef(env,menu_object);

    log_info(MENU_CTX, "Java_org_ljunkie_ve301_MenuControl_menu_1ctrl_1new() -> menu_ctrl_jni->ctrl = %p->%p\n", menu_ctrl_jni, ctrl);
    return (long) menu_ctrl_jni;

}

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_MenuControl_menu_1loop
(JNIEnv *env, jobject obj, jlong handle) {
    menu_ctrl_jni *ctrl = (menu_ctrl_jni *) handle;
    menu_ctrl_loop(ctrl->ctrl);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jobject JNICALL Java_org_ljunkie_ve301_MenuControl_menu_1ctrl_1get_1root
(JNIEnv *env, jobject obj, jlong menu_ctrl_handle) {

    menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) menu_ctrl_handle;
    const menu_jni *root_jni = ctrl_jni->ctrl->root[0]->object;
    return root_jni->j_menu;

}

JNIEXPORT jint JNICALL Java_org_ljunkie_ve301_MenuControl_menu_1set_1style
(JNIEnv *env, jobject obj, jlong menu_ctrl_handle, jstring background_color, jstring scale_color, jstring indicator_color, jstring default_color, jstring selected_color, jstring activated_color, jstring bg_image_path, jint bg_from_time) {
    char *background_color_chr = (char *) (*env)->GetStringUTFChars(env,background_color,NULL);
    char *scale_color_chr = (char *) (*env)->GetStringUTFChars(env, scale_color, NULL);
    char *indicator_color_chr = (char *) (*env)->GetStringUTFChars(env, indicator_color, NULL);
    char *default_color_chr = (char *) (*env)->GetStringUTFChars(env, default_color, NULL);
    char *selected_color_chr = (char *) (*env)->GetStringUTFChars(env, selected_color, NULL);
    char *activated_color_chr = (char *) (*env)->GetStringUTFChars(env, activated_color, NULL);
    char *bg_image_path_chr = NULL;
    if (bg_image_path) {
        bg_image_path_chr = (char *) (*env)->GetStringUTFChars(env, bg_image_path, NULL);
    }
    menu_ctrl_jni *ctrl = (menu_ctrl_jni *) menu_ctrl_handle;
    return menu_ctrl_set_style(ctrl->ctrl,background_color_chr,scale_color_chr,indicator_color_chr,default_color_chr,selected_color_chr,activated_color_chr,bg_image_path_chr,bg_from_time,1,0,0,0,NULL,0,NULL,0);
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jobject JNICALL Java_org_ljunkie_ve301_MenuControl_get_1active_1menu
(JNIEnv *env, jobject obj, jlong menu_ctrl_handle) {
    menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) menu_ctrl_handle;
    menu_ctrl *ctrl = ctrl_jni->ctrl;
    menu *active = ctrl->active;
    if (active) {
        const menu_jni *active_jni = (const menu_jni *) active->object;
        return active_jni->j_menu;
    }
    return NULL;
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_MenuControl_menu_1ctrl_1quit
(JNIEnv *env, jobject obj, jlong menu_ctrl_handle) {
    menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) menu_ctrl_handle;
    menu_ctrl *ctrl = ctrl_jni->ctrl;
    menu_ctrl_quit(ctrl);
}
