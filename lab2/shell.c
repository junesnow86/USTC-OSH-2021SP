#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_ARG_LENGTH 512      /* max length of arg*/
#define MAX_CMDLINE_LENGTH 1024 /* max cmdline length in a line*/
#define MAX_BUF_SIZE 4096       /* max buffer size */
#define MAX_CMD_ARG_NUM 32      /* max number of single command args */
#define WRITE_END 1             // pipe write end
#define READ_END 0              // pipe read end
#define MAX_ALIAS_NUM 32

typedef struct
{
    char origin[MAX_CMDLINE_LENGTH];
    char alter[MAX_CMDLINE_LENGTH];
} alias;

alias all_alias[MAX_ALIAS_NUM];
int alias_num = 0;

/*  
    int split_string(char* string, char *sep, char** string_clips);

    function:       split string by sep and delete whitespace at clips' head & tail

    arguments:      char* string, Input, string to be splitted 
                    char* sep, Input, the symbol used to split string
                    char** string_clips, Input/Output, giving an allocated char* array 
                                                and return the splitted string array

    return value:   int, number of splitted strings 
*/
int split_string(char *string, char *sep, char **string_clips)
{
    /*将传入的字符串string以传入的sep分隔符进行拆解, 将各个片段存到string_clips[i]中, 并返回拆解后片段的数量*/
    char string_dup[MAX_BUF_SIZE];
    string_clips[0] = strtok(string, sep); //根据传入的sep(分隔符)对字符串string进行拆解，string_clips[0]被赋值以第一个子串的首地址
    int clip_num = 0;                      //字符串片段的编号

    int flag = 0; //如果首次检测到单引号则置1，第二次检测到单引号时清零
    int clipnum_h, clipnum_t;

    //printf("\nhere starts the split.\n");
    do
    {
        //检测有没有成对的单引号，如果有，单引号之间的空格要忽略
        char *head, *tail;
        head = string_clips[clip_num];                    //一个片段的首地址
        tail = head + strlen(string_clips[clip_num]) - 1; //一个片段的末尾地址

        //清除首尾的空格
        while (*head == ' ' && head != tail)
            head++;
        while (*tail == ' ' && tail != head)
            tail--;
        *(tail + 1) = '\0';            //清除完首尾的空格后在末尾加上‘\0’
        string_clips[clip_num] = head; //将清除首尾空格后的片段首地址存到string_clips[clip_num]中

        char *sinquo = strchr(head, '\'');
        if (sinquo != NULL) //在一个片段中检测到了单引号
        {
            if (flag == 0) //是一对单引号中的第一个
            {
                clipnum_h = clip_num;
                flag = 1;
                //printf("h=%d\n", clipnum_h);
                //printf("%s\n", sinquo);
            }
            else //是一对单引号中的第二个
            {
                flag = 0;
                clipnum_t = clip_num;
                //printf("t=%d\n", clipnum_t);
                char *new = (char *)malloc(sizeof(char) * MAX_ARG_LENGTH); //原来的string_clips[i]所指向的字符串不能连接新的字符串，需要重新用另一片空间来存
                strcpy(new, string_clips[clipnum_h]);
                //free(string_clips[clipnum_h]);
                string_clips[clipnum_h] = new;
                for (int j = clipnum_h + 1; j <= clipnum_t; ++j)
                {
                    //把一对单引号内的字符串重新连起来
                    //printf("j=%d, %s\n", j, string_clips[j]);
                    //printf("before: %s\n", string_clips[clipnum_h]);
                    strcat(string_clips[clipnum_h], " "); //先连上一个空格
                    //printf("middle: %s\n", string_clips[clipnum_h]);
                    strcat(string_clips[clipnum_h], string_clips[j]);
                    //printf("after: %s\n", string_clips[clipnum_h]);
                    //free(string_clips[j]);
                }
                clip_num = clipnum_h;
            }
        }
        clip_num++;                                       //处理下一个字符串片段
    } while (string_clips[clip_num] = strtok(NULL, sep)); //若还有其他子字符串，继续while循环

    //printf("here ends the split.\n");
    return clip_num;
}

/*
    执行内置命令：cd、pwd、exit
    arguments:
        argc: 命令的参数个数
        argv: 依次代表每个参数，注意第一个参数就是要执行的命令，
        若执行"ls a b c"命令，则argc=4, argv={"ls", "a", "b", "c"}
    return:
        int, 若执行成功返回0，否则返回值非零
*/
int exec_builtin(int argc, char **argv)
{
    if (argc == 0)
    {
        return 0;
    }
    /* 添加和实现内置指令 */
    if (strcmp(argv[0], "cd") == 0)
    {
        //执行cd命令
        //传入的argc应等于2，argv = {"cd", "dirctory"}
        if (argc > 2)
            printf("bash: cd: too many arguments\n");
        else if (chdir(argv[1]) == -1) //调用chdir改变当前工作目录, 若失败(返回值为-1)则打印错误信息
            printf("bash: cd: %s: No such file or directionary\n", argv[1]);
        return 0;
    }
    else if (strcmp(argv[0], "pwd") == 0)
    {
        char buf[MAX_BUF_SIZE];
        getcwd(buf, sizeof(buf)); //获取当前工作目录
        printf("%s\n", buf);
        return 0;
    }
    else if (strcmp(argv[0], "exit") == 0)
    {
        exit(0);
    }
    else
    {
        // 不是内置指令时
        return -1;
    }
}

/*
    在本进程中执行命令，且执行完毕后结束进程。
    arguments:
        argc: 命令的参数个数
        argv: 依次代表每个参数，注意第一个参数就是要执行的命令，
        若执行"ls a b c"命令，则argc=4, argv={"ls", "a", "b", "c"}
    return:
        int, 若执行成功则不会返回（进程直接结束），否则返回非零
*/

int FindOrient(char **argv)
{
    /* 从命令行参数数组中寻找定向符号">" ">>" "<"，返回其下标，若没找到则返回-1*/
    for (int i = 0; argv[i] != NULL; ++i)
    {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "<<") == 0 || strcmp(argv[i], "<<<") == 0)
            return i;
    }
    return -1;
}

size_t get_exe_name(char *processdir, char *processname, size_t len)
{
    /* 获取当前执行程序的文件名和路径 */
    char *path_end;
    if (readlink("/proc/self/exe", processdir, len) <= 0)
        return -1;
    path_end = strrchr(processdir, '/'); //查找指定字符在指定字符串中从最右边开始的第一次出现的位置
    if (path_end == NULL)
        return -1;
    ++path_end;
    strcpy(processname, path_end);
    *path_end = '\0';
    return (size_t)(path_end - processdir);
}

int alias_cmd(char **argv)
{
    if (argv[1] == NULL)
    {
        //alias命令后不带任何参数，则打印当前所有的别名记录
        //printf("alias_num=%d\n", alias_num);
        for (int i = 0; i < alias_num; ++i)
        {
            printf("alias %s='%s'\n", all_alias[i].alter, all_alias[i].origin);
        }
    }
    else
    {
        //alias后带参数，给指定命令需要设置别名
        char *eq = strchr(argv[1], '='); //找到'='字符的位置
        //复制字符串
        strncpy(all_alias[alias_num].alter, argv[1], eq - argv[1]); //从'='往前将新的名称复制到alter
        //printf("alter=%s\n", all_alias[alias_num].alter);
        strncpy(all_alias[alias_num].origin, eq + 2, strlen(eq + 2) - 1); //从'='往后将原名称复制到origin
        //printf("origin=%s\n", all_alias[alias_num].origin);
        alias_num++;
    }
    //exit(0);
    return 0;
}

/* 主要的执行指令的函数 */
int execute(int argc, char **argv)
{
    /* 内置指令 */
    if (exec_builtin(argc, argv) == 0)
    {
        exit(0);
    }

    /* 运行命令 */
    /* 支持基本的文件重定向 */
    int orient = FindOrient(argv);
    if (orient >= 0) //存在定向符号
    {
        if (strcmp(argv[orient], ">") == 0)
        {
            //实现 cmd > /dev/tcp/<host>/<port>
            if (strstr(argv[orient + 1], "/dev/tcp/") != NULL)
            {
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd == 0)
                {
                    printf("socket failed.\n");
                    exit(1);
                }
                char *tcpstr[128];
                split_string(argv[orient + 1], "/", tcpstr);
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = inet_addr(tcpstr[2]);
                addr.sin_port = htons(atoi(tcpstr[3]));
                int ret = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
                if (ret == -1)
                {
                    printf("connect failed.\n");
                    exit(1);
                }
                dup2(sockfd, STDOUT_FILENO);
                argv[orient] = NULL;
                argv[orient + 1] = NULL;
            }
            else
            {
                int fd = open(argv[orient + 1], O_RDWR);
                if (fd == -1)
                {
                    printf("open %s error!\n", argv[orient + 1]);
                    exit(2);
                }
                dup2(fd, STDOUT_FILENO); //将标准输出重定向到argv[2]文件中
                close(fd);
                argv[orient] = NULL; //这样操作会不会有安全隐患？
                argv[orient + 1] = NULL;
            }
        }
        else if (strcmp(argv[orient], ">>") == 0)
        {
            int fd = open(argv[orient + 1], O_RDWR | O_APPEND);
            if (fd == -1)
            {
                printf("open %s error!\n", argv[orient + 1]);
                exit(2);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            argv[orient] = NULL;
            argv[orient + 1] = NULL;
        }
        else if (strcmp(argv[orient], "<") == 0)
        {
            //实现 cat < /dev/tcp/127.0.0.1/22
            if (strstr(argv[orient + 1], "/dev/tcp/") != NULL)
            {
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd == 0)
                {
                    printf("socket failed.\n");
                    exit(1);
                }
                char *tcpstr[128];
                split_string(argv[orient + 1], "/", tcpstr);
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = inet_addr(tcpstr[2]);
                addr.sin_port = htons(atoi(tcpstr[3]));
                int ret = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
                if (ret == -1)
                {
                    printf("connect failed.\n");
                    exit(1);
                }
                dup2(sockfd, STDIN_FILENO);
                argv[orient] = NULL;
                argv[orient + 1] = NULL;
            }
            else
            {
                int fd = open(argv[orient + 1], O_RDWR); //为argv[orient+1]所指的文件创建一个文件描述符
                if (fd == -1)
                {
                    printf("open %s error!\n", argv[orient + 1]);
                    exit(2);
                }
                dup2(fd, STDIN_FILENO); //将标准输入重定向到fd
                close(fd);              //关闭文件描述符fd
                argv[orient] = NULL;
                argv[orient + 1] = NULL;
            }
        }
        else if (strcmp(argv[orient], "<<") == 0)
        {
            FILE *file = fopen("redirection.txt", "w+");
            while (1) //不断读入字符串，直到遇到结束标识符
            {
                printf(">");
                char end[128];
                strcpy(end, argv[orient + 1]);
                strcat(end, "\n");
                char temp[128];
                fgets(temp, 128, stdin);
                if (strcmp(temp, end) == 0)
                    break;
                else
                    fputs(temp, file);
            }
            fclose(file);
            int fd = open("redirection.txt", O_RDWR);
            if (fd == -1)
            {
                printf("open redirection.txt error!\n");
                exit(2);
            }
            dup2(fd, STDIN_FILENO); //将标准输入重定向到fd
            close(fd);              //关闭文件描述符fd
            //argv作为参数传给execvp时"cmd << ..."定向符和后面那段全部不要了
            argv[orient] = NULL;
            argv[orient + 1] = NULL;
        }
        else if (strcmp(argv[orient], "<<<") == 0)
        {
            FILE *file = fopen("redirection.txt", "w+");
            fprintf(file, "%s\n", argv[orient + 1]);
            fclose(file);
            int fd = open("redirection.txt", O_RDWR);
            if (fd == -1)
            {
                printf("open redirection.txt error!\n");
                exit(2);
            }
            dup2(fd, STDIN_FILENO); //将标准输入重定向到fd
            close(fd);              //关闭文件描述符fd
            //argv作为参数传给execvp时"cmd << ..."定向符和后面那段全部不要了
            argv[orient] = NULL;
            argv[orient + 1] = NULL;
        }
    } //end orient if

    /* 支持echo $SHELL和echo ～root */
    if (strcmp(argv[0], "echo") == 0)
    {
        if (strcmp(argv[1], "$0") == 0)
        {
            //echo命令输出普通字符串时可以通过下面的execvp实现，这里针对“echo $0”命令
            char path[PATH_MAX];
            char processname[1024];
            get_exe_name(path, processname, sizeof(path));
            printf("%s%s\n", path, processname);
            exit(0);
        }
        else if (strcmp(argv[1], "$SHELL") == 0)
        {
            //echo命令输出普通字符串时可以通过下面的execvp实现，这里针对“echo $SHELL”命令
            char *env = getenv("SHELL");
            if (env == NULL)
                exit(0);
            else
            {
                printf("%s\n", env);
                exit(0);
            }
        }
        else if (strchr(argv[1], '~') != NULL)
        {
            //echo命令的参数中带有'~'，该字符表示当前账户的home目录，等于环境变量HOME的值
            if (strlen(argv[1]) == 1)
            {
                //参数中只有'~'一个字符
                printf("%s\n", getenv("HOME"));
                exit(0);
            }
            else if (strcmp(argv[1] + 1, "root") == 0)
            {
                printf("/root\n");
                exit(0);
            }
            else if (strcmp(argv[1] + 1, "bin") == 0)
            {
                printf("/bin\n");
                exit(0);
            }
        }
    } //end echo if

    /* 支持A=1 env */
    if (argv[1] != NULL && strcmp(argv[1], "env") == 0)
    {
        char *eq = strchr(argv[0], '=');
        if (eq == NULL)
        {
            printf("No such command.\n");
            exit(0);
        }
        else
        {
            printf("%s\n", argv[0]);
            char **argv_temp = &argv[1];
            execvp(argv[1], argv_temp);
            exit(0);
        }
    } //end env if

    /* 支持alias*/
    if (strcmp(argv[0], "alias") == 0)
    {
        //printf("%s %s\n", argv[0], argv[1]);
        //alias_cmd(argv);
        exit(6); //检测到alias命令后退出状态设置为6，在父进程中修改全局变量all_alias和alias_num
    }

    /* 系统调用 */
    execvp(argv[0], argv); //argv[0]为要执行的程序, argv作为参数传递给该程序
    exit(0);
}

int pid0; //定义成全局变量，是为了能在handler中作为区分父/子进程的标志

void handler(int signum)
{
    //printf("\npid0 = %d\n", pid0);
    if (pid0 == 0)
        exit(3); //退出状态设为3,表示是从shell的子进程返回shell父进程，不是要退出shell本身
    else
        printf("\n");
}

int main()
{
    int status;
    signal(SIGINT, handler); //注意！shell父进程和子进程中都要有signal()，父进程检测到Ctrl+C时输出一个换行符

    while (1) //shell
    {
        pid0 = fork(); //shell分出一个子进程
        if (pid0 == 0) //这是子进程，负责读入和执行命令
        {
            char cmdline[MAX_CMDLINE_LENGTH]; //存储键盘输入的命令行
            char *commands[128];              //由管道操作符'|'分割的命令行各个部分，每个部分是一条命令
            int cmd_count;                    //计算命令的条数

            signal(SIGINT, handler); //当检测到Ctrl+C时退出pid0子进程

            int status1;

            /* 以下是一个死循环，不断等待读取命令和执行命令 */
            while (1)
            {
                /*增加打印当前目录*/
                char buf[MAX_BUF_SIZE];
                getcwd(buf, sizeof(buf));
                printf("my_shell:%s$ ", buf);
                fflush(stdout); //刷新stdout的输出缓冲区，把输出缓冲区里的东西强制打印到标准输出设备

                fgets(cmdline, 256, stdin); //将stdin输入缓冲区中的一行存入cmdline中，最多存入(256-1)个字符，可以读进换行符
                if (cmdline[0] == 0)        //如果cmdline第一个字符为0,说明没有从输入缓冲区读入任何字符，说明遇到了Ctrl-D，退出并返回0表示退出shell
                    exit(0);
                strtok(cmdline, "\n"); //将读入的cmdline根据'\n'进行分解，函数结束后cmdline变成原先字符串的第一个'\n'前的子串

                cmd_count = split_string(cmdline, "|", commands); //根据管道操作符拆解命令行

                if (cmd_count == 0)
                {
                    continue;
                }
                else if (cmd_count == 1)
                {
                    /* 没有管道的单一命令 */
                    char *argv[MAX_CMD_ARG_NUM];
                    //printf("alias_num=%d\n", alias_num);
                    for (int k = 0; k < alias_num; ++k)
                    {
                        //printf("commands[0]=%s\n", commands[0]);
                        if (strcmp(commands[0], all_alias[k].alter) == 0) //该命令是一个别名
                        {
                            //printf("here\n");
                            strcpy(commands[0], all_alias[k].origin);
                        }
                        //printf("commands[0]=%s\n", commands[0]);
                    }
                    int argc = split_string(commands[0], " ", argv);

                    /* 在没有管道时，内置命令直接在主进程中完成，外部命令通过创建子进程完成 */
                    if (exec_builtin(argc, argv) == 0)
                    {
                        continue;
                    }
                    /* 创建子进程，运行外部命令，父进程等待命令运行结束 */
                    int pid = fork();
                    if (pid == 0) //子进程运行命令
                        execute(argc, argv);
                    waitpid(pid, &status1, 0); //等待子进程运行外部命令结束后再继续shell程序
                    //printf("status1=%d\n", WEXITSTATUS(status1));
                    if (WEXITSTATUS(status1) == 6)
                    {
                        //如果子进程的退出状态为6，说明遇到了alias命令，则在父进程中运行alias_cmd函数以添加别名记录
                        alias_cmd(argv);
                    }
                }
                else if (cmd_count == 2)
                {
                    /* 含有一个管道、两条命令 */
                    /* 创建两个子进程，每个子进程执行一条命令 */
                    /* 子进程1向管道写端写入数据，子进程2从管道读端读取数据 */
                    int pipefd[2];
                    int ret = pipe(pipefd); //创建管道, pipe函数执行成功后pipefd数组中已经存了两个文件描述符
                    if (ret < 0)
                    {
                        printf("pipe error!\n");
                        continue;
                    }
                    //子进程1
                    int pid = fork();
                    if (pid == 0)
                    {
                        /* 子进程1将标准输出重定向到管道写端 */
                        close(pipefd[READ_END]);
                        dup2(pipefd[WRITE_END], STDOUT_FILENO);
                        close(pipefd[WRITE_END]);
                        /* 在使用管道时，为了可以并发运行，所以内建命令也在子进程中运行
                           因此我们使用execute函数
                         */
                        char *argv[MAX_CMD_ARG_NUM];
                        /*
                        for (int k = 0; k < alias_num; ++k)
                        {
                            if (strcmp(commands[0], all_alias[alias_num].alter) == 0) //该命令是一个别名
                                strcpy(commands[0], all_alias[alias_num].origin);
                        }
                        */
                        int argc = split_string(commands[0], " ", argv); //注意这里是commands[0]
                        execute(argc, argv);

                        exit(255); //exit子进程1
                    }

                    //因为在shell的设计中，管道是并发执行的，所以我们不在每个子进程结束后才运行下一个
                    //即不使用wait，而是直接创建第二个子进程
                    //子进程2
                    pid = fork();
                    if (pid == 0)
                    {
                        /*子进程2将标准输入重定向到管道读端 */
                        close(pipefd[WRITE_END]);
                        dup2(pipefd[READ_END], STDIN_FILENO);
                        close(pipefd[READ_END]);

                        char *argv[MAX_CMD_ARG_NUM];
                        /*
                        for (int k = 0; k < alias_num; ++k)
                        {
                            if (strcmp(commands[1], all_alias[alias_num].alter) == 0) //该命令是一个别名
                                strcpy(commands[1], all_alias[alias_num].origin);
                        }
                        */
                        int argc = split_string(commands[1], " ", argv); //注意这里是commands[1]
                        execute(argc, argv);

                        exit(255); //exit子进程2
                    }

                    close(pipefd[WRITE_END]);
                    close(pipefd[READ_END]);

                    while (wait(NULL) > 0)
                        ; //子进程结束时wait()的返回值为子进程的pid(>0)，这时while继续循环，等待两个子进程结束
                }
                else
                {
                    /*三条及以上命令、两个及以上管道*/
                    int read_fd; //上一个管道的读端口
                    for (int i = 0; i < cmd_count; ++i)
                    {
                        /* 创建管道，n条命令只需要n-1个管道，最后一次循环不创建管道，但需要做重定向 */
                        int pipefd[2];
                        if (i != cmd_count - 1)
                        {
                            int ret = pipe(pipefd);
                            if (ret < 0)
                            {
                                printf("pipe error!\n");
                                continue;
                            }
                        }
                        int pid = fork();
                        if (pid == 0)
                        {
                            /*除了最后一条命令外，都将标准输出重定向到当前管道的写端 */
                            if (i != cmd_count - 1)
                            {
                                close(pipefd[READ_END]);
                                dup2(pipefd[WRITE_END], STDOUT_FILENO);
                                close(pipefd[WRITE_END]);
                            }
                            /*除了第一条命令外，都将标准输入重定向到上一条管道的读端*/
                            if (i != 0)
                            {
                                dup2(read_fd, STDIN_FILENO);
                                close(read_fd);
                            }

                            /*处理参数，分出命令名和参数，并使用execute运行*/
                            char *argv[MAX_CMD_ARG_NUM];
                            /*
                            for (int k = 0; k < alias_num; ++k)
                            {
                                if (strcmp(commands[i], all_alias[alias_num].alter) == 0) //该命令是一个别名
                                    strcpy(commands[i], all_alias[alias_num].origin);
                            }
                            */
                            int argc = split_string(commands[i], " ", argv); //注意是commands[i]
                            execute(argc, argv);
                            exit(255);
                        }

                        /* 父进程除了第一条命令，都需要关闭当前命令用完的上一个管道读端口
                         * 除了最后一条命令，都需要保存当前命令的管道读端口
                         * 记得关闭父进程没用的管道写端口
                         */
                        if (i != 0)
                            close(read_fd);
                        if (i != cmd_count - 1)
                        {
                            read_fd = pipefd[READ_END]; //注意前面关闭的子进程的读端，父进程中的读文件描述父仍可以使用
                            close(pipefd[WRITE_END]);
                        }
                        //管道是并发运行的，不能用wait()
                        //直接创建下一个子进程
                    }
                    while (wait(NULL) > 0)
                        ; //等待所有子进程结束
                }         //end else
            }             //end while(1)
        }
        else if (pid0 > 0) //这是shell父进程，父进程等待子进程结束
        {
            waitpid(pid0, &status, 0);
            if (status == 0)
                exit(0); //status为0表明用户在shell(实际是shell子进程)中输入了exit，说明用户实际想退出shell程序
        }
    }
} //end main