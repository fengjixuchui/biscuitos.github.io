---
layout: post
title:  "Native Linux KVM tool (2M Hugepage) on BiscuitOS"
date:   2020-10-24 10:24:00 +0800
categories: [HW]
excerpt: kvmtool.
tags:
  - KVM
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [kvmtool 项目介绍](#A)
>
> - [kvmtool 项目实践](#C)
>
> - [kvmtool 项目调试](#D)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### kvmtool 项目介绍

kvmtool 是一个轻量级的 KVM guest 工具，可以轻量级的实现虚拟机的创建和启动，如果你被 qemu-kvm 的复杂难懂的折磨不已的时候，这绝对是一个不错的工具，对理解 kvm 原理和运行逻辑不错的选择。kvmtool 目前维护在 github 上，更多 kvmtool 信息可以参考:

> [Native Linux KVM tool on github](https://github.com/kvmtool/kvmtool)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000763.png)

BiscuitOS 为 kvmtool 提供了一套完成的开发实践环境，开发者可以利用 BiscuitOS 快速部署 kvmtool 开发环境，并在 BiscuitOS 上使用 kvmtool 启动一个虚拟机。

> [kvmtool 项目实践](#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### kvmtool 项目实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

kvmtool 项目目前只支持 x86_64，本文以 x86_64 架构进行讲解，并推荐使用该架构来构建 kvmtool 项目。首先开发者基于 BiscuitOS 搭建一个 x86_64 架构的开发环境，请开发者参考如下文档:

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

在部署完毕开发环境之后, 由于需要在内核中支持 KVM 模块，因此开发者需要在内核配置中打开以下宏:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/linux/linux
make ARCH=x86_64 menuconfig

  [*] Virtualization  --->
      <*> Kernel-based Virtual Machine (KVM) support
      <*>   KVM for Intel processors support

  [*] Device Drivers  --->
      [*] Virtio drivers  --->
          <*>   PCI driver for virtio devices
          [*]     Support for legacy virtio draft 0.9.X and older devices
      [*] Block devices  --->
          <*>   Virtio block driver

make ARCH=x86_64 bzImage -j4
{% endhighlight %}

重新编译内核，内核编译完毕并重新运行 BiscuitOS，可以在 "/dev/" 目录下看到 kvm 节点:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000587.png)

接下来是安装 kvmtool 源码，开发者可以参考如下命令进行部署:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-x86_64_defconfig
make menuconfig 

  [*] Package  --->
      [*] KVM  --->
          [*] Native Linux KVM tool (2M Hugepage) on BiscuitOS+  --->

make
{% endhighlight %}

配置保存并执行 make，执行完毕之后会在指定目录下部署开发所需的文件，并在该目录下执行如下命令进行源码的部署，请参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-kvmtool-2M-github
make download
make tar
{% endhighlight %}

执行完上面的命令之后，BiscuitOS 会自动部署所需的源码文件，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000767.png)

"BiscuitOS-kvmtool-2M-github" 目录为 qemu-kvm 的源代码，目前采用 github 提供的版本; Makefile 为编译源码相关的脚本; RunBiscuitOS.sh 是在 BiscuitOS 是运行一个虚拟机的相关配置.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

部署完毕之后，接下来进行源码的编译和安装，并在 BiscuitOS 中运行. 参考如下代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-kvmtool-2M-github
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000763.png)

如上图在 BiscuitOS 运行之后，可以查看 kvmtool 使用说明，以此确认软件已经可以使用.确认完毕之后，开发者可以使用两种办法启动一台虚拟机，默认使用 BiscuitOS 提供的脚本，也就是源码目录下的 RunBiscuitOS.sh, 其在 BiscuitOS 中使用如下:

{% highlight bash %}
RunBsicuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000765.png)

脚本运行完毕之后，BiscuitOS 根据 RunBiscuitOS.sh 的工作流启动一个虚拟机，虚拟机运行如上。当想退出虚拟机的话，使用 Ctrl-C 即可. 开发者也可以采用第二种方式启动虚拟机，第二种方式也就是命令行方式，但有一个需要开发者注意的是，命令行必须在 BiscuitOS 的 "/mnt/Freeze/BiscuitOS-kvmtool" 目录下执行，具体命令参考如下:

{% highlight bash %}
mkdir -p /mnt/Freeze/BiscuitOS-kvmtool/hugetlb-2M/
mount none /mnt/Freeze/BiscuitOS-kvmtool/hugetlb-2M/ -t hugetlbfs
echo 100 > /proc/sys/vm/nr_hugepages
cd /mnt/Freeze/BiscuitOS-kvmtool

lkvm run --name BiscuitOS-kvm --cpus 2 --mem 128 --disk BiscuitOS.img --kernel bzImage --params "loglevel=3" --hugetlbfs /mnt/Freeze/BiscuitOS-kvmtool/hugetlb-2M/
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000766.png)

在 BiscuitOS 中当想退出虚拟机的话，使用 Ctrl-c 即可. 如果想退出 BiscuitOS 则使用 Ctrl-a x.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### kvmtool 项目调试

kvmtool 的调试有很多种方法，这里主要介绍通过 LOG 日志的办法。kvmtool 的源码位于:
由于 kvmtool 是一个应用成员，因此可以使用调试应用程序的办法进行调试，最简单的办法就是 kvmtool 源码中使用 printf 进行打印调试.

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
