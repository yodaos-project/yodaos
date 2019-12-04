//
//  RKLogServer.cpp
//  LuaAppEngin
//
//  Created by 董希成 on 2017/10/12.
//  Copyright © 2017年 rokid. All rights reserved.
//

#include "RKLogServer.hpp"
#include <errno.h>
#include <string.h>

#define BACKLOG 2

void RKLogServer::run() {
    int connectfd;
    int listenfd;
    struct sockaddr_in serverAddr = {0}; //服务器地址信息结构体
    struct sockaddr_in clientAddr = {0}; //客户端地址信息结构体

    //调用socket，创建监听客户端的socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("[RKLogServer: %p] Creating socket failed.\n", this);
        return;
    }

    int opt = SO_REUSEADDR;
    //设置socket属性，端口可以重用
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //初始化服务器地址结构体
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("[RKLogServer: %p] 调用bind，绑定地址和端口 %d\n", this, port_);
    if (bind(listenfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        printf("[RKLogServer: %p] Bind error.\n");
        return ;
    }

    printf("[RKLogServer: %p] 调用listen，开始监听\n", this);
    if (listen(listenfd, BACKLOG) == -1) {
        printf("[RKLogServer: %p] listen() error\n", this);
        return;
    }

    int addrlen = sizeof(struct sockaddr_in);

    while(true) {
        if ((connectfd = accept(listenfd, (struct sockaddr*)&clientAddr, (socklen_t *)&addrlen)) == -1) {
            //调用accept，返回与服务器连接的客户端描述符
            printf("[RKLogServer: %p] accept() error %s\n", this, strerror(errno));
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            printf("[RKLogServer: %p] You got a connection from %s.  \n", this, inet_ntoa(clientAddr.sin_addr));
            if (this->connectFd_ > 0) {
                close(this->connectFd_);
                this->connectFd_ = 0;
            }
            this->connectFd_ = connectfd;
        }
        usleep(1000*10);
    }
    close(listenfd);   //关闭监听socket
}

void RKLogServer::sendLog(const char* chars, long size)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (connectFd_ > 0) {
        while (size > 0) {
            long dsize = send(connectFd_, chars, size, 0);
            if (dsize > 0) {
                size -= dsize;
                chars += dsize;
            } else {
                printf("[RKLogServer: %p] socket 客户端已经关闭\n", this);
                close(connectFd_);
                connectFd_ = 0;
                break;
            }
        }
    }
}

void RKLogServer::log(const char* chars, long size){
    if (connectFd_ > 0) {
        sendLog(chars, size);
    }
}

bool RKLogServer::showLog(){
    return connectFd_ > 0;
}

static void LogerServiceLooper(RKLogServer* server){
    server->run();
}

RKLogServer::RKLogServer(int port): port_(port), thread_(LogerServiceLooper, this) { }

RKLogServer::~RKLogServer(){
    thread_.join();
}

