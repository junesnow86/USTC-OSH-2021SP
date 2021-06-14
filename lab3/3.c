/*本程序是一个服务端程序*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_MESSAGE_LENGTH 1048576
#define MAX_MESSAGE_NUM 512
#define MAX_CLIENT_NUM 32
#define SEND_BUFFER_LENGTH 4096

struct Pipe
{
    int fd_send;
    int fd_recv;
};

int main(int argc, char **argv)
{
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        return 1;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK); //将服务端套接字设置成非阻塞
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2))
    {
        perror("listen");
        return 1;
    }

    //TODO:利用IO复用技术实现多人聊天室
    fd_set rdclients;   //可读clients
    fd_set clients_bak; //由于每次select之后会更新clients，因此需要backup
    FD_ZERO(&rdclients);
    FD_ZERO(&clients_bak);
    struct timeval timeout;
    int maxfd = 2; //最大文件描述符
    char *buffer = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (!buffer)
    {
        perror("malloc");
        return 1;
    }
    while (1) //可以将该while循环看作一遍遍地扫描
    {
        rdclients = clients_bak;            //目前在线的用户集合
        timeout.tv_sec = 3;                 //设置等待时间为3s
        timeout.tv_usec = 0;                //在Linux中select函数会不断修改timeout的值，所以每次循环都应该重新赋值
        int newfd = accept(fd, NULL, NULL); //若有新请求则接受，但不会阻塞
        if (newfd == -1 && errno != EAGAIN)
        {
            perror("accept");
            return 1;
        }
        //如果没有新的连接请求，就是newfd == -1 && errno == EAGAIN，这种情况不必理会，继续执行下面的代码
        if (newfd > 0) //接受了新的用户加入
        {
            FD_SET(newfd, &rdclients);
            FD_SET(newfd, &clients_bak);
            if (newfd > maxfd)
                maxfd = newfd;
            char login[] = "Welcome to chatting room!\n";
            write(newfd, login, sizeof(login));
            printf("client%d has joined.\n", newfd);
        }
        int readnum;
        if ((readnum = select(maxfd + 1, &rdclients, NULL, NULL, &timeout)) > 0) //有用户发送了消息
        {
            int count = 0;
            //下面逐个处理用户发送的消息
            for (int fdi = 3; fdi <= maxfd && count < readnum; ++fdi)
            {
                if (FD_ISSET(fdi, &rdclients)) //fdi属于可读套接字
                {
                    memset(buffer, 0, MAX_MESSAGE_LENGTH); //每次接收一个套接字的消息前先清空buffer
                    ssize_t len = recv(fdi, buffer, MAX_MESSAGE_LENGTH, 0);
                    //printf("len=%d\n", len);
                    if (len > 0)
                    {
                        char *recv_head = buffer;
                        int len_left = MAX_MESSAGE_LENGTH;
                        while (len >= SEND_BUFFER_LENGTH)
                        {
                            recv_head = recv_head + len;
                            len_left = len_left - len;
                            len = recv(fdi, recv_head, len_left, 0);
                            //printf("len>=4096\n");
                        }
                        //分割并向其他的客户端发送刚接收的消息
                        count++;
                        printf("server has received a message from client%d: %s", fdi, buffer);
                        char **message = (char **)malloc(sizeof(char *) * MAX_MESSAGE_NUM);
                        if (!message)
                        {
                            perror("malloc");
                            exit(1);
                        }
                        int mesnum = 0;
                        message[mesnum] = strtok(buffer, "\n");
                        while (mesnum < MAX_MESSAGE_NUM && message[mesnum] != NULL) //以’\n‘为分隔符分割一次recv接收的字符串后将各片段存到message中，一个片段为一条消息
                        {
                            mesnum++;
                            message[mesnum] = strtok(NULL, "\n");
                        }
                        //下面向客户端发送消息
                        for (int i = 0; i < mesnum; ++i)
                        {
                            char *sendstr = (char *)malloc(sizeof(char) * (strlen(message[i]) + 32)); //+32是还要存“Message:”和一个换行符
                            if (!sendstr)
                            {
                                perror("malloc");
                                exit(1);
                            }
                            memset(sendstr, 0, strlen(message[i]) + 32);
                            sprintf(sendstr, "Message from %d: ", fdi);
                            strcat(sendstr, message[i]);
                            strcat(sendstr, "\n");
                            for (int fdj = 3; fdj <= maxfd; ++fdj) //服务端向其他在线的客户端发送刚接收的信息
                            {
                                if (fdj != fdi && FD_ISSET(fdj, &clients_bak))
                                    send(fdj, sendstr, strlen(sendstr), 0);
                            }
                            free(sendstr);
                        }
                    }
                    else //len == 0说明该客户端已断开连接，比如按了Ctrl+C
                    {
                        printf("client%d has exited.\n", fdi);
                        FD_CLR(fdi, &clients_bak);
                        close(fdi);
                    }
                }
            }
        }
        else
            printf("there is no client texting\n");
    }
    return 0;
}