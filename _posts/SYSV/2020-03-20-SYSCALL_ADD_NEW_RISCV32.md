---
layout: post
title:  "RISCV32 架构增加一个系统调用"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [通用原理](#A0)
>
>   - [原理简介](#A01)
>
>   - [内核添加系统调用过程](#A03)
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

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------

# <span id="A0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H0.PNG)

#### 通用原理

> - [原理简介](#A01)
>
> - [内核添加系统调用过程](#A03)
>
> - [用户空间调用新系统调用](#A05)

-------------------------------------------------

#### <span id="A01">原理简介</span>

系统调用在内核中都是必不可少的一部分，RISCV32 架构支持的系统调用达到
295 个 (linux 5.0). 由于 RISCV32 架构系统调用位于 __NR_arch_specific_syscall
之后到 260 之间的系统调用号，其系统调用入口定义在:

{% highlight bash %}
arch/riscv/include/uapi/asm/unistd.h
{% endhighlight %}

开发者向该文件添加新系统调用入口信息，接着修改:

{% highlight bash %}
arch/riscv/include/asm/vdso.h
{% endhighlight %}

向上面文件添加系统调用声明信息，就可以创建新的入口。系统调用
内核部分和用户空间部分与其他架构无差异。

------------------------------------------------

#### <span id="A03">内核添加系统调用过程</span>

添加内核系统调用的接口，首先需要确定下一个可用的系统调用号，
开发者可以参考:

{% highlight bash %}
arch/riscv/include/uapi/asm/unistd.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000401.png)

从上图可以知道，RISCV32 架构里，新增加的系统调用号与 
"\_\_NR_arch_specific_syscall" 有关，接着可以查看该宏的定义，
位于源码:

{% highlight bash %}
include/uapi/asm-generic/unistd.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000402.png)

从上面的定义可以看出，linux 为体系预留了 244 到 260 之间的系统调用号，
因此结合上述两个信息，新系统调用的系统调用号可以取 258, 即:

{% highlight bash %}
syscall_nr = __NR_arch_specific_syscall + 14 = 244 + 14 = 258
{% endhighlight %}

接下来，在 "arch/riscv/include/uapi/asm/unistd.h" 文件中添加新的系统
调用信息:

![](/assets/PDB/RPI/RPI000403.png)

在该文件中，首先定义了新添加系统调用号的名字为 "\_\_NR_hello_BiscuitOS",
其具体值为之前查找的下一个可用系统调用号 258，也可以按格式写成
"\_\_NR_arch_specific_syscall + 14". 接着使用宏 "\_\_SYSCALL_()" 
创建一个新的系统调用入口，该宏的第一个参数设置为新系统调用号，
即 "\_\_NR_hello_BiscuitOS", 该宏的第二个参数设置为系统调用内核实现，
这里填写 "sys_hello_BiscuitOS", 该函数就是新系统调用的具体实现。
添加完上面的信息后，还需要添加函数声明，位于文件:

{% highlight bash %}
arch/riscv/include/asm/vdso.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000404.png)

使用 asmlinkage 关键字，函数返回值类型为 long. 函数名为 
"sys_hello_BiscuitOS". 至此新系统调用的入口制作完成。接下来，
开发者可以在内核源码的任意位置添加 sys_hello_BiscuitOS 的具体
实现。例如在源码 "fs" 目录下，创建一个名为 BiscuitOS_syscall.c 
的文件，文件内容如下:

![](/assets/PDB/RPI/RPI000395.png)

接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

![](/assets/PDB/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

![](/assets/PDB/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight c %}
cd linux_src/
make menuconfig ARCH=riscv
{% endhighlight %}

![](/assets/PDB/RPI/RPI000406.png)

选择并进入 "File systems  --->"

![](/assets/PDB/RPI/RPI000407.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

接着重新编译内核。编译内核中会打印相关的信息如下图:

![](/assets/PDB/RPI/RPI000408.png)

从上面的编译信息可以看出，之前的修改已经生效。编译系统调用相关的脚本
自动为hello_BiscuitOS 生成了相关的系统调用，

----------------------------------------------------

#### <span id="A05">用户空间调用新系统调用</span>

调用新系统调用的最后就是在用户空间添加一个系统调用的函数，如下:

![](/assets/PDB/RPI/RPI000365.png)

用户空间可以通过 "syscall()" 函数调用系统调用。对用户空间的程序编译之后在
RISCV32 的 Linux 上运行情况如下:

![](/assets/PDB/RPI/RPI000409.png)

-----------------------------------

# <span id="B1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

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

对于零参数系统调用的返回值，返回的数据类型与传入参数无关，因此开发者
可以根据需求自行定义返回的数据。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B11">实践准备</span>

本实践基于 RISCV32 架构，因此在实践之前需要准备一个 RISCV32 架构的运行
平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
RISCV32 实践环境的开发者，可以参考如下文档进行搭建:

> - [BiscuitOS 上搭建 RISCV32 实践环境](/blog/Linux-5.0-riscv32-Usermanual/)

开发环境搭建完毕之后，可以继续下面的内容，如果开发者不想采用
BiscuitOS 提供的开发环境，可以继续参考下面的内容在开发者使用
的环境中进行实践。(推荐使用 BiscuitOS 开发环境)。搭建完毕之后，
使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-riscv32_defconfig
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000405.png)

上图显示了 RISCV32 实践环境的位置，以及相关的 README.md 文档，开发者
可以参考 README.md 的内容搭建一个运行在 QEMU 上的 RISCV32 Linux 开发
环境:

![](/assets/PDB/RPI/RPI000410.png)

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
make linux-5.0-riscv32_defconfig
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

![](/assets/PDB/RPI/RPI000405.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE0_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE0_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](/assets/PDB/RPI/RPI000411.png)

上图中，main.c 与用户空间系统调用相关的源码, 
"SYSCALL_DEFINE0_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。
"SYSCALL_DEFINE0_common-0.0.1/BiscuitOS_syscall.c" 文件是新系统调用
内核实现。因此对于用户空间的系统调用，开发者只需关注 main.c, 内容如下:

![](/assets/PDB/RPI/RPI000365.png)

根据在内核中创建的入口，这里定义了入口宏的值为 258，一定要与内核定义
的入口值相呼应。由于是无参数的系统调用，因此直接使用 "syscall()" 函数，
只需要传入调用号即可, 源码准备好之后，接下来是交叉编译源码并打包到 rootfs
里，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE0_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B12">添加内核系统调用入口</span>

添加内核系统调用的接口，首先需要确定下一个可用的系统调用号，
开发者可以参考:

{% highlight bash %}
arch/riscv/include/uapi/asm/unistd.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000401.png)

从上图可以知道，RISCV32 架构里，新增加的系统调用号与 
"\_\_NR_arch_specific_syscall" 有关，接着可以查看该宏的定义，
位于源码:

{% highlight bash %}
include/uapi/asm-generic/unistd.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000402.png)

从上面的定义可以看出，linux 为体系预留了 244 到 260 之间的系统调用号，
因此结合上述两个信息，新系统调用的系统调用号可以取 258, 即:

{% highlight bash %}
syscall_nr = __NR_arch_specific_syscall + 14 = 244 + 14 = 258
{% endhighlight %}

接下来，在 "arch/riscv/include/uapi/asm/unistd.h" 文件中添加新的系统
调用信息:

![](/assets/PDB/RPI/RPI000403.png)

在该文件中，首先定义了新添加系统调用号的名字为 "\_\_NR_hello_BiscuitOS",
其具体值为之前查找的下一个可用系统调用号 258，也可以按格式写成
"\_\_NR_arch_specific_syscall + 14". 接着使用宏 "\_\_SYSCALL_()" 
创建一个新的系统调用入口，该宏的第一个参数设置为新系统调用号，
即 "\_\_NR_hello_BiscuitOS", 该宏的第二个参数设置为系统调用内核实现，
这里填写 "sys_hello_BiscuitOS", 该函数就是新系统调用的具体实现。
添加完上面的信息后，还需要添加函数声明，位于文件:

{% highlight bash %}
arch/riscv/include/asm/vdso.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000404.png)

使用 asmlinkage 关键字，函数返回值类型为 long. 函数名为
"sys_hello_BiscuitOS". 至此新系统调用的入口制作完成。接下来，
开发者可以在内核源码的任意位置添加 sys_hello_BiscuitOS 的具体
实现.

--------------------------------------------

#### <span id="B13">添加内核实现</span>

添加完系统号之后，需要在内核中添加系统调用的具体实现。开发者
可以参考下面的例子进行添加。本例子在内核源码 "fs/" 目录下添加
一个名为 BiscuitOS_syscall.c 的文件，如下:

{% highlight bash %}
cp -rfa BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE0_common-0.0.1/BiscuitOS_syscall.c  BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs/
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs
vi BiscuitOS_syscall.c
{% endhighlight %}

![](/assets/PDB/RPI/RPI000395.png)

接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs
vi Kconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs
vi Makefile
{% endhighlight %}

![](/assets/PDB/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/
make menuconfig ARCH=riscv
{% endhighlight %}

![](/assets/PDB/RPI/RPI000406.png)

选择并进入 "File systems  --->"

![](/assets/PDB/RPI/RPI000407.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

接着重新编译内核。

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/
make ARCH=riscv CROSS_COMPILE=BiscuitOS/output/linux-5.0-riscv32/riscv32-biscuitos-linux-gnu/riscv32-biscuitos-linux-gnu/bin/riscv32-biscuitos-linux-gnu- vmlinux -j4
{% endhighlight %}

编译内核中会打印相关的信息如下图:

![](/assets/PDB/RPI/RPI000408.png)

从上面的编译信息可以看出，之前的修改已经生效。编译系统调用相关的脚本
自动为hello_BiscuitOS 生成了相关的系统调用，

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B15">运行系统调用</span>

在一切准备好之后，最后一步就是在 RISCV32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/RPI/RPI000412.png)

从运行结果可以看到，用户空间的程序已经调用到对应的内核系统调用了。此时
可以使用 strace 工具查看具体的系统调用过程，如下:

{% highlight bash %}
~ #
~ # strace SYSCALL_DEFINE0_common-0.0.1
{% endhighlight %}

![](/assets/PDB/RPI/RPI000413.png)

从 strace 打印的消息可以看出 "syscall_0x102()" 正好程序里产生的系统调用.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="B2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

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

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

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

对于传入多个参数的系统调用以及参数类型，开发者可以参考如下文档:

> - [添加零个参数的系统调用](/blog/SYSCALL_PARAMENTER_ZERO/)
>
> - [添加一个参数的系统调用](/blog/SYSCALL_PARAMENTER_ONE/)
>
> - [添加两个参数的系统调用](/blog/SYSCALL_PARAMENTER_TWO/)
>
> - [添加三个参数的系统调用](/blog/SYSCALL_PARAMENTER_THREE/)
>
> - [添加四个参数的系统调用](/blog/SYSCALL_PARAMENTER_FOUR/)
>
> - [添加五个参数的系统调用](/blog/SYSCALL_PARAMENTER_FIVE/)
>
> - [添加六个参数的系统调用](/blog/SYSCALL_PARAMENTER_SIX/)
>
> - [传递整形参数](/blog/SYSCALL_PARAMENTER_INTEGER/)
>
> - [传递字符/字符串参数](/blog/SYSCALL_PARAMENTER_STRINGS/)
>
> - [传递数组参数](/blog/SYSCALL_PARAMENTER_ARRAY/)
>
> - [传递指针参数](/blog/SYSCALL_PARAMENTER_Pointer/)

对于一个或多个参数系统调用的返回值，返回的数据类型与传入参数无关，
因此开发者可以根据需求自行定义返回的数据.

-------------------------------------------

#### <span id="B21">实践准备</span>

本实践基于 RISCV32 架构，因此在实践之前需要准备一个 RISCV32 架构的运行
平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
RISCV32 实践环境的开发者，可以参考如下文档进行搭建:

> - [BiscuitOS 上搭建 RISCV32 实践环境](/blog/Linux-5.0-arm64-Usermanual/)

开发环境搭建完毕之后，可以继续下面的内容，如果开发者不想采用
BiscuitOS 提供的开发环境，可以继续参考下面的内容在开发者使用
的环境中进行实践。(推荐使用 BiscuitOS 开发环境)。搭建完毕之后，
使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-riscv32_defconfig
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000405.png)

上图显示了 RISCV32 实践环境的位置，以及相关的 README.md 文档，开发者
可以参考 README.md 的内容搭建一个运行在 QEMU 上的 RISCV32 Linux 开发
环境:

![](/assets/PDB/RPI/RPI000410.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B24">添加用户空间实现</span>

BiscuitOS 提供了一套完整的系统调用编译系统，开发者可以使用下面步骤部署一个
简单的用户空间调用接口文件，BiscuitOS 并可以对该文件进行交叉编译，安装，
打包和目标系统上运行的功能，节省了很多开发时间。如果开发者不想使用这套
编译机制，可以参考下面的内容进行移植。开发者首先获得用户空间系统调用
基础源码，如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-riscv32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](/assets/PDB/RPI/RPI000342.png)

选择 "[\*]   strace" 和 "[\*]   System Call" 并进入 "[\*]   System Call  --->"

![](/assets/PDB/RPI/RPI000343.png)

选择并进入 "[\*]   sys_hello_BiscuitOS  --->"

![](/assets/PDB/RPI/RPI000366.png)

选择 "[\*]   SYSCALL_DEFINE1(): One Paramenter  --->" 保存配置并退出. 接下
来执行下面的命令部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000405.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE1_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE1_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](/assets/PDB/RPI/RPI000367.png)

上图中，main.c 与用户空间系统调用相关的源码, 
"SYSCALL_DEFINE1_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。
"SYSCALL_DEFINE1_common-0.0.1/BiscuitOS_syscall.c" 是系统调用的内核实现。
因此开发者只需关注 main.c, 内容如下:

![](/assets/PDB/RPI/RPI000368.png)

根据在内核中创建的入口，这里定义了入口宏的值为 258，一定要与内核定义
的入口值相呼应。由于是多个参数的系统调用，因此直接使用 "syscall()" 函数，
传入调用号和多个参数, 用户空间传递字符串 "BiscuitOS_Userpsace" 到内核
空间，并期待内核空间传递字符串到用户空间，如果成功，则打印字符串.
源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里, 使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE1_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B22">添加内核系统调用入口</span>

添加内核系统调用的接口，首先需要确定下一个可用的系统调用号，
开发者可以参考:

{% highlight bash %}
arch/riscv/include/uapi/asm/unistd.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000401.png)

从上图可以知道，RISCV32 架构里，新增加的系统调用号与
"\_\_NR_arch_specific_syscall" 有关，接着可以查看该宏的定义，
位于源码:

{% highlight bash %}
include/uapi/asm-generic/unistd.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000402.png)

从上面的定义可以看出，linux 为体系预留了 244 到 260 之间的系统调用号，
因此结合上述两个信息，新系统调用的系统调用号可以取 258, 即:

{% highlight bash %}
syscall_nr = __NR_arch_specific_syscall + 14 = 244 + 14 = 258
{% endhighlight %}

接下来，在 "arch/riscv/include/uapi/asm/unistd.h" 文件中添加新的系统
调用信息:

![](/assets/PDB/RPI/RPI000403.png)

在该文件中，首先定义了新添加系统调用号的名字为 "\_\_NR_hello_BiscuitOS",
其具体值为之前查找的下一个可用系统调用号 258，也可以按格式写成
"\_\_NR_arch_specific_syscall + 14". 接着使用宏 "\_\_SYSCALL_()" 
创建一个新的系统调用入口，该宏的第一个参数设置为新系统调用号，
即 "\_\_NR_hello_BiscuitOS", 该宏的第二个参数设置为系统调用内核实现，
这里填写 "sys_hello_BiscuitOS", 该函数就是新系统调用的具体实现。
添加完上面的信息后，还需要添加函数声明，位于文件:

{% highlight bash %}
arch/riscv/include/asm/vdso.h
{% endhighlight %}

![](/assets/PDB/RPI/RPI000414.png)

使用 asmlinkage 关键字，函数返回值类型为 long. 函数名为
"sys_hello_BiscuitOS". 至此新系统调用的入口制作完成。接下来，
开发者可以在内核源码的任意位置添加 sys_hello_BiscuitOS 的具体
实现.

--------------------------------------------

#### <span id="B23">添加内核实现</span>

添加完系统号之后，需要在内核中添加系统调用的具体实现。开发者
可以参考下面的例子进行添加。本例子在内核源码 "fs/" 目录下添加
一个名为 BiscuitOS_syscall.c 的文件，如下:

{% highlight bash %}
cp -rfa BiscuitOS/output/linux-5.0-riscv32/package/SYSCALL_DEFINE1_common-0.0.1/BiscuitOS_syscall.c  BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs/
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs
vi BiscuitOS_syscall.c
{% endhighlight %}

![](/assets/PDB/RPI/RPI000369.png)

内核使用 SYSCALL_DEFINE1() 宏定义了内核实现的接口函数，其包含一个
来自用户空间的字符串参数。内核使用 "copy_from_user()" 将用户空间的
字符串拷贝到内核空间，并打印字符串，接着用使用 "copy_to_user()" 函数
将内核字符串 kstring 拷贝到用户空间。这样就实现用户空间和内核空间互相
交换数据。接着修改内核源码 "fs/Kconfig" 文件，添加如下内容:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs
vi Kconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000333.png)

接着修改内核源码 "fs/Makefile" 文件，添加内容如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/fs
vi Makefile
{% endhighlight %}

![](/assets/PDB/RPI/RPI000334.png)

接着是配置内核，将 BiscuitOS_syscall.c 文件加入内核编译树，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/
make menuconfig ARCH=riscv
{% endhighlight %}

![](/assets/PDB/RPI/RPI000406.png)

选择并进入 "File systems  --->"

![](/assets/PDB/RPI/RPI000407.png)

选择 "\[*] BiscuitOS syscall hello" 并保存内核配置。

接着重新编译内核。

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/linux/linux/
make ARCH=riscv CROSS_COMPILE=BiscuitOS/output/linux-5.0-riscv32/riscv32-biscuitos-linux-gnu/riscv32-biscuitos-linux-gnu/bin/riscv32-biscuitos-linux-gnu- vmlinux -j4
{% endhighlight %}

编译内核中会打印相关的信息如下图:

![](/assets/PDB/RPI/RPI000408.png)

从上面的编译信息可以看出，之前的修改已经生效。编译系统调用相关的脚本
自动为hello_BiscuitOS 生成了相关的系统调用，

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B25">运行系统调用</span>

在一切准备好之后，最后一步就是在 RISCV32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-riscv32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/RPI/RPI000415.png)

从运行结果可以看到，用户空间的程序已经调用到对应的内核系统调用了。此时
可以使用 strace 工具查看具体的系统调用过程，如下:

{% highlight bash %}
~ #
~ # strace SYSCALL_DEFINE1_common-0.0.1
{% endhighlight %}

![](/assets/PDB/RPI/RPI000416.png)

从 strace 打印的消息可以看出 "syscall_0x102()" 正好程序里产生的系统调用.

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
