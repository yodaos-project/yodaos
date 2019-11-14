/*
 * Copyright (c) 2010, 2011, 2012 Ryan Flannery <ryan.flannery@gmail.com>
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

#include "str2argv.h"

#ifdef WIN32
#define err(flag, ...) printf(__VA_ARGS__)
#define errx(flag, ...) printf(__VA_ARGS__)
#endif

/* initialize empty argc/argv struct */
void
argv_init(int *argc, char ***argv)
{
   if ((*argv = (char **)calloc(ARGV_MAX_ENTRIES, sizeof(char*))) == NULL)
      err(1, "argv_init: argv calloc fail");

   if (((*argv)[0] = (char *)calloc(ARGV_MAX_TOKEN_LEN, sizeof(char))) == NULL)
      err(1, "argv_init: argv[i] calloc fail");

   memset((*argv)[0], 0, ARGV_MAX_TOKEN_LEN * sizeof(char));
   *argc = 0;
}

/* free all memory in an arc/argv */
void
argv_free(int *argc, char ***argv)
{
   int i;

   for (i = 0; i <= *argc; i++)
      free((*argv)[i]);

   free(*argv);
   *argc = 0;
}

/* add a character to the end of the current entry in an argc/argv */
void
argv_addch(int argc, char **argv, int c)
{
   int n;

   n = strlen(argv[argc]);
   if (n == ARGV_MAX_TOKEN_LEN - 1)
      errx(1, "argv_addch: reached max token length (%d)", ARGV_MAX_TOKEN_LEN);

   argv[argc][n] = c;
}

/* complete the current entry in the argc/argv and setup the next one */
void
argv_finish_token(int *argc, char ***argv)
{
   if (*argc == ARGV_MAX_ENTRIES - 1)
      errx(1, "argv_finish_token: reached max argv entries(%d)", ARGV_MAX_ENTRIES);

   if (strlen((*argv)[*argc]) == 0)
      return;

   *argc = *argc + 1;
   if (((*argv)[*argc] = (char *)calloc(ARGV_MAX_TOKEN_LEN, sizeof(char))) == NULL)
      err(1, "argv_finish_token: failed to calloc argv[i]");

   memset((*argv)[*argc], 0, ARGV_MAX_TOKEN_LEN * sizeof(char));
}

/*
 * Main parser used for converting a string (str) to an argc/argv style
 * parameter list.  This handles escape sequences and quoting.  Possibly
 * correctly.  :D
 * The argc/argv parameters passed are over-written.  After they have been
 * built by this function, the caller should use argv_free() on them to
 * free() all associated memory.
 * If the parsing goes correctly, 0 is returned.  Otherwise, 1 is returned
 * and the errmsg parameter is set to some appropriate error message and
 * both argc/argv are set to 0/NULL.
 */
int
str2argv(char *str, int *argc, char ***argv, const char **errmsg)
{
   bool in_token;
   bool in_container;
   bool escaped;
   char container_start;
   char c;
   int  len;
   int  i;

   const char *ERRORS[2] = {
      "Unmatched quotes",
      "Unused/Dangling escape sequence"
   };
   *errmsg = NULL;

   container_start = 0;
   in_token = false;
   in_container = false;
   escaped = false;

   len = strlen(str);

   argv_init(argc, argv);
   for (i = 0; i < len; i++) {
      c = str[i];

      switch (c) {
         /* handle whitespace */
         case ' ':
         case '\t':
         case '\n':
            if (!in_token)
               continue;

            if (in_container) {
               argv_addch(*argc, *argv, c);
               continue;
            }

            if (escaped) {
               escaped = false;
               argv_addch(*argc, *argv, c);
               continue;
            }

            /* if reached here, we're at end of token */
            in_token = false;
            argv_finish_token(argc, argv);
            break;

         /* handle quotes */
         case '\'':
         case '\"':

            if (escaped) {
               argv_addch(*argc, *argv, c);
               escaped = false;
               continue;
            }

            if (!in_token) {
               in_token = true;
               in_container = true;
               container_start = c;
               continue;
            }

            if (in_token && !in_container) {
               in_container = true;
               container_start = c;
               continue;
            }

            if (in_container) {
               if (c == container_start) {
                  in_container = false;
                  in_token = false;
                  argv_finish_token(argc, argv);
                  continue;
               } else {
                  argv_addch(*argc, *argv, c);
                  continue;
               }
            }

            *errmsg = ERRORS[0];
	    printf("warning fun(%s) line(%d):free inside function.\n", __func__, __LINE__);
	    fflush(stdout);
            argv_free(argc, argv);
            return 1;

         case '\\':
#ifdef WIN32
			// go through
#else
            if (in_container && str[i+1] != container_start) {
               argv_addch(*argc, *argv, c);
               continue;
            }

            if (escaped) {
               escaped = false;
               argv_addch(*argc, *argv, c);
               continue;
            }

            escaped = true;
            break;
#endif

         default:
            if (!in_token)
               in_token = true;

            if (escaped)
               escaped = false;

            argv_addch(*argc, *argv, c);
      }
   }
   argv_finish_token(argc, argv);

   if (in_container) {
      argv_free(argc, argv);
      *errmsg = ERRORS[0];
      printf("warning fun(%s) line(%d):free inside function.\n", __func__, __LINE__);
      fflush(stdout);
      return 1;
   }

   if (escaped) {
      argv_free(argc, argv);
      *errmsg = ERRORS[1];
      printf("warning fun(%s) line(%d):free inside function.\n", __func__, __LINE__);
      fflush(stdout);
      return 1;
   }

   (*argv)[*argc] = NULL;/*XXX*/

   return 0;
}

char *
argv2str(int argc, char *argv[])
{
   char *result;
   int   len;
   int   off;
   int   i;

   /* handle empty case */
   if (0 >= argc)
      return NULL;

   /* determine length of resulting string */
   len = 0;
   for (i = 0; i < argc; i++) {
      len += strlen(argv[i]) + 1;
      if (strstr(argv[i], " ") != NULL)
         len += 2;
   }

   /* allocate result */
   if ((result = (char *)calloc(len + 1, sizeof(char))) == NULL)
      err(1, "argv2str: calloc failed");
   memset(result, 0, len);

   /* build result */
   off = 0;
   for (i = 0; i < argc; i++) {
      if (strstr(argv[i], " ") == NULL)
         off += snprintf(result + off, len, "%s ", argv[i]);
      else
         off += snprintf(result + off, len, "\'%s\' ", argv[i]);
   }

   return result;
}
