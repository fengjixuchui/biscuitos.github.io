---
layout: post
title:  "BiscuitOS 应用程序用户手册"
date:   2019-05-10 14:55:30 +0800
categories: [HW]
excerpt: BiscuitOS Application().
tags:
  - CPUID
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000C.jpg)

> [Github: BBBXXX](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/cpuid/API/BBBXXX)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [移植 C 应用程序](#APP0)
>
> - [移植动态库](#APP1)
>
> - [附录](#附录)

-----------------------------------
<span id="APP0"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000C.jpg)

# 移植 C 应用程序

开发者有时需要 BiscuitOS 上运行 C 应用程序，但 BiscuitOS 主要是为内核开发准备，
但 BiscuitOS 默认支持用户自定义发行版软件定制，busybox 就是一个很好的例子。但说
过来，开发者如何在 BiscuitOS 中添加自己的应用程序呢？本文以一个实例介绍详细的过程，
开发者参照这个过程就能将应用程序加入到 BiscuitOS 中。

本文使用一个贪吃蛇游戏作为例子进行讲解。本实践教程基于 Linux 5.0 arm32, 如果没有搭建
Linux 5.0 arm32 开发环境的童鞋，可以参考下面文档：

> [搭建 Linux 5.0 arm32 开发环境](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

#### <span id="准备源码">准备源码</span>

开发者首先准备源码，例如本例中，贪吃蛇源码位于 GitHub 上，如下：

> [Snake in C GitHub]()

将源码存储到 BiscuitOS/output/linux-5.0-arm32/package/snake/ 目录下，命名为
snake.c, 由于这个 C 文件比较简单，可以直接使用交叉编译工具直接编译。使用如下命令：

{% highlight ruby %}
cd BiscuitOS/output/linux-5.0-arm32/package/
mkdir -p snake
git clone
BiscuitOS/output/linux-5.0-aarch/aarch64-linux-gnu/aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc -lncurses snake.c -o snake
{% endhighlight %}

如果开发者的程序不使用任何多余的动态库，执行上面的命令之后，就会生成 ARM32 上运行
的可执行程序；如果开发者的应用程序依赖了特定的动态库，那么请参考下面内容：

> [snake 依赖 ncurses 库移植方法](#APP1)

#### <span id="安装程序">安装程序</span>

编译完生成可执行文件之后，接下来是将应用程序安装到 rootfs 里面，rootfs 位于
BiscuitOS/output/linux-5.0-arm32/rootfs/rootfs/ 目录下，开发者应该将应用程序
安装到 rootfs 目录下的 usr/bin/ 目录里，如本例子中生成的 snake，如下：

{% highlight ruby %}
cd BiscuitOS/output/linux-5.0-arm32/package/snake/
cp snake BiscuitOS/output/linux-5.0-arm32/rootfs/rootfs/usr/bin
{% endhighlight %}

#### <span id="rootfs 打包">rootfs 打包</span>

开发者将应用程序安装到 rootfs 之后，需要更新 rootfs，这步称为 rootfs 打包，使用如下
命令即可完成：

{% highlight ruby %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunQemuKernel.sh pack
{% endhighlight %}

只要更新了 rootfs 都需要执行上面的命令。

#### <span id="运行程序">运行程序</span>

以上步骤执行完之后，最后一步就是运行应用程序，例如本例中的贪吃蛇，请参考如下步骤：

{% highlight ruby %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunQemuKernel.sh start
{% endhighlight %}

执行上面的命令之后，BiscuitOS 就能正常启动，当进入用户终端之后，由于贪吃蛇的运行
需要设置终端，其他应用程序无需这一步。如下：

{% highlight ruby %}
Freeing unused kernel memory: 1024K
Run /linuxrc as init process
 ____  _                _ _    ___  ____
| __ )(_)___  ___ _   _(_) |_ / _ \/ ___|
|  _ \| / __|/ __| | | | | __| | | \___ \
| |_) | \__ \ (__| |_| | | |_| |_| |___) |
|____/|_|___/\___|\__,_|_|\__|\___/|____/
Welcome to BiscuitOS

Please press Enter to activate this console.
/ #
/ # export TERM=vt102
/ # export TERMINFO=/usr/share/terminfo
/ # clear
/ # s
{% endhighlight %}

snake 运行效果如下：

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000065.png)

-----------------------------------
<span id="APP1"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L.jpg)

# 移植动态库

开发者在移植应用程序的过程中往往会遇到应用程序依赖特定的动态库，但该动态库交叉工具
并未提供，因此针对这种情况，开发者需要交叉编译并移植动态库，可以参照本节完成。例如
本例中 snake 依赖了 ncurses 库。

#### <span id="动态库源码">动态库源码</span>

动态库移植的第一步是获取动态库的源码，大多数库的源码位于

> [http://ftp.gnu.org/pub/gnu/](http://ftp.gnu.org/pub/gnu/)

例如贪吃蛇需要使用 ncurses 的库，可以从该 ftp 服务器上获得 ncurses 对应的源码，
例如本例子使用的 ncurses-6.1.tar.gz, 将下载好的源码放到 BiscuitOS/output/linux-5.0-arm32/package/ncurses 目录下，并解压，使用如下命令：

{% highlight c %}
cd BiscuitOS/output/linux-5.0-arm32/package/
mkdir -p ncurses
mv ~/Download/ncurses-6.1.tar.gz ncurses
cd ncurses
tar xf ncurses-6.1.tar.gz
cd ncurses-6.1
{% endhighlight %}

#### <span id="配置动态库">配置动态库</span>

编译动态库之前，首先配置动态库，在编译之前，需要将交叉工具导入到全局环境变量里，
使用如下命令：

{% highlight c %}
export PATH=$PATH:BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin
{% endhighlight %}

通过执行上面的命令，arm-linux-gnueabi- 交叉编译工具就可以直接使用，接下来就是配置
动态库，使用如下命令：

{% highlight c %}
./configure --prefix=BiscuitOS/output/linux-5.0-arm32/rootfs/rootfs/usr/ --host=arm-linux-gnueabi --with-shared --disable-stripping
{% endhighlight %}

###### --prefix

--prefix 参数用于指定安装目录，这里直接安装到 rootfs 中。

###### --host

--host 参数用于指定交叉编译工具，这里使用 arm-linux-gnueabi

###### --with-shared

--with-shared 参数用于说明编译成动态库

###### --disable-stripping

--disable-stripping 参数用于提示安装的时候。不执行 strip 功能。

成功运行上面的命令之后，动态库源码就已经配置好。

#### <span id="编译动态库">编译动态库</span>

配置好动态库之后，就是编译动态库，命令很简单，如下：

{% highlight c %}
make
{% endhighlight %}

如果没有遇到错误，那么动态库就编译成功。

#### <span id="安装动态库">安装动态库</span>

最后一步就是安装动态库，只需执行简单的命令，动态库就会被安装到指定目录，如下：

{% highlight c %}
make install
{% endhighlight %}

至此，动态库的交叉编译和移植已经完成，接下来就是使用动态库。

#### <span id="使用动态库">使用动态库</span>

动态库的使用分为主机上交叉编译时候使用，以及目标机上商用动态库。


--------------------------------------------------

# <span id="实践">实践</span>

> - [驱动源码](#驱动源码)
>
> - [驱动安装](#驱动安装)
>
> - [驱动配置](#驱动配置)
>
> - [驱动编译](#驱动编译)
>
> - [驱动运行](#驱动运行)
>
> - [驱动分析](#驱动分析)

#### <span id="驱动源码">驱动源码</span>

{% highlight c %}
 2012  /xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc  snake.c -L/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/lib/ -lcurses -I/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/include/ -o a
 2013  /xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc  snake.c -L/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/lib/ -lcurses -I/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/include/ncurses -I/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/include/ -o a
 2014  /xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc  snake.c -L/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/lib/ -lncurses -I/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/include/ncurses -I/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/ncurses/output/include/ -o a
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 cpuid.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_CPUID
+       bool "CPUID"
+
+if BISCUITOS_CPUID
+
+config DEBUG_BISCUITOS_CPUID
+       bool "BBBXXX"
+
+endif # BISCUITOS_CPUID
+
endif # BISCUITOS_DRV
{% endhighlight %}

接着修改 Makefile，请参考如下修改：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Makefile b/drivers/BiscuitOS/Makefile
index 82004c9..9909149 100644
--- a/drivers/BiscuitOS/Makefile
+++ b/drivers/BiscuitOS/Makefile
@@ -1 +1,2 @@
obj-$(CONFIG_BISCUITOS_MISC)     += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_CPUID)  += cpuid.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]CPUID
            [*]BBBXXX()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight ruby %}

{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

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
