/*
 * VE301
 *
 * Copyright (C) 2024 LJunkie <christoph.pickart@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "org_ljunkie_ve301_Application.h"
#include "menu_jni.h"

/*
 * Class:     org_ljunkie_ve301_Application
 * Method:    init_application
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_ljunkie_ve301_Application_init_1application
(JNIEnv *env, jobject obj, jstring application_name, jint log_level) {
    const char *app_name = (*env)->GetStringUTFChars(env,application_name,NULL);
    base_init(app_name,stderr,log_level);
}
