#ifndef _MY_PULSE_H_
#define _MY_PULSE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define A2DP_SIMPLE_RATE 44100
#define A2DP_FIFO_NAME "rokid_bt_sink"
#define SINK_INPUTS_MAX 20

typedef struct PulseAudio PulseAudio;
PulseAudio *pulse_create();
void pulse_destroy(PulseAudio *p);

int pulse_pipe_init(PulseAudio *pa);
int pulse_pipe_stop(PulseAudio *pa);
int pulse_pipe_uninit(PulseAudio *pa);

#ifdef __cplusplus
}
#endif
#endif
