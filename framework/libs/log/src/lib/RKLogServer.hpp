//
//  RKLogServer.hpp
//  LuaAppEngin
//
//  Created by 董希成 on 2017/10/12.
//  Copyright © 2017年 rokid. All rights reserved.
//

#ifndef RKLogServer_hpp
#define RKLogServer_hpp

#include <stdio.h>
#include <memory>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

class RKLogServer{
    
public:
    RKLogServer(int port);
    ~RKLogServer();
    
public:
    void run();
    void log(const char* chars, long size);
    bool showLog();

private:
    std::thread thread_;
    std::mutex mutex_;
    int port_;
    int connectFd_;
    
private:
    void sendLog(const char* chars, long size);
};



#endif /* RKLogServer_hpp */

