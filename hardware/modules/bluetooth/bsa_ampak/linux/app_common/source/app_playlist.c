/*****************************************************************************
**
**  Name:           app_playlist.c
**
**  Description:    Play list related functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>

#include "app_playlist.h"
#include "app_utils.h"
#include "app_wav.h"


/*******************************************************************************
 **
 ** Function         app_playlist_file_supported
 **
 ** Description      Function used to get and check AV file type
 **
 ** Returns          TRUE if the file is supported for read operations
 **
 *******************************************************************************/
static BOOLEAN app_playlist_file_supported(const char *p_fname)
{
    char *p_char;
    tAPP_WAV_FILE_FORMAT wav_format;
    char low_case_ext[10];

    /* Find the last . in the string */
    p_char = strrchr(p_fname, '.');

    /* If '.' found */
    if (p_char != NULL)
    {
        /* Skip the . */
        p_char++;
        /* Copy the extension */
        strncpy(low_case_ext, p_char, sizeof(low_case_ext));
        low_case_ext[sizeof(low_case_ext) - 1] = '\0';
        /* Convert to lower case */
        p_char = &low_case_ext[0];
        while (*p_char != '\0')
        {
            *p_char = tolower((unsigned)*p_char);
            p_char++;
        }

        /* Check if extension is wav */
        if (!strcmp(low_case_ext, "wav"))
        {
            /* Check wav file format (to verify that it's a real wav file) */
            if (app_wav_format(p_fname, &wav_format) == 0)
            {
                return TRUE;
            }
            APP_ERROR1("Unable to extract WAV format from:%s", p_fname);
        }
    }
    return FALSE;
}


/*******************************************************************************
 **
 ** Function         app_playlist_create
 **
 ** Description      Function used to create a playlist from path
 **
 ** Returns          number of file in the play list
 **
 *******************************************************************************/
int app_playlist_create(const char *p_path, char ***ppp_playlist)
{
#if !defined (BSA_ANDROID) || (BSA_ANDROID == FALSE)
    DIR             *p_directory;
    struct dirent   *p_directory_entry;
    char            **pp_playlist = NULL;
    int             nb_entry = 0;
    int             index;
    char            full_path[1000];

    /* Check ppp_playlist parameter */
    if (ppp_playlist == NULL)
    {
        APP_ERROR0("app_create_play_list ppp_playlist NULL");
        return -1;
    }
    /* Open path directory */
    p_directory = opendir(p_path);
    if (p_directory == NULL)
    {
        APP_ERROR1("opendir(%s) failed: %d", p_path, errno);
        perror("app_create_play_list:");
        return -1;
    }

    do
    {
        /* Read one element of the directory */
        p_directory_entry = readdir(p_directory);
        if (p_directory_entry != NULL)
        {
            snprintf(full_path, sizeof(full_path), "%s/%s", p_path, p_directory_entry->d_name);
            /* Check if this file extension is supported */
            if (app_playlist_file_supported(full_path))
            {
                /* (Re)allocate the playlist with the appropriate size */
                pp_playlist = realloc(pp_playlist, (nb_entry + 1) * sizeof(char *));
                if (pp_playlist == NULL)
                {
                    APP_ERROR1("realloc failed: %d", errno);
                    closedir(p_directory);
                    return -1;
                }

                APP_DEBUG1("New file Audio file found:%s", p_directory_entry->d_name);
                /* Allocate memory to store filename */
                index = strlen(full_path);
                pp_playlist[nb_entry] = malloc(index + 1);
                /* Copy filename */
                strncpy(pp_playlist[nb_entry], full_path, index);
                /* Add a trailing 0 */
                pp_playlist[nb_entry][index] = 0;
                /* One more entry */
                nb_entry++;
            }
            /* Jump to next file */
            seekdir(p_directory, telldir(p_directory));
        }
    } while(p_directory_entry != NULL);

    /* Close directory (free memory) */
    closedir(p_directory);

    app_playlist_sort(pp_playlist, nb_entry);

    /* Return the pointer to the playlist */
    *ppp_playlist = pp_playlist;

    return nb_entry;
#else
    return 0;
#endif
}

/*******************************************************************************
 **
 ** Function         app_playlist_free
 **
 ** Description      Function used to free a playlist
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_playlist_free(char **pp_playlist)
{
    int index;

    if (pp_playlist != NULL)
    {
        for (index = 0 ; pp_playlist[index] != NULL ; index++)
        {
            free(pp_playlist[index]);
        }
    }
    free(pp_playlist);
}

/*******************************************************************************
 **
 ** Function         app_playlist_sort
 **
 ** Description      Function used to sort a playlist
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_playlist_sort(char **pp_playlist, int nb_entry)
{
    int swapped;
    int index;
    char *p_tmp;

    /* Sanity check */
    if (pp_playlist == NULL)
    {
        return;
    }

    do
    {
        /* Default => nothing swapped */
        swapped = 0;

        /* For every file (- 1) */
        for (index = 0 ; index < (nb_entry - 1); index++)
        {
            /* If this name is > the next one */
            if (strcmp(pp_playlist[index], pp_playlist[index + 1]) > 0)
            {
                /* Swap filename in play list */
                p_tmp = pp_playlist[index];
                pp_playlist[index] = pp_playlist[index + 1];
                pp_playlist[index + 1] = p_tmp;
                swapped = 1;
            }
        }
    /* Let's do it until no swap occurs */
    } while (swapped == 1);
}

