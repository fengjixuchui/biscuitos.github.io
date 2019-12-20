---
layout: post
title:  "open flags 研究"
date:   2019-12-20 09:23:30 +0800
categories: [HW]
excerpt: Open flags.
tags:
  - [open]
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [open flags](#A0)
>
> - [实践部署](#B0)
>
> - [研究分析](#C0)
>
> - [实践结论](#D0)
>
> - [附录](#Z0)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### open flags

用户空间调用 open 打开一个文件时候都会附带打开标志，这些标志用于分配
给文件操作的权限和访问权限。内核收到这些标志之后，对标志在进行处理，
以便分配给打开文件正确的权限。本文用于研究用户空间 open 打开标志，
以及内核在处理打开标志使用的特殊标志。简单的分作两类:

> - [open 用户空间标志](#A0001)
>
> - [open 用户标志详解](#A00010)
>
> - [open 用户空间错误码](#A0003)
>
> - [open 内核空间标志](#A0002)

----------------------------------

<span id="A0001"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## open 用户空间标志

{% highlight c %}
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags, ...);
{% endhighlight %}

用户空间 open 打开标志可以分作三种类型:

###### 文件访问模式标志

第一类是文件访问模式标志，例如 O_RDONLY, O_WRONLY, O_RDWR 标志均
在列，在调用 open 的时候，上述三者在 open() 函数的 flags 参数中
不能同时使用，只能制定其中一种。调用 fcntl() 的 F_GETFL 操作能够
检索文件的访问模式; 第二种是文件

###### 文件创建标志

第二类标志是文件创建标志，其控制范围不拘于 open() 调用行为，还涉及
后续的 I/O 操作的哥哥选项。这些标志不能检索，也无法修改。

###### 文件状态标志

第三类是已打开文件的状态标志，使用 fcntl() 的 F_GETFL 和 F_SETFL
操作可以分别检索和修改此类标志。

----------------------------------

<span id="A00010"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### open 用户标志详解

> - [O_RDONLY](#A00011)
>
> - [O_WRONLY](#A00012)
>
> - [O_RDWR](#A00013)
>
> - [O_CLOEXEC](#A00014)
>
> - [O_DIRECTORY](#A00015)
>
> - [O_NOFOLLOW](#A00016)
>
> - [O_CREAT](#A00017)
>
> - [O_EXCL](#A00018)
>
> - [O_NOCTTY](#A00019)
>
> - [O_TMPFILE](#A0001A)
>
> - [O_TRUNC](#A0001B)
>
> - [O_APPEND](#A0001C)
>
> - [O_ASYNC](#A0001D)
>
> - [O_DIRECT](#A0001E)
>
> - [O_DSYNC](#A0001F)
>
> - [O_LARGEFILE](#A0001G)
>
> - [O_NATIME](#A0001H)
>
> - [O_NONBLOCK](#A0001J)
>
> - [O_SYNC](#A0001K)
>
> - [O_PATH](#A0001L)

----------------------------------------

###### <span id="A00011">O_RDONLY</span>

O_RDONLY 表示以只读的方式打开文件。

----------------------------------------

###### <span id="A00012">O_WRONLY</span>

O_WRONLY 表示以只写的方式打开文件。

----------------------------------------

###### <span id="A00013">O_RDWR</span>

O_RDWR 表示以可读可写的方式打开文件。

----------------------------------------

###### <span id="A00014">O_CLOEXEC</span>

O_CLOEXEC 为新创建的文件描述符启用 close-on-flag 标志 (FD_CLOEXEC).
使用 O_CLOEXEC 标志打开的文件，可以避免程序执行 fcntl() 的
F_GETFD 和 F_SETFD 操作设置 close-on-exec 标志的额外工作。在
多线程程序中执行 fcntl() 的 F_GETFD 和 F_SETFD 操作有可能导致竞争
状态，而使用 O_CLOEXEC 标志则能够避免这一点. 可能引发竞争的场景
是: 线程 A 打开一个文件描述符，尝试为该描述符标记 close-on-exec 标志，
于此同时，线程 B 执行 fork() 调用，然后调用 exec() 执行任意一个程序。(
假设再 A 打开文件描述符和调用 fcntl() 设置 close-on-exec 标志之间，
线程 B 成功执行了 fork() 和 exec() 操作)，此类竞争可能会在无意间
将打开的文件描述符泄露给不安全的程序。

----------------------------------------

###### <span id="A00015">O_DIRECTORY</span>

当调用 open() 函数时，使用 O_DIRECTORY 标志，如果打开的文件路径指
向的文件不是目录，则返回错误，错误代码是 ENOTDIR. 这个标志是专门为
了实现 opendir() 函数而设计的拓展标志。

----------------------------------------

###### <span id="A00016">O_NOFOLLOW</span>

当调用 open() 函数时，不使用 O_NOFOLLOW 标志，如果打开的文件路径
指向的文件是符号链接，open() 函数将对路径指向的文件进行解引用。
一旦在 open() 函数中指定了 O_NOFOLLOW 标志，且 pathname 参数属于
符号链接，则 open() 函数将返回失败 (错误代码是 ELOOP). 此标志在
特权程序中极为有用，能够确保 open() 函数不对符号链接解引用。

----------------------------------------

###### <span id="A00017">O_CREAT</span>

当调用 open() 函数时使用了 O_CREAT 标志，如果文件不存在，将创建
一个新的空白文件. 即使文件以只读方式打开，此标志依然有效。如果
在 open() 调用中指定了 O_CREAT 标志，那么还需要提供 mode 参数，
否则会将新文件的权限设置为栈中的某个随机值。

----------------------------------------

###### <span id="A00018">O_EXCL</span>

此标志与 O_CREAT 标志结合使用表明如果文件已经存在，则不会打开文件，且
open() 调用失败，并返回错误，错误代码为 EEXIST,换言之，此标志保证了调用
者 (open() 的调用者) 就是创建文件的进程。检查文件存在与否和创建文件这
两步属于同一原子操作。如果在 flags 参数中同时指定了 O_CREAT 和 O_EXCL
标志，且文件路径指向一个符号链接，则 open() 函数调用失败 (错误代码是
EEXIST), 之所以如此规定，是要求有特权的应用程序在已知目录下创建文件，
从而消除了如下安全隐患，使用符号链接打开文件会导致再另一个位置创建
文件。

----------------------------------------

###### <span id="A00019">O_NOCTTY</span>

如果正在打开的文件属于终端设备，O_NOCTTY 标志防止其成为控制终端。
否则用户的键盘输入信息将影响程序的执行。如果正在打开的文件不是终
端设备，则此标志无效。

----------------------------------------

###### <span id="A0001A">O_TMPFILE</span>

创建一个无名的临时文件，文件系统中会创建一个无名的 inode，当
最后一个文件描述符被关闭的时候，所有写入这个文件的内容都会丢
失，除非在此之前给了它一个名字.

----------------------------------------

###### <span id="A0001B">O_TRUNC</span>

如果文件已经存在且为普通文件，那么清空文件内容，将其长度置为 0.
在 Linux 下使用此标志，无论以读、写方式打开文件，都可清空文件
内容 (在这种情况下，都必须拥有对文件的写权限)。

----------------------------------------

###### <span id="A0001C">O_APPEND</span>

总在文件尾部追加数据i。在当下的Unix/Linux系统中，这个选项已经
被定义为一个原子操作

----------------------------------------

###### <span id="A0001D">O_ASYNC</span>

当对于 open() 调用所返回的文件描述符可以实施 I/O 操作时，系统会
产生一个信号通知进程。这一特性，也被称为信号驱动 I/O, 仅对特殊类型的
文件系统有效，诸如终端、FIFOS 及 socket。在 Linux 中，调用 open()
时指定 O_ASYNC 标志没有任何实质效果。要启用信号驱动 I/O 特性，必须
调用 fcntl() 的 F_SETFL 操作来设置 O_ASYNC 标志。

----------------------------------------

###### <span id="A0001E">O_DIRECT</span>

无系统缓冲的文件 I/O 操作。试图最小化来自 I/O 和这个文件的 
cache effect。

----------------------------------------

###### <span id="A0001F">O_DSYNC</span>

根据同步 I/O 数据完整性的完成要求来执行文件写操作。

----------------------------------------

###### <span id="A0001G">O_LARGEFILE</span>

支持以大文件方式打开文件。在 32 位系统中使用此标志，以支持大文件操作。
允许打开一个大小超过 off_t (但没超过 off64_t) 表示范围的文件。

----------------------------------------

###### <span id="A0001H">O_NATIME</span>

在读文件时，不更新文件的最近访问时间。要使用该标志，要么调用
进程的有效组用户 ID 必须与文件的拥有者相匹配，要么进程需要拥有
特权 (CAP_FOWNER), 否则 open() 调用失败，并返回错误，错误码是
EPERM。

----------------------------------------

###### <span id="A0001J">O_NONBLOCK</span>

以非租塞方式打开文件.

----------------------------------------

###### <span id="A0001K">O_SYNC</span>

以同步 I/O 方式打开文件。

----------------------------------------

###### <span id="A0001L">O_PATH</span>

获得一个能表示文件在文件系统中位置的文件描述符.

----------------------------------

<span id="A0003"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

## open 用户空间错误码

若打开文件发生错误，open() 将会返回 -1，错误号 errno 标识错误原因。

> - [EACCESS](#A00030)
>
> - [EISDIR](#A00031)
>
> - [EMFILE](#A00032)
>
> - [ENFILE](#A00033)
>
> - [ENOENT](#A00034)
>
> - [EROFS](#A00035)
>
> - [ETXTBSY](#A00036)

------------------------------------------

###### <span id="A00030">EACCESS</span>

文件权限不允许调用进程以 flags 参数制定的方式打开文件。无法访问文件，
其可能的原因有目录的限制、文件不存在并且有无法创建该文件。

------------------------------------------

###### <span id="A00031">EISDIR</span>

所指定的文件属于目录，而调用者企图打开该文件进行写操作。不允许这种
用法。

------------------------------------------

###### <span id="A00032">EMFILE</span>

进程已打开的文件描述符数量已经达到了进程资源限制所设定的上限。

------------------------------------------

###### <span id="A00033">ENFILE</span>

文件打开数量已达到系统允许的上限。

------------------------------------------

###### <span id="A00034">ENOENT</span>

要么文件不存在且未指定 O_CREAT 标志，要么指定了 O_CREAT 标志，
但打开路径指向的目录之一不存在，或者路径指向的文件为符号链接，
而该链接指向的文件正好不存在 (空链接)

------------------------------------------

###### <span id="A00035">EROFS</span>

所指定的文件隶属于只读文件系统，而调用者企图以写的方式打开文件。

------------------------------------------

###### <span id="A00036">ETXTBSY</span>

所指定的文件为可执行文件，且在运行。系统不允许修改正在运行的程序。




----------------------------------

<span id="B0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

## 实践部署

本文支持 BiscuitOS 实践，具体实践步骤请参考:

> - [Open 系统调用实践部署](https://biscuitos.github.io/blog/open/#B0)

本文实践部署如下:

{% highlight c %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig 
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000003.png)

选择 "Package" 并进入二级菜单.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000004.png)

选择 "System Call --->" 并进入二级菜单.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000005.png)

选择 "open (arm) --->" 并进入二级菜单.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000006.png)

选择 "open: path length" 和 "open: kernel" 并保存退出。

{% highlight c %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/open_pathlen-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000007.png)

接着下载，编译和安装源码:

{% highlight c %}
make prepare
make download
make 
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000008.png)

最后运行 BiscuitOS，执行程序如下:

{% highlight c %}
open_pathlen-0.0.1
{% endhighlight %}

----------------------------------

<span id="C0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

## 研究分析

基于上述的实践和原理，本节重点介绍不同的文件路径长度导致不同的
内核策略，实践可以分成如下几类:

> - [路径名长度小于 EMBEDDED_NAME_MAX](#C01)
>
> - [路径名长度等于 EMBEDDED_NAME_MAX](#C02)
>
> - [路径名长度大于 EMBEDDED_NAME_MAX 小于 PATH_MAX](#C03)
>
> - [路径名长度等于 PATH_MAX](#C04)
>
> - [路径名长度大于 PATH_MAX](#C05)


--------------------------------------

#### <span id="C01">路径名长度小于 EMBEDDED_NAME_MAX</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000009.png)

在应用程序中修改文件路径长度为 32，如上图。编译安装之后，在内核源码
"getname_flags()" 添加相应的调试信息，以此跟踪内核处理逻辑, 修改如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000011.png)

{% highlight c %}
open_pathlen-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000010.png)

从运行的结果可以看出，文件路径名长度小于 EMBEDDED_NAME_MAX 并不需要
使用分离的 struct filename. 其路径名管理如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000001.png)

--------------------------------------

#### <span id="C02">路径名长度等于 EMBEDDED_NAME_MAX</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000012.png)

在应用程序中修改文件路径长度为 EMBEDDED_NAME_MAX，如上图。编译安装
之后，在内核源码 "getname_flags()" 添加相应的调试信息，以此跟踪内核
处理逻辑, 修改如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000014.png)

{% highlight c %}
open_pathlen-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000013.png)

从运行的结果可以看出，文件路径名长度等于 EMBEDDED_NAME_MAX 需要
使用分离的 struct filename. 其路径名管理如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000002.png)

--------------------------------------

#### <span id="C03">路径名长度大于 EMBEDDED_NAME_MAX 小于 PATH_MAX</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000015.png)

在应用程序中修改文件路径长度为 "EMBEDDED_NAME_MAX + 2"，如上图。编译安装
之后，在内核源码 "getname_flags()" 添加相应的调试信息，以此跟踪内核
处理逻辑, 修改如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000014.png)

{% highlight c %}
open_pathlen-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000016.png)

从运行的结果可以看出，文件路径名长度大于 EMBEDDED_NAME_MAX，但小于
PATH_MAX, 需要使用分离的 struct filename, 并且两次从用户空间读取的
长度不同。其路径名管理如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000002.png)

--------------------------------------

#### <span id="C04">路径名长度等于 PATH_MAX</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000017.png)

在应用程序中修改文件路径长度为 PATH_MAX，如上图。编译安装之后，
在内核源码 "getname_flags()" 添加相应的调试信息，以此跟踪内核
处理逻辑, 修改如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000014.png)

{% highlight c %}
open_pathlen-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000018.png)

从运行的结果可以看出，文件路径名长度等于 PATH_MAX 需要
使用分离的 struct filename, 但就算使用分离的 struct filename,
内核还是不能管理这么路径名长度为 PATH_MAX 的情况，直接返回
"ENAMETOOLONG".

--------------------------------------

#### <span id="C05">路径名长度大于 PATH_MAX</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000019.png)

在应用程序中修改文件路径长度为 PATH_MAX，如上图。编译安装之后，
在内核源码 "getname_flags()" 添加相应的调试信息，以此跟踪内核
处理逻辑, 修改如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000014.png)

{% highlight c %}
open_pathlen-0.0.1
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000020.png)

从运行的结果可以看出，文件路径名长度等于 PATH_MAX 需要         
使用分离的 struct filename, 但就算使用分离的 struct filename, 
内核只能从用户空间读取长度为 PATH_MAX 长度的内容，并且
内核还是不能管理这么路径名长度为 PATH_MAX 的情况，直接返回
"ENAMETOOLONG".

----------------------------------

<span id="D0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

## 实践结论

从上面的实践最终可以得出以下结论:

> - 文件路径名长度不能超过 PATH_MAX
>
> - 最优文件路径名长度不能超过 EMBEDDED_NAME_MAX

-----------------------------------------------

# <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 赞赏一下吧 🙂

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
