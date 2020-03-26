---
layout: post
title:  "系统调用实践建议"
date:   2020-03-10 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

> - [解决系统调用函数打印干扰问题](#B1)
>
> - [解决回调函数定位问题](#B2)


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------------

<span id="B1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### 解决系统调用函数打印干扰问题

在调试系统调用的时候，需要在内核空间对某个系统调用实现过程添加打印消息，
例如在 sys_open 系统调用中，添加了一些打印消息，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000512.png)

如上图添加了一行打印在 sys_open 系统调用函数，重编译内核并运行系统，情况:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000513.png)

结果可以看到，只要调用 sys_open 系统调用的都会打印这一行消息，因此
这给开发者带来了很大的困惑，为了解决这个办法，这里提出一个可行的解决
办法，以便实现只有每次在用户空间触发系统调用的时候，才会打印这行信息，
其他时间均不打印。解决这个问题的思路就是向系统添加一个新的系统调用，这个
系统调用用于控制调试信息开关，用户空间需要打开调试信息的时候，就打开，
不需要的时候就关闭，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000514.png)

而在内核空间则使用如下打印方法:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000515.png)

从新编译内核并运行系统，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000516.png)

从上面的实践结果可以看出，这个办法解决了打印混乱问题，这个系统调用调试
过程中非常有用。具体实现过程请参考下面内容:

> - [实践原理](#B10)
>
> - [实践准备](#B11)
>
> - [添加用户空间实现](#B14)
>
> - [添加内核系统调用入口](#B12)
>
> - [添加内核实现](#B13)
>
> - [运行系统调用](#B15)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B10">实践原理</span>

解决办法是通过在系统中新添加一个系统调用，用户空间可以通过系统调用打开
或关闭打印开关，当打开的时候特殊的打印函数才会打印，关闭的时候就不打印
保持静默。

-------------------------------------------

#### <span id="B11">实践准备</span>

BiscuitOS 目前支持 6 大平台进行实践，本文以 ARM32 为例子进行讲解，如果
开发者需要在其他平台实践，可以参考下面文档进行实践:

> - [ARM32 架构中添加一个新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_ARM/)
>
> - [ARM64 架构中添加一个新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_ARM64/)
>
> - [i386 架构中添加一个新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_I386/)
>
> - [X86_64 架构中添一个新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_X86_64/)
>
> - [RISCV32 架构中添一个新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_RISCV32/)
>
> - [RISCV64 架构中添加一个新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_RISCV64/)

本实践基于 ARM32 架构，因此在实践之前需要准备一个 ARM32 架构的运行
平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
ARM32 实践环境的开发者，可以参考如下文档进行搭建:

> - [BiscuitOS 上搭建 ARM32 实践环境](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

开发环境搭建完毕之后，可以继续下面的内容，如果开发者不想采用
BiscuitOS 提供的开发环境，可以继续参考下面的内容在开发者使用
的环境中进行实践。(推荐使用 BiscuitOS 开发环境)。搭建完毕之后，
使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

上图显示了 ARM32 实践环境的位置，以及相关的 README.md 文档，开发者
可以参考 README.md 的内容搭建一个运行在 QEMU 上的 ARM32 Linux 开发
环境:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000340.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B14">添加用户空间实现</span>

BiscuitOS 提供了一套完整的系统调用编译系统，开发者可以使用下面步骤部署一个
简单的用户空间调用接口文件，BiscuitOS 并可以对该文件进行交叉编译，安装，
打包和目标系统上运行的功能，节省了很多开发时间。如果开发者不想使用这套
编译机制，可以参考下面的内容进行移植。开发者首先获得用户空间系统调用
基础源码，如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000342.png)

选择 "[\*]   strace" 和 "[\*]   System Call" 并进入 "[\*]   System Call  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000343.png)

选择并进入 "[\*]   sys_hello_BiscuitOS  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000517.png)

选择 "[\*]   debug_BiscuitOS() in C  --->" 保存配置并退出. 
接下来执行下面的命令部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/debug_BiscuitOS_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/debug_BiscuitOS_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000518.png)

上图中，main.c 与用户空间系统调用相关的源码,
"debug_BiscuitOS_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。
"debug_BiscuitOS_common-0.0.1/BiscuitOS_debug.c" 文件是新系统调用
内核实现。因此对于用户空间的系统调用，开发者只需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000519.png)

根据在内核中创建的入口，这里定义了入口宏的值为 400，一定要与内核定义
的入口值相呼应. 如果需要打开打印消息，那么调用:

{% highlight c %}
syscall(__NR_debug_BiscuitOS, 1);
{% endhighlight %}

关闭打印消息则调用:

{% highlight c %}
syscall(__NR_debug_BiscuitOS, 0);
{% endhighlight %}

源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里，最后在 ARM32 
上运行。使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/debug_BiscuitOS_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B12">添加内核系统调用入口</span>

ARM32 架构提供了便捷的方法在内核中添加一个新的系统调用入口。
开发者修改内核源码下 "arch/arm/tools/syscall.tbl" 文件，在
该文件的底部添加信息如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000520.png)

如上面内容所示，在文件最后一行添加了名为 debug_BiscuitOS 的
系统调用，400 代表系统调用号，debug_BiscuitOS 为系统调用的
名字，sys_debug_BiscuitOS 为系统调用在内核的实现。至此系统
号已经添加完毕。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B13">添加内核实现</span>

添加完系统号之后，需要在内核中添加系统调用的具体实现。开发者
可以参考下面的例子进行添加。本例子在内核源码 "fs/" 目录下添加
一个名为 BiscuitOS_syscall.c 的文件，如下:

{% highlight bash %}
cp -rfa BiscuitOS/output/linux-5.0-arm32/package/debug_BiscuitOS_common-0.0.1/BiscuitOS_debug.c  BiscuitOS/output/linux-5.0-arm32/linux/linux/fs/
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi BiscuitOS_debug.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000521.png)

文件定义了一个 int 全局变量 bs_debug_enable, 并使用 EXPORT_SYMBOL() 将符号
导出。文件进行使用 SYSCALL_DEFINE1() 宏定义了一个名为 sys_debug_BiscuitOS
的系统调用，该系统调用函数从内核空间接收一个整形数值，如果该值为 1，那么
将 bs_debug_enable 设置为 1; 反之如果该为 0，则将 bs_debug_enable 设置
0。接着在建议在:

{% highlight bash %}
include/linux/kernel.h
{% endhighlight %}

开发者也可以在其他头文件添加如下内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000522.png)

准备好源码之后，接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Kconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000524.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Makefile
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000525.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make menuconfig ARCH=arm
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000335.png)

选择并进入 "File systems  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000523.png)

选择 "\[*] BiscuitOS syscall debug" 并保存内核配置。

接着重新编译内核。

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j4
{% endhighlight %}

编译内核中会打印相关的信息如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000306.png)

从上面的编译信息可以看出，之前的修改已经生效。编译系统调用相关的脚本
自动为hello_BiscuitOS 生成了相关的系统调用，

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B15">运行系统调用</span>

至此一切准备完成，开发者可以尝试在某个系统调用里使用这个功能，例如
在 sys_open 中使用该功能，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000514.png)

接着在内核 sys_open 系统调用函数中添加信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000515.png)

重新编译内核和应用空间程序，最后一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000516.png)

从运行的结果可以看出，当在用户空间单独调用 sys_open 的时候，内核只打印
一次信息，因此该功能添加成功.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------------

<span id="B2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### 解决回调函数定位问题

在内核调试系统调用的时候，经常遇到如下情况:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000526.png)

从上面的代码开发者很难定位 "dentry->d_op->d_init()" 对应的到底是哪个函数，
这个源码调试带来了很大的难度，为了解决这个办法，开发者可以按如下办法进行
解决:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000527.png)

正如上图所示，使用 printk 函数打印 "dentry->d_op->d_init" 的地址，重编译
内核之后，运行系统，可以获得一个具体的打印值 0x80008144, 然后查看源码目录下的:

{% highlight bash %}
System.map
{% endhighlight %}

通过地址可以查到如下信息:

{% highlight bash %}
80008144 t __d_ramfs_init
{% endhighlight %}

于是 "dentry->d_op->d_init()" 对应的函数就是 "__d_ramfs_init".

-----------------------------------------------

#### <span id="C0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
