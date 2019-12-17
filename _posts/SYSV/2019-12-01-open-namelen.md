---
layout: post
title:  "Open 打开路径长度研究"
date:   2019-12-01 09:23:30 +0800
categories: [HW]
excerpt: Open Path length.
tags:
  - [open]
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [原理简介](#A0)
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

## 原理简介

用户空间应用程序通过 open() 函数打开一个文件，打开文件的时候，
应用程序需要提供打开文件的路径信息，本文重点研究文件路径长度
相关的内容，比如文件路径最大长度和特殊长度。

在 Linux 中通过 sys_open() 系统调用，用户程序将路径信息传递
给内核，并在内核的 getname_flags() 函数对文件路径进行处理和
管理起来。具体函数实现可以参考文档:

> - [getname_flags() 函数分析](https://biscuitos.github.io/blog/open/#C00001)

从 sys_open() 的系统实现过程中可以知道，内核支持的最大文件
路径名长度是 PATH_MAX，安全路径名长度小于 EMBEDDED_NAME_MAX。

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
