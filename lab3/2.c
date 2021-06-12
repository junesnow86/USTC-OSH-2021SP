/*本程序是一个服务端程序*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 1048576
#define MAX_MESSAGE_NUM 512
#define MAX_CLIENT_NUM 32

struct Pipe
{
    int fd_send;                 //一个发送端
    int fd_recv[MAX_CLIENT_NUM]; //32个接收端，实际只需要31个
};

struct Arg
{
    //传入handle_chat函数的参数
    struct Pipe *pipe;
    char *using;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void *handle_chat(void *data)
{
    /*服务端的handle函数*/
    struct Pipe *pipe = (*(struct Arg *)data).pipe;
    char *using = (*(struct Arg *)data).using;
    char login[] = "Welcome to chatting room!\n";
    write(pipe->fd_send, login, sizeof(login));
    printf("client%d has joined.\n", pipe->fd_send);
    char *buffer = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (!buffer)
    {
        perror("malloc");
        exit(1);
    }
    ssize_t len;
    while ((len = recv(pipe->fd_send, buffer, MAX_MESSAGE_LENGTH, 0)) > 0) //服务端接收来自一个客户端的数据，从fd_send接收数据并存到buffer中
    {
        //sleep(10);
        printf("server has received a message from client%d: %s", pipe->fd_send, buffer);
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
        //如果多个这样的线程同时向客户端发送消息就会导致冲突，所以需要在这里加互斥锁
        //这样就是允许多个客户端同时向服务端发送消息(因为每个线程都有一个buffer)
        //但不允许服务器的多个线程同时向客户端发送消息
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < mesnum; ++i)
        {
            char *sendstr = (char *)malloc(sizeof(char) * (strlen(message[i]) + 32)); //+32是还要存“Message:”和一个换行符
            if (!sendstr)
            {
                perror("malloc");
                exit(1);
            }
            strcpy(sendstr, "Message:");
            strcat(sendstr, message[i]);
            strcat(sendstr, "\n");
            for (int j = 0; j < MAX_CLIENT_NUM; ++j) //服务端向其他31个客户端发送刚接收的信息；向fd_recv发送buffer中的数据(含print功能)
                if (pipe->fd_recv[j] != -1)
                    send(pipe->fd_recv[j], sendstr, strlen(sendstr), 0);
            free(sendstr);
        }
        pthread_mutex_unlock(&mutex);
        /*
        for (int i = 0; i < mesnum; ++i)
            free(message[i]);
        free(message);
        */
    }
    free(buffer);
    buffer = NULL;
    (*using) = '0';
    printf("client%d has exited.\n", pipe->fd_send);
    return NULL;
}

int main(int argc, char **argv)
{
    printf("%s\n", argv[0]);
    printf("%s\n", argv[1]);
    int port = atoi(argv[1]); //将命令行的第二个参数(端口值的字符串形式)转换成int型
    printf("%d\n", port);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) //创建套接字
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) //绑定服务端套接字和IP地址、端口
    {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2)) //服务端套接字进入监听状态
    {
        perror("listen");
        return 1;
    }

    //TODO:利用多线程技术实现多人聊天室
    int userfd[MAX_CLIENT_NUM];
    pthread_t thread[MAX_CLIENT_NUM]; //最多支持32个用户同时在线
    struct Pipe pipe[MAX_CLIENT_NUM];
    char using[MAX_CLIENT_NUM]; //记录每个userfd[i]目前是否正在被一个客户端作为fd_send
    struct Arg arg;
    memset(using, 0, sizeof(char) * MAX_CLIENT_NUM);
    int i = 0;
    while (1) //不断接收来自客户端的请求
    {
        if (using[i] == '1')
        {
            i = (i + 1) % MAX_CLIENT_NUM;
            continue;
        }
        userfd[i] = accept(fd, NULL, NULL); //接受客户端请求并返回一个文件描述符
        if (userfd[i] == -1)
        {
            perror("accept");
            return 1;
        }
        pipe[i].fd_send = userfd[i];
        using[i] = '1';
        arg.pipe = &pipe[i];
        arg.using = &using[i];
        for (int j = 0; j < MAX_CLIENT_NUM; ++j)
        {
            if (j == i)
                pipe[i].fd_recv[j] = -1;
            else
                pipe[i].fd_recv[j] = userfd[j];
        }
        for (int j = 0; j < MAX_CLIENT_NUM; ++j) //更新目前在线的用户的接收端表
        {
            if (using[j] == '1')
            {
                pipe[j].fd_recv[i] = userfd[i];
            }
        }
        //pthread_create(&thread[i], NULL, handle_chat, (void *)&pipe[i]);
        pthread_create(&thread[i], NULL, handle_chat, (void *)&arg);
        pthread_detach(thread[i]); //将各个线程与主线程分离，以实现用户可以随时加入和退出
    }

    return 0;
}