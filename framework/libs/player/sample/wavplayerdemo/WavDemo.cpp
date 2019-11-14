#include <string.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "WavPlayer.h"

void guidance()
{
    const char* help[] = {
        "******** 1 : prePrepareWavPlayer awake wav ********",
        "******** 2 : play once have been loaded wavfile,cut connect *******",
        "******** 3 : play more than one have been loaded wavfile,cut connect *********",
        "******** 4 : play more than one have been loaded wavfile, hold connect *********",
        "******** 5 : play more than one no load wavfile,(the same format),hold connect *********",
        "******** 6 : play more than one no load wavfile,(different format),hold connect *********",
        "******** 7 : play more than one no load wavfile,(different format),cut connect *********",
        "******** 8 : twice play, have been loaded wavfile,The last time interrupt before. hold connect *********",
        "******** 9 : twice play,play 1 ->stop 1 -> play 2, hold connect*********",
        "******* q : exit ********",
    };
    for (int i = 0; i < 9; i++)
    {
        printf("%s\n", help[i]);
    }
}

int main(int argc, char** argv)
{
    guidance();
    const char* fileNames[] = { "/opt/media/awake_01.wav", "/opt/media/awake_02.wav", "/opt/media/awake_03.wav",
                                "/opt/media/awake_04.wav", "/opt/media/hibernate.wav" };

    bool exit = false;
    while (!exit)
    {
        int c = fgetc(stdin);
        int k = 2;
        int i = 0;
        switch (c)
        {
            case '1':
                prePrepareWavPlayer(fileNames, 5);
                break;
            case '2':
                prepareWavPlayer("/opt/media/awake_01.wav", NULL, true);
                startWavPlayer();
                break;
            case '3':
                for (i = 0; i < 5; i++)
                {
                    prepareWavPlayer(fileNames[i], NULL, false);
                    startWavPlayer();
                    usleep(10 * 1000);
                }
                usleep(10 * 1000);

                break;
            case '4':
                for (i = 0; i < 5; i++)
                {
                    prepareWavPlayer(fileNames[i], NULL, true);
                    startWavPlayer();
                    usleep(10 * 1000);
                }
                break;
            case '5':
                for (i = 0; i < 5; i++)
                {
                    prepareWavPlayer("/opt/media/volume.wav", NULL, false);
                    startWavPlayer();
                    usleep(10 * 1000);
                }
                break;
            case '6':
                prepareWavPlayer(fileNames[4], NULL, true);
                startWavPlayer();
                prepareWavPlayer("/opt/media/volume.wav", NULL, false);
                startWavPlayer();
                break;
            case '7':
                prepareWavPlayer(fileNames[4], NULL, false);
                startWavPlayer();
                usleep(10 * 1000);
                prepareWavPlayer("/opt/media/volume.wav", NULL, false);
                startWavPlayer();
                break;
            case '8':
                k = 2;
                while (k--)
                {
                    prepareWavPlayer("/opt/media/awake_01.wav", NULL, true);
                    startWavPlayer();
                    usleep(1 * 1000);
                }
                break;
            case '9':
                k = 2;
                while (k--)
                {
                    prepareWavPlayer("/opt/media/awake_01.wav", NULL, true);
                    startWavPlayer();
                    usleep(1 * 1000);
                    if (k == 1)
                        stopWavPlayer();
                }
                break;
            case 'q':
                exit = true;
                break;
            default:
                break;
        }
    }
    printf("test success !!!!!!!!!!!!!\n");
    return 0;
}
