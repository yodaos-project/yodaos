/* setprop.c - Set an system property
 *
 * Copyright 2015 The Android Open Source Project

config SETPROP
  bool "setprop"
  default y
  depends on propery
  help
    usage: setprop NAME VALUE

    Sets an system property.
*/

#define FOR_setprop

#include <cutils/properties.h>

int main(int argc, char** argv)
{
    char *name = argv[1], *value = argv[2];
    char *p;
    if (!name || !value) goto err;
    size_t name_len = strlen(name), value_len = strlen(value);

    // property_set doesn't tell us why it failed, and actually can't
    // recognize most failures (because it doesn't wait for init), so
    // we duplicate all of init's checks here to help the user.

    if (name_len >= PROP_NAME_MAX) {
        printf("name '%s' too long; try '%.*s'\n",
            name, PROP_NAME_MAX - 1, name);
        goto err;
    }
    if (value_len >= PROP_VALUE_MAX) {
        printf("value '%s' too long; try '%.*s'\n",
            value, PROP_VALUE_MAX - 1, value);
        goto err;
    }

    if (*name == '.' || name[name_len - 1] == '.') {
        printf("property names must not start or end with '.'\n");
        goto err;
    }
    if (strstr(name, "..")) {
        printf("'..' is not allowed in a property name\n");
        goto err;
    }
    for (p = name; *p; ++p) {
        if (!isalnum(*p) && !strchr("_.-", *p)) {
            printf("invalid character '%c' in name '%s'\n", *p, name);
            goto err;
        }
    }

    if (property_set(name, value)) {
        printf("failed to set property '%s' to '%s'\n", name, value);
    }

    return 0;
err:
    return -1;
}

