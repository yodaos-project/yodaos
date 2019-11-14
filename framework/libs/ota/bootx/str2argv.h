/*
 * Copyright (c) 2010, 2011 Ryan Flannery <ryan.flannery@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef STR2ARGV_H
#define STR2ARGV_H

#ifndef WIN32
#include <err.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* hard limits on the size of an argv and each entry/token w/in an argv */
#define ARGV_MAX_ENTRIES    255
#define ARGV_MAX_TOKEN_LEN  255

/*
 * Given a string (str), it parses it taking into account escape sequence (\),
 * quoting, etc., and builds an argc/argv style set of parameters that are
 * suitable for passing to any of the cmd_* or ecmd_* functions.
 *
 * This has been tricky for me.  It no doubt has bugs.
 * str2argv.c contains a small driver program that can be used for testing.
 * The Makefile contains the necessary build target "test_str2argv"
 */
int str2argv(char *str, int *argc, char ***argv, const char **errmsg);

/*
 * After the above function is used to build an argc/argv set of parameters,
 * this function should be used to free() all of the allocated memory.
 */
void argv_free(int *argc, char ***argv);

/*
 * This is used to un-tokenize an argv array.  Given argc/argv, this
 * constructs a string containing all of the tokens in order, with a single
 * space between each.  Tokens with multiple words are quoted.
 */
char *argv2str(int argc, char *argv[]);

#endif
