#include "LumenEffects.h"
#include <iostream>
#include <unistd.h>

LumenEffects::LumenEffects(void)
{
    _speed = 70000;
}

LumenEffects::~LumenEffects(void)
{
    DelLayers();
    if(_lumenthread.joinable()){
        _flag_stop = 1;
        _lumenthread.join();
    }
}

void LumenEffects::InitLayers(int speed)
{
    _speed = speed;
    _lumenlight.lumen_set_enable(true);
}

bool LumenEffects::LayerAddBackground(u8* color)
{
    for (int i = 0; i < _lumenlight.getLedCount(); i++){
        led_data[i*3] = color[0];
        led_data[i*3+1] = color[1];
        led_data[i*3+2] = color[2];
    }
    return true;
}

bool LumenEffects::LayerAddGroup(bool dir,u8 poi,u8 len,u8* color)
{
    if(dir){
        for(int i = 0; i < len; i++){
            led_data[(poi+i)%12*3] = color[i*3];
            led_data[(poi+i)%12*3+1] = color[i*3+1];
            led_data[(poi+i)%12*3+2] = color[i*3+2];
        }
    }
    else{
        for(int i = 0; i < len; i++){
            led_data[(poi-i+12)%12*3] = color[i*3];
            led_data[(poi-i+12)%12*3+1] = color[i*3+1];
            led_data[(poi-i+12)%12*3+2] = color[i*3+2];
        }
    }

    return true;
}

bool LumenEffects::LayerAddGroupPoint(bool dir,u8 poi,u8 len,u8* color)
{
    if(dir){
        for(int i = 0; i < len; i++){
            led_data[(poi+i)%12*3] = color[0];
            led_data[(poi+i)%12*3+1] = color[1];
            led_data[(poi+i)%12*3+2] = color[2];
        }
    }
    else{
        for(int i = 0; i < len; i++){
            led_data[(poi-i+12)%12*3] = color[0];
            led_data[(poi-i+12)%12*3+1] = color[1];
            led_data[(poi-i+12)%12*3+2] = color[2];
        }
    }

    return true;
}

bool LumenEffects::LayerAddPoint(u8 led_num,u8* color)
{
    led_data[led_num*3] = color[0];
    led_data[led_num*3+1] = color[1];
    led_data[led_num*3+2] = color[2];

    return true;
}

void LumenEffects::DelLayers(void)
{
    _lumenlight.lumen_set_enable(false);
}

bool LumenEffects::LayersSetSpeed(int speed)
{
    _speed = speed;
}

bool LumenEffects::LayersSetLight(u8 light)
{
    for (int i = 0; i < _lumenlight.getFrameSize(); i++){
        led_data[i] = led_data[i]*light/255;
    }
}

bool LumenEffects::LayersShow(void)
{
    _lumenlight.lumen_draw(led_data, sizeof(led_data));
}

bool LumenEffects::_EffectFadeIn(bool dir,u8 poi,u8 len,u8* color,int speed)
{
    u8 color_b[3] = {0,0,0};

    InitLayers(speed);
    for (int i = 0; i < len; i++)
    {
        LayerAddBackground(color_b);
        if(dir){
            LayerAddGroup(!dir,(poi+i)%12,i+1,color);
        }
        else{
            LayerAddGroup(!dir,(poi-i+12)%12,i+1,color);
        }
        LayersShow();
        usleep(speed);
        if(_flag_stop){
            _flag_stop = 0;
            return false;
        }
    }

    return true;
}

bool LumenEffects::EffectFadeIn(bool dir,u8 poi,u8 len,u8* color,int speed)
{
    if(_lumenthread.joinable()){
        _flag_stop = 1;
        _lumenthread.join();
    }
    _flag_stop = 0;
    _lumenthread = std::thread(&LumenEffects::_EffectFadeIn,this,dir,poi,len,color,speed);
}

bool LumenEffects::_EffectFadeOut(bool dir,u8 poi,u8 len,u8* color,int speed)
{
    static u8 color_b[3] = {0,0,0};

    InitLayers(speed);
    for (int i = 0; i < len; i++)
    {
        LayerAddBackground(color_b);
        LayerAddGroup(!dir,poi,len-i-1,color + 3*i+3);
        LayersShow();
        usleep(speed);
        if(_flag_stop){
            _flag_stop = 0;
            return false;
        }
    }

    return true;
}

bool LumenEffects::EffectFadeOut(bool dir,u8 poi,u8 len,u8* color,int speed)
{
    if(_lumenthread.joinable()){
        _flag_stop = 1;
        _lumenthread.join();
    }
    _flag_stop = 0;
    _lumenthread = std::thread(&LumenEffects::_EffectFadeOut,this,dir,poi,len,color,speed);
}

bool LumenEffects::_EffectPoint(int degree,u8* color_point,u8 len,u8* color_len,int speed)
{
    static u8 color_b[3] = {0,0,0};
    u8 poi = (degree/30)%12;

    static u8 color_b0[3] = {10,10,200};

    if(degree == -1)
    {
        InitLayers(speed);
        LayerAddBackground(color_b0);
        LayersShow();
        return true;
    }

    InitLayers(speed);
    for (int i = 0; i < (6+len); i++){
        LayerAddBackground(color_b);
        if(i < 6){
            LayerAddGroup(false,(poi+6+i)%12,i<len?i:len,color_len);
            LayerAddGroup(true,(poi+18-i)%12,i<len?i:len,color_len);
        }
        else{
            LayerAddGroup(false,poi,len-i+6,color_len+i-6);
            LayerAddGroup(true,poi,len-i+6,color_len+i-6);
        }
        LayerAddPoint(poi,color_point);
        LayersShow();
        usleep(speed);
        if(_flag_stop){
            _flag_stop = 0;
            return false;
        }
    }

    return true;
}

bool LumenEffects::EffectPoint(int degree)
{
    static u8 color_point[3] = {100,100,100};
    static u8 color_len[12] = {30,30,30,10,10,10,4,4,4,1,1,1};

    EffectPoint(degree,color_point,4,color_len,50000);
}

bool LumenEffects::EffectPoint(int degree,u8* color_point,u8 len,u8* color_len,int speed)
{
    if(_lumenthread.joinable()){
        _flag_stop = 1;
        _lumenthread.join();
    }
    _flag_stop = 0;
    _lumenthread = std::thread(&LumenEffects::_EffectPoint,this,degree,color_point,len,color_len,speed);
}

bool LumenEffects::_EffectStartRound(int start_degree,int end_degree,bool dir,u8 len,u8* color,int speed,int acc_spd,int max_spd)
{
    _lumenmutex.lock();
    static u8 color_b[3] = {0,0,0};
    u8 poi = (start_degree/30)%12;
    int i = 0;
    u8 end_poi;

    _end_degree = end_degree;
    _lumenmutex.unlock();
    InitLayers(speed);
    while(ENDDEGREE_WAIT == _end_degree){
        LayerAddBackground(color_b);
        if(dir){
            LayerAddGroup(!dir,poi,i,color);
            poi = poi >= 11 ? 0:poi+1;
        }
        else{
            LayerAddGroup(!dir,poi,i,color);
            poi = poi <= 0 ? 11:poi-1;
        }
        i = i >= len ? i : i+1;
        LayersShow();
        usleep(_speed);
        if(_speed >= max_spd){
            _speed -= acc_spd;
        }
        if(_flag_stop){
            _flag_stop = 0;
            return false;
        }
    }
    if(ENDDEGREE_NOW == _end_degree){
        end_poi = poi;
    }
    else{
        end_poi = (_end_degree/30)%12;
    }
    while(end_poi != poi){
        LayerAddBackground(color_b);
        if(dir){
            LayerAddGroup(!dir,poi,i,color);
            poi = poi >= 11 ? 0:poi+1;
        }
        else{
            LayerAddGroup(!dir,poi,i,color);
            poi = poi <= 0 ? 11:poi-1;
        }
        i = i >= len ? i : i+1;
        LayersShow();
        usleep(speed);
        if(_flag_stop){
            _flag_stop = 0;
            return false;
        }
    }
    for (int i = 0; i < len; i++)
    {
        LayerAddBackground(color_b);
        LayerAddGroup(!dir,poi-1,len-i-1,color + 3*i+3);
        LayersShow();
        usleep(speed);
        if(_flag_stop){
            _flag_stop = 0;
            return false;
        }
    }

    return true;
}

bool LumenEffects::EffectStartRound(void)
{
    static u8 len = 5;
    static u8 color[15] = {30,30,200,15,15,100,10,10,50,4,4,10,1,1,1};;

    EffectStartRound(0,ENDDEGREE_WAIT,true,5,color,80000,1000,70000);
}

bool LumenEffects::EffectStartRound(int start_degree)
{
    static u8 len = 5;
    static u8 color[15] = {100,100,100,30,30,30,10,10,10,4,4,4,1,1,1};

    EffectStartRound(start_degree,ENDDEGREE_WAIT,true,5,color,70000,1000,50000);
}

bool LumenEffects::EffectStartRound(int start_degree,int end_degree,bool dir,u8 len,u8* color,int speed,int acc_spd,int max_spd)
{
    if(_lumenthread.joinable()){
        _flag_stop = 1;
        _lumenthread.join();
    }
    _flag_stop = 0;
    _lumenthread = std::thread(&LumenEffects::_EffectStartRound,this,start_degree,end_degree,dir,len,color,speed,acc_spd,max_spd);
}

bool LumenEffects::EffectEndRound(void)
{
    EffectEndRound(ENDDEGREE_NOW);
}

bool LumenEffects::EffectEndRound(int end_degree)
{
    usleep(1000);
    _lumenmutex.lock();
    _end_degree = end_degree;
    _lumenmutex.unlock();
}

bool LumenEffects::EffectStop(void)
{
    static u8 color_bkg[3] = {0,0,0};

    if(_lumenthread.joinable()){
        _flag_stop = 1;
        _lumenthread.join();
    }
    _flag_stop = 0;
    LayerAddBackground(color_bkg);

    LayersShow();
}