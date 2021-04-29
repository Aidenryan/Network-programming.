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
#define USER_LIMIT 5
#define FD_LIMIT 65335

using namespace std;

//客户端数据
struct client_data
{
    sockaddr_in address;
    char* write_buff;
    char buff[BUFFER_SIZE];
};

//设置文件描述非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    return old_option;
}

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
    
    int ret = 0;

    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);//设置内核监听最大长度为5
    assert(ret != -1);

    //将socket与客户连接关联起来
    client_data* users = new client_data[FD_LIMIT];
    //为了提高poll性能，限制用户数量
    pollfd fds[USER_LIMIT+1];

    int user_cnt = 0;//客户端/用户数目
    for(int i(1); i <= USER_LIMIT; ++i)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }

    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while(1)
    {
        //               被监听文件描述符数量
        ret = poll(fds, user_cnt + 1, -1);
        if(ret < 0)
        {
            cout<<"poll error"<<endl;
            break;
        }

        for(int i(0); i < user_cnt+1; ++i)
        {
            if((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))
            {
                sockaddr_in client_add;
                socklen_t client_addlength = sizeof(client_add);

                int connfd = accept(listenfd, (sockaddr*)&client_add, &client_addlength);
                if(connfd < 0)
                {
                    perror("connection error: ");
                    continue;
                }

                //如果请求太多，则关闭新到的连接
                if(user_cnt >= USER_LIMIT)
                {
                    const char* info = "too many users\n";
                    cout<<info;
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                user_cnt++;
                users[connfd].address = client_add;
                setnonblocking(connfd);
                fds[user_cnt].fd = connfd;
                fds[user_cnt].events = POLLIN | POLLRDHUP | POLLERR; //监听可读 关闭连接 错误
                fds[user_cnt].revents = 0;
                cout<<"comes a new user, now have "<<user_cnt<<" users\n";
            }
            else if(fds[i].revents & POLLERR)
            {
                cout<<"get an error from "<<fds[i].fd<<endl;
                char errors[100];
                memset(errors, 0, 100);

                socklen_t len = sizeof(errors);
                //获取socket文件描述符状态
                if(getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0)
                {
                    cout<<"get socket option failed\n";
                }

                continue;
            }
            else if(fds[i].revents & POLLRDHUP)
            {
                //如果客户端关闭连接，则服务端也关闭连接，并将用户数目减一
                users[fds[i].fd] = users[fds[user_cnt].fd];
                close(fds[i].fd);
                fds[i] = fds[user_cnt];
                i--;
                user_cnt--;
                cout<<"a client left\n";
            }
            else if(fds[i].revents & POLLIN)
            {
                int connfd = fds[i].fd;
                memset(users[connfd].buff, 0, BUFFER_SIZE);

                ret = recv(connfd, users[connfd].buff, BUFFER_SIZE-1, 0);

                printf("get %d bytes of client data %s from %d\n", ret, users[connfd].buff, connfd);

                if(ret < 0)
                {
                    //如果是读操作出错，则关闭连接
                    if(errno != EAGAIN)
                    {
                        users[fds[i].fd] = users[fds[user_cnt].fd];
                        close(connfd);
                        fds[i] = fds[user_cnt];
                        i--;
                        user_cnt--;
                    }
                }
                else if(ret == 0)
                {
                    printf( "code should not come to here\n" );
                }
                else
                {
                    for(int j(1); j <= user_cnt; ++j)
                    {
                        if(fds[j].fd == connfd) continue;

                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buff = users[connfd].buff;
                    }
                }
            }
            else if(fds[i].revents & POLLOUT)
            {
                int connfd = fds[i].fd;
                if(!users[connfd].write_buff) continue;

                ret = send(connfd, users[connfd].write_buff, strlen(users[connfd].write_buff), 0);

                users[connfd].write_buff = NULL;
                //写完数据后需要重新注册fds[i]上面的可读事件
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete [] users;

    close(listenfd);

    return 0;
}
