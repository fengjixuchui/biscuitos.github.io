---
layout: post
title:  "Tools"
date:   2019-02-19 09:43:30 +0800
categories: [Build]
excerpt: Tools.
tags:
  - Tools
---

在编译系统之前，需要对开发主机安装必要的开发工具。以 Ubuntu 为例
安装基础的开发工具。开发者可以按如下命令进行安装：

{% highlight bash %}
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gpref libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python2.7-dev python-dev bridge-utils
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx
sudo apt-get install -y debootstrap bsdtar
{% endhighlight %}

如果开发主机是 64 位系统，请继续安装如下开发工具：

{% highlight bash %}
sudo apt-get install lib32z1 lib32z1-dev
{% endhighlight %}

第一次安装 git 工具需要对 git 进行配置，配置包括用户名和 Email，
请参照如下命令进行配置

{% highlight bash %}
git config --global user.name "Your Name"
git config --global user.email "Your Email"
{% endhighlight %}
