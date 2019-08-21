---
layout: post
title:  "GNU - glibc"
date:   2019-08-21 05:30:30 +0800
categories: [HW]
excerpt: GNU glibc.
tags:
  - GNU
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [GNU glibc 简介](#A00)
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

##### <span id="A00">GNU glibc 简介</span>

glibc 是 GNU 发布的 libc 库，即 c 运行库。glibc 是 linux 系统中
最底层的 api，几乎其它任何运行库都会依赖于 glibc。glibc 除了封装
linux 操作系统所提供的系统服务外，它本身也提供了许多其它一些必要功
能服务的实现。由于 glibc 囊括了几乎所有的 UNIX 通行的标准，可以想
见其内容包罗万象。而就像其他的 UNIX 系统一样，其内含的档案群分散于
系统的树状目录结构中，像一个支架一般撑起整个操作系统。在 GNU/Linux
系统中，其 C 函式库发展史点出了 GNU/Linux 演进的几个重要里程碑，用
glibc 作为系统的 C 函式库，是 GNU/Linux 演进的一个重要里程碑.

目前 BiscuitOS 已经支持 GNU glibc 的移植和实践。开发者
可用通过下面的章节进行 GNU glibc 的使用。

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

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000346.png)

此界面是 Package 支持软件的配置界面，开发者将光标移动到 "GNU - glibc",
按下 "Y" 按键之后再按下回车键，进入 "GNU - glibc" 配置界面。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000347.png)

上图正是 "GNU glibc" 应用程序的配置界面，"version" 选项代表当前软件的版本。
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
"glibc-2.24", 进入该目录，可以获得两个文件: Makefile 和 README.md。至此
应用程序的移植前期准备已经结束。

------------------------------------------------

#### <span id="A013">获取源码</span>

进过上面的步骤之后，开发者在 "BiscuitOS/output/linux-5.0-arm32/package/glibc-2.24"
目录下获得移植所需的 Makefile，然后开发者接下来需要做的就是下载源码，
使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/glibc-2.24
make download
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000348.png)

此时在当前目录下会获得一个新的目录 "glibc-2.24"，里面存储着源码相关的文件，
至此源码下载完毕。

------------------------------------------------

#### <span id="A0132">解压并配置源码</span>

在获取源码之后，开发者将获得源码压缩包进行解压并配置源码，由于
GNU 项目大多使用 automake 进行开发，因此开发者可以使用如下
命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/glibc-2.24
make tar
make configure
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000349.png)

至此源码配置完成。

------------------------------------------------

#### <span id="A014">源码编译</span>

获得源码之后，只需简单的命令就可以编译源码，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/glibc-2.24
make
{% endhighlight %}

编译成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000350.png)

------------------------------------------------

#### <span id="A015">动态库安装</span>

程序编译成功之后，需要将可执行文件安装到 BiscuitOS rootfs 里，
只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/glibc-2.24
make install
{% endhighlight %}

安装成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000351.png)

------------------------------------------------

#### <span id="A016">生成系统镜像</span>

程序安装成功之后，接下来需要将新的软件更新到 BiscuitOS 使用
的镜像里，只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/glibc-2.24
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

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000352.png)

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
