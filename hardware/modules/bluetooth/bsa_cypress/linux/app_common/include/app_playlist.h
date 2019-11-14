/*****************************************************************************
**
**  Name:           app_playlist.h
**
**  Description:    Play list related functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/


#ifndef _APP_PLAYLIST_H
#define _APP_PLAYLIST_H

/*******************************************************************************
 **
 ** Function         app_create_play_list
 **
 ** Description      Function used to create a playlist from path
 **
 ** Returns          number of file in the play list
 **
 *******************************************************************************/
int app_playlist_create(const char *p_path, char ***ppp_playlist);

/*******************************************************************************
 **
 ** Function         app_playlist_free
 **
 ** Description      Function used to free a playlist
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_playlist_free(char **pp_playlist);

/*******************************************************************************
 **
 ** Function         app_playlist_sort
 **
 ** Description      Function used to sort a playlist
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_playlist_sort(char **pp_playlist, int nb_entry);

#endif

