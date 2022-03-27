---
layout: post
title:  "SYSCALL: 添加零个参数的系统调用"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

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

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B10">实践原理</span>

零参数的系统调用即用户空间不传递参数，直接调用系统调用。这类系统调用很
常见，比如 "sys_getgroups()" 、"sys_getppid()" 等，这类系统在内核的实现
使用 "SYSCALL_DEFINE0()" 进行定义，例如:

{% highlight c %}
SYSCALL_DEFINE0(getppid)
{
        int pid;

        rcu_read_lock();
        pid = task_tgid_vnr(rcu_dereference(current->real_parent));
        rcu_read_unlock();

        return pid;
}
{% endhighlight %}

用户空间调用零参数的系统调用，只需向 "syscall()" 函数传递系统调用号，
例如:

{% highlight c %}
int main(void)
{
	pid_t pid;

	pid = syscall(__NR_getppid);
	return 0;
}
{% endhighlight %}

对于传入参数的类型，开发者可以参考如下文档:

> - [传递整形参数](/blog/SYSCALL_PARAMENTER_INTEGER/)
>
> - [传递字符/字符串参数](/blog/SYSCALL_PARAMENTER_STRINGS/)
>
> - [传递数组参数](/blog/SYSCALL_PARAMENTER_ARRAY/)
>
> - [传递指针参数](/blog/SYSCALL_PARAMENTER_Pointer/)

对于零参数系统调用的返回值，返回的数据类型与传入参数无关，因此开发者
可以根据需求自行定义返回的数据。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B11">实践准备</span>

BiscuitOS 目前支持 6 大平台进行实践，本文以 ARM32 为例子进行讲解，如果
开发者需要在其他平台实践，可以参考下面文档进行实践:

> - [ARM32 架构中添加一个新的系统调用](/blog/SYSCALL_ADD_NEW_ARM/)
>
> - [ARM64 架构中添加一个新的系统调用](/blog/SYSCALL_ADD_NEW_ARM64/)
>
> - [i386 架构中添加一个新的系统调用](/blog/SYSCALL_ADD_NEW_I386/)
>
> - [X86_64 架构中添一个新的系统调用](/blog/SYSCALL_ADD_NEW_X86_64/)
>
> - [RISCV32 架构中添一个新的系统调用](/blog/SYSCALL_ADD_NEW_RISCV32/)
>
> - [RISCV64 架构中添加一个新的系统调用](/blog/SYSCALL_ADD_NEW_RISCV64/)

本实践基于 ARM32 架构，因此在实践之前需要准备一个 ARM32 架构的运行
平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
ARM32 实践环境的开发者，可以参考如下文档进行搭建:

> - [BiscuitOS 上搭建 ARM32 实践环境](/blog/Linux-5.0-arm32-Usermanual/)

开发环境搭建完毕之后，可以继续下面的内容，如果开发者不想采用
BiscuitOS 提供的开发环境，可以继续参考下面的内容在开发者使用
的环境中进行实践。(推荐使用 BiscuitOS 开发环境)。搭建完毕之后，
使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000339.png)

上图显示了 ARM32 实践环境的位置，以及相关的 README.md 文档，开发者
可以参考 README.md 的内容搭建一个运行在 QEMU 上的 ARM32 Linux 开发
环境:

![](/assets/PDB/RPI/RPI000340.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

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

![](/assets/PDB/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](/assets/PDB/RPI/RPI000342.png)

选择 "[\*]   strace" 和 "[\*]   System Call" 并进入 "[\*]   System Call  --->"

![](/assets/PDB/RPI/RPI000343.png)

选择并进入 "[\*]   sys_hello_BiscuitOS  --->"

![](/assets/PDB/RPI/RPI000363.png)

选择 "[\*]   SYSCALL_DEFINE0(): Zero Paramenter --->" 保存配置并退出. 
接下来执行下面的命令部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE0_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE0_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](/assets/PDB/RPI/RPI000428.png)

上图中，main.c 与用户空间系统调用相关的源码,
"SYSCALL_DEFINE0_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。
"SYSCALL_DEFINE0_common-0.0.1/BiscuitOS_syscall.c" 文件是新系统调用
内核实现。因此对于用户空间的系统调用，开发者只需关注 main.c, 内容如下:

![](/assets/PDB/RPI/RPI000365.png)

根据在内核中创建的入口，这里定义了入口宏的值为 400，一定要与内核定义
的入口值相呼应。由于是无参数的系统调用，因此直接使用 "syscall()" 函数，
只需要传入调用号即可, 源码准备好之后，接下来是交叉编译源码并打包到 rootfs
里，最后在 ARM32 上运行。使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE0_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B12">添加内核系统调用入口</span>

ARM32 架构提供了便捷的方法在内核中添加一个新的系统调用入口。
开发者修改内核源码下 "arch/arm/tools/syscall.tbl" 文件，在
该文件的底部添加信息如下:

![](/assets/PDB/RPI/RPI000310.png)

如上面内容所示，在文件最后一行添加了名为 hello_BiscuitOS 的
系统调用，400 代表系统调用号，hello_BiscuitOS 为系统调用的
名字，sys_hello_BiscuitOS 为系统调用在内核的实现。至此系统
号已经添加完毕。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B13">添加内核实现</span>

添加完系统号之后，需要在内核中添加系统调用的具体实现。开发者
可以参考下面的例子进行添加。本例子在内核源码 "fs/" 目录下添加
一个名为 BiscuitOS_syscall.c 的文件，如下:

{% highlight bash %}
cp -rfa BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE0_common-0.0.1/BiscuitOS_syscall.c  BiscuitOS/output/linux-5.0-arm32/linux/linux/fs/
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi BiscuitOS_syscall.c
{% endhighlight %}

![](/assets/PDB/RPI/RPI000395.png)

接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Kconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Makefile
{% endhighlight %}

![](/assets/PDB/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make menuconfig ARCH=arm
{% endhighlight %}

![](/assets/PDB/RPI/RPI000335.png)

选择并进入 "File systems  --->"

![](/assets/PDB/RPI/RPI000336.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

接着重新编译内核。

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j4
{% endhighlight %}

编译内核中会打印相关的信息如下图:

![](/assets/PDB/RPI/RPI000306.png)

从上面的编译信息可以看出，之前的修改已经生效。编译系统调用相关的脚本
自动为hello_BiscuitOS 生成了相关的系统调用，

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B15">运行系统调用</span>

在一切准备好之后，最后一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/RPI/RPI000432.png)

从运行结果可以看到，用户空间的程序已经调用到对应的内核系统调用了。此时
可以使用 strace 工具查看具体的系统调用过程，如下:

{% highlight bash %}
~ #
~ # strace SYSCALL_DEFINE0_common-0.0.1
{% endhighlight %}

![](/assets/PDB/RPI/RPI000433.png)

从 strace 打印的消息可以看出 "syscall_0x190()" 正好程序里产生的系统调用.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="C0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
