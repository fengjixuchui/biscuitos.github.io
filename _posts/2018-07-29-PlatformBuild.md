---
layout: post
title:  "BiscuitOS 开发环境搭建"
date:   2018-07-29 00:00:00 +0800
categories: [Build]
excerpt: Ubuntu/Fedora/Mint Platform Build.
tags:
  - BiscuitOS
  - Ubuntu
  - Fedora
  - Mint
---

开发 BiscuitOS 需要一台 Linux 发行版作为编译主机，由于 Linux 具有很多发行
版，本文用于介绍不同的 Linux 发行版上搭建开发环境的方法。

更多平台的适配在不断进行中。

--------------------------------------------------------
目录

> Ubuntu 14.04/16.04 Vmware/VirtualBox
    
> Ubuntu 18.04/17.10 Vmware/VirtualBox
    
> Fedora
    
> Mint

--------------------------------------------------------
## Ubuntu 14.04/16.04 Vmware/VirtualBox

作为 BiscuitOS 开发的首选平台，开发者首先确认是否安装必要的开发工具，如
果没有，请安如下步骤安装：

安装必要的开发工具，使用如下命令：

{% highlight ruby %}
sudo apt-get install qemu gcc make gdb git figlet
sudo apt-get install libncurses5-dev
sudo apt-get install lib32z1 lib32z1-dev
{% endhighlight %}

注意！如果是第一次使用 git，请按如下配置 git 对应的内容：

{% highlight ruby %}
git config —global user.name “Your Name”
git config —global user.email “Your Email"
{% endhighlight %}

至此，基本的开发工具已经安装完成。

----------------------------------------------------------
## Ubuntu 18.04/17.10 Vmware/VirtualBox

类似于 Ubuntu 16.04，Ubuntu 18.04/17.10 有不同的地方，开发者首先确认是否
安装必要的开发工具，如果没有，请安如下步骤安装：

安装必要的开发工具，使用如下命令：

{% highlight ruby %}
sudo apt-get install qemu gcc make gdb git figlet
sudo apt-get install libncurses5-dev
sudo apt-get install lib32z1 lib32z1-dev
{% endhighlight %}

注意！如果是第一次使用 git，请按如下配置 git 对应的内容：

{% highlight ruby %}
git config —global user.name “Your Name”
git config —global user.email “Your Email"
{% endhighlight %}

由于 Ubuntu 18.04/17.10 默认的 gcc 版本是 7，版本导致很多编译问题，所以
开发者需要修改 gcc 版本为 4.8，具体步骤如下：

{% highlight ruby %}
sudo apt-get update

sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++
sudo apt-get install -y gcc-4.8 g++-4.8 

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7   20

sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7   20

sudo update-alternatives --set cc /usr/bin/gcc
sudo update-alternatives --set c++ /usr/bin/g++
{% endhighlight %}

每当编译 BiscuitOS 之前，请使用 gcc -v 命令确认 gcc 版本是不是 4.8，如果不是，请使用如下命令进行切换

{% highlight ruby %}
sudo update-alternatives --config gcc

sudo update-alternatives --config g++
{% endhighlight %}

![GCC_VER](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/buildroot/VFS000029.png)

每当需要切换 gcc 版本的时候，可以使用如上命令。
至此，基本的开发工具已经安装完成。

-------------------------------------------------------------
## Fedora

Fedora27，开发者首先确认是否安装必要的开发工具，如果没有，请安如下步骤安装：

安装必要的开发工具，使用如下命令：

{% highlight ruby %}
sudo yum install qemu gcc make gdb git figlet
sudo yum install ncurses-d
{% endhighlight %}

注意！如果是第一次使用 git，请按如下配置 git 对应的内容：

{% highlight ruby %}
git config —global user.name “Your Name”
git config —global user.email “Your Email"
{% endhighlight %}

至此，基本的开发工具已经安装完成。

----------------------------------------------------------
## Mint

Mint18 64Bit，开发者首先确认是否安装必要的开发工具，如果没有，请安如下步骤安装：

安装必要的开发工具，使用如下命令：

{% highlight ruby %}
sudo apt-get install qemu gcc make gdb git figlet indent
sudo apt-get install libncurses5-dev
{% endhighlight %}

注意！如果是第一次使用 git，请按如下配置 git 对应的内容：

{% highlight ruby %}
git config —global user.name “Your Name”
git config —global user.email “Your Email"
{% endhighlight %}

至此，基本的开发工具已经安装完成。
