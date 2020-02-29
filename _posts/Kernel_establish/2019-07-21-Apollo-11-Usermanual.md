---
layout: post
title:  "Apollo-11"
date:   2019-07-20 07:24:30 +0800
categories: [Build]
excerpt: Apollo-11.
tags:
  - Apollo
---

![](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/GIF000202.gif)

> [GitHub: BiscuitOS](https://github.com/BiscuitOS/BiscuitOS)
>
> Email: BuddyZhang1 <Buddy.zhang@aliyun.com>

# 目录

> - [Apollo-11 简介](#A00)
>
> - [Apollo-11 环境搭建](#B00)
>
> - [Apollo-11 源码编译](#C00)
>
> - [Apollo-11 运行使用](#D00)
>
> - [Apollo-11 源码查看](#E00)
>
> - [附录](#附录)

-----------------------------------
# <span id="A00"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/IND00000A.jpg)

## Apollo-11 简介

50 年前的今天，美国宇航员尼尔森·阿姆斯特朗从阿波罗 11 号飞船登月舱
走出，在月球表面留下了人类登月的第一个脚印.阿波罗计划历时 9 年，经历了
十次失败，Apollo-11 号才终于将人类的踪迹带到了月球。在着陆前，躲开了
陨坑和巨石宇航员阿姆斯特朗和奥尔德林在着陆点周围探索了两个多小时，他们
采集了土壤和岩石样本、留下了纪念阿波罗 11 号宇航员的奖章和写着 “我们
为全世界和平而来” 的牌匾。那是真正的 “人类群星闪耀时”！

![Apollo-11](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000212.jpeg)

从缺乏飞行经验到第一次在月球上迈出人类探索的脚步，Apollo-11 记录的是
从 0 到 1 的重大转折。而现在，完成这场宏大登月计划的制导计算机（AGC）
所有源代码，你都可以在 Github 上找到了！ BiscuitOS 也是第一时间让
开发者从源码开始，模拟曾今登月的辉煌时刻。

### 虚拟AGC：重现阿波罗登月制导指挥场景

![Apollo-11](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000221.jpg)

由 NASA 联合 The Instrumentation Laboratory、MIT、剑桥以及
MASS 共同资助的一个项目，为了给阿波罗计划登月任务中使用的机载制导
计算机提供计算机仿真。BiscuitOS 目前以及集成了 AGC 模拟器，如果
对编译阿波罗原始代码感兴趣，不妨去看看。BiscuitOS 一键从源码制作
AGC 模拟所需的要的的工具，从网站可以获取各种版本的原始AGC软件，真
的可以在电脑上重现当年阿波罗登月时的制导系统指挥场景，在自己的电脑
上体验一把登月的快乐。

> - [AGC Apollo-11 模拟器: http://www.ibiblio.org/apollo/](http://www.ibiblio.org/apollo/)
>
> - [GitHub Apollo-11 Source Code](https://github.com/chrislgarry/Apollo-11)
>
> - [GitHub: AGC](https://github.com/virtualagc/virtualagc)

-----------------------------------
# <span id="B00"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/IND00000H.jpg)

## Apollo-11 开发环境搭建

> - [搭建基础开发环境](#B01)
>
> - [获取 BiscuitOS 源码](#B02)

--------------------------------------

#### <span id="B01">搭建基础开发环境</span>

在编译系统之前，需要对开发主机安装必要的开发工具。以 Ubuntu 为例安装基础的开发
工具。开发者可以按如下命令进行安装：

{% highlight bash %}
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev

## Ubuntu 16.04
sudo add-apt-repository ppa:nilarimogard/webupd8
sudo apt-get update
sudo apt-get install libwxgtk2.8-dev
sudo apt-get install libsdl1.2-dev liballegro4-dev
sudo apt-get install libncurses5-dev libgtk2.0-dev

## Ubuntu 18.04
sudo apt-get install libwxgtk3.0-dev libsdl1.2-dev
sudo apt-get install liballegro4-dev libgtk2.0-dev
sudo apt-get install libncurses5-dev
{% endhighlight %}

第一次安装 git 工具需要对 git 进行配置，配置包括用户名和 Email，请参照如下命令
进行配置

{% highlight bash %}
git config --global user.name "Your Name"
git config --global user.email "Your Email"
{% endhighlight %}

----------------------------

#### <span id="B02">获取源码</span>

基础环境搭建完毕之后，开发者从 GitHub 上获取项目源码，使用如下命令：

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git
cd BiscuitOS
{% endhighlight %}

BiscuitOS 项目是一个用于制作精简 linux/xv6 发行版，开发者可以使用这个项目获得各种
版本的 linux/xv6 内核，包括最古老的 linux 0.11, linux 0.97, linux 1.0.1 等等，也可
以获得最新的 linux 4.20, linux 5.0 等等。只需要执行简单的命令，就能构建一个可
运行可调式的 linux/xv6 开发环境。最新版本的 BiscuitOS 已经支持 Apollo-11
项目。

-----------------------------------
# <span id="C00"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/IND00000Q.jpg)

## Apollo-11 源码编译

获得 BiscuitOS 项目之后，可以使用 BiscuitOS 构建 Apollo-11 的开发环境。开发者
只需执行如下命令就可以获得 Apollo-11 完整的 BiscuitOS，如下：

{% highlight bash %}
cd BiscuitOS
make Apollo-11_defconfig
make
{% endhighlight %}

执行 make 命令的过程中，BiscuitOS 会从网上获得系统运行所需的工具，包括
AGC 模拟器源码，以及 Apollo-11 源码等，以此构建一个完整的 Apollo-11 模拟环境。
编译过程中需要输入。编译完成之后，在命令行终端会输出多条信息，其中包括
Apollo-11 源码的位置，BiscuitOS 的位置，以及 README 位置。如下：

{% highlight perl %}
____  _                _ _    ___  ____
| __ )(_)___  ___ _   _(_) |_ / _ \/ ___|
|  _ \| / __|/ __| | | | | __| | | \___ \
| |_) | \__ \ (__| |_| | | |_| |_| |___) |
|____/|_|___/\___|\__,_|_|\__|\___/|____/

                                 Apollo-11

*******************************************************************
Apollo-11 Source Code:
BiscuitOS/output/Apollo-11/Apollo/Apollo

README:
BiscuitOS/output/Apollo-11/README.md

Blog
https://biscuitos.github.io/blog/
*******************************************************************
{% endhighlight %}

开发者首先查看 README 中的内容，README 中介绍了 Apollo-11 等编译方法，按照 README
中的提示命令进行编译。例如 README 内容如下：

{% highlight bash %}
Apollo-11 Usermanual
----------------------------

```
cd BiscuitOS/output/Apollo-11
./RunBiscuitOS.sh launch
```


Reserved by @BiscuitOS
{% endhighlight %}

-----------------------------------
# <span id="D00"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/IND00000K.jpg)

## Apollo-11 运行使用

完成 Apollo-11 的编译之后，开发者就可以运行 Apollo-11，使用如下命令即可：

{% highlight bash %}
cd BiscuitOS/output/Apollo-11
./RunBiscuitOS.sh launch
{% endhighlight %}

![Apollo-11](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000209.png)

Apollo-11 制导系统 (简称 AGC) 分为两部分，一个用于指挥，如上图；
一个用于登录舱，如下图：

![Apollo-11](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000210.png)

官方提供的 AGC 模拟器使用文档如下：

{% highlight bash %}
Running the Validation Suite of the simulated AGC

Having installed the software as above, you can test the emulated
CPU and DSKY using the "validation suite".

    1. Run the VirtualAGC program, select "Validation suite" as the
       simulation type, and hit the "Run" button.
    2. A code of "00" will appear in the PROG area of the DSKY, and
       the OPR ERR lamp will flash.  This means that the validation
       program is ready to start.
    3. Press the PRO key on the DSKY.  The OPR ERR light will go off
       and the validation program will begin.
    4. There is no indication that the test is running.  The test takes
       about 77 seconds.
    5. If all tests are passed, then the PROG area on the DSKY will
       show the code "77", and the OPR ERR lamp will flash.  (The return
       code is 77 because 77 is the largest 2-digit octal number.  It is
       just a coincidence that the test duration is also 77 seconds.)
    6. If some tests fail, an error code other than "00" or "77" will
       be displayed in the PROG area on the DSKY, and the OPR ERR lamp
       will flash.

In the latter case, you can proceed from one test to the next by
pressing the PRO key.  The meanings of the error codes are determined
by reading the file Validation/Validation.agc.
{% endhighlight %}

具体中文步骤如下：

{% highlight bash %}
1. 使用 BiscuitOS 运行了 Apollo-11 之后，在 AGC Simultion Type 栏
   选择 "Validation suite"。然后点击 Run 按钮
2. 运行之后，弹出登录舱界面，此时 "OPR ERR" 按钮不停的闪烁，代表此时 Apollo-11
   已经准备就绪。
3. 点击 PROG 按钮，Apollo-11 进行最后 77 秒自检
4. 自检通过之后，右上角 PROG 显示 77 表示 Apollo-11 自检通过，随时准备发射。
5. 如果不显示 77 代表自检不通过，不能进行发射。
{% endhighlight %}

想了解更多的细节，请参考：

> - [http://www.ibiblio.org/apollo/download.html](http://www.ibiblio.org/apollo/download.html)

-----------------------------------
# <span id="E00"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/IND00000A.jpg)

## Apollo-11 源码查看

AGC 模拟器提供 Apollo-11 汇编源码在线浏览功能，如下图：

![Apollo-11](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000209.png)

首先在 AGC Simulate Type 栏目上选在 Apollo-11，然后在
Browse Source Code 栏点击 AGC 按钮，此时浏览器就会弹出
Apollo-11 使用的汇编，如下图：

![Apollo-11](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000211.png)

如果想了解更多 Apollo-11 源码的开发者，可以参考
`BiscuitOS/output/Apollo-11/Apollo/Apollo`

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/xv6/latest/sou
rce)

## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/HAB000036.jpg)
