---
layout: post
title:  "BiscuitOS 网络使用说明"
date:   2019-05-17 05:30:30 +0800
categories: [HW]
excerpt: BiscuitOS 网络使用说明.
tags:
  - Tree
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

## 目录

> - [BiscuitOS 网络使用方法](#A03)
>
> - [telnet](#A00)
>
> - [NFS](#A01)
>
> - [附录](#附录)

-----------------------------------

<span id="A03"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

## BiscuitOS 网络使用方法

最新的 BiscuitOS 发行版上已经支持带网络功能的 Linux，并建立了一个 NAT 网桥，
加着 DNS 域名解析服务，BiscuitOS 可以访问外网，并使用通过的网络工具，例如
telnet, nfs 文件系统等。开发者可以参考如下步骤在 BiscuitOS 使用网络。

##### 获得最新源码

如果还没有安装 BiscuitOS，请参照如下教程进行安装：

> [基于 Linux 5.0 搭建 BiscuitOS 发行版](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

如果已经安装 BiscuitOS 项目，请到源码目录下执行如下命令，获得最新源码：

{% highlight bash %}
cd BiscuitOS/
git pull
{% endhighlight %}

-------------------------

##### 安装必备的工具

在使用 BiscuitOS 的网络功能之前，开发者应该确保主机已经安装必要的
开发工具，可以使用如下命令：

{% highlight bash %}
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y bridge-utils
{% endhighlight %}

--------------------------------

##### 启动带网络功能的 BiscuitOS

开发者可以像使用其他 BiscuitOS 一样使用，当需要使用 root 权限运行，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
sudo ./RunBiscuitOS.sh net
{% endhighlight %}

BiscuitOS 在启动初期会打印网卡和网桥相关的信息，如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000000.png)

BiscuitOS 就会启动，启动之后，进入 BiscuitOS 系统，使用 ifconfig 命令设置
BiscuitOS 的 eth0 网卡：

{% highlight bash %}
ifocnfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000001.png)

接下来为 eth0 配置 IP，为了使 BiscuitOS 能够访问外网，那么在配置 IP
时应遵循一下规则。

{% highlight bash %}
1) eth0 IP 必须与虚拟网桥 IP 在同一网段
2) BiscuitOS 的默认网关必须与虚拟网桥 IP 一致
{% endhighlight %}

开发者可能会困惑虚拟网关的 IP 是多少，以及虚拟网桥在哪里？
开发者可以不在知道虚拟网桥的情况下正确的给 BiscuitOS 配上正确的 IP，
首先在主机端使用 ifconfig 命令查看此时网络状况，如下：

{% highlight bash %}
ifconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000002.png)

例如在我的主机端，存在一个名为 bsBridge0 的网桥，这就是上面
提到的虚拟网桥，虚拟网桥此时的 IP 是 192.88.1.1，因此开发
者在为 BiscuitOS 的 eth0 配置 IP 时，IP 地址应该是
192.88.1.x (x 的值为 1 到 254),。通过上图开发者还可以
看到存在名为 bsTap0 和 tap0 的网卡，这些都是 qemu 为实现
 BiscuitOS 网络功能配置的 TAP 虚拟网卡，有兴趣的童鞋可以
参考 RunBiscuitOS.sh 脚本，了解其配置方法。通过上面的
讨论，接下来就是配置 BiscuitOS eth0 的 IP，例如在上图中，
192.88.1.1, 192.88.1.2, 192.88.1.3, 以及 192.88.1.255
已经被占用，开发者可以选在除他们之外，在 192.88.1 网段的 IP
即可，BiscuitOS 上配置命令如下：

{% highlight bash %}
ifconfig eth0 192.88.1.6
ifconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000003.png)

为了是 BiscuitOS 能够访问外网，需要配置 BiscuitOS 的默认
网关，上面介绍过网关的约束条件为：BiscuitOS 的默认网关必须
与虚拟网桥 IP 一致，因此从上面的分析可以知道，虚拟网桥为主机
端的 bsBridge0，其 IP 为 192.88.1.1，因此在 BiscuitOS
上使用如下命令：

{% highlight bash %}
route add default gw 192.88.1.1
route
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000004.png)

此时，可以在 BiscuitOS 上访问到虚拟网关，主机上的网卡，以及
外网上的 IP，如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000005.png)

至此，BiscuitOS 基本网络功能已经建立，开发者可以
使用网络功能进行所需的开发。

-----------------------------------

<span id="A00"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

## telnet

BiscuitOS 已经默认支持 Telnet 服务，开发者在配置完 BiscuitOS 基础
网络后，在主机端可以使用多个 Telnet 连接到 BiscuitOS。例如在主机端
打开终端，假设此时 BiscuitOS eth0 的 IP 为 192.88.1.6，那么使用如下
命令：

{% highlight bash %}
$ telnet 192.88.1.6
Trying 192.88.1.6...
Connected to 192.88.1.6.
Escape character is '^]'.

BiscuitOS login:
{% endhighlight %}

默认 Telnet 的账号和密码都是 root，登录成功如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000006.png)

-----------------------------------

<span id="A01"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

## NFS

NFS 的存在大大方便了两台网络设备之间数据共享，BiscuitOS 默认
也支持 NFS 文件系统。由于 NFS 需要主机端也建立 NFS 服务，因此
本分成 "主机端 NFS" 和 "BiscuitOS NFS" 介绍。

> - [主机端 NFS 搭建](#B01)
>
> - [BiscuitOS NFS 搭建](#B02)

------------------------------------------

#### <span id="B01">主机端 NFS 搭建</span>

主机端 NFS 搭建以 Ubuntu 18.04 为例进行讲解。首先在制定目录下创建
nfs 目录，例如在 "/home/" 目录下创建名为 nfs 的目录。创建完目录之后，
安装 NFS 服务器工具，使用如下命令:

{% highlight bash %}
$ sudo apt-get install -y nfs-kernel-server
$ sudo apt-get install -y nfs-common
{% endhighlight %}

接着讲 "/home" 目录下 nfs 目录作为主机端 NFS 挂载点，
使用如下命令：

{% highlight bash %}
sudo vi /etc/exports
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000007.png)

如上图，添加相应的配置信息，保存并退出。最后重启 nfs 服务，
使用如下命令:

{% highlight bash %}
sudo /etc/init.d/nfs-kernel-server restart
{% endhighlight %}

至此，主机端 NFS 服务器已经配置完毕。

------------------------------------------

#### <span id="B02">BiscuitOS NFS 搭建</span>

当 BiscuitOS 已经配置完基础的网络后，例如此时 BiscuitOS eth0 的 IP
是 192.88.1.6，主机端使用 "ifconfig" 命令获得 bsTap0 的 IP 为 
192.88.1.2, 那么此时 NFS 的配置命令如下：

{% highlight bash %}
mount -t nfs <主机 IP>:<主机端 nfs 目录绝对地址> <BiscuitOS 端 nfs 目录地址> -o nolock
{% endhighlight %}

例如上面所提到的信息，主机 IP 地址为 192.88.1.2, 主机端 nfs 目录绝对
地址是 "/home/nfs"，BiscuitOS 端 nfs 目录地址是 "/nfs", 因此配置命令
如下：

{% highlight bash %}
mount -t nfs 192.88.1.2:/home/nfs /nfs -o nolock
{% endhighlight %}

BiscuitOS 端 NFS:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000008.png)

主机端 NFS:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/mall/MALL000009.png)

至此 NFS 可以正常使用，开发者可以根据自己需要进行使用。

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

