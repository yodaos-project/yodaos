/* getprop.c - Get an system property
 *
 * Copyright 2015 The Android Open Source Project

config GETPROP
  bool "getprop"
  default y
  depends on property
  help
    usage: getprop [NAME [DEFAULT]]
    Gets an system property, or lists them all.
*/

#define FOR_getprop
#include <cutils/properties.h>
#include "getprop.h"

struct prop_list {
  size_t size;
  char **nv; // name/value pairs: even=name, odd=value
};

static struct prop_list TT;

static void add_property(char *name, char *value, void *unused)
{
  if (!(TT.size&31)) TT.nv = xrealloc(TT.nv, (TT.size+32)*2*sizeof(char *));

  TT.nv[2*TT.size] = xstrdup(name);
  TT.nv[1+2*TT.size++] = xstrdup(value);
}

int main(int argc, char** argv)
{
    if (argv[1]) {
        char value[PROP_VALUE_MAX];
        property_get(argv[1], value, argv[2] ? argv[2] : "");
        printf("%s\n", value);
    } else {
        size_t i;

        if (property_list((void *)add_property, 0)) error_exit("property_list");
        qsort(TT.nv, TT.size, 2*sizeof(char *), qstrcmp);
        for (i = 0; i<TT.size; i++) printf("[%s]: [%s]\n", TT.nv[i*2],TT.nv[1+i*2]);
    }
    return 0;
}

