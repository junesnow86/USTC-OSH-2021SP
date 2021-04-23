# Lab1

***

梁峻滔	PB19051175

***

## 一、Linux 内核

## 二、初始内存盘

## 三、初识Boot

* `xor ax, ax` 将 `ax` 清零，这样的清零操作在执行过程中只需读取一个寄存器放到ALU中运算，不需要占据额外的存储空间，而且执行速度快。

* `$` 表示当前指令的地址，`jmp $` 就是不断跳转到当前地址。

* 初始输出：

  ![image-20210423210140500](C:\Users\Snowball\AppData\Roaming\Typora\typora-user-images\image-20210423210140500.png)

  新增代码：

  ```assembly
  ; Print "I am OK"
  log_info AddPrint, 8, 4
  
  AddPrint: db 'I am OK!'
  ```

  新增输出：

  ![image-20210423211123749](C:\Users\Snowball\AppData\Roaming\Typora\typora-user-images\image-20210423211123749.png)

  

## 四、思考题

