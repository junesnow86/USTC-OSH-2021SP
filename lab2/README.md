# Lab 2

## 使用 strace 工具追踪系统调用

准备好 `strace` 工具后，在终端中输入以下命令：

```
strace ./Desktop/Codes/C/shell
```

得到以下输出：

<img src="/home/snowball/Desktop/syscalls.png" alt="image-20210518222346940"  />

其中没有在我的 shell 代码中出现、在此出现次数较多的3个系统调用有 `mmap`，`mprotect`，`fstat` 。以下简述它们的功能：

* `mmap`：函数原型

  ```c
  void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);
  ```

  作用是将一个文件或者其他对象映射进内存，成功执行时返回映射区的指针，即参数 start。`mmap` 让用户程序可以通过其访问设备内存，也使得进程之间通过映射同一个普通文件实现共享内存，进程可以像访问内存一样对文件进行操作，不必再调用 `read`，`write` 等操作。

  start：映射区的开始地址

  length：映射区的长度(以字节为单位)

  prot：期望的内存保护标志(规定这些页的内容是否可以被执行、读取或写入，或者是不可访问)

  flags：指定映射对象的类型

  fd：有效的文件描述符

  offset：被映射对象内容的起点

* `mprotect`：函数原型

  ```C
  int mprotect(const void *start, size_t len, int prot);
  ```

  作用是把自 start 开始的、长度为 len 的内存区的保护属性修改为 prot 指定的值，执行成功则返回0，否则返回-1。 

* `fstat`：函数原型

  ```c
  int fstat(int fildes, struct stat *buf);
  ```

  作用是将文件描述符 fildes 所指的文件的状态，复制到参数 buf 所指的结构体中，执行成功则返回0，否则返回-1。

## shell.c 中实现的选做功能

1. `echo $SHELL`：获取环境变量 `SHELL` 并显示在终端

![env](/home/snowball/Desktop/env.jpg)

​		它与 `echo $0` 有区别，`echo $0` 是获取并显示当前正在执行的 shell 的名称，而 `echo $SHELL` 是获取所有 `SHELL` 变量并显示，我的系统中`SHELL`变量的值就只有/bin/bash，如果直接在终端执行，则效果与 `echo $0` 相同。`echo $SHELL` 可以使用 `getenv` 函数实现，`echo $0` 则需要使用 `readlink` 、`strrchr` 函数来实现。

![echo_cmd](/home/snowball/Desktop/echo_cmd.jpg)

​		如上图所示，执行 `echo $SHELL` 命令后输出的是系统的 `SHELL` 变量的值，执行 `echo $0` 后显示当前执行的程序 shell 的路径和名称。

2. `A=1 env`：我在网上翻看了很多介绍 `env` 命令的文章都没有看到过有人介绍这种用法，即使是在终端中使用 `env --help` 也没有相关介绍，在[官方文档](https://www.gnu.org/software/coreutils/env)中也没有相关介绍(也可能我看漏了...)，自己在终端中运行观察，推测该语法的作用是在打印环境变量信息时在首行加入 `A=1` (只能是这种赋值的形式，否则会报告找不到命令)，但并没有在环境变量中加入，在下一次执行 `env` 时首行的 `A=1` 会消失。

![a=2333env](/home/snowball/Desktop/a=2333env.jpg)

​		因此可以通过在执行 `env` 前简单地先打印前面的赋值语句来实现。





