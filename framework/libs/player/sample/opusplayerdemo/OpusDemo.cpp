#include <string.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include "OpusPlayer.h"

static void help()
{
    const char* tip
        = "input [cmd] to handle player\n"
          "\t[p] play input opus file\n"
          "\t[e] encode input pcm file\n"
          "\t[r] reset opusplayer\n"
          "\t[x] exit \n";
    printf("help:%s", tip);
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        printf(
            "usage: %s [opusInputFileName | pcmInputFileName] [sampleRate(8|16|24|48)] [channels(1|2)] "
            "[bitrate(16000)] \n",
            argv[0]);
        return 1;
    }

    char* fileName = argv[1];

    OpusSampleRate rate = OpusSampleRate::SR_24K;
    int sampleRate = atoi(argv[2]);
    switch (sampleRate)
    {
        case 8:
            rate = OpusSampleRate::SR_8K;
            break;
        case 16:
            rate = OpusSampleRate::SR_16K;
            break;
        case 24:
            rate = OpusSampleRate::SR_24K;
            break;
        case 48:
            rate = OpusSampleRate::SR_48K;
            break;
        default:
            printf("The sample rate is wrong. Use default 24K.");
            break;
    }

    int channels = atoi(argv[3]);
    if (channels != 2 && channels != 1)
    {
        printf("The channels is wrong. Use default 1.");
        channels = 1;
    }

    int bitrate = atoi(argv[4]);
    if (bitrate != 16000)
    {
        printf("The bitrate is wrong. Use default 16000.");
        bitrate = 16000;
    }

    bool exit = false;

    static OpusPlayer* opusPlayer = new OpusPlayer(rate, channels, bitrate, (char*)"sample", (char*)"tts");

    while (!exit)
    {
        int cmd = fgetc(stdin);
        int size = 0;
        char* buffer = NULL;
        char outFile[80];

        if (cmd == 'p')
        {
            std::ifstream ifs(fileName, std::ifstream::in | std::ifstream::binary);
            ifs.seekg(0, std::ifstream::end);
            if (!ifs.good())
            {
                printf("Couldn't open opus file!\n");
                break;
            }

            size = ifs.tellg();
            buffer = new char[size];
            ifs.seekg(0, std::ifstream::beg);
            ifs.read(buffer, size);
            ifs.close();
            printf("opus Buffer =  %p; size =  %d\n", buffer, size);
        }

        if (cmd == 'e')
        {
            std::ifstream ifs(fileName, std::ifstream::in | std::ifstream::binary);
            ifs.seekg(0, std::ifstream::end);
            if (!ifs.good())
            {
                printf("Couldn't open pcm file!\n");
                break;
            }

            size = ifs.tellg();
            buffer = new char[size];
            ifs.seekg(0, std::ifstream::beg);
            ifs.read(buffer, size);
            ifs.close();

            strcpy(outFile, argv[1]);
            strcat(outFile, ".opus");
            printf("pcm Buffer =  %p; size =  %d\n", buffer, size);
        }

        switch (cmd)
        {
            case 'p':
                if (!opusPlayer)
                {
                    printf("opusPlayer is NULL!\n");
                }
                else
                {
                    opusPlayer->startOpusPlayer(buffer, size);
                    printf("opusPlayer plays over!");
                }
                break;
            case 'r':
                if (!opusPlayer)
                {
                    printf("opusPlayer is NULL!\n");
                }
                else
                {
                    opusPlayer->resetOpusPlayer();
                    printf("opusPlayer reset done!");
                }
                break;
            case 'e':
                if (!opusPlayer)
                {
                    printf("opusPlayer is NULL!\n");
                }
                else
                {
                    opusPlayer->encodePcm(buffer, size, outFile);
                    printf("opusPlayer encode pcm over!");
                }
                break;
            case 'x':
                exit = true;
                break;
            default:
                break;
        }

        if (buffer)
        {
            delete[] buffer;
        }
    }
    printf("test success !!!!!!!!!!!!!\n");
    return 0;
}
