#ifndef WAVPLAYER_H
#define WAVPLAYER_H
/**
 * Preload wav files into memory. You can load up FILE_COUNT_MAX wav files at most.
 * If the number of cache files is more than FILE_COUNT_MAX, you can only update the existing cache files.
 * If the file size is more than FILE_SIZE_MAX, it won't be preloaded.
 * This is interface is optional. Users can call prepareWavPlayer and startWavPlayer directly.
 *
 * @param filenames
 *        the file names array which will be preloaded.
 *
 * @param num
 *        number of files.
 *
 * @return int
 *         return the status, 0 : success, < 0 : failure.
 */
extern int prePrepareWavPlayer(const char* fileNames[], int num);

/**
 * Erase the specified loaded file from cache files.
 *
 * @param filePath
 *        specify the file name to be removed from cache files.
 *
 * @return int
 *         return the status, 0 : success, < 0 : failure.
 */
extern int eraseCacheFile(const char* filePath = NULL);

/**
 * Clean up cache files.
 */
extern void clearCacheFile();

/**
 * Setup player.
 *
 * @param filePath
 *         specify the file to be played.
 *
 * @param tag
 *        the volume stream type of the player.
 *
 * @param holdConnect
 *        whether to hold links with the pulseaduio server,
 *        maintaining links requires 3% of CPU and establishing a link is 100 milliseconds.
 *
 * @return int
 *         return the status, 0 : success, < 0 : failure.
 */
extern int prepareWavPlayer(const char* filePath = NULL, const char* tag = NULL, bool holdConnect = true);

/**
 * Start playing.
 *
 * @return int
 *         return the status, 0 : success, < 0 : failure.
 */
extern int startWavPlayer();

/**
 * Stop playing.
 */
extern void stopWavPlayer();
#endif
