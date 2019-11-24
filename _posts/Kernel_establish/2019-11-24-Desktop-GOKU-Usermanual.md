---
layout: post
title:  "BiscuitOS Desktop GOKU Usermanual"
date:   2019-05-06 14:45:30 +0800
categories: [Build]
excerpt: BiscuitOS Desktop GOKU Usermanual.
tags:
  - Linux
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000303.png)

> [GitHub: BiscuitOS](https://github.com/BiscuitOS/BiscuitOS)
>
> Email: BuddyZhang1 <Buddy.zhang@aliyun.com>

## 目录

> - [开发环境部署](#A)
>
>   - [项目简介](#A2)
>
>   - [硬件准备](#A0)
>
>   - [软件准备](#A1)
>
> - [BiscuitOS Desktop 使用](#Y)
>
> - [附录/捐赠](#Z)

------------------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

## 开发环境部署

> - [项目简介](#A2)
>
> - [硬件准备](#A0)
>
> - [软件准备](#A1)

-----------------------------------------------

<span id="A2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

## 项目简介

BiscuitOS 项目是一个用于制作 Linux 0.x、1.x、2.x、3.x、4.x、5.3
通用精简操作系统，其目标是为开发者提供一套纯软件的 Qemu 实践平台
或者硬件 RaspberryPi 实践平台，让开发者能够便捷、简单、快速的
在各个版本上实践 Linux。BiscuitOS Desktop 项目是 BiscuitOS 定制的
开源桌面操作系统，桌面系统为开发者提供了一个可运行，可二次开发的
桌面环境，为桌面应用开发提供了可能。更多 BiscuitOS 信息请范围下列网站:

> - [BiscuitOS 主页](https://biscuitos.github.io/)
>
> - [BiscuitOS 博客](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)

-----------------------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

## 硬件准备

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000046.JPG)

由于项目构建基于 Ubuntu，因此需要准备一台运行
Ubuntu 14.04/16.04/18.04 的主机，主机需要保持网络的连通。

-----------------------------------------------

<span id="A1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## 软件准备

BiscuitOS 制作的系统都是由源码编译而来，这个开发者带来
更多有趣的可能，开发者在使用 BiscuitOS 构建 BiscuitOS Desktop
的系统之前，需要做好如下准备:

> - [基础软件安装](#A10)
>
> - [BiscuitOS 部署](#A11)
>
> - [BiscuitOS-Desktop 镜像下载](#A12)

---------------------------------------------

#### <span id="A10">基础软件安装</span>

开发者首先准备一台 Linux 发行版电脑，推荐 Ubuntu 16.04/Ubuntu 18.04,
Ubuntu 电脑的安装可以上网查找相应的教程。准备好相应的开发主机之后，
接下来是安装运行 BiscuitOS 项目所需的基础开发工具。以 Ubuntu 为例
安装基础的开发工具。开发者可以按如下文档进行安装 (必须):

> - [BiscuitOS 基础开发工具安装指南](https://biscuitos.github.io/blog/Develop_tools)

----------------------------------------

#### <span id="A11">BiscuitOS 部署</span>

基础环境搭建完毕之后，开发者从 GitHub 上获取 BiscuitOS 项目源码，
使用如下命令：

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git --depth=1
cd BiscuitOS
{% endhighlight %}

至此，BiscuitOS 已经部署完毕.
BiscuitOS 目前已经支持 BiscuitOS-Desktop GOKU 的开发部署，开发者在
部署完 BiscuitOS 环境之后，可以参考下面命令进行部署:

{% highlight bash %}
cd BiscuitOS
make BiscuitOS-Desktop-GOKU_defconfig
make
cd BiscuitOS/output/BiscuitOS-Desktop-GOKU
{% endhighlight %}

执行上面的命令之后，BiscuitOS 项目就会自动部署 "linux 5.3 GOKU"
的开发环境，并自动生成各个模块编译规则，开发者请自行参考，例如:

{% highlight perl %}
 ____  _                _ _    ___  ____  
| __ )(_)___  ___ _   _(_) |_ / _ \/ ___| 
|  _ \| / __|/ __| | | | | __| | | \___ \ 
| |_) | \__ \ (__| |_| | | |_| |_| |___) |
|____/|_|___/\___|\__,_|_|\__|\___/|____/ 
                                          
***********************************************
Output:
 BiscuitOS/output/BiscuitOS-Desktop-GOKU 

linux:
 BiscuitOS/output/BiscuitOS-Desktop-GOKU/linux/linux 

README:
 BiscuitOS/output/BiscuitOS-Desktop-GOKU/README.md 

Blog:
 https://biscuitos.github.io/blog/BiscuitOS_Catalogue/ 

***********************************************
{% endhighlight %}

在上面的输出信息中，指出了 "BiscuitOS-Desktop GOKU" 项目
的目录位置.

----------------------------------------

#### <span id="A12">BiscuitOS-Desktop 镜像下载</span>

搭建完 BiscuitOS 开发环境之后，接下来开发这从 BiscuitOS 
官网获取 BiscuitOS-Desktop GOKU 最新镜像。官网镜像位置:

> - [BiscuitOS-Desktop GOKU Download](https://biscuitos.github.io/#projects)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000302.png)

从网上下载最新的 GOKU 镜像，下载完毕之后开发者会获得
BiscuitOS-Desktop-GOKU.tar.gz 压缩包。将压缩包拷贝到
"BiscuitOS/output/" 目录下，使用如下命令进行解压:

{% highlight bash %}
cd BiscuitOS
cp ~/Downloads/BiscuitOS-Desktop-GOKU.tar.gz BiscuitOs/output
cd output
tar -xvf BiscuitOS-Desktop-GOKU.tar.gz
{% endhighlight %}

执行完上面的命令之后，BiscuitOS-Desktop-GOKU 镜像已经安装完毕。

------------------------------------------

<span id="Y"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

## BiscuitOS-Desktop GOKU 使用

在部署好开发环境之后，开发者只需使用如下命令就可以使用
BiscuitOS-Desktop GOKU, 命令如下:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-Desktop-GOKU
./RunBiscuitOS.sh
{% endhighlight %}

如果要使用网络连接外部网络，可以使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-Desktop-GOKU
./RunBiscuitOS.sh net
{% endhighlight %}

运行上面的命令之后，BiscuitOS-Desktop-GOKU 将会运行，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000300.png)

默认登录账号 "biscuitos", 默认密码 "root".

开发者可以将鼠标移动到 BiscuitOS-Desktop-GOKU 里面，并在
BiscuitOS-Desktop-GOKU 里面使用鼠标，如果想从 BiscuitOS-Desktop-GOKU
中回复鼠标，可以使用 "Ctrl+Alt+G"。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000301.png)

BiscuitOS-Desktop-GOKU 给予 Xfce4 构建，开发者可以给予 xfce4
对 BiscuitOS-Desktop-GOKU 进行使用和二次开发，更多
BiscuitOS-Desktop-GOKU 的使用可以参考附录链接.

-----------------------------------------------

# <span id="Z">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Catalogue](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
