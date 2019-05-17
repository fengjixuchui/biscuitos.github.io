---
layout: post
title:  "BiscuitOS 网络使用说明"
date:   2019-05-17 05:30:30 +0800
categories: [HW]
excerpt: BiscuitOS 网络使用说明.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000N.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [BiscuitOS 网络使用方法](#BiscuitOS 网络使用方法)
>
> - [附录](#附录)

-----------------------------------

# <span id="BiscuitOS 网络使用方法">BiscuitOS 网络使用方法</span>

最新的 BiscuitOS 发行版上已经支持带网络功能的 Linux，开发者可以参考如下步骤
在 BiscuitOS 使用网络。

###### 获得最新源码

如果还没有安装 BiscuitOS，请参照如下教程进行安装：

> [基于 Linux 5.0 搭建 BiscuitOS 发行版](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

如果已经安装 BiscuitOS 项目，请到源码目录下执行如下命令，获得最新源码：

{% highlight ruby %}
cd BiscuitOS/
git pull
{% endhighlight %}

-------------------------

###### 安装配置文件

获得最新源码之后，编译出最新的系统，这里以 linux 5.0 arm32 为例子讲解。开发者可以
在 BiscuitOS/output/linux-5.0-arm32/package/networking/ 目录下获得三个文件
"bridge.sh"、"qemu-ifup"、"qemu-ifdown"。将 "qemu-ifup"、"qemu-ifdown" 拷贝
到 /etc 目录下。

-------------------------

###### 搭建网桥

为了让 BiscuitOS 能够访问网络，需要基于真实的网卡搭建网桥，请参考下面步骤，
在 BiscuitOS/output/linux-5.0-arm32/package/networking/ 目录下，修改
"bridge.sh" 的参数，例如：

{% highlight ruby %}
PORT=eth0
USER=BiscuitOS
NET_SET=91.10.16
{% endhighlight %}

开发者根据实际修改上面三个参数，第一个参数 PORT 对应你真实的网卡名，可以使用
ifconfig 查看，一般都是 eth0；第二个参数 USER 对应着当前用户名，开发者根据
实际情况，将 USER 参数修改成当前用户名；参数 NET_SET 代表网络 ip 地址的前三段，
开发者可以自定义这段数据。

修改完 "bridge.sh" 脚本之后，开发者接下来就是以 root 权限运行 "bridge.sh" 脚本，
如下：

{% highlight ruby %}
sudo ./bridge.sh up
{% endhighlight %}

成功运行之后，可以使用 ifconfig 命令看到新增加了 br0 和 tap0 两个网络设备。

--------------------------------

###### 启动带网络功能的 BiscuitOS

开发者可以像使用其他 BiscuitOS 一样使用，当需要使用 root 权限运行，使用如下命令：

{% highlight ruby %}
cd BiscuitOS/output/linux-5.0-arm/
sudo ./RunQemuKernel.sh net
{% endhighlight %}

BiscuitOS 就会启动，启动之后，进入 BiscuitOS 系统，使用 ifconfig 命令设置
BiscuitOS 的 eth0 网卡，如下命令：

{% highlight ruby %}
ifocnfig eth0 91.10.16.88 netmask 255.255.0.0
{% endhighlight %}

配置完之后，可以使用 ping 命令测试，例如：

{% highlight ruby %}
ping 91.10.16.4 -c 4
{% endhighlight %}

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000090.png)

###### 关闭网桥

当开发者不需要使用 BiscuitOS 网络功能之后，可以按如下命令停止使用网桥：

{% highlight ruby %}
cd BiscuitOS/output/linux-5.0-arm32/package/networking/
sudo ./bridge.sh down
{% endhighlight %}

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
>
> [搭建高效的 Linux 开发环境](https://biscuitos.github.io/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)
