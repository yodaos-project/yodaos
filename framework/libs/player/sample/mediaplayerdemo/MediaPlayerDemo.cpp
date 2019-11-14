#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <stdint.h>
#include "MediaPlayer.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/log.h>
}
#if 0
const char *test_sources[] = {
    "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.mp3",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.wav",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.aac",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.ape",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.flac",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.m4a",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.opus",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.wv",
        "http://build-6.dev.rokid-inc.com/~zhengxiong.yuan/test.ogg",
};
#endif

static int hasMediaName = 0;
static int useAsync = 0;

static int eventListener(void *opaque, int what, int arg1, int arg2, int from_thread)
{
    MediaPlayer *mp = (MediaPlayer *)opaque;
    printf("message [%d]\n", what);
    switch (what)
    {
        case MEDIA_PREPARED:
            break;
        case MEDIA_PLAYBACK_COMPLETE:
            if (useAsync)
                mp->stopAsync();
            else
                mp->stop();
            break;
        case MEDIA_ERROR:
            mp->stop();
            break;
        case MEDIA_INFO:
            break;
        case MEDIA_PAUSE:
            break;
        default:
            break;
    }

    return 0;
}

class MyListener : public MediaPlayerListener
{
public:
    MyListener(void *holder)
        : opaque(holder)
    {
    }
    void notify(int msg, int arg1, int arg2, int fromThread)
    {
        eventListener(opaque, msg, arg1, arg2, fromThread);
    }

private:
    void *opaque;
};

void help()
{
    const char *tip
        = "input [cmd] to handle player\n"
          "\t[+] add volume 5\n"
          "\t[-] sub volume 5\n"
          "\t[0] seek to  0\n"
          "\t[5] seek to   %50\n"
          "\t[8] seek to   %80\n"
          "\t[s] resume \n"
          "\t[p] pause \n"
          "\t[x] exit \n";
    printf("help:%s", tip);
}

int main(int argc, char **argv)
{
    av_log_set_level(48);
    if (argc < 2)
    {
        printf("usage: %s input_file [meidaname ] [sync interface]\n", argv[0]);
        return 1;
    }

    if (argc >= 3)
    {
        hasMediaName = 1;
        if (argc >= 4)
            useAsync = 1;
    }
    MediaPlayer *mp;
    if (hasMediaName)
        mp = new MediaPlayer(argv[2]);
    else
        mp = new MediaPlayer();
    mp->setDataSource(argv[1]);
    mp->setListener(new MyListener(mp));
    mp->setRecvBufSize(20480);
    mp->enableReconnectAtEOF(false);
    mp->setReconnectDelayMax(-5);
    if (useAsync)
    {
        mp->prepareAsync();
    }
    else
    {
        mp->prepare();
    }
    // system("date");
    mp->start();
    int duration = mp->getDuration();
    int volume = mp->getVolume();
    printf("duration %d volume %d\n", duration, volume);

    bool exit = false;
    while (!exit)
    {
        help();
        int c = fgetc(stdin);
        int v;

        switch (c)
        {
            case 'c':
                if (useAsync)
                    mp->stopAsync();
                else
                    mp->stop();
                break;
            case '+':
                volume += 5;
                mp->setVolume(volume);
                printf("now volume %d\n", mp->getVolume());
                break;
            case '-':
                volume -= 5;
                v = mp->setVolume(volume);
                printf("now volume %d\n", mp->getVolume());
                break;
            case 'p':
            case 'P':
                mp->pause();
                break;
            case 's':
                mp->resume();
                break;
            case '0':
                mp->seekTo(0);
                break;
            case '5':
                mp->seekTo(duration * 5 / 10);
                break;
            case '8':
                mp->seekTo(duration * 8 / 10);
                break;
            case 'x':
                exit = true;
                break;
            default:
                break;
        }
    }

    printf("test success !!!!!!!!!!!!!\n");
    return 0;
}
