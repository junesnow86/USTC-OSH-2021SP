# Lab1

***

梁峻滔	PB19051175

2021 年 4 月 24 日

***

<!-- GFM-TOC -->

* [一、初始内存盘](https://github.com/LiangJuntao990113/OSH-2021-Labs/blob/main/lab1/docs/Lab1.md#%E4%B8%80%E5%88%9D%E5%A7%8B%E5%86%85%E5%AD%98%E7%9B%98)
* [二、Linux内核和执行测试程序](https://github.com/LiangJuntao990113/OSH-2021-Labs/blob/main/lab1/docs/Lab1.md#%E4%BA%8Clinux-%E5%86%85%E6%A0%B8%E5%92%8C%E6%89%A7%E8%A1%8C%E6%B5%8B%E8%AF%95%E7%A8%8B%E5%BA%8F)
* [三、初识Boot](https://github.com/LiangJuntao990113/OSH-2021-Labs/blob/main/lab1/docs/Lab1.md#%E4%B8%89%E5%88%9D%E8%AF%86boot)
* [四、思考题](https://github.com/LiangJuntao990113/OSH-2021-Labs/blob/main/lab1/docs/Lab1.md#%E5%9B%9B%E6%80%9D%E8%80%83%E9%A2%98)

<!-- GFM-TOC -->

## 一、初始内存盘

* 构建 `initrd.cpio.gz` 文件，见github。

* init.c

  ```c
  #include<stdio.h>
  #include<stdlib.h>
  #include<sys/types.h>
  #include<sys/stat.h>
  #include<fcntl.h>
  #include<unistd.h>
  #include<sys/sysmacros.h>
  #include<sys/wait.h>
  
  int main()
  {
  	//create device files
  	if(mknod("./null", S_IFCHR | S_IRUSR | S_IWUSR, makedev(1, 3)) == -1)
  	{
  		perror("mknod() failed");
  	}
  	if(mknod("/dev/ttyS0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(4, 64)) == -1)
  	{
  		perror("mknod() failed");
  	}
  	if(mknod("/dev/ttyAMA0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(204, 64)) == -1)
  	{
  		perror("mknod() failed");
  	}
  	if(mknod("/dev/fb0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(29, 0)) == -1)
  	{
  		perror("mknod() failed");
  	}
  	printf("here\n\n");
  	//call 3 test procedures
  	if(fork() == 0)
  	{
  		if((execl("/tools/binary/1","1","execl",NULL)) == -1)
  		{
  			perror("execl");
  			exit(1);
  		}
  	}
  	sleep(3);
  	if(fork() == 0)
  	{
  		if((execl("/tools/binary/2","2","execl",NULL)) == -1)
  		{
  			perror("execl");
  			exit(1);
  		}
  	}
  	sleep(3);
  	if(fork() == 0)
  	{
  		if((execl("/tools/binary/3","3","execl",NULL)) == -1)
  		{
  			perror("execl");
  			exit(1);
  		}
  	}
  	while(1);
  	return 0;
  }
  ```

## 二、Linux 内核和执行测试程序

* 编译适用于树莓派的内核和裁剪内核

  ![image-20210424134527784](C:\Users\Snowball\AppData\Roaming\Typora\typora-user-images\image-20210424134527784.png)

* 执行测试程序

  ![image-20210424134852929](C:\Users\Snowball\AppData\Roaming\Typora\typora-user-images\image-20210424134852929.png)

## 三、初识Boot

* 构建 `bootloader.img` 文件，见github。

* `xor ax, ax` 将 `ax` 清零，这样的清零操作在执行过程中只需读取一个寄存器放到ALU中运算，不需要占据额外的存储空间，而且执行速度快。

* `$` 表示当前指令的地址，`jmp $` 就是不断跳转到当前地址。

* 初始输出：

  ![image-20210423210140500](C:\Users\Snowball\AppData\Roaming\Typora\typora-user-images\image-20210423210140500.png)

  在loader.asm中新增代码：

  ```assembly
  ; Print "I am OK"
  log_info AddPrint, 8, 4
  
  AddPrint: db 'I am OK!'
  ```

  新增输出：

  ![image-20210423211123749](C:\Users\Snowball\AppData\Roaming\Typora\typora-user-images\image-20210423211123749.png)

## 四、思考题

1. `Linux` 是一种操作系统，而`Ubuntu`、`Debian`、`Arch Linux`、`Fedora` 是不同的 `Linux ` 发行版，包含 `Linux` 内核和支撑内核的实用程序和库，通常还带有大量可以满足各类需求的应用程序。`Debian` 是迄今为止最遵循 `GNU` 规范的 Linux 系统 ，更新方式是 `APT`，软件包管理工具主要包括 `dpkg`和`apt-get`  ，特点是稳定性和安全性很高，很多服务器都是用 `Debian` 作为其操作系统；`Ubuntu` 是以桌面应用为主的 GNU/Linux 操作系统，特点是界面非常友好，更新方式是高级包装工具、Software Upadater 或 Ubuntu软件中心；`Fedora` 是集最新自由开源软件于一体的操作系统，始终允许任何人自由使用、修改和发布，是许多新技术的测试平台，更新方式是 `DNF` ，软件包管理系统是 `RPM` ；`Arch Linux` 注重代码正确、优雅和极简主义，特点是使用简单、系统轻量、软件更新速度快，软件包管理器叫 `pacman` 。

2. 本实验不需要把内核装到SD卡上，因为本实验所运行的三个测试程序虽然需要通过内核来运行，但并不需要在树莓派上运行，把内核装到SD卡上是为了能把内核装载到树莓派上。这样，本实验在进行内核裁剪时就可以把与SD卡相关的驱动等移除或者改为模块，而模块是可以按需随时装入和卸下的，有助于减小内核大小。

3. 树莓派启动过程：

   ![raspi setup](D:\桌面\raspi setup.png)

4. pass

5. * `binfmt_misc` 是 Linux 内核的一项功能，其使得内核可识别任意类型的可执行文件格式，并传递至特定的用户空间应用程序，如模拟器和虚拟机。`arm64` 的 `busybox` 的可执行文件格式，通过 `binfmt_misc` 注册后，就可以像原生二进制库一样被 `qemu` 运行。

   * `user mode` 和 `system mode` 是 qemu 的两种配置方式。qemu 在 `system mode` 配置下模拟出整个计算机，可以在 qemu 上运行一个操作系统；qemu 在 `user mode` 配置方式下，可以运行跟当前平台指令集不同的平台可执行程序。`qemu-system` 用于模拟运行操作系统，`qemu-user` 则用于运行可执行用户程序。

     

     