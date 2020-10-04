---
layout: post
title:  "BiscuitOS seaBIOS Usermanual"
date:   2019-05-06 14:45:30 +0800
categories: [Build]
excerpt: Linux seaBIOS Usermanual.
tags:
  - BIOS
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [BiscuitOS 基础环境部署](#A)
>
> - [BiscuitOS 内核部署](#C)
>
> - [seaBIOS 部署](#E)
>
> - [seaBIOS 基础使用](#D)
>
> - [seaBIOS 调试部署](#G)
>
> - [附录/捐赠](#Z)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

#### BiscuitOS 基础环境部署

> - [BiscuitOS 项目简介](#A2)
>
> - [BiscuitOS 硬件准备](#A0)
>
> - [BiscuitOS 软件准备](#A1)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="A2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

#### BiscuitOS 项目简介

BiscuitOS 项目是一个用于制作 Linux 0.x、1.x、2.x、3.x、4.x、5.x
通用精简操作系统，其目标是为开发者提供一套纯软件的 Qemu 实践平台
或者硬件 RaspberryPi 实践平台，让开发者能够便捷、简单、快速的
在各个版本上实践 Linux。更多 BiscuitOS 信息请范围下列网站:

> - [BiscuitOS 主页](https://biscuitos.github.io/)
>
> - [BiscuitOS 博客](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

#### BiscuitOS 硬件准备

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000046.JPG)

由于项目构建基于 Ubuntu，因此需要准备一台运行
Ubuntu 14.04/16.04/18.04 的主机，主机需要保持网络的连通。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="A1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### BiscuitOS 软件准备

开发者准好硬件设备之后，首先在 Ubuntu 上安装相应的开发工具，请参考下面文档，
其他 Linux 发行版与之类似:

> - [BiscuitOS 基础开发工具安装指南](https://biscuitos.github.io/blog/Develop_tools)

基础环境搭建完毕之后，开发者从 GitHub 上获取 BiscuitOS 项目源码，使用如下命令:

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git --depth=1
cd BiscuitOS
{% endhighlight %}

BiscuitOS 源码下载完毕后，使用 BiscuitOS 自动部署功能，部署一个 Linux 5.x 
i386 的开发部署，使用自动部署功能之后，BiscuitOS 项目会自动下载并部署 Linux 
5.x i386 所需的源码和编译工具等，可能需要一些时间，请保持网络的通畅. 开发者
参考下面命令进行部署:

{% highlight bash %}
cd BiscuitOS
make seaBIOS_defconfig
make
cd BiscuitOS/output/BiscuitOS-seaBIOS
{% endhighlight %}

成功执行之后，BiscuitOS 输出 Linux 5.x i386 项目目录，以及对应内核的目录，并且
自动生成 BiscuitOS Linux 5.x i386 用户手册 (README)，开发者应该重点参考该 
README.md.

{% highlight bash %}
 ____  _                _ _    ___  ____  
| __ )(_)___  ___ _   _(_) |_ / _ \/ ___|
|  _ \| / __|/ __| | | | | __| | | \___ \ 
| |_) | \__ \ (__| |_| | | |_| |_| |___) |
|____/|_|___/\___|\__,_|_|\__|\___/|____/ 

***********************************************
Output:
 BiscuitOS/output/BiscuitOS-seaBIOS

linux:
 BiscuitOS/output/BiscuitOS-seaBIOS/linux/linux 

README:
 BiscuitOS/output/BiscuitOS-seaBIOS/README.md 

***********************************************
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### BiscuitOS 内核部署

BiscuitOS 项目的目标就是为开发者提供一套便捷实践内核的平台，并且 BiscuitOS 支
持完整的内核开发，开发者请参考下列步骤进行开发:

> - [Linux 内核配置](#C1)
>
> - [Linux 内核编译](#C2)
>
> - [Busybox/Rootfs 制作](#C3)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="C1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

#### Linux 内核配置

对于内核配置，根据 README.md 的提示，配置内核可以参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS/linux/linux
make ARCH=i386 clean
make ARCH=i386 i386_defconfig
make ARCH=i386 menuconfig

  General setup --->
        [*]Initial RAM filesystem and RAM disk (initramfs/initrd) support

  Device Driver --->
        [*] Block devices --->
              <*> RAM block device support
              (153600) Default RAM disk size
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="C2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Y.jpg)

#### Linux 内核编译

内核配置完毕之后，接下来就是编译内核，根据 README.md 里提供的命令进行编译，
具体命令如下:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS/linux/linux
make ARCH=i386 bzImage -j4
make ARCH=i386 modules -j4
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="C3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### Busybox/Rootfs 制作

为了使 BiscuitOS 能够运行，开发者需要为 BiscuitOS 准备运行必备的工具，BiscuitOS
默认使用 busybox 进行系统 rootfs 的构建，开发者可以参考如下命令进行构建:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS/busybox/busybox
make clean
make menuconfig
{% endhighlight %}

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000003.png)

选择 **Busybox Settings --->**

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000004.png)

选择 **Build Options --->**

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000531.png)

选中 **Build BusyBox as a static binary (no shared libs)**，添加
"-m32 -march=i386 -mtune=i386" 到 Additional CFLAGS, 添加 "-m32" 
到 "Additional LDFLAGS", 保存并退出。使用如下命令编译 BusyBox

{% highlight bash %}
make -j4
make install
{% endhighlight %}

编译完上面的工具和 Linux 之后，运行前的最后一步就是制作一个可运
行的 Rootfs。开发者可以使用 README 中提供的命令进行制作，如下:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS
./RunBiscuitOS.sh pack
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="E"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

#### seaBIOS 部署

BiscuitOS 自动部署 seaBIOS，其源码目录位于:

{% highlight bash %}
BiscuitOS/output/BiscuitOS-seaBIOS/bootloader/seaBIOS
{% endhighlight %}

seaBIOS 的编译很简练，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS/bootloader/seaBIOS
make clean
make
{% endhighlight %}

至此 seaBIOS 部署完成.

------------------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### seaBIOS 基础使用

在准备完各个模块之后，开发者完成本节的内容之后，就可以运行一个完整的 
BiscuitOS 系统. BiscuitOS 的运行很简单，通过提供 RunBiscuitOS.sh 脚本将涉及
的启动命令全部整理包含成精简的命令，开发者可以通过查看 RunBiscuitOS.sh 脚本
逻辑来研究 BiscuitOS 运行机理. BiscuitOS 的运行很简单，开发者执行使用如下命
令即可:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000532.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="G"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### seaBIOS 调试部署

环境部署完毕之后，开发者可以直接运行 BiscuitOS，这样即运行最新编译
的 seaBIOS, 由于 seaBIOS 运行速度特别快，开发者如果想单独调试 seaBIOS 或者看到 s
eaBIOS 信息 输出，开发者可以参考在 Linux 开始 boot 阶段就 hook 住内核，这样就可>以便捷调试 seaBIOS。建议开发者在下面的 main.c 文件的 main() 函数中添加 “while(1)” 代码， 让内核 hook 在该位置，这样便于查看 seaBIOS 调试信息:

{% highlight bash %}
BiscuitOS/output/BiscuitOS-seaBIOS/linux/linux/arch/x86/boot/main.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000418.png)

修改完上面的配置之后并编译内核，接下来就是运行带 seaBIOS 的 BiscuitOS 了，开发者使用如下 命令:

{% highlight bash %}
cd BiscuitOS/output/BiscuitOS-seaBIOS
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000417.png)

seaBIOS 的调试方法很多，这里介绍最便捷的方法，即使用 “printf” 函数打印信息， 开发者可以在 seaBIOS 初始化完串口函数之后调用 “printf” 函数输出相关信息，例如 在 maininit() 函数中输出信息:

{% highlight bash %}
vi BiscuitOS/output/BiscuitOS-seaBIOS/bootloader/seaBIOS/src/post.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000419.png)

编译 seaBIOS 并运行 BiscuitOS, 如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000420.png)

从上图看到 seaBIOS 中新增的打印信息在运行正常输出, 开发者可以根据自己情况采用 更
多的调试方法. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Catalogue](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
