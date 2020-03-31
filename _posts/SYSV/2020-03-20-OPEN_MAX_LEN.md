---
layout: post
title:  "sys_open: 文件名最大长度研究"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

> - [问题介绍](#A0)
>
> - [快速实践打开文件名诸多问题](#B0)
>
>   - [文件名超过一定长度触发内核错误](#B250)
>
>   - [文件名为空引发的问题](#B251)
>
>   - [文件名超过 getname() 函数中的限制问题](#B252)
>
> - [问题分析](#B3)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### 问题介绍

{% highlight c %}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
{% endhighlight %}

在使用 open() 库函数打开一个文件时，需要向 open() 库函数传入需要打开文件
的路径名，正如上图定义的 pathname 参数。有时当打开一个文件名特别长的文件时，
open() 库函数就会报错. 遇到上面的问题，于是提出相关的几个问题:

> - 文件名长度的合理范围是多少?
>
> - 文件名能否为空?
>
> - 文件名太长内核如何处理?

为了让开发者切身感受以上问题，开发者请先参考下面文档进行实践复现问题:

> - [快速实践打开文件名诸多问题](#B0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000R.jpg)

#### 快速实践打开文件名诸多问题

> - [实践原理](#B20)
>
> - [实践准备](#B21)
>
> - [调试工具部署](#B24)
>
> - [复现问题](#B25)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B20">实践原理</span>

为了排除不相关因数的影响，实践绕过 libc/glibc 库，直接调用 syscall()
函数实现 open 系统调用。本实践用于创建一个调试用的工具，该工具可以便捷
创建一个任意长度的文件名，然后再传递给 syscall() 函数触发一个 sys_open 
系统调用。本实践支持多个架构上实践，开发者可以根据自身情况选择实践平台，
本文基于 ARM32 结构进行实践简介 (采用 ARM32 架构的直接进入 "实践准备"):

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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

为了方便开发者进行源码调试，请开发者参考完成下面文章提及的内容:

> - [解决系统调用调试遇到的诸多问题](https://biscuitos.github.io/blog/SYSCALL_DEBUG/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B24">调试工具部署</span>

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000504.png)

选择并进入 "[\*]   sys_open  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000535.png)

选择 "[\*]   getname(): Max open file name  --->" 保存配置并退出. 接下来执行
下面的命令部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/getname_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/getname_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000536.png)

上图中，main.c 文件是工具实现的具体源码，开发者可以根据需求任意修改。
"getname_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。因此开发者只
需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000537.png)

在该函数中，main 函数首先接收来自命令行的参数，并将其转换为 sys_open 系统
调用所需的参数。由于 sys_open 支持两个参数和三个参数的类型，因此调用
syscall() 函数的时候，可以做两种情况的处理，\_\_NR_open 是 sys_open 对应的
系统调用号，如果系统调用成功，那么 sys_open 将返回打开文件的句柄，至此
一次 sys_open 系统调用完成。为了确保应用程序的正确执行，开发者在程序结束
的时候也要显示关闭文件，这里同样调用 sys_close 系统调用进行关闭。
源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里, 使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/getname_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B25">复现问题</span>

在一切准备好之后，下一步就是在 ARM32 上复现问题，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/getname_common-0.0.1
make build
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000538.png)

开发者可以使用如下命令查看 open 系统调用工具的使用方法:

{% highlight c %}
~ # getname_common-0.0.1 -h
{% endhighlight %}

通过命令行的提示，接下来复现之前提出的问题:

> - [文件名超过一定长度触发内核错误](#B250)
>
> - [文件名为空引发的问题](#B251)
>
> - [文件名超过 getname() 函数中的限制问题](#B252)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

##### <span id="B250">文件名超过一定长度触发内核错误</span>

为了复现这个问题，开发者可以修改 getname_common-0.0.1 的 "-l" 参数来
传入不同长度的文件名，首先创建一个长度为 128 的文件名，如下:

{% highlight c %}
~ # getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000539.png)

从上图可以看出文件名长度 128 是合法的。维护排除相互影响，这里建议重启
BiscuitOS, 重启之后经创建一个长度为 4096 的文件名:

{% highlight c %}
~ # getname_common-0.0.1 -l 4096 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000540.png)

此时内核已经报错，问题已经复现，保存相关信息之后，重启 BiscuitOS，
重启之后创建一个长度为 4080 的文件名:

{% highlight c %}
~ # getname_common-0.0.1 -l 4080 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------------------------

###### <span id="B251">文件名为空引发的问题</span>

为了复现这个问题，开发者可以修改 getname_common-0.0.1 的 "-l" 参数来
传入不同长度的文件名，首先创建一个长度为 0 的文件名，如下:

{% highlight c %}
~ # getname_common-0.0.1 -l 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000541.png)

此时内核没有报错，只是返回错误码，错误码的值是 2，代表的含义是 ENOENT，
代表没有这个文件或目录.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------------------------

###### <span id="B252">文件名超过 getname() 函数中的限制问题</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000542.png)

正如上图所示，执行一次系统调用，open() 库函数将参数传递到内核系统调用
"SYSCALL_DEFINE3(open, ...)", 该函数即 sys_open(),sys_open() 接着调用
了 do_sys_open() 执行真正的打开操作，do_sys_open() 函数将用户空间传递
的文件名传递给了 getname() 函数，getname() 函数继续将参数传递给
getname_flags() 函数，getname_flags() 函数就是文件名处理的第一个核心
程序。其定义如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000543.png)

函数首先包含三个参数，第一个参数使用指向用户空间的文件名; 第二个参数是
文件打开的标志; 第三个参数是 empty。函数首先定义了一个 struct filename 
指针 result, struct filename 用于维护打开文件的文件名，更多信息可以查看

> - [struct filename 数据结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00010D)

函数接着定义了局部 char 指针 kname, int 变量 len。然后调用 
"BUILD_BUG_ON()" 宏配合 offsetof() 函数确认 struct filename 结构体中，
iname 之前的数据是否按 long 对齐，如果没有对齐，那么就报错.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000021.png)

从上面的定义可以看出 struct filename 结构 name 成员到 aname 成员占用了
16 个字节，16 正好按 long 对齐，因此此时不会报错。函数接着调用 
audit_reusename() 函数对打开文件名进行检测，但由于当前内核不支持 audit
功能，因此该函数不做任何实际操作直接返回。函数接着用 "\_\_getname()" 函数，
该函数用于从 names_cachep 缓存中为 result 分配内存，接着对分配的结果做了
简单的检查。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000544.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000546.png)

从 names_cachep 缓存中分配内存之后，struct filename 的内存布局如上，
"iname[]" 是一个零长数组，不占用 struct filename 的空间，因此 iname
指向了 offsetof(struct filename, iname) 之后的地址。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000545.png)

函数将 kname 指针指向了 struct filename 的 iname 成员，接着让 struct filename
的 name 成员指向了 kname 成员，这样就会出现下图的效果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000547.png)

函数调用 strncpy_from_user() 函数从 filename 参数指向用户空间内存拷贝
长度为 EMBEDDED_NAME_MAX 的数据到 kname 指向的内核空间。并将最终拷贝的字
节数存储在 len 变量里，此时如果 len 小于 0，表示拷贝操作失败，那么函数
调用 \_\_putname() 函数将内存返还给 names_cachep 缓存; 如果拷贝没有问题的
话，打开的文件名将被存储到 struct filename 的 iname 区域，此时开发者可以
添加打印函数进行查看文件名, 如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000548.png)

开发者可以参考上面的代码进行打印，但要注意两种打印方式，第二种是错误的，
这会引起非法访问，在用户空间使用如下命令进行测试:

{% highlight c %}
~ # getname_common-0.0.1 -l 10 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000549.png)

第一个函数正确的打印了一个长度为 10，名字为 "BBBBBBBBBB" 的文件名。而
第二个函数则直接导致系统挂住死机。因此开发者要非常注意不能在内核中使用
带 "\_\_user" 的参数，需要进行用户空间到内核的拷贝操作。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000550.png)

但打开文件名的长度为 EMBEDDED_NAME_MAX, 其定义如下:

{% highlight c %}
#define PATH_MAX                4096    /* # chars in a path name including nul */
#define EMBEDDED_NAME_MAX       (PATH_MAX - offsetof(struct filename, iname))
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000551.png)

从上图可以看出 EMBEDDED_NAME_MAX 的长度，如果此时打开文件名字长度正好为
EMBEDDED_ANME_MAX, 那么函数就首先使用 "offsetof(struct filename, iname[1])"
计算其长度，iname[1] 指向 iname[0] 的下一个地址，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000552.png)

函数将 kname 指向了 result 指向的内存空间，接着调用 kzalloc() 函数分配
长度为 size 的空间，并使用 result 指向其起始地址。接着对分配的内存检测。
然后将 result 的 name 成员指向 kname，此时内存布局如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000553.png)

此时从 kmalloc 内存分配的新 struct filename 的 name 成员指向了原先从 
names_cachep 分配的内存，此时原先的内存用于存储从用户空间拷贝的打开文件
名. 此时函数调用 strncpy_from_user() 函数再次从 filename 参数指向的用户
空间内核拷贝文件名到 kname 指向的内核空间，拷贝长度为 PATH_MAX, 如果
此时拷贝失败，那么释放 result 分配的内存，并将 kname 指向的内存空间归还给
names_cachep 缓存。如果拷贝成功，但拷贝长度为 PATH_MAX, 那么函数视为
超过长长度的文件名，函数释放相关内存之后，返回 -ENAMETOOLONG. 如果文件
名长度大于或等于 EMBEDDED_NAME_MAX 且小于 PATH_MAX, 那么算合法文件名。
接下来复现上面的问题, 在对应代码添加代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000554.png)

在做好打印消息只有，然后分别对 4079、4080、4090、4096、4097 长度的文件名:

{% highlight c %}
~ # getname_common-0.0.1 -l 4079 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}































