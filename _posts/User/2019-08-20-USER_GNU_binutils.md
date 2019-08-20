---
layout: post
title:  "GNU binutils"
date:   2019-08-20 05:30:30 +0800
categories: [HW]
excerpt: GNU binutils.
tags:
  - GNU
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [GNU binutils 简介](#A00)
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
> - [程序安装](#A015)
>
> - [生成系统镜像](#A016)
>
> - [程序运行](#A017)
>
> - [附录](#BBB)

------------------------------------------

##### <span id="A00">GNU binutils 简介</span>

GNU binutils，是 GNU Binary Utilities 的简写，一般简称为 Binutils。
中文可以翻译为 GNU 的二进制工具集。显然，Binutils 是一组二进制工具的集合。
也就是说，Binutils 不是指某一个工具，而是指一组工具，并且这些工具都是专门
针对于二进制的。注意，这里千万不要理解错了，不是说这些 Binutils 工具只提供
二进制文件，而是说这些工具的目的是用于操作二进制文件的，而不是针对于文本或
者源代码。目前 BiscuitOS 已经支持 GNU binutils 的移植和实践。开发者
可用通过下面的章节进行 GNU binutils 的使用。binutils 工具集包含了

{% highlight bash %}
ld    链接器
      将多个目标文件，链接成一个可执行文件（或目标库文件）。
as    汇编器
      将汇编源代码，编译为（目标）机器代码。
addr2line
      将地址转换为（文件名和）行号的工具，一般主要用于反汇编。
ar
      用来操作(.a)档案文件，比如创建，修改，提取内容等
c++filt
      Filter to demangle encoded C++ symbols
dlltool
      Creates files for building and using DLLs
gold
      一个新的，速度更快的，只针对于ELF的链接器（可能还不是很成熟稳定）。
gprof
      Displays profiling information
nlmconv
      Converts object code into an NLM
nm
      列出目标文件中的符号
objcopy
      拷贝并翻译（转换）文件，可用于不同格式的二进制文件的转换。
objdump
      显示目标文件中的信息。
ranlib
      Generates an index to the contents of an archive
readelf
      显示 ELF 格式的（目标）文件的信息。
size
      显示目标文件或(.a)档案文件中的节（section）的大小。
strings
      显示文件中的（可打印）的字符串信息。
strip
      去除符号。一般用来把可执行文件中的一些信息（比如 debug 信息）去除掉，
      以实现在不影响程序功能的前提下，减少可执行文件的大小，减少程序的空间占用。
windmc
      A Windows compatible message compiler
windres
      A compiler for Windows resource files
{% endhighlight %}

------------------------------------------

##### <span id="A010">前期准备</span>

应用程序移植的核心理念是在主机端使用交叉编译工具，进行交叉编译之后，
将生成的目标文件在目标上运行。GNU 项目则采用了 binutils 机制进行源码
的编译，因此再获得源码之后，开发者可以根据自身需求对源码进行配置和定制，
然后交叉编译在 BiscuitOS 上使用这些工具。前期准备包括了一台主机，交叉编译工具，
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

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000268.png)

此界面是 Package 支持软件的配置界面，开发者将光标移动到 "GNU - binutils",
按下 "Y" 按键之后再按下回车键，进入 "GNU - binutils" 配置界面。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000269.png)

上图正是 "GNU binutils" 应用程序的配置界面，"version" 选项代表当前软件的版本。
"tar type" 选项代表应用程序如果是压缩包，则压缩的类型。"url" 选项代表
软件的下载链接。"configure" 代表用户自定义的 configure 标志，
"source code list" 代表需要编译的文件, "LDFLAGS" 代表用户自定义的链接
标志，"CFLAGS" 代表用户自定义的编译标志。开发者可以使用默认属性，保存并退出，
至此，应用程序的配置已经完成。

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
"binutils-2.32", 进入该目录，可以获得两个文件: Makefile 和 README.md。至此
应用程序的移植前期准备已经结束。

------------------------------------------------

#### <span id="A013">获取源码</span>

进过上面的步骤之后，开发者在 "BiscuitOS/output/linux-5.0-arm32/package/binutils-2.32"
目录下获得移植所需的 Makefile，然后开发者接下来需要做的就是下载源码，
使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/binutils-2.32
make download
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000270.png)

此时在当前目录下会获得一个新的目录 "binutils-2.32"，里面存储着源码相关的文件，
至此源码下载完毕。

------------------------------------------------

#### <span id="A0132">解压并配置源码</span>

在获取源码之后，开发者将获得源码压缩包进行解压并配置源码，由于
GNU 项目大多使用 binutils 进行开发，因此开发者可以使用如下
命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/binutils-2.32
make tar
make configure
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000271.png)

至此源码配置完成。

------------------------------------------------

#### <span id="A014">源码编译</span>

获得源码之后，只需简单的命令就可以编译源码，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/binutils-2.32
make
{% endhighlight %}

编译成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000272.png)

------------------------------------------------

#### <span id="A015">程序安装</span>

程序编译成功之后，需要将可执行文件安装到 BiscuitOS rootfs 里，
只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/binutils-2.32
make install
{% endhighlight %}

安装成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000273.png)

------------------------------------------------

#### <span id="A016">生成系统镜像</span>

程序安装成功之后，接下来需要将新的软件更新到 BiscuitOS 使用
的镜像里，只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/binutils-2.32
make pack
{% endhighlight %}

打包成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000245.png)

------------------------------------------------

#### <span id="A017">程序运行</span>

程序安装成功之后，接下来就是在 BiscuitOS 中运行程序，
只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh start
{% endhighlight %}

程序运行成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000274.png)

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
