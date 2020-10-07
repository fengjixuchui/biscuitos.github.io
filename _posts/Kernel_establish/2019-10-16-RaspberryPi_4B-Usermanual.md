---
layout: post
title:  "BiscuitOS RaspberryPi 4B Usermanual"
date:   2019-05-06 14:45:30 +0800
categories: [Build]
excerpt: RaspberryPi 4B Usermanual.
tags:
  - Linux
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

# 目录

> - [开发环境搭建](#A000)
>
>   - [项目简介](#A003)
>
>   - [硬件准备](#A001)
>
>   - [软件准备](#A002)
>
> - [开发部署](#A010)
>
>   - [RaspberryPi 4B 项目部署](#A016)
>
>   - [内核配置](#A012)
>
>   - [内核编译](#A013)
>
>   - [Rootfs 制作](#A017)
>
>   - [BiscuitOS 安装](#A014)
>
>   - [BiscuitOS 运行](#A015)
>
> - [驱动部署](#A020)
>
>   - [BiscuitOS 驱动开发](#A0201)
>
>   - [通用驱动开发](#A0202)
>
>   - [驱动安装](#A0214)
>
>   - [驱动实践](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/#RaspberryPi)
>
> - [应用程序部署](#F)
>
>   - [BiscuitOS 部署](#F0)
>
>   - [游戏部署](#F2)
>
> - [BiscuitOS 使用](#A060)
>
>   - [升级 RaspberyPi 内核](#A0601)
>
> - [调试](#A040)
>
>   - [内核调试](#A041)
>
>   - [驱动调试](#A042)
>
>   - [应用程序调试](#A043)
>
> - [进阶](#A050)
>
> - [附录/捐赠](#附录)

------------------------------------------

<span id="A000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

## 开发环境搭建

> - [项目简介](#A003)
>
> - [硬件准备](#A001)
>
> - [软件准备](#A002)

-----------------------------------------------

## <span id="A003">项目简介</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000044.png)

树莓派由注册于英国的慈善组织 "Raspberry Pi 基金会" 开发，
Eben·Upton/埃·厄普顿为项目带头人。2012 年 3 月，英国剑桥
大学埃本·阿普顿（Eben Epton）正式发售世界上最小的台式机，
又称卡片式电脑，外形只有信用卡大小，却具有电脑的所有基本
功能，这就是 Raspberry Pi 电脑板，中文译名 "树莓派"。这一
基金会以提升学校计算机科学及相关学科的教育，让计算机变得
有趣为宗旨。基金会期望这 一款电脑无论是在发展中国家还是在
发达国家，会有更多的其它应用不断被开发出来，并应用到更多领
域。在 2006 年树莓派早期概念是基于 Atmel 的 ATmega644 单
片机，首批上市的 10000 "台" 树莓派的 "板子"，由中国台湾和
大陆厂家制造。

RaspberryPi 是一款基于 ARM 的微型电脑主板，以 SD/MicroSD 卡
为内存硬盘，卡片主板周围有 1/2/4 个 USB 接口和一个 10/100 以
太网接口（A 型没有网口），可连接键盘、鼠标和网线，同时拥有视
频模拟信号的电视输出接口和 HDMI 高清视频输出接口，以上部件全
部整合在一张仅比信用卡稍大的主板上，具备所有 PC 的基本功能只
需接通电视机和键盘，就能执行如电子表格、文字处理、玩游戏、播
放高清视频等诸多功能。 Raspberry Pi B 款只提供电脑板，无内存、
电源、键盘、机箱或连线。

树莓派基金会提供了基于 ARM 的 Debian 和 Arch Linux 的发行版供
开发者下载。还计划提供支持 Python 作为主要编程语言，支持 Java、
BBC BASIC (通过 RISC OS 映像或者 Linux 的 "Brandy Basic"克隆)、
C 和Perl等编程语言.

> - [RaspberryPi 官方网站](https://www.raspberrypi.org/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000016.png)

目前 RaspberryPi 最新的 RaspberryPi 4B 亦是一块受开发者喜爱的开源
硬件平台，众多的外设以及高性能的 CPU，以及优秀的社区文化，
RaspberryPi 4B 成为了 BiscuitOS 首推荐的开源实践平台。BiscuitOS 
目前也支持原生的系统支持，开发者可以使用 BiscuitOS 制作一款运行于
RaspberryPi 4B 上的 Linux 发行版，将各种有趣的想法带到了这块开源
硬件平台上。

-----------------------------------------------

<span id="A001"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

## 硬件准备

BiscuitOS 目前支持一个运行运 RaspberryPi 4B 开源平台上的 BiscuitOS，
为了让开发者可以动手制作一个可以在 RaspberryPi 4B 上运行的 BiscuitOS，
开发者应该准备如下资源：

##### 树莓派 4B

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000045.png)

开发者可以从淘宝或京东上购买一款 RaspberryPi 4B 开发板，
开发者可以根据自己的需求购买不同的套餐。但至少包含 SD 卡
一张、电源线一根、TTL 转 USB 串口一根、杜邦线若干、
网线一根、SD 读卡器一个。

##### 开发主机

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000046.JPG)

由于项目构建基于 Ubuntu，因此需要准备一台运行 
Ubuntu 14.04/16.04/18.04 的主机，主机需要保持网络的连通。

##### 路由器

由于后期需要构建基础的开发网络环境，所以开发者需要准备一台
路由器，家用路由器符合要求。


-----------------------------------------------

<span id="A002"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## 软件准备

运行在 RaspberryPi 4B 上的 BiscuitOS 是通过源码编译而来，这
也给内核开发和学习带来了更多的选择性与可玩性。为了让 BiscuitOS
项目正常工作，软件上需要做如下准备：

> - [基础软件安装](#A0020)
>
> - [BiscuitOS 部署](#A0021)

---------------------------------------------

#### <span id="A0020">基础软件安装</span>

开发者首先准备一台 Linux 发行版电脑，推荐 Ubuntu 16.04/Ubuntu 18.04, 
Ubuntu 电脑的安装可以上网查找相应的教程。准备好相应的开发主机之后，
接下来是安装运行 BiscuitOS 项目所需的基础开发工具。以 Ubuntu 为例
安装基础的开发工具。开发者可以按如下命令进行安装：

{% highlight bash %}
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gperf libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python2.7-dev python-dev bridge-utils
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx
sudo apt-get install -y debootstrap bsdtar
{% endhighlight %}

如果开发主机是 64 位系统，请继续安装如下开发工具：

{% highlight bash %}
sudo apt-get install lib32z1 lib32z1-dev
{% endhighlight %}

第一次安装 git 工具需要对 git 进行配置，配置包括用户名和 Email，请参照如下命令
进行配置

{% highlight bash %}
git config --global user.name "Your Name"
git config --global user.email "Your Email"
{% endhighlight %}

----------------------------------------

#### <span id="A0021">BiscuitOS 部署</span>

基础环境搭建完毕之后，开发者从 GitHub 上获取 BiscuitOS 项目源码，
使用如下命令：

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git --depth=1
cd BiscuitOS
{% endhighlight %}

BiscuitOS 项目是一个用于制作精简 Linux 发行版，开发者可以使用这个项目获得各种
版本的 Linux 内核，包括最古老的 Linux 0.11, Linux 0.97, Linux 1.0.1 等等，也可
以获得最新的 Linux 4.20, Linux 5.0 等等。只需要执行简单的命令，就能构建一个可
运行可调式的 Linux 开发环境。

------------------------------------------

<span id="A010"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

## 开发部署

BiscuitOS 项目用于基于 linux、busybox 和 qemu 源码构建一个基础的
Linux 可运行系统，开发者参照下面章节一起实践系统制作的完整过程。

> - [RaspberryPi 4B 项目部署](#A016)
>
> - [内核配置](#A012)
>
> - [内核编译](#A013)
>
> - [Rootfs 制作](#A017)
>
> - [BiscuitOS 安装](#A014)
>
> - [BiscuitOS 运行](#A015)

-------------------------------------------

<span id="A016"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### RaspberryPi 4B 项目部署

获得 BiscuitOS 项目之后，可以使用 BiscuitOS 构建 RaspberryPi 4B 
的开发环境。开发者只需执行如下命令就可以获得 RaspberryPi 4B 完整
的 BiscuitOS，如下：

{% highlight bash %}
cd BiscuitOS
make RaspberryPi_4B_defconfig
make
{% endhighlight %}

执行 make 命令的过程中，BiscuitOS 会从网上获得系统运行所需的工具，包括
binutils, GNU-GCC, linaro-GCC,Busybox 和 Qemu 等工具，以此构建一个完整的
Rootfs。编译过程中需要输入 root 密码，请自行输入，不建议以 root 用户执行
make 命令。编译完成之后，在命令行终端会输出多条信息，其中包括 Linux 源码的位
置，BiscuitOS 的位置，以及 README 位置。如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000048.png)

开发者首先查看 README 中的内容，README 中介绍了 Linux 等编译方法，按照 
README 中的提示命令进行编译。README 使用 Markdown 编写，开发者可以使用
Markdown 工具查看，例如 README 内容如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000047.png)

-------------------------------------------

#### <span id="A012"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000U.jpg)

#### 内核配置

在内核开发中，内核配置是不可或少的阶段，开发者可以根据各自的需求
进行内核配置，以此打开或关闭某些内核功能。由于默认的 BiscuitOS 版
RaspberryPi 4B Linux 内核不需要内核配置，所以开发者可以跳过内核
配置这一节，如果开发者需要内核配置，可以参考下面一个例子。
根据 README 中的提示，在运行内核之前配置 Linux 内核，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B/linux/linux
make ARCH=arm bcm2711_defconfig
make ARCH=arm menuconfig
{% endhighlight %}

在 RaspberryPi 4B 开启 I2C 总线功能，内核配置如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000033.png)

选择并进入 "Device Driver"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000034.png)

选择并进入 "I2C support --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000035.png)

以模块的方式选择 "I2C device interface"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000036.png)

选择并进入 "I2C Hardware Bus support --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000037.png)

以模块的形式选择 "Broadcom BCM2835 I2C controller", 最后保存并退出。

-------------------------------------------

#### <span id="A013"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Y.jpg)

#### 内核编译

配置完内核之后，接下来就是编译内核，根据 README 里提供的命令进行编译，
具体命令如下：

{% highlight bash %}
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/RaspberryPi_4B/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j8
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/RaspberryPi_4B/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/RaspberryPi_4B/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- modules -j4
make ARCH=arm INSTALL_MOD_PATH=BiscuitOS/output/RaspberryPi_4B/rootfs/rootfs/ modules_install
{% endhighlight %}

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000335.png)

-------------------------------------------

<span id="A017"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000R.jpg)

#### Rootfs 制作

为了使 Linux 基础系统能运行，开发者需要为 Linux 准备运行必备的工具，所有的必备
工具在 BiscuitOS 项目执行 make 之后都已经准备好，现在开发者可以选择优化或不优
化这些工具，优化的结果就是是 ROOTFS 的体积尽可能的小。如果开发者不想优化，可以
跳过这一节。使用默认配置源码编译的 BusyBox 体积较大，开发者可以参照如下命令缩
减 BusyBox.

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B/busybox/busybox
make clean
make menuconfig
{% endhighlight %}

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000003.png)

选择 **Busybox Settings --->**

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000004.png)

选择 **Build Options --->**

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000005.png)

选中 **Build BusyBox as a static binary (no shared libs)**，保存并退出。
使用如下命令编译 BusyBox

{% highlight bash %}
make CROSS_COMPILE=BiscuitOS/output/RaspberryPi_4B/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j8

make CROSS_COMPILE=BiscuitOS/output/RaspberryPi_4B/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- install
{% endhighlight %}

编译完上面的工具和 Linux 之后，运行前的最后一步就是制作一个可运行的 
Rootfs。开发者可以使用 README 中提供的命令进行制作，如下：

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B
./RunBiscuitOS.sh pack
{% endhighlight %}

命令执行完成之后，会在 "BiscuitOS/output/RaspberryPi_4B" 目录
下生成名为 BiscuitOS.img 的镜像，支持一个完整的 BiscuitOS 已经
制作完成。

-------------------------------------------

<span id="A014"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

#### BiscuitOS 安装

经过上面的步骤之后，一个可在 RaspberryPi 4B 上运行的 BiscuitOS 已经
制作完成，接下来步骤是让 BiscuitOS 在 RaspberryPi 4B 上运行。开发者
准备好一张 SD 卡和一个 SD 读卡器，然后将 SD 卡放入 SD 读卡器并插入
到主机上，此处开发者应该特别注意，当 SD 读卡器插入主机之后，系统自动
在 /dev/ 目录下生成一个名为 sdx 的节点，具体是 sdb? sdc? sdd 还需
根据开发实际情况而定，但开发者不能将这个 /dev/sdx 设备号弄错，比如
在下面例子中，SD 读卡器插入主机后，主机自动创建 /dev/sdc, 那么接下
来开发者使用如下命令将 BiscuitOS 安装到 SD 卡上：

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B

dd bs=1M if=BiscuitOS.img of=/dev/sdc
sync
{% endhighlight %}

安装完毕之后，再次将 SD 读卡器插入主机，此时 SD 卡分成了两个可用
的分区，第一个分区名为 BOOT，BOOT 分区主要存放着内核的镜像、DTB、
树莓派固件、树莓派配置文件等。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000049.png)

第二个分区名为 "BiscuitOS_rootfs"，这个分区主要存放着 BiscuitOS
的 rootfs。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000050.png)

接下来将 SD 卡插入到树莓派的 SD 卡槽上。

-------------------------------------------

<span id="A015"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### BiscuitOS 运行

完成上面的步骤之后，开发者就可以在 RaspberryPi 上运行 BiscuitOS 了，
但在运行前，开发者使用 TTL 转 USB 串口获得 RaspberryPi 4B 的串口数据，
如下图：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000051.png)

如上图，黑线连接到 TTL 串口的 GND，黄线连接到 TTL 串口的 RX，蓝线
连接到 TTL 串口的 TX 上，然后将 TTL 插入到主机的 USB 口上，此时主机
会在 /dev/ 目录下生成节点 ttyUSB0, 此时使用 minicom 工具连接到串口
上，如果没有安装过 minicom，使用如下步骤进行 minicom 安装配置：

{% highlight bash %}
sudo apt-get install -y minicom
sudo minicom -s
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000052.png)

首先选择 "Serial port setup"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000053.png)

接着按下 "A" 按键，将值设置为 "/dev/ttyUSB0"，按下回车按钮，接着
再按下 "F" 关闭流控，按下回车键之后按下 "Esc" 按键退回上一层。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000054.png)

最后选择 "Save setup as dfl" 选项之后，按下 "Esc" 按键，此时
minicom 配置完成，下次使用 minicom 直接使用命令：

{% highlight bash %}
sudo minicom
{% endhighlight %}

至此一切准备完毕，接下来上电启动，然后使用 minicom 通过串口操作
RaspberryPi 4B，如下 log 为 RaspberryPi 上电之后的 log:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000055.png)

至此, BiscuitOS 已经在 RaspberryPi 4B 上成功运行，开发者可以根据自己
兴趣和需求对内核进行魔改。


------------------------------------------

<span id="A020"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

## 驱动部署

驱动开发是内核一个很重要的部分，BiscuitOS 也支持完整驱动开发，
开发者可以使用 BiscuitOS 自动的驱动编译体系进行开发，也可以
使用通用的驱动开发方法进行开发。

> - [BiscuitOS 驱动开发](#A0201)
>
> - [通用驱动开发](#A0202)
>
> - [驱动安装](#A0214)
>
> - [驱动实践](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/#RaspberryPi)

------------------------------------------

<span id="A0201"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000G.jpg)

## BiscuitOS 驱动开发

BiscuitOS 项目提供了一套完整的驱动开发框架，开发者只需简单的命令，
就可以快速开发或移植驱动到目标板上运行，同时提供了驱动对应的应用
程序和测试工具等，本节以 AT24C08 驱动开发为例：

> - [AT24C08 BiscuitOS-RaspberryPi 实践部署](#X)

------------------------------------------

<span id="A0202"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

## 通用驱动开发

通用驱动开发模式就是按标准的内核驱动开发方式，将驱动放到内核
源码树内或者源码树之外进行开发，这里以内核源码树内开发为例：

> - [驱动源码](#A02010)
>
> - [驱动安置](#A02011)
>
> - [驱动配置](#A02012)
>
> - [驱动编译](#A02013)
>
> - [驱动安装](#A02014)

------------------------------------------

<span id="A02010"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000J.jpg)

## 驱动源码

开发者首先准备一份驱动源码，可以操作如下源码，本节中使用一份 misc 驱动，
并命名为 BiscuitOS_drv.c，具体源码如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000056.PNG)

------------------------------------------

<span id="A02011"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

## 驱动安置

开发者准备好源码之后，接着将源码添加到内核源码树。开发者在内核源码树
目录下，找到 drivers 目录，然后在 drivers 目录下创建一个名为 BiscuitOS 
的目录，然后将源码 BiscuitOS_drv.c 放入到 BiscuitOS 目录下。接着添加 
Kconfig 和 Makefile 文件，具体内容如下：

##### **Kconfig**

{% highlight bash %}
menuconfig BISCUITOS_DRV
    bool "BiscuitOS Driver"

if BISCUITOS_DRV

config BISCUITOS_MISC
    bool "BiscuitOS misc driver"

endif # BISCUITOS_DRV
{% endhighlight %}

##### **Makefile**

{% highlight bash %}
obj-$(CONFIG_BISCUITOS_MISC)  += BiscuitOS_drv.o
{% endhighlight %}

接着修改内核源码树 drivers 目录下的 Kconfig，在文件的适当位置加上如下代码：

{% highlight bash %}
source "drivers/BiscuitOS/Kconfig"
{% endhighlight %}

然后修改内核源码树 drivers 目录下的 Makefile，在文件的末尾上加上如下代码：

{% highlight bash %}
obj-$(CONFIG_BISCUITOS_DRV)  += BiscuitOS/
{% endhighlight %}

------------------------------------------

<span id="A02012"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Z.jpg)

## 驱动配置

准备好所需的文件之后，接下来进行驱动在内核源码树中的配置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B/linux/linux
make ARCH=arm menuconfig
{% endhighlight %}

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000337.png)

首先在目录中找到 **Device Driver --->** 回车并进入其中。

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000338.png)

接着在目录中找到 **BiscuitOS Driver --->** 按 Y 选中并按回车键进入。

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000339.png)

最后按 Y 键选中 **BiscuitOS mis driver**，保存并退出内核配置

------------------------------------------

<span id="A02013"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Z.jpg)

## 驱动编译

配置完驱动之后，接下来将编译驱动。开发者使用如下命令编译驱动:

{% highlight bash %}
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/RaspberryPi_4B/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- modules -j4
make ARCH=arm INSTALL_MOD_PATH=BiscuitOS/output/RaspberryPi_4B/rootfs/rootfs/ modules_install
{% endhighlight %}

从编译的 log 可以看出 BiscuitOS_drv.c 已经被编译进内核。

![LINUXP](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/BUDX000100.jpg)

------------------------------------------

<span id="A0214"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000X.jpg)

## 驱动安装

驱动编译完之后，接下来是将驱动更新到 RaspberryPi 4B 上，驱动更新
的方式可以使用多种方式，请参考如下方式：

> - [NFS 方式驱动安装](#A020140)
>
> - [SD 拷贝驱动安装](#A020141)

------------------------------------------

<span id="A020140"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000C.jpg)

## NFS 方式驱动安装

BiscuitOS 运行在 RaspberryPi 之后，连接网线或者 wifi 之后，BiscuitOS
就可以使用网络，此时可以使用 NFS 的方式对驱动进行安装，如果开发者
还未在 BiscuitOS 搭建 NFS，可以参考下面文章:

> - [BiscuitOS NFS 部署方法](https://biscuitos.github.io/blog/Networking-Usermanual/#A01)

NFS 搭建完毕之后，将 rootfs 中的驱动依赖拷贝到 RaspberryPi 4B 上，
可以参考下列命令, 主机端使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B/rootfs/rootfs
cp -rfa lib/module/ nfs/
{% endhighlight %}

此处 nfs 为主机的 NFS 目录，接着在 RaspberryPi 4B 上使用命令：

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B/rootfs/rootfs
cp -rfa lib/module/ nfs/
{% endhighlight %}

接着在 RaspberryPi 上使用命令:

{% highlight bash %}
mkdir -p nfs_dir
mount -t nfs 192.168.28.90:/nfs nfs_dir -o nolock
cp -rfa nfs_dir/module/ /lib/
cd /lib/modules/5.0.21-v7l+/kernel/drivers/BiscuitOS/
insmod BiscuitOS_drv.ko
{% endhighlight %}

至此驱动安装完毕。

------------------------------------------

<span id="A020141"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000V.jpg)

## SD 拷贝驱动安装

该方法就是通过 SD 卡方式跟新驱动，操作比较简单。将 SD 卡放入 SD
读卡器，然后插入主机，此时可以看到 "BiscuitOS_rootfs" 磁盘，
此时将驱动直接拷贝到 "BiscuitOS_rootfs" 指定目录就行，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000050.png)

开发者可以参考如下命令, 主机端:

{% highlight bash %}
cd BiscuitOS/output/RaspberryPi_4B/rootfs/rootfs
cp -rfa lib/module/ /media/XXXXX/BiscuitOS_rootfs/lib/
sync
{% endhighlight %}

接着将 SD 从主机上移除并插入到 RaspberryPi 上，此时在 
RaspberryPi 上使用命令:

{% highlight bash %}
cd /lib/modules/5.0.21-v7l+/kernel/drivers/BiscuitOS/
insmod BiscuitOS_drv.ko
{% endhighlight %}

------------------------------------------

<span id="F"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

## 应用程序部署

BiscuitOS 目前提供了一套完整的应用程序开发系统，开发者可以
使用这套系统快速实践应用程序，开发者也可以使用传统的方法
在 BiscuitOS 上实践应用程序:

> - [BiscuitOS 部署](#F0)
>
> - [游戏部署](#F1)

----------------------------------

## <span id="F0">BiscuitOS 部署</span>

BiscuitOS 提供了多种动态库、静态块、GNU 工具、BiscuitOS 应用程序，
以及各种开源项目，开发者可以参考下文进行使用:

> - [BiscuitOS 上使用 GNU hello 项目](https://biscuitos.github.io/blog/USER_hello/)
>
> - [BiscuitOS 支持应用列表](https://biscuitos.github.io/blog/APP-Usermanual/)

----------------------------------

## <span id="F1">游戏部署</span>

BiscuitOS 也支持游戏，开发者可以参考如下文章，为自己的
开发之旅带来更多的快乐:

> - [Snake 贪吃蛇](https://biscuitos.github.io/blog/USER_snake/)
>
> - [2048](https://biscuitos.github.io/blog/USER_2048/)
>
> - [tetris 俄罗斯方块](https://biscuitos.github.io/blog/USER_tetris/)

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
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 捐赠支持一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
