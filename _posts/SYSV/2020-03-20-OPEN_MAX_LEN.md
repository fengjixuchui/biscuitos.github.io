---
layout: post
title:  "sys_open: 文件名相关的问题合集"
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
> - 问题实践
>
>   - [文件名太长导致文件打开失败](#B250)
>
>   - [文件名为空导致文件打开失败](#B251)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### 问题介绍

{% highlight c %}
#incluhttps://en.wikipedia.org/wiki/Comparison_of_file_systemsde <sys/types.h>
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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="B250"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### 文件名太长导致文件打开失败

> - [问题描述](#B2500)
>
> - [问题实践](#B2501)
>
> - [问题分析与解决](#B2502)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------

#### <span id="B2500">问题描述</span>

当打开一个文件的时候，传入的文件名长度超过一定长度就会导致文件打开失败。
VFS 规定文件名最大长度不要超过 PATH_MAX. 对于这个错误可能由一下几个原因组成:

> - 文件名长度超过了 VFS 规定的最长文件名引起的
>
> - 文件名长度超过了当前挂载文件系统的最长文件名引起的
>
> - 其他原因

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B2501">问题实践</span>

为了让开发者能够实践这个问题，BiscuitOS 提供了相关的实践工具和环境，
开发者可以参考下面文档查看工具的具体使用:

> - [BiscuitOS 打开任意长度文件名工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C1)

部署好环境和工具之后，运行 BiscuitOS 并使用如下测试命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 128 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # ls -l BBBB*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000672.png)

通过上面的命令，命令打开了一个长度为 128 的文件名，通过内核源码定义可以知道
VFS 提供的最大文件名为 PATH_MAX, 此时 128 小于 PATH_MAX, 因此可以正确的创建
文件。

{% highlight c %}
~ # getname_common-0.0.1 -l 255 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # ls -l BBBB*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000673.png)

文件名长度为 255 的时候，文件能正常打开，接着使用如下命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 256 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # ls -l BBBB*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000674.png)

从上图的运行结果可以看出，当文件名是 256 的时候，文件打开失败，可是此时
文件名长度远远比 PATH_MAX 还小。此时是在 EXT4 文件系统上创建文件，或其他
文件系统测试，下面选在 tmpfs 文件系统进行测试:

{% highlight c %}
~ # mount
~ # cd /tmp
~ # getname_common-0.0.1 -l 256 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # ls -l BBBB*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000675.png)

结果 tmpfs 文件系统上也不能创建文件名长度为超过 255 的文件。以上便是复现
这个问题的过程，可以确定两个问题: 1) 文件名长度超过一定的长度之后，文件
就不能正确打开 2) 文件系统之间对最大文件系统名字存在差别。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B2502">问题分析与解决</span>

由于 VFS 规定的最大文件名长度为 PATH_MAX, 但上面讨论的问题并为遇到文件名
长度超过 PATH_MAX 的情况，那么目前把目光放到不同的文件系统支持的最大文件
名长度存在差异，通过查找资料可以得到如下信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006760.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006761.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006762.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006763.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006764.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006765.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI0006766.png)

通过上表可以知道 EXT4 文件系统支持的最大文件名长度是 255 个字节。因此
在 EXT4 文件系统中文件长度超过 255 个字节的打开操作就会报错。以上是因为
具体的文件系统决定的，更多文件系统信息可以参考如下链接:

> - [Comparison of the Filesystem](https://en.wikipedia.org/wiki/Comparison_of_file_systems)

通过这个问题，可以拓展思维研究不同的文件系统代码如何设计实现最大文件名限定
的，也可以研究如果突破特定文件系统对文件名长度的限定. 以上想法有待去实践
验证。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="B251"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### 文件名为空导致文件打开失败

> - [问题描述](#B2510)
>
> - [问题实践](#B2511)
>
> - [问题分析与解决](#B2512)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------

#### <span id="B2510">问题实践</span>

当传递参数给 open 系统调用是，文件名的长度为 0，此时会导致文件打开失败，
那么不能打开空文件名的文件吗?

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------

#### <span id="B2511">问题描述</span>

为了让开发者能够实践这个问题，BiscuitOS 提供了相关的实践工具和环境，
开发者可以参考下面文档查看工具的具体使用:

> - [BiscuitOS 打开任意长度文件名工具](https://biscuitos.github.io/blog/SYSCALL_sys_open/#C1)

部署好环境和工具之后，运行 BiscuitOS 并使用如下测试命令:

{% highlight c %}
~ # getname_common-0.0.1 -l 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
~ # ls -l BBBB*
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000677.png)

从上图的运行结果可以看出，但传递文件名长度为 0 是，文件打开失败，返回
ENOENT, 表示文件或目录不存在。以上便是问题的复现。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------

#### <span id="B2511">问题分析与解决</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000678.png)

如上图，在执行一次 open 系统调用的时候，用户空间的文件名通过 sys_open() 
传递给 do_sys_open 函数，再传递给 getname() 函数，最终在 getname_flags()
函数中进行文件名的处理，这里的处理是 VFS 层的处理，还为下沉到具体的文件系统
上处理。在 getname_flags() 函数中有管文件名长度为 0 的处理如下:

> - [getname_flags() 函数解析]()

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000679.png)

当函数检测到文件名长度为 0 时，函数首先判断 empty 是否有效，如果有效则
将 empty 指针引用设置为 1. 如果在文件名长度为 0 的情况下，如果 flags
中不包含 LOOKUP_EMPTY 标志，那么函数判定这是非法的操作，那么函数进行相应
的处理并返回 -ENOENT, 以此表示文件或路径不存在。跟踪 open 系统调用的
整个调用逻辑，open 系统调用的 flags 中不可能带有 LOOKUP_EMPTY 标志，
该标志只可能是其他函数调用 getname_flags() 函数时赋予的。因此找到
出现问题的地方。开发者可以在 BiscuitOS 上调试这个过程，修改源码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000680.png)

重新编译内核运行 BiscuitOS，并使用如下测试命令:

{% highlight c %}
getname_common-0.0.1 -l 0 -f O_RDWR,O_CREAT -m S_IRUSR,S_IRGRP
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000681.png)

从上图的运行结果可以看出，open 系统调用不能打开文件名长度为 0 的文件。

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
