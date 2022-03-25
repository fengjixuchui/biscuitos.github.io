---
layout: post
title:  "通过 Kernel 历史树实践内核"
date:   2020-02-02 08:35:30 +0800
categories: [HW]
excerpt: Kernel.
tags:
  - Kernel
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

> - [项目简介](#A0)
>
> - [项目部署](#B0)
>
> - [项目使用](#B02)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### 问题简介

每当在高版本内核中学习研究一个新子系统的时候，往往由于代码的复杂性导致
困难重重。这里提出一个简单的思路，开发者可以根据自己的需求进行采纳。

在内核中，任何复杂的子系统都是由最简单的元数据发展而来，并且 Linux 内核
强大的 git 支撑，因此换个想法是不是可以将某个子系统的 patch 历史独立出来，
然后从第一个基础 patch 开始，一个一个的 patch 研究子系统的发展过程，并将
这一过程在 BiscuitOS 上实践，这样的学习过程对学习某个子系统来说是一个不错
的选择。因此本文用于介绍如何在 BiscuitOS 上部署和实践这个项目。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="B0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### 项目部署

> - [实践准备](#B00)
>
> - [实践部署](#B01)
>
> - [项目使用](#B02)
>
> - [内核实统调用调试](#B22)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B00">实践准备</span>

开发者首先搭建 BiscuitOS 项目，如果已经搭建了 BiscuitOS 的开发者，可以跳过
这一节。开发者参考下面文档安装基础开发工具:

> - [BiscuitOS 基础开发工具安装指南](https://biscuitos.github.io/blog/Develop_tools)

基础环境搭建完毕之后，开发者从 GitHub 上获取 BiscuitOS 项目源码，
使用如下命令：

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git --depth=1
cd BiscuitOS
{% endhighlight %}

至此，BiscuitOS 已经部署完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B01">实践部署</span>

准备好 BiscuitOS 项目之后，开发者使用如下命令进行项目部署:

{% highlight bash %}
cd BiscuitOS
git pull
make linux-history_defconfig
make
{% endhighlight %}

接下来 BiscuitOS 项目会从 github 上拉去最新的 linux 源码和安装必要的
源码开发工具，接下来就是漫长的等待。等所有的内容下载完毕之后，BiscuitOS
会输出如下信息:

![](/assets/PDB/RPI/RPI000726.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="B02">项目使用</span>

搭建完毕之后，开发者可以参考本节进行子系统源码搭建，此处以 linux 源码树
下的 "virt/kvm" 为例，该目录下是 kvm 子系统的基础源码 (其他子系统以目录为准)，
使用如下命令:

{% highlight bash %}
cd BiscuitOS
make linux-history_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000727.png)

勾选并进入 "[\*]  kernel history  --->"

![](/assets/PDB/RPI/RPI000728.png)

选中 "[\*]   Build new split subdir (NEW)", 此时会出现:

![](/assets/PDB/RPI/RPI000729.png)

此时出现三个新的选项，第一个选项 "Build new split subdir" 用于指定需要
分离的子系统所在的目录，例如选中该选项，然后输入 "virt/kvm":

![](/assets/PDB/RPI/RPI000730.png)

选中 "OK" 保存，第二个选项是 "The new directory/branch name" 用于指定
独立之后在指定的目录下创建新的文件夹存储子系统的源码，此时该名字就是新
文件夹的名字。例如选中该选项，然后配置新的文件夹名字也是 "virt/kvm":

![](/assets/PDB/RPI/RPI000731.png)

设置完毕选中 "OK" 保存。最后一个选项 "Kernel tag" 用于指定从具体的 Kernel
版本中分离出子系统，开发者可以设置过个选项，其中 normal 代表当前源码树上获得
子系统代码，newest 代表从最新的源码树上获得子系统源码，也可以是 Linux 具体
的某个 tag，如 "v2.6.15", 开发者可以通过在内核源码数下使用 git tag 命令获得
更多可用的信息。此处保持默认值。在设置完以上的配置之后，保存并退出。接着
使用如下命令:

{% highlight bash %}
make
{% endhighlight %}

接下来就是漫长的等待，等一些处理完毕之后可以在一下目录找到独立出来的源码，
如下:

{% highlight bash %}
cd BiscuitOS/output/linux-history/History
ls
{% endhighlight %}

此时可以看到之前设置的 "virt/kvm" 代码目录，接着进入到 "virt/kvm" 源码树下
查看:

![](/assets/PDB/RPI/RPI000732.png)

可以看出独立出来的 kvm 源码，接着可以使用 gitk、gitkraken 等工具查看所有
patch:

{% highlight bash %}
cd BiscuitOS/output/linux-history/History/virt/kvm
gitk .
{% endhighlight %}

![](/assets/PDB/RPI/RPI000733.png)

从上图可以看到历史提交记录。

![](/assets/PDB/RPI/RPI000734.png)

gitkraken 查看效果，开发者也可以使用其他工具查看。

有了上面的提交记录，可以根据提交记录找到相应的提交邮件，可以在以下网站中
获得:

> - [index : kernel/git/torvalds/linux.git](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/)
>
> - [Patchwork](https://patchwork.kernel.org/)

接下来开发者可以根据这个历史提交记录进行学习研究，更多后续文章会在 BiscuitOS
社区继续分享.

------------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
