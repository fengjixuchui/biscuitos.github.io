---
layout: post
title:  "ARM32 架构增加一个系统调用"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [通用原理](#A0)
>
>   - [原理简介](#A01)
>
>   - [syscall.tbl](#A02)
>
>   - [内核添加系统调用过程](#A03)
>
>   - [内核添加成功的结果](#A04)
>
>   - [用户空间调用新系统调用](#A05)
>
> - 实践部署
>
>   - [添加零参数的系统调用](#B1)
>
>   - [添加一个或多个参数的系统调用](#B2)
>
> - [附录/捐赠](#C0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------

# <span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H0.PNG)

#### 通用原理

> - [简介](#A01)
>
> - [syscall.tbl](#A02)
>
> - [内核添加系统调用过程](#A03)
>
> - [内核添加成功的结果](#A04)
>
> - [用户空间调用新系统调用](#A05)

-------------------------------------------------

#### <span id="A01">简介</span>

系统调用在内核中都是必不可少的一部分，ARM 结构对系统调用的支持相比其他
架构有很多改进，其化繁为简，为开发者提供了一个便捷的方法添加一个新的系统
调用。这里涉及 ARM 架构的系统调用表 syscall.tbl, 以及 ARM 架构会根据
syscall.tbl 生成系统调用所需的多个接口。因此开发者相比其他架构，只需
修改 syscall.tbl 的信息就能便捷创建一个系统调用的入口。有了系统调用的
入口之后，接下来是向内核创建一个系统调用入口的具体实现，也就是这个新的
系统调用要实现的功能。最后准备好以上内容之后就是在用户空间调用这个系统
调用。

------------------------------------------------

#### <span id="A02">syscall.tbl</span>

Linux 5.x 之后，arm 体系增加一个系统调用已经变得很便捷，arm
架构将系统调用的重复性操作从内核移除，开发者只需简单几部就能
添加一个新的系统调用。ARM 架构将所有的系统调用入口信息全部放置
在 "syscall.tbl" 文件中，该文件用于描述 ARM 架构中所有系统调用
入口的信息。在内核源码编译阶段，ARM 架构会运行指定的脚本，将
syscall.tbl 内的系统调用信息转换为真正的系统调用入口。
ARM 架构系统调用的信息放置在:

{% highlight bash %}
arch/arm/tools/syscall.tbl
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000330.png)

syscall.tabl 维护的是一个特定格式的数组，数组中每个成员的组成格式如下:

{% highlight c %}
<num>    <abi>    <name>    [<entry point>    [<oabi compat entry point>]]
{% endhighlight %}

##### num

"\<num>" 代表系统调用号，例如在 ARM 架构中 open 的系统调用号就是 5,
这个信息是一个系统调用必须写入的. 

##### abi 

"\<abi>" 即 ARM 架构的 ABI, 其含义 application binary interface, 
应用程序二进制接口，ARM 架构的 API 是使操作系统与用户空间的接口
代码间的兼容，ABI 使得程序的二进制 (级别) 的兼容, 而 OABI 指的
是 "Old"、"Leacy" 旧的、过时的 ABI; EABI 表示 "Embedded ABI", 
是一种新的 ABI，有时也称为 GNU EABI. "<abi>" 选项支持三种配置:

{% highlight c %}
common 同时兼容 eabi 和 oabi
oabi   支持 legacy ABI
eabi   支持 new ABI
{% endhighlight %}

例如 ARM 架构的 open 系统调用，其 "<abi>" 设置为 "common", 因此
ARM 架构的 open 系统调用同时支持 EABI 和 OABI. 例如 ARM 架构的
stime 系统调用就只支持 OABI.

##### name

"\<name>" 选项是系统调用的名字, 该选项必填。例如 ARM 架构的 open 
系统调用的名字是 "open"。

##### entry point

"\<entry point>" 代表系统调用在内核的接口函数。例如 ARM 架构的
open 系统调用的内核接口是 "sys_open". sys_open 函数定义在内核里，
如下:

{% highlight c %}
SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
        if (force_o_largefile())
                flags |= O_LARGEFILE;

        return do_sys_open(AT_FDCWD, filename, flags, mode);
}
{% endhighlight %}

##### oabi compat entry point

"\<oabi compat entry point>" 代表系统调用的 OABI 接口。对于 "\<abi>"
是 "abi" 和 "common" 的系统调用可以提供 OABI 内核接口。例如 ARM
架构的 ipc 系统调用，其提供了 ABI 调用接口 "sys_ipc", 也提供了 OABI
接口 "sys_oabi_ipc", 其内核定义如下:

{% highlight c %}
SYSCALL_DEFINE6(ipc, unsigned int, call, int, first, unsigned long, second,
                unsigned long, third, void __user *, ptr, long, fifth)
{
	....
}

asmlinkage int sys_oabi_ipc(uint call, int first, int second, int third,
                            void __user *ptr, long fifth)
{
	....
}
{% endhighlight %}

------------------------------------------------

#### <span id="A03">内核添加系统调用过程</span>

首先在内核源码中修改文件 "arch/arm/tools/syscall.tbl" 的底部添加
如下内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000310.png)

如上面内容所示，在文件最后一行添加了名为 hello_BiscuitOS 的
系统调用，400 代表系统调用号，hello_BiscuitOS 为系统调用的
名字，sys_hello_BiscuitOS 为系统调用在内核的实现。接下来，
开发者可以在内核源码的任意位置添加 sys_hello_BiscuitOS 的具体
实现。例如在源码 "fs" 目录下，创建一个名为 BiscuitOS_syscall.c 
的文件，文件内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000332.png)

接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight c %}
cd linux_src/
make menuconfig ARCH=arm
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000335.png)

选择并进入 "File systems  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000336.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

接着重新编译内核。编译内核中会打印相关的信息如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000306.png)

从上面的编译信息可以看出，之前的修改已经生效。编译系统调用相关的脚本
自动为hello_BiscuitOS 生成了相关的系统调用，

----------------------------------------------------

#### <span id="A04">内核添加成功的结果</span>

内核成功编译新添加的系统调用之后，新的系统调用信息已经更新
到多个文件，可以参考下面的内容进行查看。首先可以查看
"arch/arm/include/generated/asm/unistd-nr.h" 已经自动
生成当前系统调用的总数:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000307.png)

可以看出系统调用的总数 __NR_syscalls 变成了 404. 接着
在 "arch/arm/include/generated/uapi/asm/unistd-common.h"
文件中自动生成了系统调用号，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000308.png)

从上面的文件中看到新添加系统调用的系统号为 "__NR_hello_BiscuitOS".
在 "arch/arm/include/generated/" 目录下自动生成两个文件 "calls-eabi.S"
"calls-oabi.S", 该汇编文件中包含了系统调用的入口，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000309.png)

从上图可以看到 hello_BiscuitOS 系统调用的入口为
"NATIVE(400, sys_hello_BiscuitOS)". 内核已经准备好了系统调用的接口，
最后就缺用户空间的调用接口函数。

----------------------------------------------

#### <span id="A05">用户空间调用新系统调用</span>

调用新系统调用的最后就是在用户空间添加一个系统调用的函数，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000337.png)

用户空间可以通过 "syscall()" 函数调用系统调用。对用户空间的程序编译之后在
ARM32 的 Linux 上运行情况如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000338.png)

-----------------------------------

# <span id="B1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### 添加零参数的系统调用

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

对于零参数系统调用的返回值，返回的数据类型与传入参数无关，因此开发者
可以根据需求自行定义返回的数据。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B11">实践准备</span>

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000363.png)

选择 "[\*]   SYSCALL_DEFINE0(): Zero Paramenter --->" 保存配置并退出. 
接下来执行下面的命令部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000428.png)

上图中，main.c 与用户空间系统调用相关的源码,
"SYSCALL_DEFINE0_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。
"SYSCALL_DEFINE0_common-0.0.1/BiscuitOS_syscall.c" 文件是新系统调用
内核实现。因此对于用户空间的系统调用，开发者只需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000365.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B12">添加内核系统调用入口</span>

ARM32 架构提供了便捷的方法在内核中添加一个新的系统调用入口。
开发者修改内核源码下 "arch/arm/tools/syscall.tbl" 文件，在
该文件的底部添加信息如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000310.png)

如上面内容所示，在文件最后一行添加了名为 hello_BiscuitOS 的
系统调用，400 代表系统调用号，hello_BiscuitOS 为系统调用的
名字，sys_hello_BiscuitOS 为系统调用在内核的实现。至此系统
号已经添加完毕。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000395.png)

接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Kconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Makefile
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make menuconfig ARCH=arm
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000335.png)

选择并进入 "File systems  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000336.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

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

在一切准备好之后，最后一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000432.png)

从运行结果可以看到，用户空间的程序已经调用到对应的内核系统调用了。此时
可以使用 strace 工具查看具体的系统调用过程，如下:

{% highlight bash %}
~ #
~ # strace SYSCALL_DEFINE0_common-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000433.png)

从 strace 打印的消息可以看出 "syscall_0x190()" 正好程序里产生的系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="B2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### 添加一个或多个参数的系统调用

> - [实践原理](#B20)
>
> - [实践准备](#B21)
>
> - [添加用户空间实现](#B24)
>
> - [添加内核系统调用入口](#B22)
>
> - [添加内核实现](#B23)
>
> - [运行系统调用](#B25)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B20">实践原理</span>

一个或多个参数的系统调用即用户空间传递一个或多个参数来调用系统调用。
这类系统调用很常见，比如 "sys_times()" 、"sys_getpgid()" 等，
这类系统在内核的实现使用 "SYSCALL_DEFINE1()"、"SYSCALL_DEFINE2()" 
等宏进行定义，例如:

{% highlight c %}
SYSCALL_DEFINE1(getpgid, pid_t, pid)
{
        return do_getpgid(pid);
}

SYSCALL_DEFINE2(utime, char __user *, filename, struct utimbuf __user *, times)
{
        struct timespec64 tv[2];

        if (times) {
                if (get_user(tv[0].tv_sec, &times->actime) ||
                    get_user(tv[1].tv_sec, &times->modtime))
                        return -EFAULT;
                tv[0].tv_nsec = 0;
                tv[1].tv_nsec = 0;
        }
        return do_utimes(AT_FDCWD, filename, times ? tv : NULL, 0);
}
{% endhighlight %}

用户空间调用一个参数的系统调用，需向 "syscall()" 函数传递系统调用号，
以及一个参数，例如:

{% highlight c %}
int main(void)
{
	pid_t pid = 978, current_pid;
	char *filename = "/BiscuitOS_file";
	struct utimebuf buf;

	current_pid = syscall(__NR_getpgid, pid);

	syscall(__NR_utime, filename, &buf);
	return 0;
}
{% endhighlight %}

对于一个或多个参数系统调用的返回值，返回的数据类型与传入参数无关，
因此开发者可以根据需求自行定义返回的数据.

-------------------------------------------

#### <span id="B21">实践准备</span>

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

#### <span id="B24">添加用户空间实现</span>

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000366.png)

选择 "[\*]   SYSCALL_DEFINE1(): One Paramenter  --->" 保存配置并退出. 
接下来执行下面的命令部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE1_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE1_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000429.png)

上图中，main.c 与用户空间系统调用相关的源码,
"SYSCALL_DEFINE1_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。
"SYSCALL_DEFINE1_common-0.0.1/BiscuitOS_syscall.c" 是系统调用的内核实现。
因此开发者只需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000368.png)

根据在内核中创建的入口，这里定义了入口宏的值为 400，一定要与内核定义
的入口值相呼应。由于是多个参数的系统调用，因此直接使用 "syscall()" 函数，
传入调用号和多个参数, 用户空间传递字符串 "BiscuitOS_Userpsace" 到内核
空间，并期待内核空间传递字符串到用户空间，如果成功，则打印字符串.
源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里, 使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/SYSCALL_DEFINE1_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B22">添加内核系统调用入口</span>

ARM32 架构提供了便捷的方法在内核中添加一个新的系统调用入口。
开发者修改内核源码下 "arch/arm/tools/syscall.tbl" 文件，在
该文件的底部添加信息如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000310.png)

如上面内容所示，在文件最后一行添加了名为 hello_BiscuitOS 的
系统调用，400 代表系统调用号，hello_BiscuitOS 为系统调用的
名字，sys_hello_BiscuitOS 为系统调用在内核的实现。至此系统
号已经添加完毕。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B23">添加内核实现</span>

添加完系统号之后，需要在内核中添加系统调用的具体实现。开发者
可以参考下面的例子进行添加。本例子在内核源码 "fs/" 目录下添加
一个名为 BiscuitOS_syscall.c 的文件，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi BiscuitOS_syscall.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000349.png)

接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Kconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/fs
vi Makefile
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make menuconfig ARCH=arm
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000335.png)

选择并进入 "File systems  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000336.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

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

#### <span id="B25">运行系统调用</span>

在一切准备好之后，最后一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000430.png)

从运行结果可以看到，用户空间的程序已经调用到对应的内核系统调用了。此时
可以使用 strace 工具查看具体的系统调用过程，如下:

{% highlight bash %}
~ #
~ # strace SYSCALL_DEFINE1_common-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000431.png)

从 strace 打印的消息可以看出 "syscall_0x190()" 正好程序里产生的系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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
