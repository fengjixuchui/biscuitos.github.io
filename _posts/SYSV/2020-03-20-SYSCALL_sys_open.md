---
layout: post
title:  "SYSCALL: sys_open"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

> - 通用原理
>
>   - [原理简介](#A0)
>
>   - 标志解析
>
>     - [打开文件标志](#A10)
>
>     - [用户权限标志](#A11)
>
>     - [文件类型标志](#A12)
>
>     - [访问权限标志](#A13)
>
>     - [文件查找标志](#A14)
>
>     - [错误码](#A15)
>
>   - 数据结构解析
>
>     - [struct filename](#A00010D)
>
>     - [struct files_struct](#A00011D)
>
>     - [struct fdtable](#A00012D)
>
>     - [struct nameidata](#A00013D)
>
>     - [struct path](#A00014D)
>
>     - [struct qstr](#A00015D)
>
>     - [filp_cachep](#A00016D)
>
>     - [struct open_flags](#A00017D)
>
>   - [源码解析](https://biscuitos.github.io/blog/OPEN_SOURCE_CODE/)
>
> - 实践部署
>
>   - [快速实践 open 系统调用](#B2)
>
>   - [实践建议](#B3)
>
> - 工具合集
>
>   - [open 系统调用调试工具](#C0)
>
>   - [任意长度文件名工具](#C1)
>
>   - [打开任意个文件工具](#C2)
>
>   - [strace](#C3)
>
>   - [/proc/sys/fs 参数合集](#C4)
>
>   - [ulimit](#C5)
> <span id="HHH"></span>
> - 进阶研究
>
>   - [文件名太长导致文件打开失败](https://biscuitos.github.io/blog/OPEN_MAX_LEN/#B250)
>
>   - [文件名为空导致文件打开失败](https://biscuitos.github.io/blog/OPEN_MAX_LEN/#B251)
>
>   - [进程打开文件数太多导致失败问题](https://biscuitos.github.io/blog/OPEN_NR/#B250)
>
>   - [进程文件描述符超过 sysctl_nr_open 导致打开失败问题](https://biscuitos.github.io/blog/OPEN_NR/#B251)
>
>   - [sysctl_nr_open 不能够准确控制打开文件的数量问题](https://biscuitos.github.io/blog/OPEN_NR/#B252)
>
>   - [不同文件系统支持进程最大文件打开数问题](https://biscuitos.github.io/blog/OPEN_NR/#B253)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### 原理简介

open 系统调用用于打开或创建一个文件，用户空间 C 库 提供的 open 函数定义在:

{% highlight c %}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
{% endhighlight %}

用户空间的 open() 函数支持两个参数和三个参数的形式，pathname 参数用于
指向需要打开的文件路径名，flags 参数用于控制打开文件的方式，mode 参数
用于控制打开文件的访问者权限。内核空间已知相对应的 open 系统调用 
sys_open 定义在:

{% highlight c %}
linux_source_code/fs/open.c

SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
        if (force_o_largefile())
                flags |= O_LARGEFILE;

        return do_sys_open(AT_FDCWD, filename, flags, mode);
}
{% endhighlight %}

内核中不同架构系统调用入口不一，但 sys_open 内核实现都是统一的定义如上。
使用 SYSCALL_DEFINE3() 宏定义了一个名为 sys_open 函数，其包含三个参数，
第一个参数的类型是 "const char __user *", 用于指明这是一个指向用户空间
的字符指针。第二个参数是一个 int 类型的变量，第三个参数是一个 umode_t 类
型的变量。

开发者可以在用户空间使用 open() 函数，传递不同的参数和权限值，内核的 
sys_open() 函数就会执行指定的任务。open() 函数是使用对频繁的系统调用，
开发者可以参考文章的其他章节进行了解.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A10"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### 文件打开标志

{% highlight c %}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
{% endhighlight %}

文件打开标志指的是调用 open() 库函数时，传递的 flags 参数。用户空间 
open 文件打开标志可以分作三种类型:

###### 文件访问模式标志

第一类是文件访问模式标志，例如 O_RDONLY, O_WRONLY, O_RDWR 标志均
在列，在调用 open 的时候，上述三者在 open() 函数的 flags 参数中
不能同时使用，只能制定其中一种。调用 fcntl() 的 F_GETFL 操作能够
检索文件的访问模式, 例如 O_RDONLY、O_WRONLY、O_RDWR.

###### 文件创建标志

第二类标志是文件创建标志，其控制范围不拘于 open() 调用行为，还涉及
后续的 I/O 操作的哥哥选项。这些标志不能检索，也无法修改。例如:
O_CLOEXEC、O_CREAT、O_DIRECT、O_DIRECTORY、O_EXCL、O_LARGEFILE、
O_NOATIME、O_NOCTTY、O_NOFOLLOW、O_TRUNC.

###### 文件状态标志

第三类是已打开文件的状态标志，使用 fcntl() 的 F_GETFL 和 F_SETFL
操作可以分别检索和修改此类标志。例如: O_APPEND、O_ASYNC、O_DSYNC、
O_NONBLOCK、O_SYNC。

###### open 用户标志详解

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
> - [\_\_O_TMPFILE](#A0001A)
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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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

###### <span id="A0001A">\_\_O_TMPFILE</span>

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A11"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### 用户权限标志

{% highlight c %}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
{% endhighlight %}

用户权限标志指的是调用 open() 库函数时，传递的 mode 参数。 每个文件都
有一个与之关联的用户 ID (UID) 和组 ID (GID), 以便可以判断文件的属性和
属性组。文件创建时，其用户 ID 取自进程的有效用户 ID, 而新建的组 ID 则取
自进程的有效组 ID, 或父目录的组 ID. 在描述文件权限的结构 struct stat 中，
st_mod 字段的第 12 位定义了文件权限。其中的前 3 位为专有位，分别是 
set-user-ID 位、set-group-ID 位和 sticky 位(在下图中标记为 U、G、T 位)。
其余 9 bit 构成了定义权限的掩码，分别授予访问文件的各类用户.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000035.png)

文件权限掩码分 3 类:

{% highlight bash %}
1. Owner (亦为 user): 授予文件属性的权限。
2. Group: 授予文件属性组成员的权限.
3. Other: 授予其他用户的权限。
{% endhighlight %}

可以为每一类用户授予的权限如下:

{% highlight bash %}
1. Read: 可读文件的内容
2. Write: 可更改文件的内容
3. Execute: 可以执行文件
{% endhighlight %}

针对上面的内容，开发者可以使用 `ls -l` 命令直接查看文件的权限，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000036.png)

如上图所示的文件权限，Owner 具有可读可写可执行的权限，Group 具有可读
可执行的权限，Other 具有可读和可执行的权限。Linux 支持的用户权限标志如下:

> - [S_ISUID](#A000437)
>
> - [S_ISGID](#A000438)
>
> - [S_ISVTX](#A000439)
>
> - [S_IRUSR](#A00043A)
>
> - [S_IWUSR](#A00043B)
>
> - [S_IXUSR](#A00043C)
>
> - [S_IRGRP](#A00043D)
>
> - [S_IWGRP](#A00043E)
>
> - [S_IXGRP](#A00043F)
>
> - [S_IROTH](#A00043G)
>
> - [S_IWOTH](#A00043H)
>
> - [S_IXOTH](#A00043J)
>
> - [S_IRWXU](#A00043K)
>
> - [S_IRWXG](#A00043L)
>
> - [S_IRWXO](#A00043M)
>
> - [S_IRWXUGO](#A00043N)
>
> - [S_IALLUGO](#A00043P)
>
> - [S_IRUGO](#A00043Q)
>
> - [S_IWUGO](#A00043R)
>
> - [S_IXUGO](#A00043S)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------

###### <span id="A000437">S_ISUID</span>

S_ISUID 是文件权限标志，其值是: 04000, 用于 "Set-user-ID".
当该位置位，则文件执行时，内核将其进程的有效用户 ID 设置为
文件所有者的用户 ID。

-----------------------------------

###### <span id="A000438">S_ISGID</span>

S_ISGID 是文件权限标志，其值是: 02000, 用于 "Set-group-ID".
当该位置位，则文件执行时，内核将其进程的有效用户组 ID 设置为文件的
所有组ID。

-----------------------------------

###### <span id="A000439">S_ISVTX</span>

S_ISVTX 是文件权限标志，其值是: 01000, 用于设置 Sticky。

-----------------------------------

###### <span id="A00043A">S_IRUSR</span>

S_IRUSR 是文件权限标志，其值是: 0400, 代表 User-read 标志位。
该位置位代表文件拥有者具有读文件内容的权限; 该位清零代表文件
的拥有者不具有读文件内容的权限。

-----------------------------------

###### <span id="A00043B">S_IWUSR</span>

S_IWUSR 是文件权限标志，其值是: 0200, 代表 User-write 标志位。
该位置位代表文件拥有者具有写文件内容的权限; 该位清零代表文件
的拥有者不具有写文件内容的权限。

-----------------------------------

###### <span id="A00043C">S_IXUSR</span>

S_IXUSR 是文件权限标志，其值是: 0100, 代表 User-execute 标志位。
该位置位代表文件拥有者具有文件的执行权限; 该位清零代表文件的
拥有者不具有执行文件的权限。

-----------------------------------

###### <span id="A00043D">S_IRGRP</span>

S_IRGRP 是文件权限标志，其值是: 040, 代表 Group-read 标志位。
该位置位代表用户组拥有文件的读权限; 该位清零代表用户组不具有
文件的读权限。

-----------------------------------

###### <span id="A00043E">S_IWGRP</span>

S_IWGRP 是文件权限标志，其值是: 020, 代表 Group-write 标志位。
该位置位代表用户组拥有文件的写权限; 该位清零代表用户组不具有文件
的写权限。

-----------------------------------

###### <span id="A00043F">S_IXGRP</span>

S_IXGFP 是文件权限标志，其值是: 010, 代表 Group-execute 标志位。
该位置位代表用户组可以执行该文件; 该位清零代表用户组不具有执行的权限。

-----------------------------------

###### <span id="A00043G">S_IROTH</span>

S_IROTH 是文件权限标志，其值是: 04, 代表 Other-read 标志位。
该位置位代表其他用户具有文件的读权限; 该位清零代表其他用户不具有文件
的读权限。

-----------------------------------

###### <span id="A00043H">S_IWOTH</span>

S_IWOTH 是文件权限标志，其值是: 02, 代表 Other-write 标志位。
该位置位代表其他用户具有文件的写权限; 该位清零代表其他用户不具有
该文件的写权限。

-----------------------------------

###### <span id="A00043J">S_IXOTH</span>

S_IXOTH 是文件权限标志，其值是: 01, 代表 Other-execute 标志位。
该位置位代表其他用户具有文件的执行权限; 该位清零代表其他用户不具有
该文件的执行权限.

-----------------------------------

###### <span id="A00043K">S_IRWXU</span>

S_IRWXU 是文件权限拥有者的掩码，其值是: 0700.

-----------------------------------

###### <span id="A00043L">S_IRWXG</span>

S_IRWXG 是文件权限拥有组的掩码，其值是: 070.

-----------------------------------

###### <span id="A00043M">S_IRWXO</span>

S_IRWXO 是文件权限对其他用户的掩码，其值是: 07.

-----------------------------------

###### <span id="A00043N">S_IRWXUGO</span>

S_IRWXUGO 是一个掩码，用户获得文件的所有权限，其定义如下:

{% highlight c %}
#define S_IRWXUGO       (S_IRWXU|S_IRWXG|S_IRWXO)
{% endhighlight %}

-----------------------------------

###### <span id="A00043P">S_IALLUGO</span>

S_IALLUGO 是文件权限与 sticky 的掩码，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000035.png)

掩码的范围除了最高的 4bit，其余都掩上，其定义如下:

{% highlight c %}
#define S_IALLUGO       (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
{% endhighlight %}

-----------------------------------

###### <span id="A00043Q">S_IRUGO</span>

S_IRUGO 是拥有者、用户组以及其他用户的读权限掩码. 其定义如下:

{% highlight c %}
#define S_IRUGO         (S_IRUSR|S_IRGRP|S_IROTH)
{% endhighlight %}

-----------------------------------

###### <span id="A00043R">S_IWUGO</span>

S_IWUGO 是拥有者、用户组以及其他用户的写权限掩码，其定义如下:

{% highlight c %}
#define S_IWUGO         (S_IWUSR|S_IWGRP|S_IWOTH)
{% endhighlight %}

-----------------------------------

###### <span id="A00043S">S_IXUGO</span>

S_IXUGO 是拥有者、用户组以及其他用户的执行权限掩码，其定义如下:

{% highlight c %}
#define S_IXUGO         (S_IXUSR|S_IXGRP|S_IXOTH)
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000W.jpg)

#### 文件类型标志

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000035.png)

linux 使用 struct stat 结构维护打开文件相关信息，其中 st_mode
成员包含了文件的类型和文件权限。正如上图所示，st_mode 的高 4 位
用于描述文件的类型。众所周知，linux 与 Unix 的万物皆文件的思想
深入各个子模块的设计中，因此文件需要使用文件类型字段描述文件
所属的类型。Linux 支持的文件类型标志如下:

> - [S_IFREG](#A000430)
>
> - [S_IFDIR](#A000431)
>
> - [S_IFCHR](#A000432)
>
> - [S_IFBLK](#A000433)
>
> - [S_IFIFO](#A000434)
>
> - [S_IFSOCK](#A000435)
>
> - [S_IFLNK](#A000436)
>
> - [S_IFMT](#A000436X)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------

###### <span id="A000430">S_IFREG</span>

S_IFREG 是文件类型标志，用于指明文件属于普通文件。

-----------------------------------

###### <span id="A000431">S_IFDIR</span>

S_IFDIR 是文件类型标志，用于指明文件属于目录。

-----------------------------------

###### <span id="A000432">S_IFCHR</span>

S_IFCHR 是文件类型标志，用于指明文件属于字符设备。

-----------------------------------

###### <span id="A000433">S_IFBLK</span>

S_IFBLK 是文件类型标志，用于指明文件属于块设备。

-----------------------------------

###### <span id="A000434">S_IFIFO</span>

S_IFIFO 是文件类型标志，用于指明文件属于 FIFO 或管道。

-----------------------------------

###### <span id="A000435">S_IFSOCK</span>

S_IFSOCK 是文件类型标志，用于指明文件属于套接字。

-----------------------------------

###### <span id="A000436">S_IFLNK</span>

S_IFLNK 是文件类型标志，用于指明文件属于符号链接。

-----------------------------------

###### <span id="A000436X">S_IFMT</span>

S_IFMT 是文件类型的掩码，在 linux 内核中，文件类型的布局如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000035.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A14"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Z.jpg)

#### 文件查找标志

内核在查找过程中，会使用一套查找标志，用于控制查找过程，Linux 支持
的查找标志如下:

> - [LOOKUP_FOLLOW](#A000440)
>
> - [LOOKUP_DIRECTORY](#A000441)
>
> - [LOOKUP_AUTOMOUNT](#A000442)
>
> - [LOOKUP_PARENT](#A000443)
>
> - [LOOKUP_REVAL](#A000444)
>
> - [LOOKUP_NO_REVAL](#A000445)
>
> - [LOOKUP_OPEN](#A000446)
>
> - [LOOKUP_CREATE](#A000447)
>
> - [LOOKUP_EXCL](#A000448)
>
> - [LOOKUP_RENAME_TARGET](#A000449)
>
> - [LOOKUP_JUMPED](#A00044A)
>
> - [LOOKUP_ROOT](#A00044B)
>
> - [LOOKUP_EMPTY](#A00044C)
>
> - [LOOKUP_DOWN](#A00044D)


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A000440">LOOKUP_FOLLOW</span>

LOOKUP_FOLLOW 标志用于如果路径的最后一个名字是符号链接，则解释（追踪）它。

-------------------------------------

###### <span id="A000441">LOOKUP_DIRECTORY</span>

LOOKUP_DIRECTORY 标志用于表示路径的最后一个名字必须是目录。

-------------------------------------

###### <span id="A000442">LOOKUP_AUTOMOUNT</span>

LOOKUP_MOUNT 表示文件查找时强制自动装载事件的能力。其说明如下:

{% highlight bash %}
Since we've now turned around and made LOOKUP_FOLLOW *not* force an
automount, we want to add the ability to force an automount event on
lookup even if we don't happen to have one of the other flags that force
it implicitly (LOOKUP_OPEN, LOOKUP_DIRECTORY, LOOKUP_PARENT..)

Most cases will never want to use this, since you'd normally want to
delay automounting as long as possible, which usually implies
LOOKUP_OPEN (when we open a file or directory, we really cannot avoid
the automount any more).

But Trond argued sufficiently forcefully that at a minimum bind mounting
a file and quotactl will want to force the automount lookup. Some other
cases (like nfs_follow_remote_path()) could use it too, although
LOOKUP_DIRECTORY would work there as well.

This commit just adds the flag and logic, no users yet, though. It also
doesn't actually touch the LOOKUP_NO_AUTOMOUNT flag that is related, and
was made irrelevant by the same change that made us not follow on
LOOKUP_FOLLOW.
{% endhighlight %}

-------------------------------------

###### <span id="A000443">LOOKUP_PARENT</span>

LOOKUP_PARENT 表示在查找文件路径过程中，已经找到文件所在的目录。

-------------------------------------

###### <span id="A000444">LOOKUP_REVAL</span>

如果该标志存在，那么 dentry 中的内容不被信任，强制执行一个真实的
查找，即从父目录的文件目录项中查找。

-------------------------------------

###### <span id="A000445">LOOKUP_NO_REVAL</span>

LOOKUP_NO_REVAL 标志。

-------------------------------------

###### <span id="A000446">LOOKUP_OPEN</span>

LOOKUP_OPEN 标志表示试图打开一个文件。

-------------------------------------

###### <span id="A000447">LOOKUP_CREATE</span>

LOOKUP_CREATE 标志表示试图去创建一个文件。

-------------------------------------

###### <span id="A000448">LOOKUP_EXCL</span>

LOOKUP_EXCL 标志表示查找的文件可能不存在。

-------------------------------------

###### <span id="A000449">LOOKUP_RENAME_TARGET</span>

LOOKUP_RENAME_TARGET 标志表示

-------------------------------------

###### <span id="A00044A">LOOKUP_JUMPED</span>

LOOKUP_JUMPED 标志表示

-------------------------------------

###### <span id="A00044B">LOOKUP_ROOT</span>

LOOKUP_ROOT 标志表示如果开始查找的文件时候，如果目录是根节点，
那么设置该标志，以此表示查找过程是从根节点开始的。

-------------------------------------

###### <span id="A00044C">LOOKUP_EMPTY</span>

-------------------------------------

###### <span id="A00044D">LOOKUP_DOWN</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A15"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### 错误码

当调用 open() 库函数之后，sys_open() 系统调用函数会根据处理的结果返回
一个整形值给用户空间，如果在执行系统调用过程中发生了错误，那么 sys_open()
将返回一个错误码，该错误码用于描述发生错误的原因，目前 Linux 支持的错误码
如下:

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A00010D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### struct filename

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000021.png)

"struct filename" 结构定义在 "include/linux/fs.h"，用于管理
用户空间传递下来的路径名。成员 name 用于指向用户空间路径名拷贝到
内核空间的位置; 参数 uptr 用于指向用户空间文件路径名; 参数
refcnt 用于指定该文件路径名引用的次数; aname 参数用于内核的
审计功能，如果没有启用 CONFIG_AUDIT, 那么该成员设置为 NULL;
参数 iname 为一个字符数组，如果采用下面介绍的第一种管理方式，
那么该成员和成员 name 一致指向用户空间路径名拷贝到内核空间的
位置，如果采用第二种管理方式，那么该成员不使用。

###### 内核文件路径名管理策略

内核采用两种方式管理文件路径名，第一
种针对的是文件路径名长度小于 EMBEDDED_NAME_MAX 的文件路径名，
内核从 names_cachep 缓存中分配大小为 PATH_MAX 的内存空间，
"struct filename" 结构占用了这部分内存的头部，剩余的部分用于
存储从用户空间传递下来的路径名，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000001.png)

第二种针对文件路径名长度大于等于 EMBEDDED_NAME_MAX 而小于
PATH_MAX 的情况，这种情况下，内核首先从 names_cachep 中分配
一个缓存，其长度也是 PATH_MAX, 然后再从 SLAB 中分配一个
"struct filename" 大小的内存，使用这个 "struct filename"
的 name 成员指向了从 names_cachep 中分配出来的内存，这个内存
就用来存储用户空间传递下来的文件路径名，其逻辑如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000002.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A00011D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

#### struct files_struct

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000717.png)

struct files_struct 结构用于管理进程打开文件的信息。

###### fdt/fdtab

fdt 成员 fdtab 成员都是 struct fdtable 结构，是进程的文件描述符表。每个
打开的文件都从这个成员里分配可用的文件描述符。其用于分配和回收文件描述符。

> - [struct fdtable 结构详解](#A00012D)

###### resize_wait/resize_in_progress

resize_wait 成员和 resize_in_progress 成员用于当进程扩充文件描述符表或者
其他信息的时候，会将 resize_in_progress 设置为 true，当修改完毕之后将其
设置为 false。如果进程想要修改 struct files_struct 某些数据的时候，发现
resize_in_progress 已经设置为 true，那么进程进入 resize_wait queue，等待
resize_in_progress 为 false 时候唤醒。

###### next_fd

next_fd 成员是用于下一个进程可用的文件描述符。

###### fd_array

fd_array 成员是进程所有打开的文件数组。在 Linux 中，每个打开的文件都使用
struct file 结构进行维护，进程将自己所有打开的文件的 struct file 维护在
fd_array 中，并使用文件描述符进行索引.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A00012D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

#### struct fdtable

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000023.png)

struct fdtable 数据结构用来管理进程的文件描述符。在进程中，每个打开
的文件使用一个文件描述符进行管理，文件描述符是一个整形数值。struct
fdtable 通过对文件描述符的管理实现了进程对文件打开和关闭是文件描述
符的分配与回收。

###### max_fds

max_fds 成员用于管理进程最大文件描述符，也可以表示为进程最大可打开文件数。
默认情况下，进程创建时，将进程的最大文件描述符设置为 32. 每当进程打开
数量超过 32 的时候，内核就会扩充进程的最大文件描述符。文件描述符的扩充
根据一定逻辑进行扩充，基本扩充逻辑是:

{% highlight bash %}
32 --> 256 --> 512 --> 1024 --> ...
{% endhighlight %}

max_fds 虽然表示进程最大文件描述符，但进程不一定可以打开这么多的文件，其
还有进程的 "rlimit(RLIMIT_NOFILE)" 和 sysctl_nr_open 进行限制，只有三者
配合使用，进程才能获得正确的最大文件描述符。

###### fd

fd 成员又称为进程的 fdarray。在进程中每个打开的文件都会使用 struct file
进行维护，每个打开的文件又有一个文件描述符，因此文件描述符通过 fdarray
就可以找到对应的 struct file 结构。因此 fd 成员是文件描述符和 struct file
之间的转换桥梁。

###### open_fds

open_fds 成员是一个 bitmap，其长度按 BITS_PER_LONG 对齐。open_fds 的每个
bit 指向一个文件描述符，文件描述符在 open_fds 中按顺序表示，即文件描述符 0
指向 open_fds 的第一个 bit。某个 bit 置位，那么表示对应的文件描述符已经
分配，如果某个 bit 清零，那么对应的文件描述符未分配。通过这个 bitmap，
struct fdtable 实现了简单的文件描述符分配回收逻辑。open_fds 中 bit 的个数
与 max_fds 成员有密切的关系，它们的关系如下:

{% highlight bash %}
bits = ((max_fds + (BITS_PER_LONG - 1)) / BITS_PER_LONG) * BITS_PER_LONG
{% endhighlight %}

open_fds 成员与 close_on_exec 成员的关系是两者都是 bitmap，且长度一致，
只是每个 bit 的含义不同。open_fds 成员与 full_fds_bits 成员存在一定的
联系，full_fds_bits 也是一个 bitmap，每个 bit 用于表示 open_fds 中
BITS_PER_LONG 个 bit 的使用情况。

###### close_on_exec

close_on_exec 成员是一个 bitmap，其长度按 BITS_PER_LONG 对齐。close_on_exec 
的每个 bit 指向一个文件描述符，文件描述符在 close_on_exec 中按顺序表示，即
文件描述符 0 指向 close_on_exec 的第一个 bit。某个 bit 置位，那么表示对应的
文件描述符在调用系统调用 execve() 时需要关闭的文件句柄. 当一个程序使用 
fork() 函数创建了一个子进程时，通常会在该子进程中调用 execve() 函数加载执
行另一个新程序。 此时子进程将完全被新程序替换掉，并在子进程中开始执行新程序。
若一个文件描述符在 close_on_exec 中的对应比特位被设置，那么在执行 execve() 
时该描述符将被关闭，否则该描述符将始终处于打开状态。当打开一个文件时， 默
认情况下文件句柄在子进程中也处于打开状态。因此 sys_open() 中要复位对应比
特位。close_on_exec 中 bit 的个数与 max_fds 成员有密切的关系，它们的关系如下:

{% highlight bash %}
bits = ((max_fds + (BITS_PER_LONG - 1)) / BITS_PER_LONG) * BITS_PER_LONG
{% endhighlight %}

###### full_fds_bits

full_fds_bits 成员也是一个 bitmap，bitmap 中每个 bit 按顺序表示 open_fds
bitmap 中 BITS_PER_LONG 个 bit 的分配情况。例如 open_fds 中的第 n 个
BITS_PER_LONG bit 全部置位，那么对应 full_fds_bits 的 bit 也置位，以此
表示 open_fds 对应的文件描述符全部分配。其关系如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000025.png)

这样的设计就是利用了空间换时间，也为加速 struct fdtable 能够快速找到
一个可用的文件描述符。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A00013D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### struct nameidata

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000030.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A00016D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000G.jpg)

#### filp_cachep

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000033.png)

filp_cachep 是一个 "struct file" 的缓存。由于内核运行过程中，需要
大量的申请和释放 "struct file" 结构，为了减小这样操作带来的开销，
系统在初始化过程中从 SLAB 中分配了一段缓存用于 "struct file" 的分配
和释放。

filp_cachep 的初始化如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000034.png)

系统初始化过程中，调用 files_init() 函数，并在函数内部调用
kmem_cache_create() 函数分配缓存，缓存的名字是 "filp", 大小为
"sizeof(struct file)".

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A00017D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### struct open_flags

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000038.png)

struct open_flags 结构用于协助 open 系统调用管理相关的标志。open 系统调用
将文件打开标志和文件访问标志传递到内核之后，内核调用 build_open_flags()
函数将两个参数进行处理之后存储在 struct open_flags 结构中，以便后续函数
使用。

###### open_flag

open_flag 成员用于存储合法有效的文件打开标志。更多有效文件打开标志如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000559.png)

> - [文件打开标示详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A10)

###### mode

mode 成员由于存储文件的访问权限和文件类型信息。更多标志如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000035.png)

> - [文件权限标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A11)

###### acc_mode

acc_mode 成员是用于存储文件的 permission 信息。该信息主要用于检测文件系统，
inode 等是否具有相应的 permission 信息. 更多 permission 信息如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000558.png)

###### intent

intent 成员用于辅助 open 系统调用在查找过程中使用的标志信息。

###### lookup_flags

lookup_flags 成员用于存储系统调用在文件系统中查找时候所使用的标志，更多
查找标志可以查看:

> - [文件查找标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A14)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="B2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### 快速实践系统调用

> - [实践原理](#B20)
>
> - [实践准备](#B21)
>
> - [添加用户空间实现](#B24)
>
> - [运行系统调用](#B25)
>
> - [内核实统调用调试](#B22)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B20">实践原理</span>

Linux 提供了很多的系统调用，本节用于讲解如何在一个指定的架构上实践
sys_open 系统调用。在用户空间使用 syscall() 直接触发 sys_open 系统调用，
并传入可选的参数到系统调用里。本节以 ARM32 为例子进行讲解，如果需要在其他架构
上进行实践，可以参考下面文档细节:

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000504.png)

选择并进入 "[\*]   sys_open  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000505.png)

选择 "[\*]   open() in C  --->" 保存配置并退出. 接下来执行下面的命令部署
用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000506.png)

上图中，main.c 与用户空间系统调用相关的源码,
"open_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。因此开发者只
需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000507.png)

在该函数中，main 函数首先接收来自命令行的参数，并将其转换为 sys_open 系统
调用所需的参数。由于 sys_open 支持两个参数和三个参数的类型，因此调用
syscall() 函数的时候，可以做两种情况的处理，\_\_NR_open 是 sys_open 对应的
系统调用号，如果系统调用成功，那么 sys_open 将返回打开文件的句柄，至此
一次 sys_open 系统调用完成。为了确保应用程序的正确执行，开发者在程序结束
的时候也要显示关闭文件，这里同样调用 sys_close 系统调用进行关闭。
源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里, 使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="B25">运行系统调用</span>

在一切准备好之后，下一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000508.png)

开发者可以使用如下命令查看 open 系统调用工具的使用方法:

{% highlight c %}
~ # open_common-0.0.1 -h
{% endhighlight %}

可以看出工具的完整使用方法，接下来使用建议的命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

这个命令的行的作用就是以 O_RDWR 和 O_CREAT 的方式打开文件，如果文件不存在，
那么直接创建它，并且文件只给文件拥有者读权限以及文件拥有组读权限。命令
行执行之后，在当前目录下创建了名为 BiscuitOS_file, 查看其权限如下:

{% highlight c %}
~ # ls -l BiscuitOS_file
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000509.png)

从运行结果可以看出，系统创建了指定的文件 BiscuitOS_file, 并且文件的拥有
者和组权限都设置成指定的读权限。接着可以使用 strace 工具查看具体的系统调
用过程，如下:

{% highlight c %}
~ #
~ # strace open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000510.png)

从 strace 打印的消息可以看出:

{% highlight c %}
open("BiscuitOS_file", O_RDWR|O_CREAT, 0440) = 3
close(3)
{% endhighlight %}

sys_open 系统调用已经成功触发，接下来进入系统调用内核部分。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="B22">内核系统调用调试</span>

当用户空间触发了指定的系统调用之后，可以在内核空间添加相应的调试手段，
进而跟踪系统调用的执行。通用的调试手段就是添加打印，打印虽然是一个好的
调试手段，但存在一个问题，由于在某个系统调用中添加了打印信息，所有用户
空间调用这个系统调用的函数都会打印该信息，这样会导致调试信息更加混乱，给
调试带来了很多的不便。还有一个调试问题就是，在跟踪系统调用代码执行过程中，
往往会遇到很多回调函数，也就是看到通过嵌套好几层才调用一个函数，而且不能
直观的看出调用的是那个函数。针对上面提出的问题，开发者如果要调试内核系统
调用，请先参考下面文档之后再进行调试:

> - [解决系统调用调试遇到的诸多问题](https://biscuitos.github.io/blog/SYSCALL_DEBUG/)

准备好上述的内容之后，要调试内核系统调用函数，首先要找到函数的定义。在本
例子中，sys_open 系统调用定义在:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux
vi fs/open.c
{% endhighlight %}

开发者可以在内核系统调用函数添加打印信息，结合用户空间系统调用传递的参数，
进行调试，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000511.png)

在文件中添加如下信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000528.png)

添加完毕之后，在用户空间 sys_open 系统调用代码中添加如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
vi open_common-0.0.1/main.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000529.png)

重新编译内核和应用程序，并运行系统，执行下面命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
make kernel
make build
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000530.png)

通过上面的运行结果可以看出，采用这样的办法可以快速高效的调试系统调用。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

#### open 系统调用调试工具

> - [工具简介](#C00)
>
> - [工具参数介绍](#C01)
>
> - [工具部署](#C02)
>
> - [工具使用](#C03)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### <span id="C00">工具简介</span>

open 系统调用调试工具是 BiscuitOS 开发的一款用于方便开发者调试 open
系统调用的工具。工具将 open() 库函数的参数灵活的传入设置，以达到一个
工具实现创造多种 open 情况。该工具直接绕过 C 库，将参数传递到 sys_open,
这样有效隔离了 Glibc 或者 libc 对影响，确保了调试的准确性。open 系统
调用调试工具使用方式如下:

{% highlight bash %}
open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

从上面的使用情况可以看出，开发者可以自定义打开的文件名，同理可以自定义
打开的标志以及自定义打开的模式，工具支持多个打开标志同时使用。详细工具
的使用参考下面章节。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="C01">工具参数介绍</span>

在使用 open 系统调用调试工具之前，开发者可以使用如下命令查看其使用
说明:

{% highlight bash %}
open_common-0.0.1 -h
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000604.png)

从 open 系统调用调试工具的使用情况可以看出，open_common 必须提供一个文件
路径名、打开文件标志以及文件模式，这样才能正常使用这个工具。

###### <-p pathname>

"-p" 参数提示用户这个参数之后需要使用一个文件路径名，这个路径名可以
一个文件的名字，或者是一个绝对路径等，例如:

{% highlight bash %}
open_common-0.0.1 -p /ect/fstab -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
open_common-0.0.1 -p README.md -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
open_common-0.0.1 -p ../dev/name -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

该参数是工具必须的参数，通过这个参数 open 系统调用会传递一个用户空间
的路径名到内核空间的 sys_open 里。

###### <-f flags>

"-f" 参数后面跟一个打开标志，用于文件的打开标志。当前工具支持的文件
打开标志已经完整罗列在上图中，开发者可以根据实际情况传递这些标志到内核
的 sys_open 系统调用里. 使用方法如下:

{% highlight bash %}
open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
open_common-0.0.1 -p BiscuitOS_file -f O_RDONLY -m S_IRUSR,S_IRGRP
open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_NONBLOCK -m S_IRUSR,S_IRGRP
{% endhighlight %}

更多文件打开标志可以参考如下:

> - [文件打开标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A10)

###### <-m mode>

"-m" 参数后面跟一个模式标志，用于文件的打开模式。当前工具支持的文件打开
模式已经完整罗列在上图中，开发者可以根据实践情况传递这些标志到内核的
sys_open 系统调用里，使用方法如下:

{% highlight bash %}
open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

更多文件打开模式标志可以参考如下:

> - [文件打开模式标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A11)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="C02"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### 工具部署

open 系统调用调试工具支持多个平台，本实践以 ARM32 为例子进行讲解，其他
平台部署以此相似。本实践基于 ARM32 架构，因此在实践之前需要准备一个 ARM32 
架构的运行平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
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

如果需要在其他架构上部署该工具，可以参考下面文档:

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

BiscuitOS 提供了一套完整的系统调用编译系统，开发者可以使用下面步骤快速简单
的部署该工具，BiscuitOS 并可以对该工具从源码进行交叉编译，安装，
打包和目标系统上运行的功能，节省了很多开发时间。如果开发者不想使用这套
编译机制，可以参考下面的内容进行移植。开发者首先获得工具的基础源码，如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000342.png)

选择 "[\*]   strace" 和 "[\*]   System Call" 并进入 "[\*]   System Call  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000605.png)

选择并进入 "[\*]   sys_open  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000606.png)

选择 "[\*]   open() in C  --->" 保存配置并退出. 接下来执行下面的命令部署
用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000506.png)

上图中，main.c 与用户空间系统调用相关的源码,
"open_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。因此开发者只
需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000607.png)

在该函数中，main 函数首先接收来自命令行的参数，并将其转换为 sys_open 系统
调用所需的参数。由于 sys_open 支持两个参数和三个参数的类型，因此调用
syscall() 函数的时候，可以做两种情况的处理，\_\_NR_open 是 sys_open 对应的
系统调用号，如果系统调用成功，那么 sys_open 将返回打开文件的句柄，至此
一次 sys_open 系统调用完成。为了确保应用程序的正确执行，开发者在程序结束
的时候也要显示关闭文件，这里同样调用 sys_close 系统调用进行关闭。
源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里, 使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/open_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C03">工具使用</span>

在一切准备好之后，下一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000508.png)

开发者可以使用如下命令查看 open 系统调用工具的使用方法:

{% highlight c %}
~ # open_common-0.0.1 -h
{% endhighlight %}

可以看出工具的完整使用方法，接下来使用建议的命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

这个命令的行的作用就是以 O_RDWR 和 O_CREAT 的方式打开文件，如果文件不存在，
那么直接创建它，并且文件只给文件拥有者读权限以及文件拥有组读权限。命令
行执行之后，在当前目录下创建了名为 BiscuitOS_file, 查看其权限如下:

{% highlight c %}
~ # ls -l BiscuitOS_file
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000509.png)

从运行结果可以看出，系统创建了指定的文件 BiscuitOS_file, 并且文件的拥有
者和组权限都设置成指定的读权限。接着可以使用 strace 工具查看具体的系统调
用过程，如下:

{% highlight c %}
~ #
~ # strace open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000510.png)

从 strace 打印的消息可以看出:

{% highlight c %}
open("BiscuitOS_file", O_RDWR|O_CREAT, 0440) = 3
close(3)
{% endhighlight %}

从上面的运行结果可以看出，该工具已经将指定的参数精确的传递，sys_open
系统调用已经成功触发，接下开发者可以使用这个工具为 sys_open() 源码创造
指定的情况.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

#### 任意长度文件名工具

> - [工具简介](#C10)
>
> - [工具参数介绍](#C11)
>
> - [工具部署](#C12)
>
> - [工具使用](#C13)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### <span id="C10">工具简介</span>

在有些应用场景需要提供不同长度的文件名，因此任意长度文件名工具就是一款
用于产生任意长度文件名的工具。工具基于 open 系统调用调试工具进行改进，
实现可以根据使用者的需求，产生任意长度的文件名，并将该文件名和其他参数
传递给 sys_open 系统调用。该工具直接绕过 C 库，将参数传递到 sys_open,
这样有效隔离了 Glibc 或者 libc 对影响，确保了调试的准确性。任意长度文件名
工具使用方式如下:

{% highlight bash %}
getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

从上面的使用情况可以看出，开发者可以自定义文件名的长度，同理可以自定义
打开的标志以及自定义打开的模式，工具支持多个打开标志同时使用。详细工具
的使用参考下面章节。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="C11">工具参数介绍</span>

在使用任意长度文件名工具之前，开发者可以使用如下命令查看其使用
说明:

{% highlight bash %}
getname_common-0.0.1 -h
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000608.png)

从任意长度文件名工具的使用情况可以看出，getname_common 必须提供文件
名长度、打开文件标志以及文件模式，这样才能正常使用这个工具。

###### <-l length>

"-l" 参数用于指明文件名的长度，文件名的长度可以为 0，也可以为任意正数，
例如:

{% highlight bash %}
getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
getname_common-0.0.1 -l 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

该参数是工具必须的参数，通过这个参数 open 系统调用会传递一个用户空间
的路径名到内核空间的 sys_open 里。

###### <-f flags>

"-f" 参数后面跟一个打开标志，用于文件的打开标志。当前工具支持的文件
打开标志已经完整罗列在上图中，开发者可以根据实际情况传递这些标志到内核
的 sys_open 系统调用里. 使用方法如下:

{% highlight bash %}
getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
getname_common-0.0.1 -l 128 -f O_RDONLY -m S_IRUSR,S_IRGRP
getname_common-0.0.1 -l 128 -f O_RDWR,O_NONBLOCK -m S_IRUSR,S_IRGRP
{% endhighlight %}

更多文件打开标志可以参考如下:

> - [文件打开标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A10)

###### <-m mode>

"-m" 参数后面跟一个模式标志，用于文件的打开模式。当前工具支持的文件打开
模式已经完整罗列在上图中，开发者可以根据实践情况传递这些标志到内核的
sys_open 系统调用里，使用方法如下:

{% highlight bash %}
getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

更多文件打开模式标志可以参考如下:

> - [文件打开模式标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A11)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="C12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### 工具部署

任意长度文件名工具支持多个平台，本实践以 ARM32 为例子进行讲解，其他
平台部署以此相似。本实践基于 ARM32 架构，因此在实践之前需要准备一个 ARM32 
架构的运行平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
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

如果需要在其他架构上部署该工具，可以参考下面文档:

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

BiscuitOS 提供了一套完整的系统调用编译系统，开发者可以使用下面步骤快速简单
的部署该工具，BiscuitOS 并可以对该工具从源码进行交叉编译，安装，
打包和目标系统上运行的功能，节省了很多开发时间。如果开发者不想使用这套
编译机制，可以参考下面的内容进行移植。开发者首先获得工具的基础源码，如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000342.png)

选择 "[\*]   strace" 和 "[\*]   System Call" 并进入 "[\*]   System Call  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000605.png)

选择并进入 "[\*]   sys_open  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000609.png)

选择 "[\*]   getname(): Max open file name  --->" 保存配置并退出. 接下来执
行下面的命令部署用户空间系统调用程序部署:

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000610.png)

上图中，main.c 与用户空间系统调用相关的源码,
"getname_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。因此开发者只
需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000611.png)

在该函数中，main 函数首先接收来自命令行的参数，函数获得文件名的长度之后，
将 path 字符数组的全部字节设置为 'B' 字符，并在字符数组的指定长度末尾设置
为 '\0'，然后再将新的文件名和其他参数通过 syscall() 函数传递到内核中对应的
sys_open() 函数。由于 sys_open 支持两个参数和三个参数的类型，因此调用
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

#### <span id="C13">工具使用</span>

在一切准备好之后，下一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000612.png)

开发者可以使用如下命令查看 open 系统调用工具的使用方法:

{% highlight c %}
~ # getname_common-0.0.1 -h
{% endhighlight %}

可以看出工具的完整使用方法，接下来使用建议的命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

这个命令的行的作用就是以 O_RDWR 和 O_CREAT 的方式打开文件名长度为 128 的
文件，如果文件不存在，那么直接创建它，并且文件只给文件拥有者读权限以及文
件拥有组读权限。命令行执行之后，在当前目录下创建了名为 BBB....BB, 查
看其权限如下:

{% highlight c %}
~ # ls -l BB*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000613.png)

从运行结果可以看出，系统创建了文件名长度为 128 的文件, 并且文件的拥有
者和组权限都设置成指定的读权限。接着可以使用 strace 工具查看具体的系统调
用过程，如下:

{% highlight c %}
~ #
~ # strace getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000614.png)

从 strace 打印的消息可以看出:

{% highlight c %}
open("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", O_RDWR|O_CREAT, 0440) = 3
close(3)
{% endhighlight %}

从上面的运行结果可以看出，该工具已经将指定的参数精确的传递，sys_open
系统调用已经成功触发，接下开发者可以使用这个工具为 sys_open() 源码创造
指定的情况.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

#### 打开任意个文件工具

> - [工具简介](#C20)
>
> - [工具参数介绍](#C21)
>
> - [工具部署](#C22)
>
> - [工具使用](#C23)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### <span id="C20">工具简介</span>

在有些应用场景需要在一个进程中同时打开多个文件，因此打开任意个文件工具就
是一款用于在一个进程中打开多个文件的工具。工具基于 open 系统调用调试工具
进行改进， 实现可以根据使用者的需求，在同一个进程中打开任意个文件，并将
指定文件名和其他参数传递给 sys_open 系统调用。该工具直接绕过 C 库，将参
数传递到 sys_open, 这样有效隔离了 Glibc 或者 libc 对影响，确保了调试的准
确性。打开任意个文件工具使用方式如下:

{% highlight bash %}
number_open_common-0.0.1 -n 2 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

从上面的使用情况可以看出，开发者可以自定义在一个进程中打开文件的个数，
同理可以自定义打开的标志以及自定义打开的模式，工具支持多个打开标志同时
使用。详细工具的使用参考下面章节。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="C21">工具参数介绍</span>

在使用打开任意个文件工具之前，开发者可以使用如下命令查看其使用说明:

{% highlight bash %}
number_open_common-0.0.1 -h
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000615.png)

从打开任意个文件工具的使用情况可以看出，open_common 必须提供一个文件
路径名、打开文件标志以及文件模式，这样才能正常使用这个工具。

###### <-n num>

"-n" 参数用于指明在当前进程中，打开文件的数量，例如:

{% highlight bash %}
number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
number_open_common-0.0.1 -n 122 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
number_open_common-0.0.1 -n 512 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

该参数是工具必须的参数，通过这个参数工具会在同一个进程中打开指定个数的
文件。

###### <-d debug>

"-d" 参数用于跟踪某一次打开操作。在调试对应的内核源码时，需要对某一次
打开操作进行特定的调试，因此该功能正好满足需求，使用如下:

{% highlight bash %}
number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
number_open_common-0.0.1 -n 512 -d 511 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

###### <-f flags>

"-f" 参数后面跟一个打开标志，用于文件的打开标志。当前工具支持的文件
打开标志已经完整罗列在上图中，开发者可以根据实际情况传递这些标志到内核
的 sys_open 系统调用里. 使用方法如下:

{% highlight bash %}
number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
number_open_common-0.0.1 -n 2 -d 0 -f O_RDONLY -m S_IRUSR,S_IRGRP
number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_NONBLOCK -m S_IRUSR,S_IRGRP
{% endhighlight %}

更多文件打开标志可以参考如下:

> - [文件打开标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A10)

###### <-m mode>

"-m" 参数后面跟一个模式标志，用于文件的打开模式。当前工具支持的文件打开
模式已经完整罗列在上图中，开发者可以根据实践情况传递这些标志到内核的
sys_open 系统调用里，使用方法如下:

{% highlight bash %}
number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

更多文件打开模式标志可以参考如下:

> - [文件打开模式标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A11)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="C22"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### 工具部署

打开任意个文件工具支持多个平台，本实践以 ARM32 为例子进行讲解，其他
平台部署以此相似。本实践基于 ARM32 架构，因此在实践之前需要准备一个 ARM32 
架构的运行平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
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

如果需要在其他架构上部署该工具，可以参考下面文档:

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

BiscuitOS 提供了一套完整的系统调用编译系统，开发者可以使用下面步骤快速简单
的部署该工具，BiscuitOS 并可以对该工具从源码进行交叉编译，安装，
打包和目标系统上运行的功能，节省了很多开发时间。如果开发者不想使用这套
编译机制，可以参考下面的内容进行移植。开发者首先获得工具的基础源码，如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000342.png)

选择 "[\*]   strace" 和 "[\*]   System Call" 并进入 "[\*]   System Call  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000605.png)

选择并进入 "[\*]   sys_open  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000616.png)

选择 "[\*]   open number files  --->" 保存配置并退出. 接下来执行下面的命令
部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/number_open_common-0.0.1
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/number_open_common-0.0.1
make prepare
make download
{% endhighlight %}

执行上面的命令之后，BiscuitOS 自动部署了程序所需的所有文件，如下:

{% highlight bash %}
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000617.png)

上图中，main.c 与用户空间系统调用相关的源码,
"number_open_common-0.0.1/Makefile" 是 main.c 交叉编译的逻辑。因此开发者只
需关注 main.c, 内容如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000618.png)

在该函数中，main 函数首先接收来自命令行的参数，解析出需要打开的文件数量，
为文件描述符指针数组分配指定长度的内存，然后使用 for 循环分配打开一个文件，
文件的名字使用 sprintf() 函数构造，open 调用返回成功的文件描述符存储在文件
描述符数组里。如果某次文件打开失败，则跳转到 out 处关闭之前所有打开的文件;
若文件都正确打开，那么打印文件名字和文件描述符相关的信息，最后关闭所有
打开的文件。在打开每一个文件的过程中，将解析的参数转换为 sys_open 系统
调用所需的参数。由于 sys_open 支持两个参数和三个参数的类型，因此调用
syscall() 函数的时候，可以做两种情况的处理，\_\_NR_open 是 sys_open 对应的
系统调用号，如果系统调用成功，那么 sys_open 将返回打开文件的句柄，至此
一次 sys_open 系统调用完成。为了确保应用程序的正确执行，开发者在程序结束
的时候也要显示关闭文件，这里同样调用 sys_close 系统调用进行关闭。
源码准备好之后，接下来是交叉编译源码并打包到 rootfs 里, 使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/number_open_common-0.0.1
make
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C23">工具使用</span>

在一切准备好之后，下一步就是在 ARM32 上运行系统调用，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000619.png)

开发者可以使用如下命令查看工具的使用方法:

{% highlight c %}
~ # number_open_common-0.0.1 -h
{% endhighlight %}

可以看出工具的完整使用方法，接下来使用建议的命令:

{% highlight c %}
~ # number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

这个命令的行的作用就是以 O_RDWR 和 O_CREAT 的方式打开 2 个文件，如果文件不存在，
那么直接创建它，并且文件只给文件拥有者读权限以及文件拥有组读权限。命令
行执行之后，在当前目录下创建了名为 BiscuitOS_file, 查看其权限如下:

{% highlight c %}
~ # ls -l BiscuitOS*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000620.png)

从运行结果可以看出，系统创建了指定的文件 BiscuitOS-*, 并且文件的拥有
者和组权限都设置成指定的读权限。接着可以使用 strace 工具查看具体的系统调
用过程，如下:

{% highlight c %}
~ #
~ # strace number_open_common-0.0.1 -n 2 -d 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000621.png)

从 strace 打印的消息可以看出:

{% highlight c %}
open("BiscuitOS-0", O_RDWR|O_CREAT, 0440) = 3
write(1, "BiscuitOS-0 Open-fd: 3\n", 23BiscuitOS-0 Open-fd: 3
open("BiscuitOS-1", O_RDWR|O_CREAT, 0440) = 4
write(1, "BiscuitOS-1 Open-fd: 4\n", 23BiscuitOS-1 Open-fd: 4
close(4)                                = 0
close(3) 
{% endhighlight %}

从上面的运行结果可以看出，该工具已经将指定的参数精确的传递，sys_open
系统调用已经成功触发，接下开发者可以使用这个工具为 sys_open() 源码创造
指定的情况.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000R.jpg)

#### strace

> - [工具简介](#C30)
>
> - [工具参数介绍](#C31)
>
> - [工具部署](#C32)
>
> - [工具使用](#C33)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### <span id="C30">工具简介</span>

strace 常用来跟踪进程执行时的系统调用和所接收的信号。 在 Linux 世界，
进程不能直接访问硬件设备，当进程需要访问硬件设备 (比如读取磁盘文件，接收
网络数据等等) 时，必须由用户态模式切换至内核态模式，通过系统调用访问硬件
设备。strace 可以跟踪到一个进程产生的系统调用, 包括参数，返回值，执行消耗
的时间等。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### <span id="C31">工具参数</span>

strace 参数如下:

{% highlight base %}
-c    统计每一系统调用的所执行的时间,次数和出错的次数等. 
-d    输出 strace 关于标准错误的调试信息. 
-f    跟踪由 fork 调用所产生的子进程. 
-ff   如果提供 -o filename, 则所有进程的跟踪结果输出到相应的 filename.pid 
      中, pid 是各进程的进程号. 
-F    尝试跟踪 vfork 调用. 在 -f 时, vfork 不被跟踪. 
-h    输出简要的帮助信息. 
-i    输出系统调用的入口指针. 
-q    禁止输出关于脱离的消息. 
-r    打印出相对时间关于,,每一个系统调用. 
-t    在输出中的每一行前加上时间信息. 
-tt   在输出中的每一行前加上时间信息,微秒级. 
-ttt  微秒级输出,以秒了表示时间. 
-T    显示每一调用所耗的时间. 
-v    输出所有的系统调用. 一些调用关于环境变量、状态、输入输出等调用由于使
      用频繁, 默认不输出. 
-V    输出 strace 的版本信息. 
-x    以十六进制形式输出非标准字符串 
-xx   所有字符串以十六进制形式输出. 
-a column    设置返回值的输出位置.默认 为40. 
-e expr      指定一个表达式,用来控制如何跟踪. 格式如下: 

             [qualifier=][!]value1[,value2]... 

             qualifier 只能是 trace,abbrev,verbose,raw,signal,read,write 其
             中之一. value 是用来限定的符号或数字. 默认的 qualifier 是 trace.
             感叹号是否定符号.例如: 
             -eopen 等价于 -e trace=open, 表示只跟踪 open 调用. 而
             -etrace!=open 表示跟踪除了 open 以外的其他调用.有两个特殊的符号
             all 和 none. 
             注意有些 shell 使用 ! 来执行历史记录里的命令, 所以要使用 \\. 
-e trace=set           只跟踪指定的系统 调用.例如: -e trace=open,close,rean,
                       write 表示只跟踪这四个系统调用. 默认的为 set=all. 
-e trace=file          只跟踪有关文件操作的系统调用. 
-e trace=process       只跟踪有关进程控制的系统调用. 
-e trace=network       跟踪与网络有关的所有系统调用. 
-e strace=signal       跟踪所有与系统信号有关的 系统调用 
-e trace=ipc           跟踪所有与进程通讯有关的系统调用 
-e abbrev=set          设定 strace 输出的系统调用的结果集. -v 等与 abbrev=none.
                       默认为 abbrev=all. 
-e raw=set             将指定的系统调用的参数以十六进制显示. 
-e signal=set          指定跟踪的系统信号.默认为all.如 signal=!SIGIO (或者
                       signal=!io), 表示不跟踪 SIGIO 信号. 
-e read=set            输出从指定文件中读出的数据. 例如: -e read=3,5 
-e write=set           输出写入到指定文件中的数据. 
-o filename            将 strace 的输出写入文件 filename 
-p pid                 跟踪指定的进程 pid. 
-s strsize             指定输出的字符串的最大长度. 默认为 32.文件名一直全部输出.
-u username            以 username 的 UID 和 GID 执行被跟踪的命令.
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

# <span id="C32"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

#### 工具部署

strace 工具支持多个平台，本实践以 ARM32 为例子进行讲解，其他
平台部署以此相似。本实践基于 ARM32 架构，因此在实践之前需要准备一个 ARM32 
架构的运行平台，开发者可以在 BiscuitOS 进行实践，如果还没有搭建 BiscuitOS
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

如果需要在其他架构上部署该工具，可以参考下面文档:

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

BiscuitOS 提供了一套完整的系统调用编译系统，开发者可以使用下面步骤快速简单
的部署该工具，BiscuitOS 并可以对该工具从源码进行交叉编译，安装，
打包和目标系统上运行的功能，节省了很多开发时间。如果开发者不想使用这套
编译机制，可以参考下面的内容进行移植。开发者首先获得工具的基础源码，如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000341.png)

选择并进入 "[\*] Package  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000342.png)

选择 "[\*]   strace  --->" 保存配置并退出. 接下来执行下面的命令
部署用户空间系统调用程序部署:

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000339.png)

执行完毕后，终端输出相关的信息, 接下来进入源码位置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/strace-5.0
{% endhighlight %}

这个目录就是用于部署用户空间系统调用程序，开发者继续使用命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/strace-5.0
make prepare
make download
make tar
make configure
make
{% endhighlight %}

执行完上面的命令之后，strace 已经制作好，接下来就是安装到 BiscuitOS 上，
使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/strace-5.0
make install
make pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C33">工具使用</span>

在一切准备好之后，下一步就是在 ARM32 上运行 strace 了，参考下面
命令进行运行:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000718.png)

开发者可以使用如下命令查看工具:

{% highlight c %}
~ # strace mkdir BiscuitOS
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000719.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### /prco/sys/fs 参数合集

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000722.png)

Linux 在 /proc/sys/fs 节点下提供了很多参数供开发者动态修改或获得特定的系统
参数，其中关于 open 系统调用相关的参数如下:

> - [nr_open](#C40)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

#### <span id="C40">nr_open</span>

nr_open 参数用于控制进程最大可打开文件的数量，可以使用如下命令查看当前进程
最大文件打开数:

{% highlight c %}
cat /proc/sys/fs/nr_open
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000720.png)

也可以动态修改进程最大文件打开数, 使用如下命令:

{% highlight c %}
echo 544 > /proc/sys/fs/nr_open
cat /proc/sys/fs/nr_open
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000721.png)

值得注意的是 nr_open 不一定能够控制进程最大文件打开数，需要和 "ulimit -n"
参数，以及特定的文件系统结合才有效果，但一般情况下，只要 nr_open 小于
"ulimit -m" 的时候，设置 nr_open 是可以起到效果的。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

#### ulimit

ulimit 是一种 Linux 系统的内键功能，它具有一套参数集，用于为由它生成的 
shell 进程及其子进程的资源使用设置限制。使用格式如下:

{% highlight c %}
ulimit -a
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000723.png)

###### 参数说明

{% highlight bash %}
-a                     显示目前资源限制的设定。
-c <core文件上限>      设定 core 文件的最大值，单位为区块。
-d <数据节区大小>      程序数据节区的最大值，单位为 KB。
-f <文件大小>          shell 所能建立的最大文件，单位为区块。
-H                     设定资源的硬性限制，也就是管理员所设下的限制。
-m <内存大小>          指定可使用内存的上限，单位为 KB。
-n <文件数目>          指定同一时间最多可开启的文件数。
-p <缓冲区大小>        指定管道缓冲区的大小，单位 512 字节。
-s <堆叠大小>          指定堆叠的上限，单位为 KB。
-S                     设定资源的弹性限制。
-t <CPU时间>           指定CPU使用时间的上限，单位为秒。
-u <程序数目>          用户最多可开启的程序数目。
-v <虚拟内存大小>      指定可使用的虚拟内存上限，单位为 KB.
{% endhighlight %}

###### 工具使用

ulimit 工具可以查看某个资源或动态设置某个资源，例如查看并设置当前进程最大
打开文件数，使用如下命令:

{% highlight c %}
ulimit -n
ulimit -n 4096
ulimit -n
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000724.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

#### 实践建议

开发者如果想对 open 系统调用进行实践的话，这里有一个可行的建议，开发者
可以参考这个建议进行实践. 首先开发者应该准备一个实践平台，可以参考下面
的文档进行部署:

> - [open 系统调用实践环境部署](#B2)

实践环境搭建好之后，建议开发者了解一些调试技巧，这些调试技巧会让你的调试
变得简单搞笑，请参考下面文章:

> - [系统调用调试建议](https://biscuitos.github.io/blog/SYSCALL_DEBUG/)

准备好上面的内容之后，接下来就是进入源码级实践调试，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000670.png)

上图是 open 系统调用相关的某个函数调用过程，开发者可以根据上图的调用
关系，在下面的文档中找到合适的入口进行源码调试:

> - [open 系统调用源码实践分析](https://biscuitos.github.io/blog/OPEN_SOURCE_CODE/)

有了上面的实践基础之后，开发者可以进行更深入问题的研究讨论，可以查看本文的
进阶研究部分，以此对 open 系统调用进行更深入的研究。

> - [open 系统调用进阶研究](#HHH)

写在最后，开发者也可以在 BiscuitOS 社区进行 open 系统调用的讨论。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="Z0">附录</span>

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
