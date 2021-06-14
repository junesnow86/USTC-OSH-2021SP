/*本程序是一个服务端程序*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 1048576 //一次recv最多可接收1MiB的消息
#define MAX_MESSAGE_NUM 512        //一次recv的信息最多可以被分割成的消息条数
#define SEND_BUFFER_LENGTH 4096    //发送缓冲的大小

struct Pipe
{
    int fd_send;
    int fd_recv;
};

void *handle_chat(void *data)
{
    /*服务端的handle函数*/
    struct Pipe *pipe = (struct Pipe *)data;
    char *buffer = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (!buffer)
    {
        perror("malloc");
        exit(1);
    }
    ssize_t len;
    //while ((len = recv(pipe->fd_send, buffer, MAX_MESSAGE_LENGTH, 0)) > 0)
    while (1)
    {
        //sleep(10);
        len = recv(pipe->fd_send, buffer, MAX_MESSAGE_LENGTH, 0); //服务端接收来自一个客户端(fd_send)的数据并存到buffer中，len为实际接收到的字节数
        if (len <= 0)
            break;
        char *recv_head = buffer;
        int len_left = MAX_MESSAGE_LENGTH;
        while (len >= SEND_BUFFER_LENGTH) //实际接收的字节数>=发送缓冲大小，可能是一条长消息，客户端一次send发送不完，需要继续调用recv
        {
            recv_head = recv_head + len;
            len_left = len_left - len;
            len = recv(pipe->fd_send, recv_head, len_left, 0);
        }
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
        for (int i = 0; i < mesnum; ++i)
        {
            char *sendstr = (char *)malloc(sizeof(char) * (strlen(message[i]) + 32)); //+32是为了在开头加上“Message:”和末尾加一个换行符
            if (!sendstr)
            {
                perror("malloc");
                exit(1);
            }
            strcpy(sendstr, "Message:");
            strcat(sendstr, message[i]);
            strcat(sendstr, "\n");
            send(pipe->fd_recv, sendstr, strlen(sendstr), 0); //服务端向另一个客户端(fd_recv)发送刚接收的信息
            free(sendstr);
        }
        free(message);
    }
    free(buffer);
    buffer = NULL;
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

    //接受来自两个客户端的信息，只有两个客户端的请求都接受了(两个accept)才能继续执行下面的代码
    //输入nc 127.0.0.1 6666即向服务端发起请求
    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1)
    {
        perror("accept");
        return 1;
    }

    //test
    char str1[] = "Hello, client1.\n";
    char str2[] = "Hello, client2.\n";
    write(fd1, str1, sizeof(str1));
    write(fd2, str2, sizeof(str2));

    pthread_t thread1, thread2; //线程ID
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;

    //创建两个线程(两个客户端)，相当于服务端在两条路线上执行handle_chat函数
    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1); //第四个参数为要传入handle_chat函数的参数
    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}