#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#define BUFFER_SIZE 64

using namespace std;
int main(int argc, char* argv[])
{
    if(argc <= 2)
    {
        cout<<"usage: ./exe ip_address port_number"<<endl;
        return -1;
    }

    //设置ip和端口号
    const char* ip = argv[1];
    int port = atoi(argv[2]);//Convert a string to an integer.
    
    //设置ip和端口
    sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;//地址族
    inet_pton(AF_INET, ip, &server_address.sin_addr);//将字符串表示的ip地址转换为网络字节序表示的ip地址
    server_address.sin_port = htons(port);
    //                   协议族    tcp/udp
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("sockfd error\n");
        return -1;
    }

    if(connect(sockfd, (sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        perror("connection failed\n");
        close(sockfd);
        return -1;
    }

    pollfd fds[2];
    //注册文件描述符0（标准输入）和文件描述符sockfd上可读事件
    fds[0].fd = 0;
    fds[0].events = POLLIN; //POLL事件类型 可写
    fds[0].revents = 0;//实际发生的事件，由内核填充

    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP; //可写 或 tcp连接关闭
    fds[1].revents = 0;//实际发生的事件，由内核填充

    char read_buff[BUFFER_SIZE];
    int pipefd[2];

    int ret = pipe(pipefd);
    assert(ret != -1);

    while (1)
    {
        ret = poll(fds, 2, -1);
        if(ret < 0)
        {
            cout<<"poll failed\n";
            break;
        }

        if(fds[1].revents & POLLRDHUP)
        {
            cout<<"Server close the connection\n";
            break;
        }
        else if(fds[1].revents & POLLIN)//接受信息
        {
            memset(read_buff, 0, BUFFER_SIZE);
            recv(fds[1].fd, read_buff, BUFFER_SIZE-1, 0);
            cout<<read_buff<<endl;
        }
        if(fds[0].revents & POLLIN)//写入信息
        {
            //使用splice 将用户输入的数据直接写到sockfd(零拷贝)
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }

    close(sockfd);
    
    return 0;

}