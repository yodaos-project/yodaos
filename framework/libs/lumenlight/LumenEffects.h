#ifndef __LUMENEFFECTS_H__
#define __LUMENEFFECTS_H__

#include "LumenLight.h"
#include <thread>
#include <mutex>

typedef unsigned char u8;

#define MAX_LED_NUM 100

#define FUNC_IDLE 0

#define ENDDEGREE_NOW -1
#define ENDDEGREE_WAIT -2

class LumenEffects{
public:
    LumenEffects(void);
    ~LumenEffects(void);

    void InitLayers(int speed);
    bool LayerAddBackground(u8* color);
    bool LayerAddGroup(bool dir,u8 poi,u8 len,u8* color);
    bool LayerAddGroupPoint(bool dir,u8 poi,u8 len,u8* color);
    bool LayerAddPoint(u8 led_num,u8* color);
    void DelLayers(void);

    bool LayersSetSpeed(int speed);
    bool LayersSetLight(u8 light);
    bool LayersShow(void);

    bool EffectFadeIn(bool dir,u8 poi,u8 len,u8* color,int speed);
    bool EffectFadeOut(bool dir,u8 poi,u8 len,u8* color,int speed);
    bool EffectPoint(int degree);
    bool EffectPoint(int degree,u8* color_point,u8 len,u8* color_len,int speed);
    bool EffectStartRound(void);
    bool EffectStartRound(int start_degree);
    bool EffectStartRound(int start_degree,int end_degree,bool dir,u8 len,u8* color,int speed,int acc_spd,int max_spd);
    bool EffectEndRound(void);
    bool EffectEndRound(int end_degree);
    bool EffectStop(void);

private:
    LumenLight _lumenlight;
    u8 led_data[3*MAX_LED_NUM];
    volatile int _speed;

    volatile int _flag_stop;
    volatile int _end_degree;

    std::thread _lumenthread;
    std::mutex _lumenmutex;

    bool _EffectFadeIn(bool dir,u8 poi,u8 len,u8* color,int speed);
    bool _EffectFadeOut(bool dir,u8 poi,u8 len,u8* color,int speed);
    bool _EffectPoint(int degree,u8* color_point,u8 len,u8* color_len,int speed);
    bool _EffectStartRound(int start_degree,int end_degree,bool dir,u8 len,u8* color,int speed,int acc_spd,int max_spd);
};

#endif