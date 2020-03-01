---
layout: post
title:  "Linux 内核源码开发工具合集"
date:   2019-03-07 18:30:30 +0800
categories: [Build]
excerpt: Linux 内核源码开发工具合集.
tags:
  - Linux
---

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

---------------------------------------------------

# 目录

> - [内核源码辅助工具 Ctag + Cscope](#CTAG+CSCOPE)
>
> - [二进制文件编辑工具](#二进制文件编辑工具)
>
>   - [bless](#bless)
>
>   - [hexdump](#hexdump)
>
>   - [hexedit](#hexedit)
>
> - [程序员计算器](#程序员计算器)
>
> - [源码对比工具](#源码对比工具)



---------------------------------------------------
<span id="CTAG+CSCOPE"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

# 内核源码辅助工具 Ctag + Cscope

内核开发过程中需要对源码进行深入分析，经常遇到查找函数，宏，变量的定义，以及
函数，宏，变量被引用的地方。Windows 工具 Source Insight 可以完整解决这个问题，
但在 Linux 发行版上如何搭建一个简单，高效的源码索引平台呢？这里推荐使用 Ctag
和 Cscope 的组合，并结合一些技巧让平台简单，高效的运转。

##### 准备源码

Linux 内核源码默认就支持 ctags 和 cscope 工具，并根据 Linux 内核源码的特定做了优化，
原生支持，所以可以轻松使用这两个工具搭建一个高效的 Linux 源码浏览工具。

##### 安装工具

本教程基于 Linux 5.0 讲解，如果还没有搭建开发环境，可以参考文档：

> [搭建基于 ARM 的 Linux 5.0 源码开发环境](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建好 Linux 5.0 开发环境后，可以在 Linux 5.0 源码中直接配置 ctags 和 cscope 工具，
使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux
make ARCH=arm tags
make ARCH=arm cscope
{% endhighlight %}

执行完命令之后，会在源码目录下生成 tags, cscope.out, cscope.in.out, 和 cscope.po.out
等多个文件，同样，为了减少不必要的输入，也可以将这个 tags 和 cscope 作为 vim 默认的数据库，
将其添加到默认 vim 配置文件中，使用如下命令：

{% highlight bash %}
vi ~/.vimrc

向文件中添加如下内容：

" Ctags
set tags=BiscuitOS/output/linux-5.0-arm32/linux/linux/tags
" Cscope
: cscope add BiscuitOS/output/linux-5.0-arm32/linux/linux/cscope.out
set cscopetag
set csto=1
set cspc=1
{% endhighlight %}

### 使用工具

安装和配置完工具之后，接下来就是开始使用工具了，使用 vim 在源码目录下打开一个源文件，
这里讲解基础功能的使用：

##### 查看函数，宏，全局变量的定义

查找函数，宏，全局变量的定义时，在 vim 中将光标移动到函数，宏，或全局变量处，按下
**Ctrl+]** , 之后就会跳转到定义的位置。如果函数，宏，或全局变量有多处定义，那么
cscope 工具会在按下 **Ctrl+]** 之后，在底部打印出函数所有的定义出处，并使用数字进行
排列，开发者此时可以根据需要的情况输入指定的数字，并按下回车，接着就可以跳转到定义的
位置。例如查找 early_fixmap_init() 函数的定义, 将光标移动到 early_fixmap_init()
函数处，之后自动打印如下信息：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000008.png)

输入 2 并按回车，vim 就跳转到 arch/arm/mm/mmu.c 第 387 行。

##### 从查看中返回

当查看完一个函数，宏，或全局变量的定义之后，开发者可以使用 **Ctrl+T** 进行返回上一个
跳转点，这样利于源码的浏览。

##### 查看函数，宏，全局变量被调用的地方

有时开发者需要查看函数。宏，全局变量在什么地方被调用，那么可以使用如下命令进行查找：

在 vim 按下 Esc 按键，接着输入 : （冒号） 进入命令行模式，输入如下命令：

{% highlight c %}
:cs find c func
{% endhighlight %}

例如，需要查找源码中什么地方引用了 adjust_lowmem_bounds() 函数，使用如下命令：

{% highlight c %}
:cs find c adjust_lowmem_bounds
{% endhighlight %}

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000009.png)

但查看完一个函数的引用之后，需要返回上一次光标位置，可以使用 **Ctrl+T** 组合命令进行
返回

##### 查看局部变量的定义

在一个很长的函数里，需要查找某个局部变量的定义，可以使用 gd 命令，也就是先将光标移动到
局部变量的位置，然后先按 g 键，再按 d 键，就可以跳转到局部变量定义的地方。当查看完局部
变量的定义之后，再按 **Ctrl+o** 组合按键返回之前的位置。

更多ctags 和 cscope 工具的使用，可以参考文章：

> [Linux cscope usage on vim](https://blog.csdn.net/hunter___/article/details/80333543)

---------------------------------------------------
<span id="二进制文件编辑工具"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

# 二进制文件编辑工具

内核开发过程中少不了对二进制文件的读写和对比操作，这里给开发者介绍与二进制有关
的工具集合：

> [二进制文件查看工具: hexdump](#hexdump)
>
> [二进制文件编辑工具：hexedit](#hexedit)
>
> [二进制文件编辑图形工具: bless](#bless)

### <span id="hexdump">hexdump</span>

hexdump 主要用来查看 “二进制” 文件的十六进制编码。hexdump 能够查看任何文件，不
限于与二进制文件.

##### 安装

Ubuntu 下安装 hexdump 工具使用如下命令：

{% highlight bash %}
sudo apt-get install hexdump
{% endhighlight %}

##### 使用方法

{% highlight bash %}
hexdump [选项] [文件]…

选项

    -n length：格式化输出文件的前 length 个字节
    -C 输出规范的十六进制和ASCII码
    -b 单字节八进制显示
    -c 单字节字符显示
    -d 双字节十进制显示
    -o 双字节八进制显示
    -x 双字节十六进制显示
    -s 从偏移量开始输出
    -e 指定格式字符串，格式字符串由单引号包含，格式字符串形如：’a/b “format1” “format2”。
       每个格式字符串由三部分组成，每个由空格分割，如 a/b 表示，b 表示对每b个输入字节应用
       format1 格式，a 表示对每个 a 输入字节应用 format2，一般 a > b，且 b 只能为 1,2,4，
       另外 a 可以省略，省略 a=1。format1 和 format2 中可以使用类似printf的格斯字符串。
           %02d：两位十进制
           %03x：三位十六进制
           %02o：两位八进制
           %c：单个字符等
           %_ad：标记下一个输出字节的序号，用十进制表示
           %_ax：标记下一个输出字节的序号，用十六进制表示
           %_ao：标记下一个输出字节的序号，用八进制表示
           %_p：对不能以常规字符显示的用.代替
       同一行显示多个格式字符串，可以跟多个-e选项
{% endhighlight %}

#### 实践

{% highlight bash %}
hexdump zImage
{% endhighlight %}

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000002.png)

### <span id="hexedit">hexedit</span>

hexedit 是 linux 下的一款二进制文件编辑工具，可以使用 hexedit 工具对二进制
文件进行编辑和查看。

##### 安装

Ubuntu 上安装 hexedit 可以使用如下命令：

{% highlight bash %}
sudo apt-get install hexedit
{% endhighlight %}

##### 使用方法

{% highlight bash %}
hexedit <filename>

,           移动到文件首部/尾部(go to start/end of the file)
→           下一个字符(next character)
←           上一个字符(previous character)
↑           上一行(previous line)
↓           下一行(next line)
Home        行首(beginning of line)
End         行尾(end of line)
PageUp      上一页(page forward)
PageDown    下一页(page backward)

杂项(Miscellaneous)

F1          帮助(help)
F2          保存(save)
F3          载入(load file)
Ctrl+X      保存并退出(save and exit)
Ctrl+C      不保存退出(exit without save)
Tab         十六进制/ASCII码切换(toggle hex/ascii)
Backspace   撤销前一个字符(undo previous character)
Ctrl+U      撤销全部操作(undo all)
Ctrl+S      向下查找(search forward)
Ctrl+R      向上查找(search forward)
{% endhighlight %}

##### 使用实例

{% highlight bash %}
hexedit zImage
{% endhighlight %}

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000003.png)

### <span id="bless">bless</span>

Bless 是一个十六进制编辑器，其主要功能包括：支持编辑大数据文件及块设备、能够执行
搜索与替换操作、具有类似 Firefox 的标签浏览特性、可将数据输出为文本或 HTML、包含
插件系统.

##### 安装

Ubuntu 上安装 bless 使用如下命令：

{% highlight bash %}
sudo apt-get install bless
{% endhighlight %}

##### 使用方法

{% highlight bash %}
bless <filename>
{% endhighlight %}

##### 实践

{% highlight bash %}
bless zImage
{% endhighlight %}

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000004.png)

---------------------------------------------------
<span id="程序员计算器"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

# 程序员计算器

> - [Ubuntu Calculator](#Ubuntu Calculator)
>
> - [Windows Calculator](#Windows Calculator)

### <span id="Ubuntu Calculator">Ubuntu Calculator</span>

程序开发过程中，经常会遇到各种进制的数学运算，这里推荐一款程序员专用的计算器,
Ubuntu 自带的 Calculator 就能切换成程序员模式进行各种进制的计算，如下图：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000005.png)

### <span id="Windows Calculator">Windows Calculator</span>

程序开发过程中，经常会遇到各种进制的数学运算，这里推荐一款程序员专用的计算器,
Windows 自带的 Calculator 就能切换成程序员模式进行各种进制的计算，如下图：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000006.png)

---------------------------------------------------
<span id="源码对比工具"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

# 源码对比工具

> - [图形化源码对比工具: meld](#meld)

### <span id="meld">meld</span>

在开发过程中，需要对比两次提交或者两个版本间源码的不同点，这时就需要源码对比工具。源码
对比工具就是用于大到工程，小到源文件之间的对比。

##### meld 安装

Ubuntu 上安装 meld 可以使用如下命令：

{% highlight bash %}
sudo apt-get install meld
{% endhighlight %}

##### meld 使用

meld 可以对比两个或者三个对象，可以使用命令行的方式启动 meld，也可以直接点击 meld 图标
进行启动，例如使用命令行启动，如下格式：

{% highlight bash %}
meld file0 file1
{% endhighlight %}

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000010.png)

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
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
