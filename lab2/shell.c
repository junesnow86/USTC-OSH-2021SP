#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#define MAX_ARG_LENGTH 32       /* max length of arg*/
#define MAX_CMDLINE_LENGTH 1024 /* max cmdline length in a line*/
#define MAX_BUF_SIZE 4096       /* max buffer size */
#define MAX_CMD_ARG_NUM 32      /* max number of single command args */
#define WRITE_END 1             // pipe write end
#define READ_END 0              // pipe read end

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

    do
    {
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

        clip_num++;                                       //处理下一个字符串片段
    } while (string_clips[clip_num] = strtok(NULL, sep)); //若还有其他子字符串，继续while循环
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
    /* TODO: 添加和实现内置指令 */
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
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0 || strcmp(argv[i], "<") == 0)
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

int execute(int argc, char **argv)
{
    if (exec_builtin(argc, argv) == 0)
    { //执行内置指令
        exit(0);
    }
    /* 运行命令 */
    /* TODO:支持基本的文件重定向 */
    //if (argv[1] == ">" || argv[1] == ">>" || argv[1] == "<")
    int orient = FindOrient(argv);
    if (orient >= 0 && strcmp(argv[orient], ">") == 0)
    {

        //int fd = open(argv[2], O_RDWR);
        //printf("%s\n%s\n", argv[1], argv[2]);
        int fd = open(argv[orient + 1], O_RDWR);
        if (fd == -1)
        {
            printf("open %s error!\n", argv[orient + 1]);
            exit(2);
        }
        dup2(fd, STDOUT_FILENO); //将标准输出重定向到argv[2]文件中
        close(fd);
        //free(argv[1]);
        //free(argv[2]);
        argv[orient] = NULL; //这样操作会不会有安全隐患？
        argv[orient + 1] = NULL;
    }
    else if (orient >= 0 && strcmp(argv[orient], ">>") == 0)
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
    else if (orient >= 0 && strcmp(argv[orient], "<") == 0)
    {
        int fd = open(argv[orient + 1], O_RDWR);
        if (fd == -1)
        {
            printf("open %s error!\n", argv[orient + 1]);
            exit(2);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        argv[orient] = NULL;
        argv[orient + 1] = NULL;
    } //end orient if

    if (strcmp(argv[0], "echo") == 0 && strcmp(argv[1], "$0") == 0)
    {
        //echo命令输出普通字符串时可以通过下面的execvp实现，这里针对“echo $0”命令
        char path[PATH_MAX];
        char processname[1024];
        get_exe_name(path, processname, sizeof(path));
        printf("%s%s\n", path, processname);
        exit(0);
    }
    else if (strcmp(argv[0], "echo") == 0 && strcmp(argv[1], "$SHELL") == 0)
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
    while (1)                //shell
    {
        pid0 = fork(); //shell分出一个子进程
        //printf("pid0 = %d\n", pid0);
        if (pid0 == 0) //子进程负责读入和执行命令
        {
            char cmdline[MAX_CMDLINE_LENGTH]; //存储键盘输入的命令行
            char *commands[128];              //由管道操作符'|'分割的命令行各个部分，每个部分是一条命令
            int cmd_count;                    //计算命令的条数

            signal(SIGINT, handler); //当检测到Ctrl+C时退出pid0子进程

            while (1)
            {
                /*增加打印当前目录*/
                char buf[MAX_BUF_SIZE];
                getcwd(buf, sizeof(buf));
                printf("my_shell:%s$ ", buf);
                fflush(stdout); //刷新stdout的输出缓冲区，把输出缓冲区里的东西强制打印到标准输出设备

                fgets(cmdline, 256, stdin); //将stdin输入缓冲区中的一行存入cmdline中，最多存入(256-1)个字符
                //printf("cmdline[0] = %d\n", cmdline[0]);
                if (cmdline[0] == 0) //如果cmdline第一个字符为0,说明没有从输入缓冲区读入任何字符，说明遇到了Ctrl-D，退出并返回0表示退出shell
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
                    int argc = split_string(commands[0], " ", argv);

                    //printf("%s\n", argv[1]);

                    /* 在没有管道时，内置命令直接在主进程中完成，外部命令通过创建子进程完成 */
                    if (exec_builtin(argc, argv) == 0)
                    {
                        continue;
                    }
                    /* 创建子进程，运行外部命令，父进程等待命令运行结束 */
                    int pid = fork();
                    if (pid == 0) //子进程运行命令
                        execute(argc, argv);
                    wait(NULL); //等待子进程运行外部命令结束后再继续shell程序
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
        else if (pid0 > 0) //shell父进程
        {
            waitpid(pid0, &status, 0);
            if (status == 0)
                exit(0); //status为0表明用户在shell(实际是shell子进程)中输入了exit，说明用户实际想退出shell程序
        }
    }
} //end main