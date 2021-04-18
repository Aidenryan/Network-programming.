#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#define buf_size 1024//设置缓冲区大小

int main(int argc, char* argv[])
{
    if(argc <=2)
    {
        cout<<"usage: ./exe ip_address port_number"<<endl;
        return -1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    //创建socket
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    //绑定
    int ret = bind(sockfd, (sockaddr*)&server_address, sizeof(server_address));
    assert(ret != -1);

    //监听
    ret = listen(sockfd, 5);
    assert(ret != -1);

    int cfd = accept(sockfd, NULL, NULL);
    
    if(cfd < 0)
    {
        perror("Accept error");
    }
    else
    {
        char buff[buf_size];

        memset(buff, 0, buf_size);
        ret = recv(cfd, buff, buf_size-1, 0);
        cout<<"Got "<<ret<<" bytes of normal data "<<buff<<endl;

        memset(buff, 0, buf_size);
        ret = recv(cfd, buff, buf_size-1, MSG_OOB);
        cout<<"Got "<<ret<<" bytes of normal data "<<buff<<endl;

        memset(buff, 0, buf_size);
        ret = recv(cfd, buff, buf_size-1, 0);
        cout<<"Got "<<ret<<" bytes of normal data "<<buff<<endl;
    }
    
    close(cfd);
    close(sockfd);
    return 0;
    
}
