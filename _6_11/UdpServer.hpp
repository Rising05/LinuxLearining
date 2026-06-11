#pragma once 

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nocopy.hpp"
#include "Log.hpp"
#include "Comm.hpp"
#include "InetAddr.hpp"

// 默认端口号
const static uint16_t DEFAULT_PORT = 8080;

//默认文件描述符
const static int DEFAULT_FD = -1;

//默认缓冲区大小
const static int DEFAULT_BUFSIZE = 1024;

class UdpServer : public nocopy {
public:
    UdpServer(uint16_t port = DEFAULT_PORT)
    : _port(port)
    ,_sockfd(DEFAULT_FD) {}
    
    void Init() {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            lg.LogMessage("socket error: " + std::string(strerror(errno)));
            exit(Socket_Err);
        }

        lg.LogMessage("socket created successfully, fd: " + std::to_string(_sockfd));
    }

private:
    uint16_t _port; // 服务器监听端口
    int _sockfd; // socket 文件描述符
};