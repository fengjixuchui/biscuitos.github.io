---
layout: post
title:  "MIT xv6 Usermanual"
date:   2019-06-06 07:24:30 +0800
categories: [Build]
excerpt: xv6 Usermanual.
tags:
  - xv6
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [开发环境搭建](#开发环境搭建)
>
> - [BiscuitOS 编译运行](#BiscuitOS 编译运行)
>
> - [运行 xv6 内核](#运行 xv6 内核)
>
> - [编译 xv6 内核](#编译 xv6 内核)
>
> - [调试 xv6 内核](#xv6 内核调试)
>
> - [附录](#附录)

-----------------------------------------------

# <span id="开发环境搭建">开发环境搭建</span>

> - [准备开发主机](#准备开发主机)
>
> - [安装基础开发工具](#安装基础开发工具)
>
> - [获取源码](#获取源码)

#### <span id="准备开发主机">准备开发主机</span>

开发者首先准备一台 Linux 发行版电脑，推荐 Ubuntu 16.04/Ubuntu 18.04, Ubuntu 电
脑的安装可以上网查找相应的教程。准备好相应的开发主机之后，请参照如下文章进行开
发主机细节配置。

> [BiscuitOS 开发环境搭建](https://biscuitos.github.io/blog/PlatformBuild/)

#### <span id="安装基础开发工具">安装基础开发工具</span>

在编译系统之前，需要对开发主机安装必要的开发工具。以 Ubuntu 为例安装基础的开发
工具。开发者可以按如下命令进行安装：

{% highlight bash %}
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
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


#### <span id="获取源码">获取源码</span>

基础环境搭建完毕之后，开发者从 GitHub 上获取项目源码，使用如下命令：

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git
cd BiscuitOS
{% endhighlight %}

BiscuitOS 项目是一个用于制作精简 linux/xv6 发行版，开发者可以使用这个项目获得各种
版本的 linux/xv6 内核，包括最古老的 linux 0.11, linux 0.97, linux 1.0.1 等等，也可
以获得最新的 linux 4.20, linux 5.0 等等。只需要执行简单的命令，就能构建一个可
运行可调式的 linux/xv6 开发环境。

------------------------------------------------------

# <span id="BiscuitOS 编译运行">BiscuitOS 编译并运行</span>

> - [BiscuitOS 编译](#BiscuitOS 编译)
>
> - [运行 xv6 内核](#运行 xv6 内核)
>
> - [配置 xv6 内核](#配置 xv6 内核)
>
> - [编译 xv6 内核](#编译 xv6 内核)
>

#### <span id="BiscuitOS 编译">BiscuitOS 编译</span>

获得 BiscuitOS 项目之后，可以使用 BiscuitOS 构建 xv6 的开发环境。开发者
只需执行如下命令就可以获得 xv6 完整的 BiscuitOS，如下：

{% highlight bash %}
cd BiscuitOS
make MIT-xv6_rev11_defconfig
make
{% endhighlight %}

执行 make 命令的过程中，BiscuitOS 会从网上获得系统运行所需的工具，包括
binutils, GNU-GCC，ROOTFS 等工具，以此构建一个完整的 Rootfs。编译过程中需要输入
root 密码，请自行输入，但请不要以 root 用户执行 make 命令。编译完成之后，在命令
行终端会输出多条信息，其中包括 xv6 源码的位置，BiscuitOS 的位置，以及 README
位置。如下：

{% highlight perl %}
 ____  _                _ _    ___  ____
| __ )(_)___  ___ _   _(_) |_ / _ \/ ___|
|  _ \| / __|/ __| | | | | __| | | \___ \
| |_) | \__ \ (__| |_| | | |_| |_| |___) |
|____/|_|___/\___|\__,_|_|\__|\___/|____/

*******************************************************************
Kernel Path:
 BiscuitOS/output/xv6-rev11/xv6/xv6/

README:
 BiscuitOS/output/xv6-rev11/README.md

Blog
 https://biscuitos.github.io
*******************************************************************
{% endhighlight %}

开发者首先查看 README 中的内容，README 中介绍了 xv6 等编译方法，按照 README
中的提示命令进行编译。例如 README 内容如下：

{% highlight bash %}
xv6-rev11 Usermanual
----------------------------

# Build Kernel

```
cd BiscuitOS/output/xv6-rev11/xv6/xv6
make
```

# Running Kernel

```
cd BiscuitOS/output/xv6-rev11/xv6/xv6
make qemu
```

Reserved by @BiscuitOS
{% endhighlight %}

#### <span id="运行 xv6 内核">运行 xv6 内核</span>

完成上面的步骤之后，开发者就可以运行 xv6 内核，使用如下命令即可：

{% highlight bash %}
cd BiscuitOS/output/xv6-0.11/xv6/xv6
make qemu
{% endhighlight %}

![LINUXP](/assets/PDB/BiscuitOS/boot/BOOT000130.png)

#### <span id="编译 xv6 内核">编译 xv6 内核</span>

配置完内核之后，接下来就是编译内核，根据 README 里提供的命令进行编译，具体命
令如下：

{% highlight bash %}
cd BiscuitOS/output/xv6-rev11/xv6/xv6
make
{% endhighlight %}

编译输出如下 log，以此编译完成

![LINUXP](/assets/PDB/BiscuitOS/boot/BOOT000129.png)

至此，一个 xv6 内核已经运行，开发者可以根据自己兴趣和需求对内核进行魔改。

-----------------------------------------------------

# <span id="xv6 内核调试">xv6 内核调试</span>

BiscuitOS 的 xv6 内核也支持多种调试，其中比较重要的方式就是 GDB 调试，开发者
可以使用 GDB 对内核进行单步调试，具体步骤如下：

首先使用 QEMU 的调试工具，将内核挂起等待调试，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/xv6-rev11/xv6/xv6
make qemu
{% endhighlight %}

![LINUXP](/assets/PDB/BiscuitOS/boot/BOOT000131.png)


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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
