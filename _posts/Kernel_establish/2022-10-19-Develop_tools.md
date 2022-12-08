---
layout: post
title:  "Tools"
date:   2019-02-19 09:43:30 +0800
categories: [Build]
excerpt: Tools.
tags:
  - Tools
---

> - [Ubuntu 14.x/16.x/18.x](#A)
>
> - [Ubunut 20.x](#B)
>
> - [Ubunut 22.x](#C)


##### <span id="A">Ubuntu 14.x/16.x/18.x</span>

{% highlight bash %}
sudo apt-get update
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl wget
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison lbssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gperf libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python2.7-dev python-dev bridge-utils
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx libsdl2-dev libsdl1.2-dev
sudo apt-get install -y debootstrap bsdtar libssl-dev
sudo apt-get install -y libelf-dev gcc-multilib g++-multilib
sudo apt-get install -y libcap-ng-dev binutils-dev
sudo apt-get install -y libmount-dev libselinux1-dev libffi-dev libpulse-dev
sudo apt-get install -y liblzma-dev python-serial
sudo apt-get install -y libnuma-dev libnuma1 ninja-build
sudo apt-get install -y libtool libsysfs-dev libasan
{% endhighlight %}

如果开发主机是 64 位系统，请继续安装如下开发工具：

{% highlight bash %}
sudo apt-get install lib32z1 lib32z1-dev libc6:i386
{% endhighlight %}

第一次安装 git 工具需要对 git 进行配置，配置包括用户名和 Email，请参照如下命令进行配置

{% highlight bash %}
git config --global user.name "Your Name"
git config --global user.email "Your Email"
{% endhighlight %}

----------------------------------------------------------

##### <span id="B">Ubuntu 20.x</span>

{% highlight bash %}
#### Add GCC sourcelist
vi /etc/apt/sources.list
+ deb [arch=amd64] http://archive.ubuntu.com/ubuntu focal main universe

sudo apt-get update
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl wget
sudo apt-get install -y device-tree-compiler qemu-system-x86
sudo apt-get install -y flex bison libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gperf libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python2.7-dev python-dev bridge-utils
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx libsdl2-dev libsdl1.2-dev
sudo apt-get install -y debootstrap libarchive-tools
sudo apt-get install -y libelf-dev gcc-multilib g++-multilib
sudo apt-get install -y libcap-ng-dev binutils-dev
sudo apt-get install -y libmount-dev libselinux1-dev libffi-dev libpulse-dev
sudo apt-get install -y liblzma-dev libssl-dev
sudo apt-get install -y libnuma-dev libnuma1 ninja-build
sudo apt-get install -y libtool libsysfs-dev
sudo apt-get install -y libtool libmpc-dev

GCC must be GCC-7, change default GCC as:

sudo apt-get update
sudo apt-get install -y gcc-7 g++-7
sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 20
#### Choose GCC-7 and G++-7
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
{% endhighlight %}

如果开发主机是 64 位系统，请继续安装如下开发工具：

{% highlight bash %}
sudo apt-get install lib32z1 lib32z1-dev libc6:i386
{% endhighlight %}

第一次安装 git 工具需要对 git 进行配置，配置包括用户名和 Email，请参照如下命令进行配置

{% highlight bash %}
git config --global user.name "Your Name"
git config --global user.email "Your Email"
{% endhighlight %}

----------------------------------------------------------

##### <span id="C">Ubuntu 22.x</span>

{% highlight bash %}
#### Add GCC sourcelist
vi /etc/apt/sources.list
+ deb [arch=amd64] http://archive.ubuntu.com/ubuntu focal main universe

sudo apt-get update
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl wget qemu-system-x86
sudo apt-get install -y device-tree-compiler build-essential
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python3 pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gperf libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python3-dev bridge-utils
sudo apt-get install -y net-tools binutils-dev
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx libsdl2-dev libsdl1.2-dev
sudo apt-get install -y debootstrap
sudo apt-get install -y libelf-dev gcc-multilib g++-multilib
sudo apt-get install -y libcap-ng-dev
sudo apt-get install -y libmount-dev libselinux1-dev libffi-dev libpulse-dev
sudo apt-get install -y liblzma-dev python3-serial
sudo apt-get install -y libnuma-dev libnuma1 ninja-build
sudo apt-get install -y libtool libsysfs-dev
sudo apt-get install -y libntirpc-dev libtirpc-dev
sudo ln -s /usr/bin/python3 /usr/bin/python

GCC must be GCC-7, change default GCC as:

sudo apt-get update
sudo apt-get install -y gcc-7 g++-7
sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 20
#### Choose GCC-7 and G++-7
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
{% endhighlight %}

如果开发主机是 64 位系统，请继续安装如下开发工具：

{% highlight bash %}
sudo apt-get install lib32z1 lib32z1-dev libc6:i386
{% endhighlight %}

第一次安装 git 工具需要对 git 进行配置，配置包括用户名和 Email，请参照如下命令进行配置

{% highlight bash %}
git config --global user.name "Your Name"
git config --global user.email "Your Email"
{% endhighlight %}

----------------------------------------------------------
