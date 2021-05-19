# Lab2

## 使用 strace 工具追踪系统调用

准备好 `strace` 工具后，在终端中输入以下命令：

```
strace ./Desktop/Codes/C/shell
```

得到以下输出：

![strace syscalls](https://github.com/LiangJuntao990113/OSH-2021-Labs/blob/main/lab2/pictures/syscalls.png)

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





