---
layout: post
title:  "Linux 内核源码调试工具及技巧"
date:   2019-03-07 18:30:30 +0800
categories: [Build]
excerpt: Linux 内核源码调试工具及技巧.
tags:
  - Linux
---

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

---------------------------------------------------

# 目录

> - [内核源码辅助工具 Ctag + Cscope](#CTAG+CSCOPE)



---------------------------------------------------

# <span id="CTAG+CSCOPE">内核源码辅助工具 Ctag + Cscope</span>

内核开发过程中需要对源码进行深入分析，经常遇到查找函数，宏，变量的定义，以及
函数，宏，变量被引用的地方。Windows 工具 Source Insight 可以完整解决这个问题，
但在 Linux 发行版上如何搭建一个简单，高效的源码索引平台呢？这里推荐使用 Ctag 
和 Cscope 的组合，并结合一些技巧让平台简单，高效的运转。

> Platform: Ubuntu

### 安装工具

推荐在 Ubuntu 上进行源码级别开发，该平台上安装工具使用如下命令：

{% highlight bash %}
sudo apt-get install ctags
sudo apt-get install cscope
{% endhighlight %}

### 准备源码

在使用工具之前，开发者可以考虑一个问题，当查找一个函数定义的时候，如果这个函
数与体系有关，那么函数会在不同的体系中进行定义，这会导致在查找函数的引用中会
增加很多不必要的浪费，为了解决这个问题，开发者可以按如下办法进行处理，这个问
题就能得到很好的解决。比如开发者使用的的源码是关于 arm 32 位系统的，那么 
arch/ 目录下其他平台的源码对调试来说就是多余的代码，所以可以删除 arch/ 目录下
除 arm/ 和 Kconfig 之外的所有文件，这样做之后，也许第一次编译无法通过，可以通
过编译提示的错误修改对应的 Kconfig 文件，将错误处注释掉即可。这样做有利于提供
开发效率。

### 配置工具

首先准备 Linux 源码，源码目录假设位于 "BiscuitOS/output/linux-5.0/"，然后在终端中
切换到源码目录，首先建立 ctags 数据库，使用如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0/
ctags -R
{% endhighlight %}

命令运行完之后会在源码目录下生成 tags 文件，为了加速内核开发，开发者可以将该 tags 作
位默认的 ctags 的数据库，如果在将来有新的 ctags 数据库也可以参照这个方法进行修改。为
了节省不必要的命令输入，开发者可以通过如下设置将该 tags 作为默认的数据库，修改主目录
下的 vim 配置文件，如下：

{% highlight bash %}
vi ~/.vimrc

向文件中添加如下内容：

set tags=BiscuitOS/output/linux-5.0/tags
{% endhighlight %}

至此 ctags 配置完成，接下来配置 cscope，同样也是在源码目录下，使用如下命令来建立
cscope 的数据库：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0/
cscope -Rbqk
{% endhighlight %}

执行完命令之后，会在源码目录下生成 cscope.out, cscope.in.out, 和 cscope.po.out 三个
文件，同样，为了减少不必要的输入，也可以将这个 cscope.out 作为默认的数据库，将其添加
到默认 vim 配置文件中，使用如下命令：

{% highlight bash %}
vi ~/.vimrc

向文件中添加如下内容：

: cscope add BiscuitOS/output/linux-5.0/cscope.output
set cscopetag
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

{% highlight c %}
early_fixmap_init();
early_ioremap_init();

parse_early_param();

#ifdef CONFIG_MMU
early_mm_init(mdesc);
#endif
setup_dma_zone(mdesc);
xen_early_init();
efi_init();
Cscope tag: early_fixmap_init                                 1117,5-12     83%
#   line  filename / context / line
1     63  arch/arm/include/asm/fixmap.h <<early_fixmap_init>>
     static inline void early_fixmap_init(void ) { }
2    387  arch/arm/mm/mmu.c <<early_fixmap_init>>
     void __init early_fixmap_init(void )
Type number and <Enter> (empty cancels):
{% endhighlight %}

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

输出内容：

adjust_lowmem_bounds();
arm_memblock_init(mdesc);
/* Memory may have been removed so recalculate the bounds. */
adjust_lowmem_bounds();

early_ioremap_reset();

paging_init(mdesc);
request_standard_resources(mdesc);

if (mdesc->restart)
Cscope tag: adjust_lowmem_bounds
#   line  filename / context / line
1   1132  arch/arm/kernel/setup.c <<setup_arch>>
     adjust_lowmem_bounds();
2   1135  arch/arm/kernel/setup.c <<setup_arch>>
     adjust_lowmem_bounds();
Type number and <Enter> (empty cancels):
{% endhighlight %}

但查看完一个函数的引用之后，需要返回上一次光标位置，可以使用 **Ctrl+T** 组合命令进行
返回

##### 查看局部变量的定义

在一个很长的函数里，需要查找某个局部变量的定义，可以使用 gd 命令，也就是先将光标移动到
局部变量的位置，然后先按 g 键，再按 d 键，就可以跳转到局部变量定义的地方。当查看完局部
变量的定义之后，再按 **Ctrl+o** 组合按键返回之前的位置。

更多ctags 和 cscope 工具的使用，可以参考文章：

> [Linux cscope usage on vim](https://blog.csdn.net/hunter___/article/details/80333543)
