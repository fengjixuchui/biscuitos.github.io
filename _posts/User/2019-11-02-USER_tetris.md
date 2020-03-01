---
layout: post
title:  "tetris"
date:   2019-08-21 05:30:30 +0800
categories: [HW]
excerpt: Game tetris.
tags:
  - Application
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [tetris 简介](#A00)
>
> - [实践准备](#B00)
>
>   - [硬件准备](#B000)
>
>   - [软件准备](#B001)
>
> - [tetris 部署](#C00)
>
>   - [BiscuitOS 部署](#C0000)
>
>   - [工程实践部署](#C0001)
>
> - [tetris 使用](#D00)
>
> - [tetris 测试](#E00)
>
> - [tetris 进阶研究](#F0)
>
> - [附录/捐赠](#Donate)

------------------------------------------

<span id="A00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000G.jpg)

## tetris 简介

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000343.png)

顾名思义，俄罗斯方块自然是俄罗斯人发明的。这人叫阿列克谢·帕基特诺
夫（Алексей Пажитнов 英文：Alexey Pazhitnov）。俄罗斯方块原名是
俄语Тетрис（英语是Tetris），这个名字来源于希腊语tetra，意思是“四”，
而游戏的作者最喜欢网球（tennis）。于是，他把两个词tetra和tennis合
而为一，命名为 Tetris，这也就是俄罗斯方块名字的由来。

俄罗斯方块由小方块组成的不同形状的板块陆续从屏幕上方落下来，玩家通过
调整板块的位置和方向，使它们在屏幕底部拼出完整的一条或几条。这些完整
的横条会随即消失，给新落下来的板块腾出空间，与此同时，玩家得到分数奖
励。没有被消除掉的方块不断堆积起来，一旦堆到屏幕顶端，玩家便告输，游
戏结束。目前 BiscuitOS 已经支持 Game tetris 的移植和实践。开发者
可用通过下面的章节进行 Game tetris 的使用。

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

BiscuitOS 对 tetris 的实践分别提供了纯软件实践平台和硬件实践
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

在进行 tetris 开发之前，开发者应该准备 BiscuitOS 的开发
环境，开发者应该根据不同的需求进行准备，如下:

> - [BiscuitOS Qemu 软件方案开发环境部署](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)
>
> - [BiscuitOS RaspberryPi 硬件方案开发环境部署](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/#RaspberryPi)

------------------------------------------

<span id="C00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

## tetris 部署

tetris 可以在 BiscuitOS 上实践，也可以在实际的工程实践
中使用，开发者可以参考下面的目录进行使用:

> - [BiscuitOS 部署](#C0000)
>
> - [工程实践部署](#C0001)

------------------------------------------

<span id="C0000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

## BiscuitOS 部署

BiscuitOS 以及完整支持 tetris，并基于 Kbuild 编译系统，制作了一套
便捷的 tetris 开发环境，开发者可以参考如下步骤进行快速开发。

> - [tetris 源码获取](#C00000)
>
> - [tetris 源码编译](#C00001)
>
> - [tetris 应用安装](#C00002)

--------------------------------------------

#### <span id="C00000">tetris 源码获取</span>

开发者在准备好了 BiscuitOS 开发环境之后，只需按照下面步骤就可以
便捷获得 tetris 开发所需的源码及环境, 以 RaspberryPi 4B 硬件
开发环境为例，其他 Linux 版本类似，开发者自行进行修改:

{% highlight bash %}
cd BiscuitOS
make RaspberryPi_4B_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000038.png)

选择 "Package --->" 并进入下一级菜单

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPIALL.png)

设置 "tetris --->" 为 "Y"。设置完毕之后，
保存并退出.

-----------------------------------------------

#### <span id="C00001">tetris 源码编译</span>

tetris 的编译很简单，只需执行如下命令就可以快速编
译 (以 RaspberryPi 4B 为例):

{% highlight bash %}
cd BiscuitOS/
make
cd BiscuitOS/output/RaspberryPi_4B/package/tetris-x.x.x/
make prepare
make download
make
make install
make pack
{% endhighlight %}

---------------------------------------------------

#### <span id="C00002">tetris 应用安装</span>

开发者由于采用了 QEMU 方案或者硬件 RaspberryPi 方案进行实践，
应用更新采用不同的方式，两种方案的更新方式如下:

开发者如果使用 QEMU 方案，那么只需执行如下命令就能
进行软件更新:

{% highlight bash %}
cd BiscuitOS/
cd BiscuitOS/output/RaspberryPi_4B/package/tetris-x.x.x/
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

开发者也可以将 tetris 部署到实际的工程项目中，开发者
可以根据 BiscuitOS 中部署的方法加入到工程实践中，请参考
如下章节:

> - [BiscuitOS tetris 部署](#C0000)

接着使用如下命令 (以 RaspberryPi 4B 为例):

{% highlight bash %}
cd BiscuitOS/
cd BiscuitOS/output/RaspberryPi_4B/package/tetris-x.x.x/
make download
{% endhighlight %}

在该目录下，README 和 Makefile 文档介绍了 tetris 的
使用方法，以及 Makefile 编译方法，开发者可以参考以上内容
进行工程部署。

------------------------------------------

<span id="D00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

## tetris 使用

tetris 安装完毕之后，启动 BiscuitOS，在 BiscuitOS 上
当进入到 BiscuitOS shell 中端之后，开发者输入下面的配置俄罗斯方块的
运行环境，如下：

{% highlight bash %}
export TERM=vt102
export TERMINFO=/usr/share/terminfo
clear
tetris-0.0.1
{% endhighlight %}

使用 tetris 如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000342.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000344.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000345.png)

------------------------------------------

<span id="E00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

## tetris 测试

等待更新

------------------------------------------

<span id="F00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

## tetris 进阶研究

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
