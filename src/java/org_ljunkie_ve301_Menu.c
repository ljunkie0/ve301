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

#include"org_ljunkie_ve301_Menu.h"
#include "menu_jni.h"

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jlong JNICALL Java_org_ljunkie_ve301_Menu_menu_1new
(JNIEnv * env, jobject obj, jlong menu_ctrl_handle) {

    menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) menu_ctrl_handle;
    menu_ctrl *ctrl = ctrl_jni->ctrl;
    menu *m = menu_new(ctrl);
    menu_jni *m_jni = malloc(sizeof(menu_jni));
    m_jni->m = m;
    m_jni->j_menu = (*env)->NewGlobalRef(env,obj);
    m->object = m_jni;
    return (long) m_jni;

}

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jobject JNICALL Java_org_ljunkie_ve301_Menu_menu_1add_1sub_1menu
(JNIEnv *env, jobject obj, jlong menu_handle, jstring label, jlong sub_menu_handle) {

    menu_jni *m_jni = (menu_jni *) menu_handle;
    menu *m = m_jni->m;
    menu_jni *sub_m_jni = (menu_jni *) sub_menu_handle;
    menu *sub_m = sub_m_jni->m;
    const char *label_chr = (*env)->GetStringUTFChars(env,label,NULL);
    menu_item *sub_menu_item = menu_add_sub_menu(m,label_chr,sub_m);
    menu_item_jni *jni = malloc(sizeof(menu_item_jni));
    jni->item = sub_menu_item;
    jclass menu_item_class = (*env)->FindClass(env,MENU_ITEM_CLASS);
    jmethodID constructor_id = (*env)->GetMethodID(env,menu_item_class,"<init>","(J)V");
    jobject menu_item_object = (*env)->NewObject(env,menu_item_class,constructor_id,(long)jni);
    jni->j_menu_item = (*env)->NewGlobalRef(env,menu_item_object);

    return menu_item_object;

}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jobject JNICALL Java_org_ljunkie_ve301_Menu_get_1menu_1control
(JNIEnv *env, jobject obj, jlong menu_handle) {
    menu_jni *m_jni = (menu_jni *) menu_handle;
    menu *m = m_jni->m;
    menu_ctrl *ctrl = m->ctrl;
    menu_ctrl_jni *ctrl_jni = (menu_ctrl_jni *) ctrl->object;
    return ctrl_jni->j_menu_ctrl;
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_Menu_menu_1open
(JNIEnv *env, jobject obj, jlong menu_handle) {
    menu_jni *m_jni = (menu_jni *) menu_handle;
    menu_open(m_jni->m);
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jint JNICALL Java_org_ljunkie_ve301_Menu_menu_1get_1active_1id
(JNIEnv *env, jobject obj, jlong menu_handle) {
    menu_jni *m_jni = (menu_jni *) menu_handle;
    return m_jni->m->active_id;
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_Menu_menu_1set_1transient
(JNIEnv *env, jobject obj, jlong menu_handle, jint trans) {
    menu_jni *m_jni = (menu_jni *) menu_handle;
    m_jni->
        m->
        transient =
        (unsigned char) trans;
}

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
JNIEXPORT jint JNICALL Java_org_ljunkie_ve301_Menu_menu_1get_1transient
(JNIEnv *env, jobject obj, jlong menu_handle) {
    menu_jni *m_jni = (menu_jni *) menu_handle;
    return m_jni->m->transient;
}
