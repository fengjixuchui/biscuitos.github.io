---
layout: post
title:  "library - libuuid"
date:   2019-08-21 05:30:30 +0800
categories: [HW]
excerpt: library uuid.
tags:
  - library
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [library uuid 简介](#A00)
>
> - [前期准备](#A010)
>
> - [项目配置](#A011)
>
> - [生成 Makefile](#A012)
>
> - [获取源码](#A013)
>
> - [解压并配置源码](#A0132)
>
> - [源码编译](#A014)
>
> - [动态库安装](#A015)
>
> - [生成系统镜像](#A016)
>
> - [动态库使用](#A017)
>
> - [附录](#BBB)

------------------------------------------

##### <span id="A00">library uuid 简介</span>

uuid 含义是通用唯一识别码 (Universally Unique Identifier)，这是一个
软件建构的标准，也是被开源软件基金会 (Open Software Foundation, OSF)
的组织应用在分布式计算环境 (Distributed Computing Environment, DCE)
领域的一部分。

uuid 是指在一台机器上生成的数字，它保证对在同一时空中的所有机器都是唯
一的。通常平台会提供生成的 API。按照开放软件基金会 (OSF) 制定的标准计
算，用到了以太网卡地址、纳秒级时间、芯片ID码和许多可能的数字 uuid 由以
下几部分的组合：

当前日期和时间，uuid 的第一个部分与时间有关，如果你在生成一个 uuid 之后，
过几秒又生成一个 uuid，则第一个部分不同，其余相同, 时钟序列。全局唯一的
IEEE 机器识别号，如果有网卡，从网卡 MAC 地址获得，没有网卡以其他方式获得。
uuid 的唯一缺陷在于生成的结果串会比较长。关于 uuid 这个标准使用最普遍的是
微软的 GUID(Globals Unique Identifiers)。在 ColdFusion 中可以用
CreateUUID() 函数很简单地生成 uuid，其格式为：

{% highlight ruby %}
xxxxxxxx-xxxx- xxxx-xxxxxxxxxxxxxxxx(8-4-4-16)
{% endhighlight %}

其中每个 x 是 0-9 a-f 范围内的一个十六进制的数字。而标准的 uuid 格式为：

{% highlight ruby %}
xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (8-4-4-4-12)
{% endhighlight %}

目前 BiscuitOS 已经支持 library uuid 的移植和实践。开发者
可用通过下面的章节进行 library uuid 的使用。

------------------------------------------

##### <span id="A010">前期准备</span>

动态库移植的核心理念是在主机端使用交叉编译工具，进行交叉编译之后，
将生成的 so 文件即动态库作为目标机的运行库。前期准备包括了一台主机，交叉编译工具，
以及 BiscuitOS。开发者可以在 BiscuitOS linux 5.0 arm32 上进行实践，
因此如果开发者还没有 Linux 5.0 开发环境的，可以参考下面的文档进行搭建：

> [搭建 Linux 5.0 arm32 开发环境](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

-------------------------------------------

#### <span id="A011">项目配置</span>

BiscuitOS 项目中已经包含了默认的配置，开发者可以在 BiscuitOS 中打开
相应的配置，配置完毕后就可以获得对应的文件。因此首先应该基于项目进行
BiscuitOS 配置，步骤如下，首先使用命令启动 Kbuild 配置界面：

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000240.png)

Kbuild 编译系统启用之后如上图，对应应用程序，开发者应该选择
"Package" 并按下回车：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000314.png)

此界面是 Package 支持软件的配置界面，开发者将光标移动到 "libuuid",
按下 "Y" 按键之后再按下回车键，进入 "libuuid" 配置界面。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000315.png)

上图正是 "library uuid" 应用程序的配置界面，"version" 选项代表当前软件的版本。
"tar type" 选项代表应用程序如果是压缩包，则压缩的类型。"url" 选项代表
软件的下载链接。"configure" 代表用户自定义的 configure 标志。
开发者可以使用默认属性，保存并退出，至此，应用程序的配置已经完成。

------------------------------------------------

#### <span id="A012">生成 Makefile</span>

配置完毕之后，开发者接下来编译并生成 Makefile，使用如下命令：

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

编译完毕之后，输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000243.png)

此时会在 BiscuitOS/output/linux-5.0-arm32/package/ 目录下生成一个目录
"libuuid-1.0.3", 进入该目录，可以获得两个文件: Makefile 和 README.md。至此
应用程序的移植前期准备已经结束。

------------------------------------------------

#### <span id="A013">获取源码</span>

进过上面的步骤之后，开发者在 "BiscuitOS/output/linux-5.0-arm32/package/libuuid-1.0.3"
目录下获得移植所需的 Makefile，然后开发者接下来需要做的就是下载源码，
使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/libuuid-1.0.3
make download
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000316.png)

此时在当前目录下会获得一个新的目录 "libuuid-1.0.3"，里面存储着源码相关的文件，
至此源码下载完毕。

------------------------------------------------

#### <span id="A0132">解压并配置源码</span>

在获取源码之后，开发者将获得源码压缩包进行解压并配置源码，由于
library 项目大多使用 automake 进行开发，因此开发者可以使用如下
命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/libuuid-1.0.3
make tar
make configure
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000317.png)

至此源码配置完成。

------------------------------------------------

#### <span id="A014">源码编译</span>

获得源码之后，只需简单的命令就可以编译源码，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/libuuid-1.0.3
make
{% endhighlight %}

编译成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000318.png)

------------------------------------------------

#### <span id="A015">动态库安装</span>

程序编译成功之后，需要将可执行文件安装到 BiscuitOS rootfs 里，
只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/libuuid-1.0.3
make install
{% endhighlight %}

安装成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000319.png)

------------------------------------------------

#### <span id="A016">生成系统镜像</span>

程序安装成功之后，接下来需要将新的软件更新到 BiscuitOS 使用
的镜像里，只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/libuuid-1.0.3
make pack
{% endhighlight %}

打包成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000245.png)

------------------------------------------------

#### <span id="A017">动态库使用</span>

程序安装成功之后，接下来就是在 BiscuitOS 中运行程序，
只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh start
{% endhighlight %}

程序运行成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000320.png)

-----------------------------------------------

# <span id="BBB">附录</span>

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
