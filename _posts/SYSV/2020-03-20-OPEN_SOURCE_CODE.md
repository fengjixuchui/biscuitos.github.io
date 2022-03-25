---
layout: post
title:  "sys_open: 源码解析"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

> - [build_open_flags](#A0000000)
>
> - [ACC_MODE](#A0000001)
>
> - [getname/getname_flags](#A0000002)
>
> - [\_\_alloc_fd](#A0000003)
>
> - [find_next_fd](#A0000004)
>
> - [expand_files](#A0000005)
>
> - [expand_fdtable](#A0000006)
>
> - [alloc_fdtable](#A0000007)
>
> - [\_\_free_fdtable](#A0000008)
>
> - [copy_fdtable](#A0000009)
>
> - [copy_fd_bitmaps](#A0000010)
>
> - [\_\_set_open_fd](#A0000011)
>
> - [\_\_set_close_on_exec](#A0000012)
>
> - [\_\_clear_close_on_exec](#A0000013)
>
> - [get_unused_fd_flags](#A0000014)
>
> - [rlimit](#A0000015)
>
> - [task_rlimit](#A0000016)


![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000">build_open_flags</span>

![](/assets/PDB/RPI/RPI000555.png)

build_open_flags 函数用于处理 open 系统调用从用户空间传递下来的 flags
参数和 mode 参数，经过处理合成内核打开文件所需的标志, 标志存在在 
struct open_flags 结构中。(源码较长分段解析)。开发者如果需要在 BiscuitOS 
上实践这段代码请参考下面文档，源码分析过程中数据均依据 BiscuitOS 实践的结
果而定:

> - [BiscuitOS open 系统调用调试工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C0)

###### 源码 1

![](/assets/PDB/RPI/RPI000556.png)

参数 flags 是从用户空间传递下来的文件打开标志，mode 参数是 open 系统调用
的 mode 参数，op 参数是一个指向 struct open_flags 的指针。struct open_flags
的结构体定义如下:

![](/assets/PDB/RPI/RPI000557.png)

open_flag 成员由于存储 open 系统调用打开标志; mode 用于存储 open 系统调用
的 mode 标志; acc_mode 用于存储文件的访问标志; intent 成员用于存储;
lookup_flags 标志用于存储查找文件的标志. 以上成员存储这不同的标志会导致
不同的打开行为，开发者可以参考下文对各个标志的理解:

> - [struct open_flags](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00017D)
>
> - [打开文件标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A10)
>
> - [用户权限标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A11)
>
> - [文件类型标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A12)
>
> - [访问权限标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A13)
>
> - [文件查找标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A14)

回到 build_open_flags() 函数，函数定义了一个局部 int 变量 lookup_flags,
并将其初始化为 0，接着定义了一个局部 int 变量 acc_mode, 并将 ACC_MODE(flags)
的返回值存储在 acc_mode 里面，ACC_MODE() 函数用于解析 flags 参数，从中
获取访问相关的标志，具体实现如下:

> - [ACC_MODE 源码解析](#A0000001)

函数接着将 flags 参数与 VALID_OPEN_FLAGS 进行与操作，并将结果存储在 flags
变量里。VALID_OPEN_FLAGS 宏包含了内核支持的所有打开标志，上面的代码处理之后，
内核过滤了 flags 参数不合法的文件打开标志。

![](/assets/PDB/RPI/RPI000559.png)

开发者可以利用 BiscuitOS 提供的 open 工具调试上面的代码，以此查看每个
结果的具体导, 例如在 BiscuitOS 上使用 open 工具传递不同的文件打开标志。

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR -m S_IRUSR,S_IRGRP
~ # open_common-0.0.1 -p BiscuitOS_file -f O_NONBLOCK -m S_IRUSR,S_IRGRP
{% endhighlight %}

###### 源码 2

![](/assets/PDB/RPI/RPI000560.png)

函数接着检查文件打开参数中是否包含了 O_CREAT 或者 \_\_O_TMPFILE 标志，
如果包含，那么函数将 mode 参数与 S_IALLUGO 相与之后并加上 S_IFREG 标志
存储到 struct open_flags 的 mode 成员中，S_IFREG 标志表示文件是一个普通文
件。如果打开标志不包含以上两个标志中的任意一个，那么将 struct open_flags 
的 mode 成员设置为 0. 开发者可以使用 BiscuitOS 提供的 open 工具进行调试:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,__O_TMPFILE -m S_IRUSR,S_IRGRP
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR -m S_IRUSR,S_IRGRP
{% endhighlight %}

并在源码中添加如下打印信息:

![](/assets/PDB/RPI/RPI000561.png)

重新编译内核并运行 open 工具，内核打印如下信息:

![](/assets/PDB/RPI/RPI000562.png)

在上面的测试命令中，由于 mode 默认的参数中就包含了 S_IRUSR 和 S_IRGRP, 其
值为 0x120, 此时 S_IFREG 的值是 0x8000, 因此从运行的结果来看，只要包含了
O_CREAT 或者 \_\_O_TMPFILE 标志，那么 struct open_flags 的 mode 成员就会
加上 S_IFREG 标志，也就是包含了以上两个标志的，打开的文件必须是普通文件。
然而不包含以上标志的，那么 struct open_flags 的 mode 标志设置为 0. 

回到原函数，函数接着将 FMODE_NONOTIFY 和 O_CLOEXEC 从 flags 参数中移除，
移除的原因是用户空间不应该设置这两个标志。开发者也可以试着在用户空间加上
O_CLOEXEC 标志，看看测试结果，源码修改如下:

![](/assets/PDB/RPI/RPI000563.png)

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CLOEXEC -m S_IRUSR,S_IRGRP
{% endhighlight %}

重新编译内核，运行上面的命令，结果如下:

![](/assets/PDB/RPI/RPI000564.png)

从上图可以看到 O_CLOEXEC 的值是 0x80000, 经过上面的代码处理之后，O_CLOEXEC
已经从 flags 参数中剔除了.

###### 源码 3

![](/assets/PDB/RPI/RPI000565.png)

函数检测打开参数中是否包含 \_\_O_SYNC, 该标志用于表示以同步 I/O 方式打开
文件，具体分析请看如下链接:

> - [\_\_O_SYNC 标志解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001K)

在源码中，如果包含 \_\_O_SYNC，那么将 O_DSYNC 标志加入到 flags 变量里面。
这里注释解释了 O_SYNC 标志由 \_\_O_SYNC 和 O_DSYNC 
标志组成。但在很多地方，当执行同步命令的时候，系统只检测了 O_DSYNC 标志，
而不检查 \_\_O_SYNC 标志，因此为了放置恶意的应用程序只设置了 \_\_O_SYNC
标志。BiscuitOS 上的测试命令如下:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,__O_SYNC -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000566.png)

重新编译内核和运行调试命令, 结果如下:

![](/assets/PDB/RPI/RPI000567.png)

从运行结果可以看出，源码将 O_DSYNC(0x1000) 标志加到 flags 变量里. 这个标志
在执行同步时候有使用。

回到源码，函数继续检查打开标志中是否包含 \_\_O_TMPFILE 标志，该标志用于
创建一个无名的临时文件，文件系统中会创建一个无名的 inode，当最后一个文件
描述符被关闭的时候，所有写入这个文件的内容都会丢失，除非在此之前给了它
一个名字.

> - [\_\_O_TMPFILE 标志解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001A)

如果包含 \_\_O_TMPFILE 标志，那么函数将打开标志与 O_TMPFILE_MASK 相与操
作，以此隔离出 TMPFILE 相关的标志，此时查看隔离出来的标志是否等于 O_TMPFILE 标志，其定义如下:

{% highlight c %}
/* a horrid kludge trying to make sure that this will fail on old kernels */
#define O_TMPFILE (__O_TMPFILE | O_DIRECTORY)
#define O_TMPFILE_MASK (__O_TMPFILE | O_DIRECTORY | O_CREAT)
{% endhighlight %}

从上面的定义，以此可以看出上面源码的作用，如果打开文件包含了 \_\_O_TMPFILE
标志，此时从打开标志中隔离出 O_DIRECTORY、O_CREAT 和 \_\_O_TMPFILE 标志，
此时 \_\_O_TMPFILE 标志一定包含, 那么此时如果不包含 "O_DIRECTORY" 标志，
或者此时 "O_CREAT" 标志存在，那么内核就报错，此时可以在 BiscuitOS 进行
如下测试, 内核中加入调试代码:

![](/assets/PDB/RPI/RPI000568.png)

重新编译内核并在 BiscuitOS 运行测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

运行结果如下:

![](/assets/PDB/RPI/RPI000569.png)

从运行结果来看，内核并没有在指定的位置报错，只是从其他位置报错。为了保证
测试不受上一次测试影响，重新启动 BiscuitOS，接着使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000570.png)

从运行结果来看，内核从指定位置报错，因此这样的打开标志是不正确的。为了保证
测试不受上一次测试影响，重新启动 BiscuitOS，接着使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000571.png)

从运行结果来看，内核从指定位置报错，这组打开标志也不正确。为了保证
测试不受上一次测试影响，重新启动 BiscuitOS，接着使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_CREAT,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000572.png)

从运行结果看出，内核从指定的位置报错了，这组标志也不正确，因此结合上面几组
测试结果，可以得知，当打开标志包含 __O_TMPFILE 标志的时候，组合情况如下:

{% highlight c %}
1) __O_TMPFILE: 不合法
2) __O_TMPFILE | O_CREAT: 不合法
3) __O_TMPFILE | O_DIRECTORY: 合法
4) __O_TMPFILE | O_CREAT | O_DIRECTORY: 不合法
{% endhighlight %}

回到源码，在打开标志中包含 \_\_O_TMPFILE 的情况下, 还需要写权限，因此在
ACC_MODE() 宏处理打开标志中，需要包含可写权限，从 ACC_MODE 和 MAY_WRITE
的定义可以知道 MAY_WRITE 权限通过 O_RDWR 标志给出，具体计算方法请参看:

> - [ACC_MODE 源码解析](#A0000001)

因此在使用 \_\_O_TMPFILE 标志的时候，需要加上 O_RDWR 和 O_DIRECTORY 标志，
接下来在 BiscuitOS 上进行调试，源码中添加如下调试信息:

![](/assets/PDB/RPI/RPI000573.png)

重新编译内核，运行 BiscuitOS 并使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_RDWR,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000574.png)

从运行的结果可以看出，虽然没有正确创建文件，但使用上面的命令之后，这段代码
是可以正确执行。如果此时不给 O_RDWR 标志，那么重新启动 BiscuitOS，运行如下
命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000575.png)

从运行的结果可以看出，如果此时不包含 O_RDWR 命令，那么内核就返回 EINVAL 错误。

回到源码，此时函数检查打开标志中是否不包含 \_\_O_TMPFILE 标志而包含 O_PATH
标志，O_PATH 标志用于获得一个能表示文件在文件系统中位置的文件描述符。标志
的具体含义请看:

> - [O_PATH 标志详细解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001L)

当满足上面的情况之后，函数会过滤打开标志中的多个标志，只保留 O_DIRECTORY、
O_NOFOLLOW、O_PATH 中的一个或者多个标志。并且将 acc_mode 设置为 0. 三个标志
的具体含义可以参看下面链接:

> - [文件打开标志](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A10)

开发者可以在 BiscuitOS 上测试但包含 O_PATH flags 的时候，是否不能包含上面
的三个标志之外的其他标志，添加测试代码如下:

![](/assets/PDB/RPI/RPI000576.png)

重新编译内核，运行 BiscuitOS 并使用如下命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_PATH,O_RDWR -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000577.png)

从上图的运行情况可以看出，O_RDWR 标志已经从 flags 变量中移除。重启 BiscuitOS，
再使用如下命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_PATH,O_RDWR,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000578.png)

从上图可以看出，O_RDWR 标志从 flags 变量中移除，但 O_DIRECTORY 标志和 O_PATH
标志都保留在 flags 变量中。

###### 源码 4

![](/assets/PDB/RPI/RPI000579.png)

函数将处理完的 flags 变量存储在 struct open_flags 的 open_flags 成员里。
如果此时打开标志中包含了 O_TRUNC 标志，该标志如果文件已经存在且为普通文件，
那么清空文件内容，将其长度置为 0. 在 Linux 下使用此标志，无论以读、写方式
打开文件，都可清空文件 内容 (在这种情况下，都必须拥有对文件的写权限)。

> - [O_TRUNC 标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001B)

如果 O_TRUNC 标志存在，那么也就以为了会对打开的文件执行写操作，因此将 acc_mode
变量增加 MAY_WRITE 权限。在 BiscuitOS 中实践这段代码，添加如下调试信息:

![](/assets/PDB/RPI/RPI000582.png)

重新编译内核，运行 BiscuitOS，使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT,O_TRUNC -m S_IRUSR
{% endhighlight %}

![](/assets/PDB/RPI/RPI000581.png)

从上图的运行情况来看，由于起初 open 只传递了 S_IRUSR 的读权限，经过上面
代码处理之后，acc_mode 中增加了 MAY_WRITE 的权限. 

接着分析源码，如果此时打开标志中包含了 O_APPEND 标志，该标志表示
总在文件尾部追加数据。在当下的 Unix/Linux 系统中，这个选项已经被定义为一个
原子操作。

> - [O_APPEND 标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001C)

如果此时打开标志中包含了 O_APPEND, 那么函数中就让 acc_mode 变量增加
MAY_APPEND 标志。同理在 BiscuitOs 上实践这段代码，在源码做如下修改:

![](/assets/PDB/RPI/RPI000580.png)

重新编译源码，运行 BiscuitOS，并使用如下调试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT,O_APPEND -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](/assets/PDB/RPI/RPI000583.png)

从运行结果可以看出，acc_mode 中添加了 MAY_APPEND 的支持. 回到源码，处理
完上面的代码之后，函数将 acc_mode 变量存储到 struct open_flags 的 acc_mode
成员里。

###### 源码 5

![](/assets/PDB/RPI/RPI000584.png)

函数再次判断此时 flags 变量中是否包含 O_PATH 标志，该标志用于获得一个能表
示文件在文件系统中位置的文件描述符. 具体含义参考如下:

> - [O_PATH 标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001L)

因此如果此时 flags 中包含 O_PATH 标志，那么将 struct open_flags 结构中
的 intent 成员设置为 0; 反之设置为 LOOKUP_OEPN 标志。函数继续检查 flags
变量是否包含 O_CREAT 标志，如果包含，那么函数会给 struct open_flags 结构体
的 intent 成员增加 LOOKUP_CREATE 标志。最后在 flags 包含 O_CREAT 的情况下，
函数继续检查 flags 变量是否包含 O_EXCL 标志，此标志与 O_CREAT 标志结合使用
表明如果文件已经存在。具体含义参考如下:

> - [O_EXCL 标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00018)

如果 O_CREAT 和 O_EXCL 标志同时存在，那么函数将给 struct open_flags 结构的 
intent 成员增加 LOOKUP_EXCL 标志。开发者可以在 BiscuitOS 上实践上面的代码，
添加源码修改如下:

![](/assets/PDB/RPI/RPI000585.png)

重新编译内核源码，运行 BiscuitOS，使用如下调试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT,O_EXCL -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](/assets/PDB/RPI/RPI000586.png)

从上面的运行结果可以看出，使用上面的标志，struct open_flags 的 intent 成员
已经包含了 LOOKUP_CREATE、LOOKUP_OPEN 和 LOOKUP_EXCL 标志。

###### 源码 6

![](/assets/PDB/RPI/RPI000587.png)

函数检查 flags 变量此时是否包含 O_DIRECTORY 即这是与目录有关的操作，那么
函数将 LOOKUP_DIRECTORY 标志加入到 lookup_flags 变量里。接着函数检查
flags 变量是否不包含 O_NOFOLLOW 标志，如果不包含表示打开是一个链接操作，
因此将 LOOKUP_FOLLOW 标志加入到 lookup_flags 变量里。最后函数将 
struct open_flags 的 lookup_flags 成员设置为 lookup_flags 变量，至此
build_open_flags() 函数分析完毕。总结该函数对打开标志和权限标志进行了
各种处理之后，将结果存储在 struct open_flags 结果里面，以供接下来的函数
使用。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000001"></span>

ACC_MODE 宏定义如下:

{% highlight c %}
#define O_ACCMODE       00000003
#define ACC_MODE(x)     ("\004\002\006\006"[(x)&O_ACCMODE])
{% endhighlight %}

从上面的宏定义可以看出, ACC_MODE() 宏从 flags 参数中读取最低两个 bit 的
值，如果值为 0，那么 ACC_MODE 则返回 "04"; 值为 1 则返回 "02"; 值为 2，
则返回 "06"; 值为 3，则返回 "06". ACC_MODE() 的返回值代表的含义与内核可能
的访问权限有关，其具体含义如下:

![](/assets/PDB/RPI/RPI000558.png)

MAY 标志的具体含义可以参考如下文档:

> - [MAY 标志解析]()

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000002">gename/getname_flags</span>

![](/assets/PDB/RPI/RPI000542.png)

getname()/getname_flags() 函数用于从用户空间拷贝字符串到内核空间作为
文件在内核中的名字 (源码较长分段解析). 开发者如果需要在 BiscuitOS
上实践这段代码请参考下面文档，源码分析过程中数据均依据 BiscuitOS 实践的结
果而定:

> - [BiscuitOS open 系统调用调试工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C0)

正如上图所示，执行一次系统调用，open() 库函数将参数传递到内核系统调用
"SYSCALL_DEFINE3(open, ...)", 该函数即 sys_open(),sys_open() 接着调用
了 do_sys_open() 执行真正的打开操作，do_sys_open() 函数将用户空间传递
的文件名传递给了 getname() 函数，getname() 函数继续将参数传递给
getname_flags() 函数，getname_flags() 函数就是文件名处理的第一个核心
程序。其定义如下:

###### 源码 1

![](/assets/PDB/RPI/RPI000543.png)

函数首先包含三个参数，第一个参数使用指向用户空间的文件名; 第二个参数是
文件打开的标志; 第三个参数是 empty。函数首先定义了一个 struct filename
指针 result, struct filename 用于维护打开文件的文件名，更多信息可以查看

> - [struct filename 数据结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00010D)

函数接着定义了局部 char 指针 kname, int 变量 len。然后调用
"BUILD_BUG_ON()" 宏配合 offsetof() 函数确认 struct filename 结构体中，
iname 之前的数据是否按 long 对齐，如果没有对齐，那么就报错.

![](/assets/PDB/HK/HK000021.png)

从上面的定义可以看出 struct filename 结构 name 成员到 aname 成员占用了
16 个字节，16 正好按 long 对齐，因此此时不会报错。函数接着调用
audit_reusename() 函数对打开文件名进行检测，但由于当前内核不支持 audit
功能，因此该函数不做任何实际操作直接返回。函数接着用 "\_\_getname()" 函数，
该函数用于从 names_cachep 缓存中为 result 分配内存，接着对分配的结果做了
简单的检查。

![](/assets/PDB/RPI/RPI000544.png)

![](/assets/PDB/RPI/RPI000546.png)

从 names_cachep 缓存中分配内存之后，struct filename 的内存布局如上，
"iname[]" 是一个零长数组，不占用 struct filename 的空间，因此 iname
指向了 offsetof(struct filename, iname) 之后的地址。

###### 源码 2

![](/assets/PDB/RPI/RPI000545.png)

函数将 kname 指针指向了 struct filename 的 iname 成员，接着让 struct filename
的 name 成员指向了 kname 成员，这样就会出现下图的效果:

![](/assets/PDB/RPI/RPI000547.png)

函数调用 strncpy_from_user() 函数从 filename 参数指向用户空间内存拷贝
长度为 EMBEDDED_NAME_MAX 的数据到 kname 指向的内核空间。并将最终拷贝的字
节数存储在 len 变量里，此时如果 len 小于 0，表示拷贝操作失败，那么函数
调用 \_\_putname() 函数将内存返还给 names_cachep 缓存; 如果拷贝没有问题的
话，打开的文件名将被存储到 struct filename 的 iname 区域，此时开发者可以
添加打印函数进行查看文件名, 如下:

![](/assets/PDB/RPI/RPI000548.png)

开发者可以参考上面的代码进行打印，但要注意两种打印方式，第二种是错误的，
这会引起非法访问，在用户空间使用如下命令进行测试:

{% highlight c %}
~ # getname_common-0.0.1 -l 10 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000549.png)

第一个函数正确的打印了一个长度为 10，名字为 "BBBBBBBBBB" 的文件名。而
第二个函数则直接导致系统挂住死机。因此开发者要非常注意不能在内核中使用
带 "\_\_user" 的参数，需要进行用户空间到内核的拷贝操作。

###### 源码 3

![](/assets/PDB/RPI/RPI000550.png)

但打开文件名的长度为 EMBEDDED_NAME_MAX, 其定义如下:

{% highlight c %}
#define PATH_MAX                4096    /* # chars in a path name including nul */
#define EMBEDDED_NAME_MAX       (PATH_MAX - offsetof(struct filename, iname))
{% endhighlight %}

![](/assets/PDB/RPI/RPI000551.png)

从上图可以看出 EMBEDDED_NAME_MAX 的长度，如果此时打开文件名字长度正好为
EMBEDDED_ANME_MAX, 那么函数就首先使用 "offsetof(struct filename, iname[1])"
计算其长度，iname[1] 指向 iname[0] 的下一个地址，如下图:

![](/assets/PDB/RPI/RPI000552.png)

函数将 kname 指向了 result 指向的内存空间，接着调用 kzalloc() 函数分配
长度为 size 的空间，并使用 result 指向其起始地址。接着对分配的内存检测。
然后将 result 的 name 成员指向 kname，此时内存布局如下图:

![](/assets/PDB/RPI/RPI000553.png)

此时从 kmalloc 内存分配的新 struct filename 的 name 成员指向了原先从
names_cachep 分配的内存，此时原先的内存用于存储从用户空间拷贝的打开文件
名. 此时函数调用 strncpy_from_user() 函数再次从 filename 参数指向的用户
空间内核拷贝文件名到 kname 指向的内核空间，拷贝长度为 PATH_MAX, 如果
此时拷贝失败，那么释放 result 分配的内存，并将 kname 指向的内存空间归还给
names_cachep 缓存。如果拷贝成功，但拷贝长度为 PATH_MAX, 那么函数视为
超过长长度的文件名，函数释放相关内存之后，返回 -ENAMETOOLONG. 如果文件
名长度大于或等于 EMBEDDED_NAME_MAX 且小于 PATH_MAX, 那么算合法文件名。
接下来复现上面的问题, 在对应代码添加代码如下:

![](/assets/PDB/RPI/RPI000554.png)

在做好打印消息只有，然后分别对 4080、4090、4096、4097 长度的文件名:

{% highlight c %}
~ # getname_common-0.0.1 -l 4080 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](/assets/PDB/RPI/RPI000589.png)

从上图运行的情况来看，文件名长度为 4080 能正确拷贝和存储到指定位置。接着
重启 BiscuitOS，使用如下命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 4090 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](/assets/PDB/RPI/RPI000590.png)

从上图运行的情况来看，文件名长度为 4090 的时候能正常拷贝和存储到指定位置。
接着重启 BiscuitOS, 然后使用如下命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 4096 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](/assets/PDB/RPI/RPI000591.png)

从运行的情况来看，系统可以正常从用户空间拷贝长度为 PATH_MAX 的字符串，
但由于长度等于 PATH_MAX, 那么函数判断名字太长, 函数直接返回 ENAMETOOLONG.

###### 源码 4

![](/assets/PDB/RPI/RPI000592.png)

如果上面的代码执行成功，函数将 struct filename 的 refcnt 成员设置为 1，
以此代表该数据结构已经开始维护一个文件名字，可以供其他函数使用。如果此时
函数检测到文件名的长度为 0，那么函数会进一步处理。首先如果此时 empty 参数
不为零，那么函数将 empty 指针引用的值设置为 1. 如果此时文件打开标志 flags
变量里不包含 LOOKUP_EMPTY, 那么函数判定为错误的情况，因为此时在不查找空
文件名的情况下文件名长度为 0，函数由于文件名长度为 0，因此只需释放 result
指向的内存到 names_cachep 缓存里，最后返回 ENOENT; 反之如果文件名的长度
为 0，且文件打开标志中包含 LOOKUP_EMPTY 标志，那么函数将进行执行长度为 0
的文件名操作。

处理完上面的代码之后，函数将 filename 参数存储在 struct filename 的 uptr
指向 filename 参数，然后将 aname 设置为 NULL。 最后返回 struct filename
结构的 result 变量.

综上所述，getname()/getname_flags() 函数从用户空间拷贝打开文件的文件名
到内核空间，并将其存储在 struct filename 结构中。内核只要访问 struct filename
的 name 成员就可以获得打开文件的文件名.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000003">\_\_alloc_fd</span>

![](/assets/PDB/RPI/RPI000593.png)

\_\_alloc_fd() 函数用于从当前进程中分配一个可用文件描述符. (源码较长分段
解析). 开发者如果需要在 BiscuitOS 上实践这段代码请参考下面文档，源码分析
过程中数据均依据 BiscuitOS 实践的结果而定:

> - [BiscuitOS open 工具之打开任意个文件](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C2)

正如上图所示，执行一次系统调用，open() 库函数将参数传递到内核系统调用
"SYSCALL_DEFINE3(open, ...)", 该函数即 sys_open(),sys_open() 接着调用
了 do_sys_open() 执行真正的打开操作，do_sys_open() 函数将用户空间传递
的文件打开标志 flags 传入到 "get_unused_fd_flags()" 函数，函数再调用
"\_\_alloc_fd" 函数，其定义如下:

###### 源码 1

![](/assets/PDB/RPI/RPI000594.png)

\_\_alloc_fd() 函数的第一个参数一个 struct files_struct 结构指针, 用于指向
当前进程的文件描述结构; 第二个参数是 start，用于指向用于分配文件描述符的起始
值; 第三个参数是 end, 用于指向用于分配文件描述符的终止地址; 第四个参数是
文件打开文件标志。函数定义了一个局部 unsigned int 变量 fd, int 变量 error，
以及 struct fdtable 结构体指针 fdt. 对于上面的参数中，struct files_struct 
的定义如下:

![](/assets/PDB/RPI/RPI000595.png)

每个进程维护一个 struct files_struct 结构，用户管理当前进程所有打开文件
的信息，因此可以通过:

{% highlight c %}
current->files
{% endhighlight %}

通过上面的代码可以获得当前进行打开文件信息。详细的 struct files_struct 
结构体描述，请查看:

> - [struct files_struct 解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00011D)

在 struct files_struct 结构中比较重要的成员是 struct fdtbale 结构体类型
的 fdt 和 fdtab 两个成员。struct fdtable 的定义如下:

![](/assets/PDB/RPI/RPI000596.png)

struct fdtable 数据结构用于管理和维护当前进程的文件描述符。在 Linux 内核
中，使用一个整数表示文件描述符，对于同一个进程，进程可以使用唯一的文件描
述符管理一个打开的文件; 而对于不同的两个或多个进程，虽然文件描述符相同，
但不一定指向同一个打开的文件。因此文件描述符在进程内讨论才有意义。
struct fdtbale 结构的 max_fds 用于表示当前进程支持的最大文件描述符数; 
close_on_exec 成员是一个 bitmap，每个 bitmap 对应一个文件描述符，每个 bit 
的代表一个打开的文件描述符，用于确定在调用系统调用 execve() 时需要关闭的
文件句柄。当一个程序使用 fork() 函数创建了一个子进程时，通常会在该子进程
中调用 execve() 函数加载执行另一个新程序。 此时子进程将完全被新程序替换掉，
并在子进程中开始执行新程序。若一个文件描述符在 close_on_exec 中的对应比特
位被设置，那么在执行 execve() 时 该描述符将被关闭，否则该描述符将始终处于
打开状态。当打开一个文件时， 默认情况下文件句柄在子进程中也处于打开状态。
因此 sys_open() 中要复位 对应比特位。

struct fdtable 结构中 full_fds_bits 和 open_fds 都是 bitmap， max_fds 代表
该结构能够维护最大文件描述数，max_fds 的值决定了 open_fds 的 bits 数。在 
open_fds 指向的 bitmap 中，bit 从低位开始编码，每个 bit 对应一个文件描述符，
即 bit0 代表的文件描述符就是 0，bit1 代表的文件描述符就是 1. 在 open_fds 的
bitmap 中，置位的 bit 对应的文件描述符已经分配， 反之该 bit 清零则表示该文
件描述符没有分配。同理 full_fds_bits 也是一个 bitmap，并且从低位开始编码，但 
full_fds_bits 的每个 bit 指代 open_fds 中的 32 个 bit，即 full_fds_bits 的 
bit0 代表了 open_fds 的 0 到 31 bit, full_fds_bits 的 bit-1 代表了 open_fds 
的 32 到 63 bit, 如下图:

![](/assets/PDB/HK/HK000025.png)

fd 是 struct file 的二级指针，用于指向当前进程所有打开文件的文件结构指针
数组。每个打开的文件都会使用一个 struct file 进行管理，并将这个 struct file
加入到当前进程的 struct files_struct 的 fd_array[] 数组里，然后 fd 就是
指向 struct files_struct 的 fd_arrayp[] 数组。更多 struct fdtable 信息:

> - [struct fdtable 详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

###### 源码 2

![](/assets/PDB/RPI/RPI000597.png)

函数首先对 files 结构上自旋锁，然后调用 files_fdtable() 函数从 files 中
获得当前进程的文件描述符表，函数定义如下:

{% highlight c %}
#define files_fdtable(files) \
        rcu_dereference_check_fdtable((files), (files)->fdt)
{% endhighlight %}

函数将 fd 变量的值设置为 start，以此代表从文件描述符表 start 处开始查看
下一个可用的文件描述符。从上面的讨论可以知道，struct files_struct 的 
next_fd 成员存储着当前进程下一个可用的文件描述符。如果此时 fd 的值小于
当前进程下一个可用文件描述符，那么将 fd 设置为当前进程下一个可用的文件
描述符。如果 fd 变量值小于文件描述符表的最大可用文件描述符，那么函数
调用 find_next_fd() 函数从 fdt 中获得一个有效的文件描述符。find_next_fd()
函数实现参考如下:

> - [find_next_fd() 函数解析](#A0000004)

###### 源码 3

![](/assets/PDB/RPI/RPI000622.png)

如果此时函数将 error 变量设置为 -EMFILE, 如果此时获得文件描述符大于或
等于函数限定的最大文件描述符，那么函数跳转到 out 处继续执行。函数接着
调用 expand_files() 函数检测当前进程的文件相关信息中，是否需要扩大当前
所支持的最大打开文件，默认情况下一个进程的最大打开文件数是 32，如果
此时找到的文件描述符大于当前最大文件打开数，那么函数就是在 expand_files
将当前进程最大文件打开数扩大一定的数量，以此容纳更多的打开文件。其具体
实现如下:

> - [expand_files() 函数解析](#A0000005)

###### 源码 4

![](/assets/PDB/RPI/RPI000661.png)

如果此时进程已经将其文件描述表进行了扩容，那么 error 的值不为零，函数将
跳转到 repeat 处，在新的文件描述符表下再做一次分配。开发者可以在 BiscuitOS
跟踪该过程，修改源码如下:

![](/assets/PDB/RPI/RPI000662.png)

重新编译源码并运行 BiscuitOS，使用如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000663.png)

从上图可以看出，进程原始最大文件打开数是 32，函数对进程的最大文件打开数
进行扩容，扩容后的最大文件打开数是 256，此时函数跳转到 repeat 处重新分配
可用的文件描述符。

回到函数，函数检测 start 参数是否小于或等于进程下一个可用文件描述符，
如果条件满足，那么将进程 struct files_struct 的 next_fd 成员设置为
待分配文件描述符的下一个文件描述符。函数接着调用 \_\_set_open_fd() 函数，
该函数将 fd 文件描述符在 struct fdtable 的 open_fds bitmap 和 full_fds_bit
bitmap 置位。函数的具体实现可以查看如下:

> - [\_\_set_open_fd() 函数解析](#A0000011)

回到函数，如果此时 flags 标志中包含 O_CLOEXEC, 那么函数调用 
\_\_set_close_on_exec() 函数将 fd 描述符在 struct fdtable close_on_exec
bitmap 中对应的 bit 置位，以此进程返回时关闭指定的文件描述符; 反之则调
用 \_\_clear_close_on_exec() 函数将 fd 描述符对应的 bit 在 struct fdtable 
的 close_on_exec bitmap 中清零。open 系统调用的文件打开标志禁止使用 
O_CLOEXEC 标志。具体函数实现可以参考如下:

> - [\_\_set_close_on_exec() 函数解析](#A0000012)
>
> - [\_\_clear_close_on_exec() 函数解析](#A0000013)

函数处理完上面的操作之后，再进行一些检测之后，将获得的文件描述符返回。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000004">find_next_fd</span>

![](/assets/PDB/RPI/RPI000598.png)

函数的作用是在 struct fdtable 结构的 full_fds_bits 和 open_fds 成员中
找到一个可用的文件描述符，struct fdtable 结构的详细分析如下:

> - [struct fdtbale 解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

由 struct fdtable 的 full_fds_bits 和 open_fds 成员可以得到如下关系:

![](/assets/PDB/HK/HK000025.png)

open_fds 是一个 unsigned long 的 bitmap，其总长度由 struct fdtbale 的
max_fds 决定，struct fdtable 的 open_fds 成员的每个 bit 代表一个文件描
述符的使用情况，如果某个 bit 置位，那么代表该 bit 指向的文件描述符已经
分配，反之如果某个 bit 清零，那么代表该 bit 指向的文件描述符未分配。
full_fds_bits 也是一个 bitmap，每个 bit 指向一个 unsigned long 的 bitmap，
根据上图的定义，full_fds_bits 的第一个 bit 指向 open_fds 最开始的
"sizeof(unsigned long) * 8" 个 bit，如果这些 bit 全部置位，那么
full_fds_bits 对应的 bit 也会置位，反之清零表示对应的 bit 还可以分配。
回到函数，函数具有两个参数，第一个参数指向 struct fdtable 结构，第二个
参数 start 指向一个文件描述符的起始值。函数定义了第一个局部变量 maxfd 
存储当前进程支持的最大文件描述符，第二个局部变量 maxbit 用于表示最大文
件描述符位于 full_fds_bits 的偏移位置，同理第三个局部变量 bitbit 用于表
示 start 参数对应的文件描述符在 full_fds_bits 中的偏移。

获得上面信息之后，函数直接调用 find_next_zero_bit() 函数找到 full_fds_bits
bitmap bitbit 开始处第一个清零的 bit 位置，然后将该偏移乘以 BITS_PER_LONG,
以此获得具体的 open_fds bitmap 的位置。如果此时 bitbit 大于 maxfd，那么
直接返回 maxfd，如果 bitbit 大于 start，那么将 start 设置为 bitbit，以上
两个检测的作用确保下一个可用的文件描述符在当前进程支持的最大文件描述符内。
最后函数调用 find_next_zero_bit() 函数开始在 open_fds 的 bitbit 之后找到
第一个为 0 的值，这就是下一个可用的文件描述。函数返回该值。

> - [find_next_zero_bit 函数解析](https://biscuitos.github.io/blog/BITMAP_find_next_zero_bit/)


![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000005">expand_files</span>

![](/assets/PDB/RPI/RPI000623.png)

expand_files() 函数用于在满足特定条件下，扩充进程最大文件打开数。
参数 files 用于指向进程的 struct file_struct 结构，nr 用于指向一个待分配
的文件描述符，struct file_struct 的详细介绍如下:

> - [struct file_struct 结构详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00011D)

函数定义了一个 struct fdtable 的局部指针，以及一个 int 变量 expanded, 其值
为 0.

###### 源码 1

![](/assets/PDB/RPI/RPI000624.png)

函数首先获得进程的 struct files_struct 对应的 fdt 成员，开发者可有先了解
struct fdtable 结构体的信息，如下:

> - [struct fdtable 结构详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

struct fdtable 的 max_fds 成员用于描述对应进程的最大文件打开数，如果此时
待分配的文件描述符小于对应进程最大文件打开数，那么进程不需要扩充最大文件
打开数，直接返回 0; 反之如果待分配的文件描述符已经大于进程最大文件打开数，
那么进程需要进行扩充最大文件打开数。函数在扩充之前会判断待分配的文件描述符
是否大于等于 sysctl_nr_open, sysctl_nr_open 用于空间进程最大文件打开数。
如果待打开文件数大于 sysctl_nr_open, 那么此时函数是不能进行扩充最大文件
打开数的，函数返回 -EMFILE. 函数准备扩充进程的最大打开文件数，此时函数先
判断 struct files_struct 的 resize_in_progress 成员是否为真，如果为真代表
也有其他函数在修改 struct files_struct 的 fdt 成员，因此暂时不能扩充，那么
函数解除 struct files_struct 的 file_lock, 将 expanded 设置为 1， 然后调用
wait_event() 函数等待 resize_in_progress 的值等于 false 的时候，进入等待;
反之 struct files_struct 的 resize_in_progress 为假或者函数等到了
resize_in_progress 由真变为假，那么函数就可以进行扩充。

开发者也可以在 BiscuitOS 进行实践来让进行扩充最大文件打开数，开发者
可以参考:

> - [BiscuitOS open 工具之打开任意个文件](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C2)

在源码中添加调试信息，如下:

![](/assets/PDB/RPI/RPI000625.png)

重新编译源码，运行 BiscuitOS，并使用如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000626.png)

从上面运行的效果可以看出，number_open_common 工具在当前进行打开了 30 个
文件，但此时文件最大文件打开数是 32，而此时打开第 30 个文件时候，带分配
的文件描述符是 32, 因此不符合上面代码的要求，因此进行扩充最大文件打开数，
而且此时 struct files_struct 的 resize_in_progress 成员已经 false，因此
函数可以对进程进行扩充最大打开文件数。

###### 源码 2

![](/assets/PDB/RPI/RPI000627.png)

一切准备之后，函数将 struct files_struct 的 resize_in_progress 设置为 
true，以此告诉其他需要修改进程 struct files_struct 的 fdt 成员，现在正在修改，
请等待一会。函数接着调用 expand_fdtable() 函数进行实际的扩容进程的最大打开
文件数，扩容完毕之后，函数将 struct files_struct 的 resize_in_progress 设置
为 false，以此告诉其他函数可以修改 struct files_struct 的 fdt 成员。最后函数
调用 wake_up_all() 函数告诉所有等待 struct files_struct 的 fdt 成员的函数，
现在可以修改了. 具体 expand_fdtable() 实现过程可以参考如下:

> - [expand_fdtable() 函数解析](#A0000006)

开发者也可以在 BiscuitOS 实践，以此来跟踪该过程，修改源码如下:

![](/assets/PDB/RPI/RPI000628.png)

重新编译原理，运行 BiscuitOS，并使用如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000629.png)

从上图可以看出 struct files_struct 的 fdt 成员，其 max_fds 由原先的 32
变成了 256, 因此可以确定函数确实将当前进程的最大打开文件数从 32 扩大
到 256. 开发者还可以使用如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 254 -d 253 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000630.png)

从上图可以看出 struct files_struct 的 fdt 成员，其 max_fds 由原先的 256
变成了 512, 因此可以确定函数确实将当前进程的最大打开文件数从 256 扩大
到 512. 开发者还可以使用如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 510 -d 509 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000631.png)

从上图可以看出 struct files_struct 的 fdt 成员，其 max_fds 由原先的 512
变成了 1024, 因此可以确定函数确实将当前进程的最大打开文件数从 512 扩大
到 1024. 开发者还可以使用如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 1022 -d 1021 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000632.png)

从上图的运行结果可以看出，当待分配的文件描述符是 1024 的时候，分配失败，
函数直接返回错误码 EMFILE, 表示太多文件，这与具体的文件系统有关。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000006">expand_fdtable</span>

![](/assets/PDB/RPI/RPI000633.png)

expand_fdtable() 函数的作用是扩充进程最大打开文件数。struct files_struct
参数是特定进程的打开文件信息，参数 nr 用于指明待分配的文件描述符。函数
定义了两个 struct fdtable 指针 new_fdt 和 cur_fdt. struct fdtable 用于
维护打开文件的信息。具体结构描述如下:

> - [struct files_struct 结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00011D)
>
> - [struct fdtable 结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

###### 源码 1

![](/assets/PDB/RPI/RPI000634.png)

函数先将进程 struct files_struct 的 file_lock 锁解锁，然后调用 
alloc_fdtable() 函数根据 nr 指向的文件描述符大小，分配一个新的
struct fdtable 结构。如果此时通过原子读发现进程 struct files_struct
的 count 成员值大于 1, 如果大于那么调用 synchronize_rcu() 函数进行同步。
函数接着将进程 struct files_struct 的 file_lock 锁加锁。最后检查分配
的 new_fdt 是否成功，不成功则直接返回 -ENOMEM.

> - [alloc_fdtable() 函数解析](#A0000007)

###### 源码 2

![](/assets/PDB/RPI/RPI000651.png)

新分配的 struct fdtable 之后，函数检测新扩容的最大文件打开数是否小于
或等于待分配的文件描述符，如果小于，那么函数认为扩容是失败的，那么函数
会将新分配的 struct fdtable 释放掉，并返回 EMFILE 表示当前系统没有那么
多文件描述符可以分配。具体释放过程可以查看:

> - [\_\_free_fdtable() 函数解析](#A0000008)

如果新扩从的最大文件打开数大于待分配的文件描述符，那么函数获得当前进程
struct fdtable 结构，如果待分配的文件描述符小于当前进程最大文件打开数，
那么函数会认定为这是一个 BUG。如果此时没有任何 BUG，那么函数调用 
copy_fdtable() 函数将当前进程的 struct fdtable 拷贝到新的 struct fdtable
里，具体实现过程如下:

> - [copy_fdtable() 函数解析](#A0000009)

###### 源码 3

![](/assets/PDB/RPI/RPI000660.png)

新的 struct fdtable 已经初始化完毕，函数将其设置为进程正在使用的文件描述
符表。最后添加相应的同步机制使上面的机制有效。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000007">alloc_fdtable</span>

![](/assets/PDB/RPI/RPI000635.png)

alloc_fdtable() 函数用于分配一个 struct fdtable 结构, 并根据 nr 参数的需求
初始化新 struct fdtable 中的每个成员。参数 nr 用于指定当前
某个进程支持的最大打开文件数，函数可以根据这个参数创建一个支持更大文件打开数
的 struct fdtable 结构。函数定义了一个 struct fdtable 局部指针，以及一个
void 类型的指针。

###### 源码 1

![](/assets/PDB/RPI/RPI000636.png)

通过注释可以知道，函数规划每次增加 1024 Bytes 的空间给 struct fdtable 的
fdarray, 如下图:

![](/assets/PDB/RPI/RPI000637.png)

struct fdtable 的 fd 成员就是一个 struct file 的指针数组，每个指针占用 
"sizeof(struct file *)" 个字节，然后函数规划每次扩充 fdtable 的时候，第一次
增加 1024 个字节，后续按倍数增加，因此函数中的增加方法。函数首先确定上一次
扩容或默认时最大打开文件数，于是就使用了下面的逻辑:

{% highlight c %}
nr = nr / (1024 / sizeof(struct file *))
{% endhighlight %}

如果 nr 小于 (1024 / sizeof(struct file *)), 那么函数是第一次扩容，那么
为 struct fdtable 的 fd 成员扩容 1024 个字节。roundup_pow_of_two() 函数
的作用是得到参数最靠近的 2 次幂，例如:

{% highlight c %}
roundup_pow_of_two(0):      2
roundup_pow_of_two(1):      1
roundup_pow_of_two(2):      2
roundup_pow_of_two(3):      4
roundup_pow_of_two(4):      4
roundup_pow_of_two(5):      8
roundup_pow_of_two(6):      8
roundup_pow_of_two(7):      8
roundup_pow_of_two(8):      8
{% endhighlight %}

结合上面的数据，函数最后计算出扩容的个数:

{% highlight c %}
nr = nr * (1024 / sizeof(struct file *))
{% endhighlight %}

通过上面的分析，那么 struct fdtable 扩容的数据如下:

{% highlight c %}
32 -> 256 -> 512 -> 1024 -> 2048 -> 4096
{% endhighlight %}

开发者也可以在 BiscuitOS 上实践这部分内容，在源码中添加如下内容:

![](/assets/PDB/RPI/RPI000638.png)

重新编译源码并运行 BiscuitOS，使用如下测试命令:

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000639.png)

从上图的运行情况可以看出，struct fdtable 从 32 扩容到 256. 也可以使用
如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 254 -d 253 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000640.png)

从上图的运行情况可以看出，struct fdtable 从 256 扩容到 512. 也可以使用
如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 510 -d 509 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000641.png)

从上图的运行情况可以看出，struct fdtable 从 512 扩容到 1024.

###### 源码 2

![](/assets/PDB/RPI/RPI000642.png)

函数检测扩容后的最大文件打开数是否超过 sysctl_nr_open, 如果超过，那么
函数将 nr 设置为:

{% highlight c %}
nr = ((sysctl_nr_open - 1) | (BITS_PER_LONG - 1)) + 1;
{% endhighlight %}

sysctl_nr_open 可以动态修改，开发者可以在 BiscuitOS 上实践上面代码，通过
动态修改 sysctl_nr_open 的大小，让上面的代码执行，修改代码如下:

![](/assets/PDB/RPI/RPI000643.png)

重新编译源码并运行 BiscuitOS，使用如下命令:

{% highlight c %}
echo 513 > /proc/sys/fs/nr_open
number_open_common-0.0.1 -n 510 -d 509 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
cat /proc/sys/fs/nr_open
{% endhighlight %}

![](/assets/PDB/RPI/RPI000644.png)

从上面的命令可以知道，通过设置 sysctl_nr_open 为 513，然后打开第 509 个
文件的时候，待分配的文件描述符是 512， 此时当前进程最大文件打开数是 512，
因此需要扩容，经过上面的计算，最大打开文件数会扩容到 1024，可是当前
sysctl_nr_open 的值是 513，此时 nr 大于 sysctl_nr_open 的值，那么函数认为
当前最大文件打开数不能扩容到 1024, 需要重新计算。由于最大打开文件数使用
bitmap 进行管理，其管理逻辑如下:

![](/assets/PDB/HK/HK000025.png)

struct fdtable 的 open_fds 成员的 bitmap 每个 bit 代表一个文件，而 
full_fds_bits 的每一个 bit 指向 BIT_PER_LONG 个文件，因此 open_fds
含有多个 BIT_PER_LONG 的 BITMAP，因此 struct fdtable 能打开的文件数都
是按 BITS_PER_LONG 对齐，因此 sysctl_nr_open 也需要按 BIT_PER_LONG
进行对齐，因此出现了下面的计算公式:

{% highlight c %}
nr = ((sysctl_nr_open - 1) | (BITS_PER_LONG - 1)) + 1;
{% endhighlight %}

根据上面的分析，因此可以知道虽然 sysctl_open 限定了某个值，其实最大打开文件
数是可以超过 sysctl_open 的，可以验证上面的设计, 在 BiscuitOS 上测试如下命令:

{% highlight c %}
echo 513 > /proc/sys/fs/nr_open
number_open_common-0.0.1 -n 541 -d 540 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
cat /proc/sys/fs/nr_open
{% endhighlight %}

![](/assets/PDB/RPI/RPI000645.png)

从上图可以看出，虽然 sysctl_nr_open 设置为 513，但文件描述符 543 是合法的。

{% highlight c %}
echo 513 > /proc/sys/fs/nr_open
number_open_common-0.0.1 -n 542 -d 541 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
cat /proc/sys/fs/nr_open
{% endhighlight %}

![](/assets/PDB/RPI/RPI000646.png)

从上图可以知道，sysctl_nr_open 设置为 513，但文件描述符 544 是不合法的，
正好符合之前的设计。因此 sysctl_nr_open 并不能准确的控制进程的最大打开文件
数。

![](/assets/PDB/RPI/RPI000642.png)

回到源码，函数使用 kmalloc() 函数为新的 struct fdtable 分配了内存空间，
并检测了分配的结果。接着将更新后的最大文件打开数存储在新 struct fdtable
的 max_fds 成员里，因此可以通过通过下面的方法获得当前进程的最大文件打开
数:

{% highlight c %}
nr = current->files->fdtab.max_fds
{% endhighlight %}

函数接着调用了 kvmalloc() 函数分配了一个 "struct file" 指针数组的内存，
数组中每个成员都是一个指针，占用 "sizeof(struct file \*)" 个字节, 然后
新的 struct fdtable 可以存储 nr 个 "struct file \*". 分配成功之后，将
新的 struct fdtable 的 fd 指向了这块新的内存。至此新的 struct fdtable
已经初始化好了两个成员。

###### 源码 3

![](/assets/PDB/RPI/RPI000647.png)

接下来函数初始化新的 struct fdtable 剩下的成员。回到 struct fdtable 剩下
成员的定义:

> - [struct fdtable 结构详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

![](/assets/PDB/HK/HK000025.png)

open_fds 是一个 bitmap，每个 bitmap 维护一个打开文件，因此 open_fds 总共
有 max_fds 个 bit，然后 close_on_exec 成员维护的 bitmap 的长度和 open_fds
一致，也是具有 max_fds 个 bit。full_fds_bits 的一个 bit 代表 open_fds
里面 BITS_PER_LONG 个 bit，因此 full_fds_bits 与 max_fds 的关系就是:

{% highlight c %}
size = max_fds / BITS_PER_LONG / BITS_PER_LONG
{% endhighlight %}

函数这里使用了 BITBIT_SIZE() 宏实现的效果同上，因此为剩下三个成员分配的内存
大小为:

{% highlight c %}
size = 2 * max_fds + max_fds / BITS_PER_LONG / BITS_PER_LONG
{% endhighlight %}

由于从之前代码计算出来新的 max_fds 已经按 BITS_PER_LONG 对齐过，因此这里
可以直接使用。但由于 full_fds_bits 的 bitmap 需要进行 BITS_PER_LONG 对齐，
因此最终计算公式如下:

{% highlight c %}
size = 2 * max_fds + ((max_fds / BITS_PER_LONG) + (BITS_PER_LONG -1)) / BITS_PER_LONG
{% endhighlight %}

函数使用了 kvmalloc() 分配了指定大小的内存，然后存储在 data里面，并对分配的
结果进行检测。

![](/assets/PDB/RPI/RPI000648.png)

根据上图结合源码，函数首先将新的 struct fdtable 的 open_fds 成员指向了
第一块内存区域，以此作为 open_fds 的 bitmap 使用; 同理 close_on_exec
指向了第二块内存区域，作为其 bitmap 使用; 剩余的内存就作为 full_fds_bits
的 bitmap 使用。从上面的分配看出内核精准不带浪费的为新 struct fdtable
分配了内存。至此新的 struct fdtable 分配和初始化完毕。开发者也可以在
BiscuitOS 上跟踪上面的分配过程，源码修改如下:

![](/assets/PDB/RPI/RPI000649.png)

重新编译内核并运行 BiscuitOS，使用如下测试命令:

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000650.png)

从上图的运行结果可以分析得出，当前最大文件打开数是 256, 因此需要 open_fds
的 bitmap 需要 256 个 bit 维护这些打开文件，256 个 bit 占用 32 个字节，因此
open_fds 的结束地址就是起始地址之后的 32 个字节:

{% highlight c %}
open_fds:
--> Start Address: 0x9dae3e00
--> End   Address: 0x9dae3e20
{% endhighlight %}

同理 close_on_exec 的其实地址就是 0x90dae3e20, 那么其对应的 bitmap 也需要 256
个 bit，共占用 3 个字节，那么地址分布如下:

{% highlight c %}
close_on_exec:
--> Start Address: 0x9dae3e20
--> End   Address: 0x9dae3e40
{% endhighlight %}

最后 full_fds_bits 的一个 bit 可以表示 open_fds 的 BITS_PER_LONG 个 bit，
在 32 系统上，open_fds 的 256 个 bit 需要 full_fds_bits 的 8 个 bit，
由于 full_fds_bits 里面的 bitmap 是按 BITS_PER_LONG 对齐，因此 full_fds_bits
占用 BITS_PER_LONG 个 bit。因此此次分配总共占用了 68 个字节。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000008">\_\_free_fdtable</span>

![](/assets/PDB/RPI/RPI000652.png)

\_\_free_fdtable() 函数的作用是释放一个 struct fdtable。函数包含了一个 
struct fdtable 指针，用于指向即将释放的 struct fdtable. 该函数与之对应的
分配函数是 alloc_fdtable() 函数，分配细节可以通过下面了解:

> - [alloc_fdtable() 函数解析](A0000007)

在 alloc_fdtable() 函数里，函数使用 kvmalloc() 函数为 struct fdtable 的
fd 成员、open_fds 成员、close_on_exec 成员、full_fds_bits 成员分配内存，
但由于 open_fds 成员、close_on_exec 成员、full_fds_bits 成员连续使用一块
从 kvmalloc() 函数分配的内存，因此只需调用 kvfree(fdt->open_fds) 释放
内存就够了，由于 fdt 参数本身通过 kmalloc() 函数分配，因此需要使用 kfree
进行释放。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000009">copy_fdtable</span>

![](/assets/PDB/RPI/RPI000653.png)

copy_fdtable() 函数用于拷贝一个旧的 struct fdtable 到新的 struct fdtable.
参数 nfdt 就是新的 struct fdtable, 参数 ofdt 就是旧的 struct fdtable. 
函数定义了两个整形的局部变量 cpy 和 set 用于控制拷贝过程。拷贝之前可以查
看 struct fdtable 的详细描述:

> - [struct fdtable 结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

函数首先检测新的最大文件打开数是否小于原始的最大文件打开数，如果小于，函数
判定这是一个 BUG，与初衷相悖直接报错。struct fdtable 的 fd 成员是一个
"struct file \*" 数组，struct fdtable 的 max_fds 成员指定了最大文件打开数，
两者相乘可以计算出原始 struct fdtable 的 fd 指针数组占用的字节数。函数接着
函数将新的最大文件打开数减去原始的最大文件打开数，就可以知道在新的 
struct fdtable 的 fd 指针数组中需要初始化的字节数。接下来函数调用 memcpy()
函数将需要拷贝的 fd 指针数组内容从原始的 fd 数组拷贝到了新 struct fdtable
的 fd 指针数组里，并调用 memset() 函数将新的 struct fdtable 的 fd 指针数组
需要清零的部分清零。至此函数完成了 fd 指针数组的拷贝，开发者也可以跟踪这个
过程，修改源码如下:

![](/assets/PDB/RPI/RPI000654.png)

重新编译内核，运行 BiscuitOS，参考下面的工具，并使用调试命令:

> - [BiscuitOS 打开任意个文件工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C2)

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000655.png)

从运行的结果可以看出，struct fdtable 的 max_fds 从 32 扩大到 256，那么
表示需要拷贝的字节数如下 (32bit):

{% highlight c %}
size = 32 * sizeof(struct file *) = 32 * 4 = 128
{% endhighlight %}

在新的 fd 指针数组中需要初始化的字节数计算如下 (32bit):

{% highlight c %}
size = (256 - 32) * sizeof(struct file *) = 224 * 4 = 896
{% endhighlight %}

计算的结果和实际运行的结果一致。接下来函数继续调用 copy_fd_bitmaps()
函数将 struct fdtable 剩下的成员进行拷贝，具体过程请参考:

> - [copy_fd_bitmaps() 函数解析](#A0000010)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000010">copy_fd_bitmaps</span>

![](/assets/PDB/RPI/RPI000656.png)

copy_fd_bitmaps() 函数用于拷贝 struct fdtable 的 bitmap 相关的成员。参数
nfds 指向新的 struct fdtable, ofdt 参数指向原始的 struct fdtable. count
参数代表需要拷贝的 bit 数。

> - [struct fdtable 结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

函数根据 bit 参数，首先计算出需要拷贝的字节数，由于 count 参数已经按 
BITS_PER_LONG 对齐过，因此可以直接使用 count 除以 BITS_PER_BYTE 获得，
以此知道在 bitmap 中需要拷贝的字节数。函数将新的 struct fdtable 的 
max_fds 成员减去 count 之后就可以知道需要初始化新的 struct fdtable 的
open_fds 和 close_one_exec 的 bit 数了，然后将这个数除以 BITS_PER_BYTE 
就可以知道对应初始化的字节数了。有了上面两个数据，直接使用 memcpy() 函数
和 memset() 函数对 open_fds 和 close_on_exec 进行拷贝和初始化操作.
开发者也可以在 BiscuitOS 上跟踪这一过程，修改源码如下:

![](/assets/PDB/RPI/RPI000657.png)

重新编译内核，运行 BiscuitOS，参考下面的工具，并使用调试命令:

> - [BiscuitOS 打开任意个文件工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C2)

{% highlight c %}
number_open_common-0.0.1 -n 30 -d 29 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000658.png)

从上图的运行效果可以看出原始最大文件打开数是 32 个，新的是 256 个，因此
open_fds 和 close_on_exec 需要拷贝的 bit 数是 32 个，在 32bit 系统上，对应
的字节数是 4 个。那么不需要拷贝只需要初始化的个数就是:

{% highlight c %}
size = (256 - 32) / 8 = 28
{% endhighlight %}

因此初始化的个数是 28 个字节，与实际运算的结果一致。

###### 源码 2

![](/assets/PDB/RPI/RPI000659.png)

接下来计算 full_fds_bit 需要拷贝的字节数, 在 struct fdtable 中:

![](/assets/PDB/HK/HK000025.png)

full_fds_bit 的一个 bit 代表 open_fds 中 BITS_PER_LONG 个 bit，因此可以
使用内核提供的 BITBIT_SIZE() 宏计算出 open_fds 的 bit 数在 full_fds_bit
中的个数。函数首先计算了 count 参数在 full_fds_bit 中的字节数，然后再
计算新 struct fdtable 的 max_fds 总共占用了 full_fds_bit 中的字节数，两者
相减就可以计算出需要初始化的字节数，接着就是调用 memcpy() 函数和 memset()
函数进行拷贝和初始化了。支持 struct fdtable 相关的 bitmap 已经全部拷贝完成。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000011">\_\_set_open_fd</span>

![](/assets/PDB/RPI/RPI000664.png)

\_\_set_open_fd() 函数的作用是在 fdt 参数对应的文件描述符表中，将 fd 文件
描述符对应的 open_fds 和 full_fds_bits bitmap 指定的 bit 上置位。参数
fd 指向将要置位的文件描述符，fdt 参数指向文件描述符表。首先了解
struct fdtable 结构和用于实践的信息:

> - [struct fdtable 结构解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)
>
> - [BiscuitOS 打开任意个文件工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C2) 

![](/assets/PDB/HK/HK000025.png)

由 struct fdtable 结构的定义可获得上图的关系，进程将其文件描述符放置在
struct fdtable 里面，其中 open_fds 成员是一个 bitmap，每个 bit 用于管理
一个文件描述符，如果某个文件描述符分配，那么会将其对应的 bit 置位。
full_fds_bits 成员也是一个 bitmap，每个 bit 则表示 open_fds 中的
BITS_PER_LONG 个 bit。因此函数首先根据 fd 参数将 open_fds 对应的 bit 置位，
然后根据 fd 参数计算出其在 full_fds_bits 中对应的位置。在 full_fds_bits
中的每个 bit 如果置位，那么在 open_fds 的 BITS_PER_LONG 个 bit 全部置位。
因此，如果此时 fd 所在的 BITS_PER_LONG 已经全置位，那么也将对应的 bit 置位。
开发者可以在 BiscuitOS 跟踪这个过程，源码修改如下:

![](/assets/PDB/RPI/RPI000665.png)

重新编译内核并运行 BiscuitOS，使用如下测试命令:

{% highlight c %}
number_open_common-0.0.1 -n 28 -d 27 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000666.png)

从上图的运行结果可以看出，系统之前已经打开了 26 个文件，下一个可用的文件
描述符是 29，因此此时 open_fds 的第一个 BITS_PER_LONG 的值为 0x3fffffff,
将文件描述符 30 置位之后，open_fds 的第一个 BITS_PER_LONG 的值为 0x7fffffff,
由于所有 bit 没有置位，因此 full_fds_bits 对应的 bit 不能置位。接下来使用
如下命令:

{% highlight c %}
number_open_common-0.0.1 -n 29 -d 28 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](/assets/PDB/RPI/RPI000667.png)

从上图的运行结果可以看出，当文件描述符是 31 时候，将 open_fds 对应的 bit
置位之后，其 BITs_PER_LONG 全为 1，那么此时 full_fds_bits 对应的位可以置位，
因此 full_fds_bits 的值变为 1.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000012">\_\_set_close_on_exec</span>

![](/assets/PDB/RPI/RPI000668.png)

\_\_set_close_on_exec() 函数的作用是将参数 fd 描述符在参数 fdt 的 
close_on_exec bitmap 中指定的位置上做置位操作。struct fdtable 的 close_on_exec
是一个 bitmap，每个文件描述符在该 bitmap 上都有一个对应的 bit。函数调用
\_\_set_bit() 函数将 bitmap 中对应的位置置位。

> - [struct fdtable 结构体解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)
>
> - [\_\_set_bit() 函数解析](https://biscuitos.github.io/blog/BITMAP___set_bit/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000013">\_\_clear_close_on_exec</span>

![](/assets/PDB/RPI/RPI000669.png)

\_\_clear_close_on_exec() 函数的作用是将参数 fd 描述符在参数 fdt 的
close_on_exec bitmap 中指定的位置上做清零操作。struct fdtable 的 close_on_exec
是一个 bitmap，每个文件描述符在该 bitmap 上都有一个对应的 bit。函数首先
调用 test_bit() 函数查看 fd 对应的 bit 是否已经置位，如果置位就调用
\_\_clear_bit() 函数将 bitmap 中对应的位置清零。

> - [struct fdtable 结构体解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)
>
> - [test_bit() 函数解析](https://biscuitos.github.io/blog/BITMAP_test_bit/)
>
> - [\_\_clear_bit() 函数解析](https://biscuitos.github.io/blog/BITMAP_clear_bit/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000014">get_unused_fd_flags</span>

![](/assets/PDB/RPI/RPI000670.png)

get_unused_fd_flags() 函数的作用是从当前进程分配一个未使用的文件描述符。
正如上图所示，执行一次系统调用，open() 库函数将参数传递到内核系统调用
"SYSCALL_DEFINE3(open, ...)", 该函数即 sys_open(),sys_open() 接着调用
了 do_sys_open() 执行真正的打开操作，do_sys_open() 函数将用户空间传递
的文件打开标志传递给了 get_unused_fd_flags() 函数，其定义如下:

![](/assets/PDB/RPI/RPI000671.png)

参数 flags 是文件打开标志。与打开文件有关的信息全部存储在进程的 files 成员里，
"rlimit(RLIMIT_NOFILE)" 和 0 用于限制查找一个可用文件描述符的范围,
"rlimit(RLIMIT_NOFILE)" 用于限制一个进程最大文件打开数，其定义如下:

![](/assets/PDB/RPI/RPI000691.png)

> - [rlimit() 函数解析](#A0000015)

开发者可以在 BiscuitOS 使用 ulimit 工具读取进程的限制信息，如下:

{% highlight c %}
ulimit -a
ulimit -n
{% endhighlight %}

![](/assets/PDB/RPI/RPI000698.png)

使用 "ulimit -n" 之后可以看出当前进程支持的最大文件打开数是 1024.
函数将这些参数传递给 "\_\_alloc_fd()" 函数，该函数会从进程的文件描述符表中查看
可用的文件描述符，并将查找的结果返回。

> - [\_\_alloc_fd() 函数解析](#A0000003)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000015">rlimit</span>

![](/assets/PDB/RPI/RPI000694.png)

rlimit() 函数用于读取当前进程的某个资源限制。参数 limit 就是需要读取的限制
项。目前支持的限制项目如下:

![](/assets/PDB/RPI/RPI000695.png)

函数通过调用 task_rlimit() 函数可以获得当前进程资源的限制信息。其实现如下:

> - [task_rlimit() 函数解析](#A0000016)

开发者可以在 BiscuitOS 使用 ulimit 工具读取进程的限制信息，如下:

{% highlight c %}
ulimit -a
{% endhighlight %}

![](/assets/PDB/RPI/RPI000698.png)

开发者也可以动态修改某个限制，例如:

{% highlight c %}
ulimit -n
ulimit -n 4096
ulimit -n
{% endhighlight %}

![](/assets/PDB/RPI/RPI000699.png)

从运行结果可以看出进程最大文件打开数已经被动态修改了。更多 ulimit 信息
可以查看:

> - [ulimit 工具详解]()

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000016">task_rlimit</span>

![](/assets/PDB/RPI/RPI000696.png)

task_rlimit() 函数用于读取进程某个限定的资源信息。资源信息通过 struct rlimit
结构定义，如下:

![](/assets/PDB/RPI/RPI000697.png)

从上面的定义可以知道，rlimit 的限制信息包含了当前限制和最大限制两个数据。
当前进程支持的资源信息如下:

![](/assets/PDB/RPI/RPI000695.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

