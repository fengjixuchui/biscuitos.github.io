---
layout: post
title:  "library - ncurses"
date:   2019-08-21 05:30:30 +0800
categories: [HW]
excerpt: library ncurses.
tags:
  - library
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [library ncurses 简介](#A00)
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

##### <span id="A00">library ncurses 简介</span>

在那个广泛使用电传打字机的年代，电传打字机作为中央电脑的输出终端，
通过电缆和中央电脑连接。用户要向终端程序发送一系列特定的控制命令，
才可以控制终端屏幕的输出。比如要在改变光标在屏幕上的位置，清除屏
幕某一区域的内容，卷动屏幕，切换显示模式，给文字添加下划线，改变字
符的外 观、颜色、亮度等等。这些控制都是通过一种叫做转义序
列 (escape sequence) 的字符串实现的。被叫做转义序列是因为这些连
续字节都是以一个 "0x1B" 字符，即转义字符 (按下ESC键所输入的字符)
作为字符串的开头。即使在现在，我们也可以通过向终端仿真程序输入转义
序列得到与当年电传打字终端同样的输出效果。

如果你想在终端 (或者终端仿真程序) 屏幕输出一段背景是彩色的文字，
可以将以下这段转义序列输入到你的命令行提示符：

{% highlight ruby %}
echo "^[[0;31;40mIn Color"
{% endhighlight %}

在这里 "^[" 就是所谓的转义字符。(注意：在这里 "^[" 是一个字符。
不是依次键入 "^" 和 "[" 字符。要打印出这个字符，你必须先按下
Ctrl+V，然后按下ESC键) 执行以上的命令后。你应该可以看 见 "In Color"
的背景变为红色了, 从此以后显示的文本信息都是以这样的方式输出的。
如果想终止这种方式并回到原来的显示方式可以使用以下的命令：

{% highlight ruby %}
echo "^[[0;37;40m"
{% endhighlight %}

为了避免这种不兼容情况，能够在不同的终端上输出统一的结果。UNIX 的
设计者发明了一种叫做 termcap 的机制。termcap 实际上是一个随同转义
序列共同发布的文件。这个文件罗列出当前终端可以正确执行的所有转义序列，
使转义序列的执行结 果符合这个文件中的规定。但是，在这种机制发明后的
几年中，一种叫做 terminfo 的机制逐渐取代 termcap。从此用户在编程时
不用翻阅繁琐的 termcap 中的转义序列规定，仅通过访问 terminfo 的数据
库就可以控制屏幕的输出了。

ncurses 是一个从 System V Release 4.0 (SVr4) 中 CURSES 的克隆。
这是一个可自由配置的库，完全兼容旧版本的 CURSES。简而言之，他是一个
可以使应用程序直接控制终端屏幕显示的库。当后面提到 CURSES 库的时候，
同时也是指代NCURSES库。

ncurses 包由 Pavel Curtis 发起，Zeyd Ben-Halim <<a>zmbenhal@netcom.com>
和 Eric S. Raymond <<a>esr@snark.thyrsus.com> 是最初的维护人员，
他们在 1.8.1 及以后版本中增加了很多的新功能。ncurses 不仅仅只是封装
了底层的终端功能，而且提供了一个相当稳固的工作框架（Framework）用
以产生漂亮的界面。它包含了一些创建窗口的函数。而它的姊妹库 Menu、Panel
和 Form 则是对 CURSES 基础库的扩展。这些库一般都随同 CURSES 一起发行。
我们可以建立一个同时包含多窗口 (multiple windows)、菜单(menus)、
面板 (panels) 和表单 (forms) 的应用程序。窗口可以被独立管理，例如让它
卷动 (scrollability) 或者隐藏。

目前 BiscuitOS 已经支持 library ncurses 的移植和实践。开发者
可用通过下面的章节进行 library ncurses 的使用。

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

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000307.png)

此界面是 Package 支持软件的配置界面，开发者将光标移动到 "GNU - ncurses",
按下 "Y" 按键之后再按下回车键，进入 "gnu - ncurses" 配置界面。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000308.png)

上图正是 "library ncurses" 应用程序的配置界面，"version" 选项代表当前软件的版本。
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
"ncurses-6.1", 进入该目录，可以获得两个文件: Makefile 和 README.md。至此
应用程序的移植前期准备已经结束。

------------------------------------------------

#### <span id="A013">获取源码</span>

进过上面的步骤之后，开发者在 "BiscuitOS/output/linux-5.0-arm32/package/ncurses-6.1"
目录下获得移植所需的 Makefile，然后开发者接下来需要做的就是下载源码，
使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/ncurses-6.1
make download
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000309.png)

此时在当前目录下会获得一个新的目录 "ncurses-6.1"，里面存储着源码相关的文件，
至此源码下载完毕。

------------------------------------------------

#### <span id="A0132">解压并配置源码</span>

在获取源码之后，开发者将获得源码压缩包进行解压并配置源码，由于
library 项目大多使用 automake 进行开发，因此开发者可以使用如下
命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/ncurses-6.1
make tar
make configure
{% endhighlight %}

此时终端输出相关的信息，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000310.png)

至此源码配置完成。

------------------------------------------------

#### <span id="A014">源码编译</span>

获得源码之后，只需简单的命令就可以编译源码，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/ncurses-6.1
make
{% endhighlight %}

编译成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000311.png)

------------------------------------------------

#### <span id="A015">动态库安装</span>

程序编译成功之后，需要将可执行文件安装到 BiscuitOS rootfs 里，
只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/ncurses-6.1
make install
{% endhighlight %}

安装成功输出如下信息：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000312.png)

------------------------------------------------

#### <span id="A016">生成系统镜像</span>

程序安装成功之后，接下来需要将新的软件更新到 BiscuitOS 使用
的镜像里，只需简单的命令就可以实现，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/ncurses-6.1
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

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000313.png)

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
