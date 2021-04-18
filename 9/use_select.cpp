#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

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
    int port = atoi(argv[2]);

    //创建socket
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(lfd >= 0);

    //设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(int));

    //绑定
    sockaddr_in serv;
    bzero(&serv, sizeof(serv));
    serv.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serv.sin_addr);
    serv.sin_port = htons(port);
    int ret = bind(lfd, (sockaddr*)&serv, sizeof(serv));
    if(ret < 0)
    {
        perror("bind error");
        return -1;
    }

    //监听
    ret = listen(lfd, 128);

    fd_set readfds, tempfds;//定义fd_set类型变量

    FD_ZERO(&readfds); FD_ZERO(&tempfds);

    //委托内核
    FD_SET(lfd, &readfds);

    int maxfd = lfd;
    int nread, cfd;

    while (1)
    {
        //tmpfds是输入输出参数:
        //输入:告诉内核要监测哪些文件描述符
        //输出:内核告诉应用程序有哪些文件描述符发生了变化
        tempfds = readfds;

        nread = select(maxfd + 1, &tempfds, NULL, NULL, NULL);
        if(nread < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            break;
        }

        //有客户端请求到来
        if(FD_ISSET(lfd, &tempfds))
        {
            cfd = accept(lfd, NULL, NULL);

            //将cfd加入文件描述集
            FD_SET(cfd, &readfds);

            //修改内核监控范围
            maxfd = max(cfd, maxfd);

            if(--nread == 0) continue; 
        }

        //有客户端数据过来
        for(int i(lfd + 1); i <= maxfd; ++i)
        {
            int sockfd = i;
            //判断是否有数据过来
            if(FD_ISSET(sockfd, &tempfds))
            {
                char buff[1024];
                memset(buff, 0, sizeof(buff));

                int n = recv(sockfd, buff, sizeof(buff), 0);
                if(n <= 0)
                {
                    close(sockfd);
                    FD_CLR(sockfd, &readfds);
                }
                else
                {
                    cout<<"Got "<<n<<" bytes of normal data: "<<buff<<endl;
                    for(int k(0); k < n; ++k)
                    {
                        buff[k] = toupper(buff[k]);
                    }
                    send(sockfd, buff, n, 0);
                }

                if(--nread == 0) break;//这行千万注意别写在外面这个框中，因为只有改变的事件才能--nread。要不然会导致只有一个连接读写
            }

            
        }
    }

   
    
    close(cfd);
    close(lfd);

    return 0;
}