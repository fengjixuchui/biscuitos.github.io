---
layout: post
title:  "ncurses"
date:   2019-08-21 05:30:30 +0800
categories: [HW]
excerpt: Library ncurses.
tags:
  - Application
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [ncurses 简介](#A00)
>
> - [实践准备](#B00)
>
>   - [硬件准备](#B000)
>
>   - [软件准备](#B001)
>
> - [ncurses 部署](#C00)
>
>   - [BiscuitOS 部署](#C0000)
>
>   - [工程实践部署](#C0001)
>
> - [ncurses 使用](#D00)
>
> - [ncurses 测试](#E00)
>
> - [ncurses 进阶研究](#F0)
>
> - [附录/捐赠](#Donate)

------------------------------------------

<span id="A00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000G.jpg)

## ncurses 简介

在那个广泛使用电传打字机的年代，电传打字机作为中央电脑的输出终端，
通过电缆和中央电脑连接。用户要向终端程序发送一系列特定的控制命令，
才可以控制终端屏幕的输出。比如要在改变光标在屏幕上的位置，清除屏
幕某一区域的内容，卷动屏幕，切换显示模式，给文字添加下划线，改变字
符的外 观、颜色、亮度等等。这些控制都是通过一种叫做转义序
列 (escape sequence) 的字符串实现的。被叫做转义序列是因为这些连
续字节都是以一个 "0x1B" 字符，即转义字符 (按下ESC键所输入的字符)
作为字符串的开头。即使在现在，我们也可以通过向终端仿真程序输入转义
序列得到与当年电传打字终端同样的输出效果。

如果你想在终端 (或者终端仿真程序) 屏幕输出一段背景是彩色的文字，
可以将以下这段转义序列输入到你的命令行提示符：

{% highlight ruby %}
echo "^[[0;31;40mIn Color"
{% endhighlight %}

在这里 "^[" 就是所谓的转义字符。(注意：在这里 "^[" 是一个字符。
不是依次键入 "^" 和 "[" 字符。要打印出这个字符，你必须先按下
Ctrl+V，然后按下ESC键) 执行以上的命令后。你应该可以看 见 "In Color"
的背景变为红色了, 从此以后显示的文本信息都是以这样的方式输出的。
如果想终止这种方式并回到原来的显示方式可以使用以下的命令：

{% highlight ruby %}
echo "^[[0;37;40m"
{% endhighlight %}

为了避免这种不兼容情况，能够在不同的终端上输出统一的结果。UNIX 的
设计者发明了一种叫做 termcap 的机制。termcap 实际上是一个随同转义
序列共同发布的文件。这个文件罗列出当前终端可以正确执行的所有转义序列，
使转义序列的执行结 果符合这个文件中的规定。但是，在这种机制发明后的
几年中，一种叫做 terminfo 的机制逐渐取代 termcap。从此用户在编程时
不用翻阅繁琐的 termcap 中的转义序列规定，仅通过访问 terminfo 的数据
库就可以控制屏幕的输出了。

ncurses 是一个从 System V Release 4.0 (SVr4) 中 CURSES 的克隆。
这是一个可自由配置的库，完全兼容旧版本的 CURSES。简而言之，他是一个
可以使应用程序直接控制终端屏幕显示的库。当后面提到 CURSES 库的时候，
同时也是指代NCURSES库。

ncurses 包由 Pavel Curtis 发起，Zeyd Ben-Halim <<a>zmbenhal@netcom.com>
和 Eric S. Raymond <<a>esr@snark.thyrsus.com> 是最初的维护人员，
他们在 1.8.1 及以后版本中增加了很多的新功能。ncurses 不仅仅只是封装
了底层的终端功能，而且提供了一个相当稳固的工作框架（Framework）用
以产生漂亮的界面。它包含了一些创建窗口的函数。而它的姊妹库 Menu、Panel
和 Form 则是对 CURSES 基础库的扩展。这些库一般都随同 CURSES 一起发行。
我们可以建立一个同时包含多窗口 (multiple windows)、菜单(menus)、
面板 (panels) 和表单 (forms) 的应用程序。窗口可以被独立管理，例如让它
卷动 (scrollability) 或者隐藏。

目前 BiscuitOS 已经支持 library ncurses 的移植和实践。开发者
可用通过下面的章节进行 library ncurses 的使用。

------------------------------------------

<span id="B00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Y.jpg)

## 实践准备

> - [硬件准备](#B000)
>
> - [软件准备](#B001)

------------------------------------------

<span id="B000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000R.jpg)

## 硬件准备

BiscuitOS 对 ncurses 的实践分别提供了纯软件实践平台和硬件实践
平台，如果开发者需要在硬件平台上实践，那么需要准备本节提到的内容。

> - [硬件平台](#B0000)
>
> - [逻辑分析仪](#B0001)
>
> - [示波器](#B0002)

---------------------------------------------------

##### <span id="B0000">硬件平台</span>

开发者需要准备一块 RaspberryPi 4B 开发板，并配有 SD 卡，SD 读卡器，
TTL 转 USB 串口一个。

> - [RaspberryPi 官网](https://www.raspberrypi.org/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000016.png)

------------------------------------------

##### <span id="B0001">逻辑分析仪</span>

逻辑分析仪能够帮助开发者快速分析数据，测试硬件信号功能，稳定性，
大量数据采样等。逻辑分析仪不是必须的，这里推荐使用 DreamSourceLab
开发的 DSLogic:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000012.jpg)

DSLogic 逻辑分析仪数据工具：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000062.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000063.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000066.png)

> - [DreamSourceLab: DSLogic 官网](https://dreamsourcelab.cn/product/dslogic-plus/)

----------------------------------------

##### <span id="B0002">示波器</span>

示波器能够帮助开发者对硬件总线进行最透彻的分析，示波器测量
的数据具有可靠性高，精度高的特定，是分析 I2C 问题不可或缺的
工具。示波器建议准备，这里推荐使用 DreamSourceLab 开发的 DsCope:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000013.jpg)

DSCope 示波器采用样图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000006.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000057.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000069.png)

> - [DreamSourceLab: DSLogic 官网](https://dreamsourcelab.cn/product/dscope-u2p20/)

------------------------------------------

<span id="B001"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## 软件准备

在进行 ncurses 开发之前，开发者应该准备 BiscuitOS 的开发
环境，开发者应该根据不同的需求进行准备，如下:

> - [BiscuitOS Qemu 软件方案开发环境部署](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)
>
> - [BiscuitOS RaspberryPi 硬件方案开发环境部署](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/#RaspberryPi)

------------------------------------------

<span id="C00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

## ncurses 部署

ncurses 可以在 BiscuitOS 上实践，也可以在实际的工程实践
中使用，开发者可以参考下面的目录进行使用:

> - [BiscuitOS 部署](#C0000)
>
> - [工程实践部署](#C0001)

------------------------------------------

<span id="C0000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

## BiscuitOS 部署

BiscuitOS 以及完整支持 ncurses，并基于 Kbuild 编译系统，制作了一套
便捷的 ncurses 开发环境，开发者可以参考如下步骤进行快速开发。

> - [ncurses 源码获取](#C00000)
>
> - [ncurses 源码编译](#C00001)
>
> - [ncurses 应用安装](#C00002)

--------------------------------------------

#### <span id="C00000">ncurses 源码获取</span>

开发者在准备好了 BiscuitOS 开发环境之后，只需按照下面步骤就可以
便捷获得 ncurses 开发所需的源码及环境, 以 RaspberryPi 4B 硬件
开发环境为例，其他 Linux 版本类似，开发者自行进行修改:

{% highlight bash %}
cd BiscuitOS
make RaspberryPi_4B_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000038.png)

选择 "Package --->" 并进入下一级菜单

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPIALL.png)

设置 "ncurses --->" 为 "Y"。设置完毕之后，
保存并退出.

-----------------------------------------------

#### <span id="C00001">ncurses 源码编译</span>

ncurses 的编译很简单，只需执行如下命令就可以快速编
译 (以 RaspberryPi 4B 为例):

{% highlight bash %}
cd BiscuitOS/
make
cd BiscuitOS/output/RaspberryPi_4B/package/ncurses-x.x.x/
make prepare
make download
make
make install
make pack
{% endhighlight %}

---------------------------------------------------

#### <span id="C00002">ncurses 应用安装</span>

开发者由于采用了 QEMU 方案或者硬件 RaspberryPi 方案进行实践，
应用更新采用不同的方式，两种方案的更新方式如下:

开发者如果使用 QEMU 方案，那么只需执行如下命令就能
进行软件更新:

{% highlight bash %}
cd BiscuitOS/
cd BiscuitOS/output/RaspberryPi_4B/package/ncurses-x.x.x/
make install
make pack
{% endhighlight %}

通过上面的命令就可以将软件更新到系统了，接下来就是运行 BiscuitOS.

开发者如果使用是硬件 RaspberryPi 方案，更新的方式多种方式，
可以参考下面方式:

> - [BiscuitOS NFS 方式更新](https://biscuitos.github.io/blog/RaspberryPi_4B-Usermanual/#A020140)
>
> - [BiscuitOS SD 卡方式更新](https://biscuitos.github.io/blog/RaspberryPi_4B-Usermanual/#A020141)

------------------------------------------

<span id="C0001"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000U.jpg)

## 工程实践部署

开发者也可以将 ncurses 部署到实际的工程项目中，开发者
可以根据 BiscuitOS 中部署的方法加入到工程实践中，请参考
如下章节:

> - [BiscuitOS ncurses 部署](#C0000)

接着使用如下命令 (以 RaspberryPi 4B 为例):

{% highlight bash %}
cd BiscuitOS/
cd BiscuitOS/output/RaspberryPi_4B/package/ncurses-x.x.x/
make download
{% endhighlight %}

在该目录下，README 和 Makefile 文档介绍了 ncurses 的
使用方法，以及 Makefile 编译方法，开发者可以参考以上内容
进行工程部署。

------------------------------------------

<span id="D00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

## ncurses 使用

ncurses 安装完毕之后，启动 BiscuitOS，在 BiscuitOS 上
使用 ncurses 如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000108.png)

------------------------------------------

<span id="E00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

## ncurses 测试

等待更新

------------------------------------------

<span id="F00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

## ncurses 进阶研究

等待更新

------------------------------------------

<span id="Donate"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## 附录

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Catalogue](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
