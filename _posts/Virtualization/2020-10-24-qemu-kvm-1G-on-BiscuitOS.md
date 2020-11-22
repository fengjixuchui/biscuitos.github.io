---
layout: post
title:  "QEMU-KVM 1G HugePage on BiscuitOS (QKO-1G)"
date:   2020-10-24 10:24:00 +0800
categories: [HW]
excerpt: QEMU-KVM 1G Running on BiscuitOS.
tags:
  - KVM
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [QKO-1G 项目介绍](#A)
>
> - [QKO-1G 项目实践](#C)
>
> - [QKO-1G 项目调试](#D)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### QKO-1G 项目介绍

QKO-1G 项目全称 "QEMU-KVM on BiscuitOS", 该项目用于在 BiscuitOS 快速为开发者直接创建一套 QEMU-KVM 1G HugePage 开发调试环境。开发者只需专注与 QEMU-KVM 和 KVM 代码的开发与调试，其余部署相关的任务 BiscuitOS 会一站式解决。

KVM 全称 "Kernel-Based Virtual Machine", 是基于内核的虚拟机，它由一个 Linux 内核模块组成，该模块可以将 Linux 变成一个 Hypervisor。KVM 由 Quramnet 公司开发，于 2008 年被 Read Hat 收购。KVM 支持 x86 (32bit and 64bit), s390, PowerPC 等 CPU。KVM 从 Linux 2.6.20 起作为一个模块包含到 Linux 内核中。KVM 支持 CPU 和 内存的虚拟化。

QEMU 是一个主机上的 VMM (Virtual machine monitor), 通过动态二进制模拟 CPU，并提供一系列的硬件模型，使 Guest OS 能够与 Host 硬件交互。在 QEMU-KVM 中，QEMU 负责模拟 IO 设备 (网卡，磁盘等)。KVM 运行在内核空间，QEMU-KVM 则运行在用户空间，并创建、管理各种虚拟硬件。QEMU-KVM 通过 ioctl 调用 /dev/kvm 与 KVM 进行交互，从而将 CPU 指令的部分交给内核模块来做，KVM 则实现了 CPU 和内存虚拟化，但 KVM 不能虚拟其他硬件设备。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000753.png)

本文重点介绍如何在 BiscuitOS 快速部署 QEM-KVM 开发环境。BiscuitOS 提供的开发环境包括了一个静态的 qemu-kvm 可执行文件，并通过 qemu-kvm 工具在 BiscuitOS 上启动一个新的 BiscuitOS, 并且新的 BiscuitOS 的内存采用 1G 的 HugePage。具体请参见:

> [QKO-1G 项目实践](#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### QKO-1G 项目实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

QKO-1G 项目目前只支持 x86_64，本文以 x86_64 架构进行讲解，并推荐使用该架构来构建 QKO-1G 项目。首先开发者基于 BiscuitOS 搭建一个 x86_64 架构的开发环境，请开发者参考如下文档:

> - [BiscuitOS Linux 5.0 X86_64 环境部署](https://biscuitos.github.io/blog/Linux-5.0-x86_64-Usermanual/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0001">实践部署</span>

首先确认 Ubuntu 已经安装 KVM 模块，如果没有安装请在 Ubuntu 上安装相应的模块，使用如下命令:

{% highlight bash %}
sudo apt-get install -y qemu-kvm 
sudo apt-get install -y kvm
sudo modprobe kvm
sudo modprobe kvm-intel
{% endhighlight %}

由于要让 KVM 支持 1 Gig 的映射，那么需要对 BiscuitOS 的内存进行相应的配置，并且支持 1 Gig 的映射需要修改 CMDLINE 的内容，因此开发者接着要修改 RunBiscuitOS.sh 文件中的相关内容，修改如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64
vi RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000746.png)

正如上图所示，在 RunBiscuitOS.sh 脚本中找到 BiscuitOS 内存配置，其通过变量 "RAM_SIZE" 进行配置，另外 BiscuitOS 使用的 CMDLINE 存储在变量 "CMDLINE" 中，为了支持 1Gig 映射实践，所以将 BiscuitOS 的物理内存修改为 4Gig，并且在 CMDLINE 中添加 1Gig HugePage 的设置 "default_hugepagesz", 如下:

{% highlight bash %}
RAM_SIZE=4096
CMDLINE="root=/dev/sda rw rootfstype=${FS_TYPE} console=ttyS0 init=/linuxrc loglevel=8 default_hugepagesz=1G"
{% endhighlight %}

在部署完毕开发环境之后, 由于需要在内核中支持 KVM 模块，因此开发者需要在内核配置中打开以下宏:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/linux/linux
make ARCH=x86_64 menuconfig

  [*] Virtualization  --->
      <*>   Kernel-based Virtual Machine (KVM) support
      <*>     KVM for Intel processors support

make ARCH=x86_64 bzImage -j4
{% endhighlight %}

重新编译内核，内核编译完毕并重新运行 BiscuitOS，可以在 "/dev/" 目录下看到 kvm 节点:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000587.png)

接下来是安装 QEMU-KVM 源码，开发者可以参考如下命令进行部署:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-x86_64_defconfig
make menuconfig 

  [*] Package  --->
      [*] KVM  --->
          [*] QEMU KVM 1G HugePage on BiscuitOS+  --->

make
{% endhighlight %}

配置保存并执行 make，执行完毕之后会在指定目录下部署开发所需的文件，并在该目录下执行如下命令进行源码的部署，请参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-qemu-kvm-1G-4.0.0
make download
make tar
{% endhighlight %}

执行完上面的命令之后，BiscuitOS 会自动部署所需的源码文件，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000754.png)

"BiscuitOS-qemu-kvm-1G-4.0.0" 目录为 qemu-kvm 的源代码，目前采用 4.0.0 版本; Makefile 为编译源码相关的脚本; RunBiscuitOS.sh 是在 BiscuitOS.sh 上运行 qemu-kvm 相关配置.

如果是刚解压的源码，需要对 qemu-kvm 项目进行配置，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-qemu-kvm-1G-4.0.0
make configure
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000755.png)

BiscuitOS 在配置 qemu-kvm 时默认使用的配置如下:

{% highlight bash %}
--target-list=x86_64-softmmu --enable-kvm --enable-virtfs --static --disable-libusb --audio-drv-list=oss --disable-werror
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

部署完毕之后，接下来进行源码的编译和安装，并在 BiscuitOS 中运行. 参考如下代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-qemu-kvm-1G-4.0.0
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000621.png)

如上图在 BiscuitOS 运行之后，可以查看 qemu-kvm 的版本，以此确认软件已经可以使用.确认完毕之后，开发者可以使用两种办法启动一台虚拟机，默认使用 BiscuitOS 提供的脚本，也就是源码目录下的 RunBiscuitOS.sh, 其在 BiscuitOS 中使用如下:

{% highlight bash %}
RunBsicuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000622.png)

脚本运行完毕之后，BiscuitOS 根据 RunBiscuitOS.sh 的工作流启动一个虚拟机，虚拟机运行如上。当想退出虚拟机的话，使用 Ctrl-C 即可. 开发者也可以采用第二种方式启动虚拟机，第二种方式也就是命令行方式，但有一个需要开发者注意的是，命令行必须在 BiscuitOS 的 "/mnt/Freeze/BiscuitOS" 目录下执行，具体命令参考如下:

{% highlight bash %}
#!/bin/ash

mkdir -p /mnt/HugePagefs
mount none /mnt/HugePagefs -t hugetlbfs -o pagesize=1G

echo 2 > /proc/sys/vm/nr_hugepages

cd /mnt/Freeze/BiscuitOS

mkdir -p /var/log/

qemu-kvm -bios bios.bin -cpu host -m 64M -enable-kvm -mem-path /mnt/HugePagefs -mem-prealloc -nographic -kernel bzImage -append "root=/dev/sda rw rootfstype=ext4 console=ttyS0 init=/linuxrc loglevel=3" -hda BiscuitOS.img -serial stdio -nodefaults -D /var/log/BiscuitOS_qemu.log
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000753.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000622.png)

当想退出虚拟机的话，使用 Ctrl-C 即可.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### QKO-1G 项目调试

QEMU-KVM 的调试有很多种方法，这里主要介绍通过 LOG 日志的办法。QEMU-KVM 的源码位于:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-qemu-kvm-1G-4.0.0/BiscuitOS-qemu-kvm-1G-4.0.0
{% endhighlight %}

开发者可以在源码中使用 "qemu_log()" 函数将 log 信息输出到 BiscuitOS 的 "/var/log/BiscuitOS_qemu.log" 里，或者可以直接在 QEMU-KVM 源码中直接使用 "printf()" 函数将 log 信息输出到控制台.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
