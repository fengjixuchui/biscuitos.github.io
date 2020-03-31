---
layout: post
title:  "sys_open: 源码解析"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000">build_open_flags</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000555.png)

build_open_flags 函数用于处理 open 系统调用从用户空间传递下来的 flags
参数和 mode 参数，经过处理合成内核打开文件所需的标志, 标志存在在 
struct open_flags 结构中。(源码较长分段解析)。开发者如果需要在 BiscuitOS 
上实践这段代码请参考下面文档，源码分析过程中数据均依据 BiscuitOS 实践的结
果而定:

> - [BiscuitOS 快速实践 open 系统调用](https://biscuitos.github.io/blog/SYSCALL_sys_open/#B2)

###### 源码 1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000556.png)

参数 flags 是从用户空间传递下来的文件打开标志，mode 参数是 open 系统调用
的 mode 参数，op 参数是一个指向 struct open_flags 的指针。struct open_flags
的结构体定义如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000557.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000559.png)

开发者可以利用 BiscuitOS 提供的 open 工具调试上面的代码，以此查看每个
结果的具体导, 例如在 BiscuitOS 上使用 open 工具传递不同的文件打开标志。

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR -m S_IRUSR,S_IRGRP
~ # open_common-0.0.1 -p BiscuitOS_file -f O_NONBLOCK -m S_IRUSR,S_IRGRP
{% endhighlight %}

###### 源码 2

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000560.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000561.png)

重新编译内核并运行 open 工具，内核打印如下信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000562.png)

在上面的测试命令中，由于 mode 默认的参数中就包含了 S_IRUSR 和 S_IRGRP, 其
值为 0x120, 此时 S_IFREG 的值是 0x8000, 因此从运行的结果来看，只要包含了
O_CREAT 或者 \_\_O_TMPFILE 标志，那么 struct open_flags 的 mode 成员就会
加上 S_IFREG 标志，也就是包含了以上两个标志的，打开的文件必须是普通文件。
然而不包含以上标志的，那么 struct open_flags 的 mode 标志设置为 0. 

回到原函数，函数接着将 FMODE_NONOTIFY 和 O_CLOEXEC 从 flags 参数中移除，
移除的原因是用户空间不应该设置这两个标志。开发者也可以试着在用户空间加上
O_CLOEXEC 标志，看看测试结果，源码修改如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000563.png)

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CLOEXEC -m S_IRUSR,S_IRGRP
{% endhighlight %}

重新编译内核，运行上面的命令，结果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000564.png)

从上图可以看到 O_CLOEXEC 的值是 0x80000, 经过上面的代码处理之后，O_CLOEXEC
已经从 flags 参数中剔除了.

###### 源码 3

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000565.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000566.png)

重新编译内核和运行调试命令, 结果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000567.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000568.png)

重新编译内核并在 BiscuitOS 运行测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

运行结果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000569.png)

从运行结果来看，内核并没有在指定的位置报错，只是从其他位置报错。为了保证
测试不受上一次测试影响，重新启动 BiscuitOS，接着使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000570.png)

从运行结果来看，内核从指定位置报错，因此这样的打开标志是不正确的。为了保证
测试不受上一次测试影响，重新启动 BiscuitOS，接着使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000571.png)

从运行结果来看，内核从指定位置报错，这组打开标志也不正确。为了保证
测试不受上一次测试影响，重新启动 BiscuitOS，接着使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_CREAT,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000572.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000573.png)

重新编译内核，运行 BiscuitOS 并使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_RDWR,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000574.png)

从运行的结果可以看出，虽然没有正确创建文件，但使用上面的命令之后，这段代码
是可以正确执行。如果此时不给 O_RDWR 标志，那么重新启动 BiscuitOS，运行如下
命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f __O_TMPFILE,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000575.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000576.png)

重新编译内核，运行 BiscuitOS 并使用如下命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_PATH,O_RDWR -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000577.png)

从上图的运行情况可以看出，O_RDWR 标志已经从 flags 变量中移除。重启 BiscuitOS，
再使用如下命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_PATH,O_RDWR,O_DIRECTORY -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000578.png)

从上图可以看出，O_RDWR 标志从 flags 变量中移除，但 O_DIRECTORY 标志和 O_PATH
标志都保留在 flags 变量中。

###### 源码 4

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000579.png)

函数将处理完的 flags 变量存储在 struct open_flags 的 open_flags 成员里。
如果此时打开标志中包含了 O_TRUNC 标志，该标志如果文件已经存在且为普通文件，
那么清空文件内容，将其长度置为 0. 在 Linux 下使用此标志，无论以读、写方式
打开文件，都可清空文件 内容 (在这种情况下，都必须拥有对文件的写权限)。

> - [O_TRUNC 标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001B)

如果 O_TRUNC 标志存在，那么也就以为了会对打开的文件执行写操作，因此将 acc_mode
变量增加 MAY_WRITE 权限。在 BiscuitOS 中实践这段代码，添加如下调试信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000582.png)

重新编译内核，运行 BiscuitOS，使用如下测试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT,O_TRUNC -m S_IRUSR
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000581.png)

从上图的运行情况来看，由于起初 open 只传递了 S_IRUSR 的读权限，经过上面
代码处理之后，acc_mode 中增加了 MAY_WRITE 的权限. 

接着分析源码，如果此时打开标志中包含了 O_APPEND 标志，该标志表示
总在文件尾部追加数据。在当下的 Unix/Linux 系统中，这个选项已经被定义为一个
原子操作。

> - [O_APPEND 标志详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A0001C)

如果此时打开标志中包含了 O_APPEND, 那么函数中就让 acc_mode 变量增加
MAY_APPEND 标志。同理在 BiscuitOs 上实践这段代码，在源码做如下修改:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000580.png)

重新编译源码，运行 BiscuitOS，并使用如下调试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT,O_APPEND -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000583.png)

从运行结果可以看出，acc_mode 中添加了 MAY_APPEND 的支持. 回到源码，处理
完上面的代码之后，函数将 acc_mode 变量存储到 struct open_flags 的 acc_mode
成员里。

###### 源码 5

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000584.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000585.png)

重新编译内核源码，运行 BiscuitOS，使用如下调试命令:

{% highlight c %}
~ # open_common-0.0.1 -p BiscuitOS_file -f O_RDWR,O_CREAT,O_EXCL -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000586.png)

从上面的运行结果可以看出，使用上面的标志，struct open_flags 的 intent 成员
已经包含了 LOOKUP_CREATE、LOOKUP_OPEN 和 LOOKUP_EXCL 标志。

###### 源码 6

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000587.png)

函数检查 flags 变量此时是否包含 O_DIRECTORY 即这是与目录有关的操作，那么
函数将 LOOKUP_DIRECTORY 标志加入到 lookup_flags 变量里。接着函数检查
flags 变量是否不包含 O_NOFOLLOW 标志，如果不包含表示打开是一个链接操作，
因此将 LOOKUP_FOLLOW 标志加入到 lookup_flags 变量里。最后函数将 
struct open_flags 的 lookup_flags 成员设置为 lookup_flags 变量，至此
build_open_flags() 函数分析完毕。总结该函数对打开标志和权限标志进行了
各种处理之后，将结果存储在 struct open_flags 结果里面，以供接下来的函数
使用。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000558.png)

MAY 标志的具体含义可以参考如下文档:

> - [MAY 标志解析]()

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000002">gename/getname_flags</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000542.png)

getname()/getname_flags() 函数用于从用户空间拷贝字符串到内核空间作为
文件在内核中的名字 (源码较长分段解析). 开发者如果需要在 BiscuitOS
上实践这段代码请参考下面文档，源码分析过程中数据均依据 BiscuitOS 实践的结
果而定:

> - [BiscuitOS 快速实践 open 系统调用](https://biscuitos.github.io/blog/SYSCALL_sys_open/#B2)

正如上图所示，执行一次系统调用，open() 库函数将参数传递到内核系统调用
"SYSCALL_DEFINE3(open, ...)", 该函数即 sys_open(),sys_open() 接着调用
了 do_sys_open() 执行真正的打开操作，do_sys_open() 函数将用户空间传递
的文件名传递给了 getname() 函数，getname() 函数继续将参数传递给
getname_flags() 函数，getname_flags() 函数就是文件名处理的第一个核心
程序。其定义如下:

###### 源码 1

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

###### 源码 2

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

###### 源码 3

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

在做好打印消息只有，然后分别对 4080、4090、4096、4097 长度的文件名:

{% highlight c %}
~ # getname_common-0.0.1 -l 4080 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000589.png)

从上图运行的情况来看，文件名长度为 4080 能正确拷贝和存储到指定位置。接着
重启 BiscuitOS，使用如下命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 4090 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000590.png)

从上图运行的情况来看，文件名长度为 4090 的时候能正常拷贝和存储到指定位置。
接着重启 BiscuitOS, 然后使用如下命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 4096 -f O_RDWR,O_CREAT -m S_IRUSR,S_IWUSR
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000591.png)

从运行的情况来看，系统可以正常从用户空间拷贝长度为 PATH_MAX 的字符串，
但由于长度等于 PATH_MAX, 那么函数判断名字太长, 函数直接返回 ENAMETOOLONG.

###### 源码 4

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000592.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000003">\_\_alloc_fd</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000593.png)

\_\_alloc_fd() 函数用于从当前进程中分配一个可用文件描述符. (源码较长分段
解析). 开发者如果需要在 BiscuitOS 上实践这段代码请参考下面文档，源码分析
过程中数据均依据 BiscuitOS 实践的结果而定:

> - [BiscuitOS 快速实践 open 系统调用](https://biscuitos.github.io/blog/SYSCALL_sys_open/#B2)

正如上图所示，执行一次系统调用，open() 库函数将参数传递到内核系统调用
"SYSCALL_DEFINE3(open, ...)", 该函数即 sys_open(),sys_open() 接着调用
了 do_sys_open() 执行真正的打开操作，do_sys_open() 函数将用户空间传递
的文件打开标志 flags 传入到 "get_unused_fd_flags()" 函数，函数再调用
"\_\_alloc_fd" 函数，其定义如下:

###### 源码 1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000594.png)

\_\_alloc_fd() 函数的第一个参数一个 struct files_struct 结构指针, 用于指向
当前进程的文件描述结构; 第二个参数是 start，用于指向用于分配文件描述符的起始
值; 第三个参数是 end, 用于指向用于分配文件描述符的终止地址; 第四个参数是
文件打开文件标志。函数定义了一个局部 unsigned int 变量 fd, int 变量 error，
以及 struct fdtable 结构体指针 fdt. 对于上面的参数中，struct files_struct 
的定义如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000595.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000596.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000025.png)

fd 是 struct file 的二级指针，用于指向当前进程所有打开文件的文件结构指针
数组。每个打开的文件都会使用一个 struct file 进行管理，并将这个 struct file
加入到当前进程的 struct files_struct 的 fd_array[] 数组里，然后 fd 就是
指向 struct files_struct 的 fd_arrayp[] 数组。更多 struct fdtable 信息:

> - [struct fdtable 详解](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

###### 源码 2

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000597.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000004">find_next_fd</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000598.png)

函数的作用是在 struct fdtable 结构的 full_fds_bits 和 open_fds 成员中
找到一个可用的文件描述符，struct fdtable 结构的详细分析如下:

> - [struct fdtbale 解析](https://biscuitos.github.io/blog/SYSCALL_sys_open/#A00012D)

由 struct fdtable 的 full_fds_bits 和 open_fds 成员可以得到如下关系:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000025.png)

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


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

#### <span id="A0000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

