/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/properties.h>
#include <utils/Log.h>

int main(int argc, char** argv) {
    char name[PROP_NAME_MAX];
    char value[PROP_VALUE_MAX];

    property_get("ro.rokid.build.platform", value, "0");
    printf("ro.rokid.build.platform=%s\n", value);
    property_get("ro.rokid.build.version.release", value, "0");
    printf("ro.rokid.build.version.release=%s\n", value);
    property_get("ro.rokid.test.default", value, "0");
    printf("ro.rokid.test.default=%s\n", value);
    property_get("ro.rokid.test.build", value, "0");
    printf("ro.rokid.test.build=%s\n", value);
    property_set("sweet.fan.test", "tian");
    property_get("sweet.fan.test", value, "0");
    printf("sweet.fan.test = %s\n", value);
    property_get("persist.sweet.fan.test", value, "0");
    printf("before persist.sweet.fan.test =%s\n", value);
    property_set("persist.sweet.fan.test", "persist");
    property_get("persist.sweet.fan.test", value, "0");
    printf("persist.sweet.fan.test =%s\n", value);
    return 0;
}
