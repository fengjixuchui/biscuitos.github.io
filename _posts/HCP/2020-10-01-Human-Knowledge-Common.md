---
layout: post
title:  "人类知识共同体计划"
date:   2020-10-01 06:00:00 +0800
categories: [HW]
excerpt: Common for knowledge.
tags:
  - HKC
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [HKC 计划原理](#A)
>
> - [HKC 计划使用](#B)
>
> - [HKC 计划实践](#C)
>
> - [HKC 生态共享](#H000001)

> - MMU Shrinker
>
>   - [register_shrinker/unregister_shrinker](#H000001)
>
> - [附录](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000V.jpg)

#### HKC 计划使用

使用过 BiscuitOS 的开发者应该使用过 BiscuitOS 快速开发框架，开发者只需通过图形化的界面勾选所需的项目，那么 BiscuitOS 可以自动部署对应的项目，并可以在 BiscuitOS 上快速执行项目。设想一下，如果提供一个足够充足的 BiscuitOS 项目库，开发者在开发过程中需要进行某些功能模块的验证或使用，只需简单的配置一下 BiscuitOS，那么 BiscuitOS 就会快速搭建所需项目的开发环境，并支持快速跨平台验证，这样将大大提高一线工程师、学生研究学者、极客爱好者开发进度，因此这里提出了面向程序员的 "人类知识共同体" 计划。

"人类知识共同体" 计划面向每个开发者，提供一套完整的生态逻辑，开发者可以提供自己的 "独立代码" 到 BiscuitOS 代码库里，也可自由使用 biscuitOS 代码库里面的代码，相辅相成，共同促进生态的可持续发展.因此本文为开发者介绍加入和使用 "人类知识共同体" 计划。如果开发者需要使用 "人类知识共同体" 计划，请参考如下:

> - [HKC 计划实践](#C)

如果开发者想向 "人类知识共同体" 计划提供 "独立代码"，请参考:

> - [HKC 计划使用](#B)

项目捐赠，捐赠资金将用于维护 BiscuitOS 社区的发展 :)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

#### HKC 计划使用

"人类知识共同体" 计划基于 BiscuitOS 提供了一套完整的运行机制，开发者可以参与其中，本节用于介绍加入 "人类知识共同体" 计划。正如 "人类知识共同体" 计划介绍，开发者可以将 "独立程序" 提交加入到项目中，以便和其他人分享。首先开发者应该将自己的 "独立程序" 进行分类，目前项目支持的程序分类如下:

* **编译进内核源码树代码**
   指的是开发者提供的 "独立代码" 必须编译进内核才能使用，譬如内核源码树内使用 "obj-y" 指定的源文件.
* **独立内核模块代码**
   指的是开发者提供的 "独立代码" 可以在内核源码数之外进行编译，并通过 KO 的模式使用.
* **独立 Application 代码**
   指的是开发者提供的 "独立代码" 是一个应用程序.

开发者首先将自己的 "独立程序" 进行分类，如果是 "编译进内核源码树代码", 那么请点击下面链接:

> - [编译进内核源码树代码 -- 独立程序开发办法](#B0)

如果是 "独立内核模块代码", 开发者可以参考如下链接:

> - [独立内核模块代码 -- 独立程序开发办法](#B1)

如果是 "独立 Application 代码", 开发者可以参考如下链接:

> - [独立 Application 代码 -- 独立程序开发办法](#B2)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### 编译进内核源码树研究

BiscuitOS 支持将独立代码编译进内核，开发者可以使用这种办法将你的 "独立代码" 进行提交，具体使用办法如下. 首先开发者基于 BiscuitOS 项目构建指定架构的环境，这里以 "linux 5.0" i386 架构进行讲解，其他架构类型，开发者参考使用。开发者首先在 BiscuitOS 项目下使用如下命令:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] BiscuitOS Demo Code  --->
          [*] Kernel Demo Code on BiscuitOS  ---> 

make
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-kernel-0.0.1/
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000477.png)

开发者使用如下命令部署所需的环境，正如上图，选择 "Kernel Demo Code on  BiscuitOS" 选项，保存并退出配置，接着使用 make 进行部署，部署成功之后，进入部署的目录, 接着获取 Demo 源码使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-kernel-0.0.1/
make download
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000478.png)

通过上面的命令之后，BiscuitOS 会自动部署一个 Demo 程序，其中 BiscuitOS-kernel-0.0.1 目录下的 main.c 函数就是源码的位置，同级的 Makefile 就是源码编译描述。Demo 中的 main.c 源码很简单，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000479.png)

Demo 源码很简单，就是在 "device_initcall" 阶段调用 "BiscuitOS_init()" 函数，并打印 "Hello BiscuitOS on  kernel." 字符串。开发者使用如下命令进行源码编译，并在 BiscuitOS 上运行:
 
{% highlight bash %}
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000480.png)

从内核启动 log 中看到打印的 "Hello BiscuitOS on  kernel", 至此 Demo 程序演示完毕，接下来讲解开发者如何将 "独立代码" 添加到 BiscuitOS 机制。 开发者可以将自己的 "独立程序" 替换 Demo 程序中的 main.c 函数，并提供对应的 Makefile 文件，例如开发者的独立程序包含了一个源文件 "usage.c", 那么可以基于 Demo 程序，将 main.c 函数移除并添加 "usage.c" 文件到该目录下，并修改 Makefile 文件，如下:

{% highlight bash %}
obj-y += usage.o
{% endhighlight %}

修改完毕之后，就是编译源码和在 BiscuitOS 验证你的独立程序。如果在 BiscuitOS 上验证无误的话，开发者接下来按如下步骤提交独立代码, 并合入 BiscuitOS 项目里。开发者需要发一份邮件给我，按如下内容进行描述:

{% highlight bash %}
1. 独立项目的功能
2. 独立代码在什么架构上验证的，内核版本号信息
3. 附件添加对应的源码
3. 开发者的介绍和联系邮箱
{% endhighlight %}

开发者准备好如上内容发送至 "buddy.zhang@aliyun.com" 邮箱，并微信通知我。待我审批通过之后合入 BiscuitOS 并进行发布.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### 独立内核模块研究

BiscuitOS 支持将独立内核模块开发，如果开发者的 "独立代码" 可以通过内核模块方式使用，那么开发者可以使用这种办法将你的 "独立代码" 进行提交，具体使用办法如下. 首先开发者基于 BiscuitOS 项目构建指定架构的环境，这里以 "linux 5.0" i386 架构进行讲解，其他架构类型，开发者参考使用。开发者首先在 BiscuitOS 项目下使用如下命令:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] BiscuitOS Demo Code  --->
          [*] Module Demo Code on BiscuitOS  --->
          [*] Module Project Demo on BiscuitOS  --->
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000477.png)

开发者使用如下命令部署所需的环境，正如上图，选择 "Module Demo Code on BiscuitOS" 选项和 "Module Project Demo on BiscuitOS"，保存并退出配置，"Module Demo Code on BiscuitOS" 选项是一个简单的模块 Demo，如果开发者的 "独立程序" 只是一个简单的功能，那么可以参考这个 Demo 进行部署; 如果开发者的 "独立程序" 是一个复杂多层的项目，那么开发者可以参考 "Module Project Demo on BiscuitOS"，其是一个简单的多层源码 Demo. 接着以 "Module Demo Code on BiscuitOS" Demo 进行讲解，其他 Demo 类似. 使用 make 进行部署，部署成功之后，进入部署的目录, 接着获取 Demo 源码使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-modules-0.0.1/
make download
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000481.png)

通过上面的命令之后，BiscuitOS 会自动部署一个 Demo 程序，其中 BiscuitOS-modules-0.0.1 目录下的 main.c 函数就是源码的位置，同级的 Makefile 就是源码编译描述。Demo 中的 main.c 源码很简单，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000482.png)

Demo 源码很简单，就是在 KO 加载的时候打印 "Hello modules on BiscuitOS" 字符串。开发者使用如下命令进行源码编译，并在 BiscuitOS 上运行:

{% highlight bash %}
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000483.png)

BiscuitOS 启动之后，动态加载模块后看到打印相应的字符串, 至此 Demo 程序演示完毕，接下来讲解开发者如何将 "独立代码" 添加到 BiscuitOS 机制。 开发者可以将自己的 "独立程序" 替换 Demo 程序中的 main.c 函数，并提供对应的 Makefile 文件，例如开发者的独立程序包含了一个源文件 "usage.c", 那么可以基于 Demo 程序，将 main.c 函数移除并添加 "usage.c" 文件到该目录下，并其 Makefile 文件如下:

{% highlight bash %}
#
# Module Common
#
# (C) 2020.03.14 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

ifneq ($(KERNELRELEASE),)

## Target
ifeq ("$(origin MODULE_NAME)", "command line")
MODULE_NAME		:= $(MODULE_NAME)
else
MODULE_NAME		:= BiscuitOS_mod
endif
obj-m			:= $(MODULE_NAME).o

## Source Code
$(MODULE_NAME)-m	+= usage.o

## CFlags
ccflags-y		+= -DCONFIG_BISCUITOS_MODULE 
## Header
ccflags-y		+= -I$(PWD)

else

## Parse argument
## Default support ARM32
ifeq ("$(origin BSROOT)", "command line")
BSROOT=$(BSROOT)
else
BSROOT=/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32
endif

ifeq ("$(origin ARCH)", "command line")
ARCH=$(ARCH)
else
ARCH=arm
endif

ifeq ("$(origin CROSS_TOOLS)", "command line")
CROSS_TOOLS=$(CROSS_TOOLS)
else
CROSS_TOOLS=arm-linux-gnueabi
endif

## Don't Edit
KERNELDIR=$(BSROOT)/linux/linux
CROSS_COMPILE_PATH=$(BSROOT)/$(CROSS_TOOLS)/$(CROSS_TOOLS)/bin
CROSS_COMPILE=$(CROSS_COMPILE_PATH)/$(CROSS_TOOLS)-
INCLUDEDIR=$(KERNELDIR)/include
ARCHINCLUDEDIR=$(KERNELDIR)/arch/$(ARCH)/include
INSTALLDIR=$(BSROOT)/rootfs/rootfs/

## X86/X64 Architecture
ifeq ($(ARCH), i386)
CROSS_COMPILE	:=
else ifeq ($(ARCH), x86_64)
CROSS_COMPILE	:=
endif

## Compile
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

# FLAGS
CFLAGS += -I$(INCLUDEDIR) -I$(ARCHINCLUDEDIR)

all: $(OBJS)
	make -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) \
                CROSS_COMPILE=$(CROSS_COMPILE) modules

install:
	make -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) \
		INSTALL_MOD_PATH=$(INSTALLDIR) modules_install

clean:
	@rm -rf *.ko *.o *.mod.o *.mod.c *.symvers *.order \
               .*.o.cmd .tmp_versions *.ko.cmd .*.ko.cmd

endif
{% endhighlight %}

开发者可以使用这个 Makefile 模板，只需修改 "usage.o" 对应的内容即可。修改完毕之后，就是编译源码和在 BiscuitOS 验证你的独立程序。如果在 BiscuitOS 上验证无误的话，开发者接下来按如下步骤提交独立代码, 并合入 BiscuitOS 项目里。开发者需要发一份邮件给我，按如下内容进行描述:

{% highlight bash %}
1. 独立项目的功能
2. 独立代码在什么架构上验证的，内核版本号信息
3. 附件添加对应的源码
3. 开发者的介绍和联系邮箱
{% endhighlight %}

开发者准备好如上内容发送至 "buddy.zhang@aliyun.com" 邮箱，并微信通知我。待我审批通过之后合入 BiscuitOS 并进行发布.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### 应用程序研究

BiscuitOS 支持独立应用程序开发，如果开发者的 "独立代码" 是一个应用程序，那么开发者可以使用这种办法将你的 "独立代码" 进行提交，具体使用办法如下. 首先开发者基于 BiscuitOS 项目构建指定架构的环境，这里以 "linux 5.0" i386 架构进行讲解，其他架构类型，开发者参考使用。开发者首先在 BiscuitOS 项目下使用如下命令:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] BiscuitOS Demo Code  --->
          [*] Application Demo Code on BiscuitOS  --->
          [*] Application Project Demo on BiscuitOS  --->
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000477.png)

开发者使用如下命令部署所需的环境，正如上图，选择 "Application Demo Code on BiscuitOS" 选项和 "Application Project Demo on BiscuitOS"，保存并退出配置，"Application Demo Code on BiscuitOS" 选项是一个简单的应用程序 Demo，如果开发者的 "独立程序" 只是一个简单的功能，那么可以参考这个 Demo 进行部署; 如果开发者的 "独立程序" 是一个复杂多层的项目，那么开发者可以参考 "Application Project Demo on BiscuitOS"，其是一个简单的多层源码 Demo. 接着以 "Application Demo Code on BiscuitOS" Demo 进行讲解，其他 Demo 类似. 使用 make 进行部署，部署成功之后，进入部署的目录, 接着获取 Demo 源码使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-Application-0.0.1/
make download
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000484.png)

通过上面的命令之后，BiscuitOS 会自动部署一个 Demo 程序，其中 BiscuitOS-Application-0.0.1 目录下的 main.c 函数就是源码的位置，同级的 Makefile 就是源码编译描述。Demo 中的 main.c 源码很简单，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000485.png)

Demo 源码很简单，就是应用程序运行的时候打印 "Hello Application Demo on BiscuitOS" 字符串。开发者使用如下命令进行源码编译，并在 BiscuitOS 上运行:

{% highlight bash %}
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000486.png)

BiscuitOS 启动之后，运行应用程序后看到打印相应的字符串, 至此 Demo 程序演示完毕，接下来讲解开发者如何将 "独立代码" 添加到 BiscuitOS 机制。 开发者可以将自己的 "独立程序" 替换 Demo 程序中的 main.c 函数，并提供对应的 Makefile 文件，例如开发者的独立程序包含了一个源文件 "usage.c", 那么可以基于 Demo 程序，将 main.c 函数移除并添加 "usage.c" 文件到该目录下，并其 Makefile 文件如下:

{% highlight bash %}
#
# Application Project
#
# (C) 2020.02.02 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

## Target
ifeq ("$(origin TARGETA)", "command line")
TARGET			:= $(TARGETA)
else
TARGET			:= BiscuitOS_app
endif

## Source Code
SRC			+= usage.c

## CFlags
LCFLAGS			+= -DCONFIG_BISCUITOS_APP
## Header
LCFLAGS			+= -I./ -I$(PWD)/include

DOT			:= -
## X86/X64 Architecture
ifeq ($(ARCH), i386)
CROSS_COMPILE	=
LCFLAGS		+= -m32
DOT		:=
else ifeq ($(ARCH), x86_64)
CROSS_COMPILE   :=
DOT		:=
endif

# Compile
B_AS		= $(CROSS_COMPILE)$(DOT)as
B_LD		= $(CROSS_COMPILE)$(DOT)ld
B_CC		= $(CROSS_COMPILE)$(DOT)gcc
B_CPP		= $(CC) -E
B_AR		= $(CROSS_COMPILE)$(DOT)ar
B_NM		= $(CROSS_COMPILE)$(DOT)nm
B_STRIP		= $(CROSS_COMPILE)$(DOT)strip
B_OBJCOPY	= $(CROSS_COMPILE)$(DOT)objcopy
B_OBJDUMP	= $(CROSS_COMPILE)$(DOT)objdump

## Install PATH
ifeq ("$(origin INSPATH)", "command line")
INSTALL_PATH		:= $(INSPATH)
else
INSTALL_PATH		:= ./
endif

all:
	$(B_CC) $(LCFLAGS) -o $(TARGET) $(SRC)

install:
	@cp -rfa $(TARGET) $(INSTALL_PATH)

clean:
	@rm -rf $(TARGET) *.o
{% endhighlight %}

开发者可以使用这个 Makefile 模板，只需修改 "usage.c" 对应的内容即可。修改完毕之后，就是编译源码和在 BiscuitOS 验证你的独立程序。如果在 BiscuitOS 上验证无误的话，开发者接下来按如下步骤提交独立代码, 并合入 BiscuitOS 项目里。开发者需要发一份邮件给我，按如下内容进行描述:

{% highlight bash %}
1. 独立项目的功能
2. 独立代码在什么架构上验证的，内核版本号信息
3. 附件添加对应的源码
3. 开发者的介绍和联系邮箱
{% endhighlight %}

开发者准备好如上内容发送至 "buddy.zhang@aliyun.com" 邮箱，并微信通知我。待我审批通过之后合入 BiscuitOS 并进行发布.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

#### HKC 计划实践

> - [HKC 计划实践 -- 独立模块](#C0)
>
> - [HKC 计划实践 -- 内核源码树](#C1)
>
> - [HKC 计划实践 -- 应用程序](#C2)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

#### HKC 计划实践 -- 独立模块

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

BiscuitOS 支持多种架构、多种内核版本的实践，开发者可以根据需要自行选择，这里以 i386 架构的 linxu 5.0 为例子进行讲解，其余开发者可以在如下链接中进行参考:

> - [BiscuitOS linux 5.0 i386 环境部署](https://biscuitos.github.io/blog/Linux-5.0-i386-Usermanual/)
>
> - [BiscuitOS linux 环境部署汇总](https://biscuitos.github.io/blog/Kernel_Build/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

开发环境部署完毕之后，接下来是使用 BiscuitOS 代码库中的代码，这里以一个 i386 架构的 Demo 做为介绍，其余架构与之类似:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] BiscuitOS Demo Code  --->
          [*] Module Demo Code on BiscuitOS  --->
          [*] Module Project Demo on BiscuitOS  --->
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000477.png)

开发者使用如下命令部署所需的环境，正如上图，选择 "Module Demo Code on BiscuitOS" 选项和 "Module Project Demo on BiscuitOS"，保存并退出配置，"Module Demo Code on BiscuitOS" 选项是一个简单的模块 Demo，接着运行 make 进行部署，部署成功之后，进入部署的目录, 接着获取 Demo 源码使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-modules-0.0.1/
make download
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000481.png)

通过上面的命令之后，BiscuitOS 会自动部署一个 Demo 程序，其中 BiscuitOS-modules-0.0.1 目录下的 main.c 函数就是源码的位置，同级的 Makefile 就是源码编译描述。Demo 中的 main.c 源码很简单，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000482.png)

Demo 源码很简单，就是在 KO 加载的时候打印 "Hello modules on BiscuitOS" 字符串。开发者使用如下命令进行源码编译，并在 BiscuitOS 上运行:

{% highlight bash %}
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000483.png)

以上便是一个最基础的 "人类知识共同体" 计划实践.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

#### HKC 计划实践 -- 内核源码树

> - [实践准备](#C1000)
>
> - [实践部署](#C1001)
>
> - [实践执行](#C1002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C1000">实践准备</span>

BiscuitOS 支持多种架构、多种内核版本的实践，开发者可以根据需要自行选择，这里以 i386 架构的 linxu
 5.0 为例子进行讲解，其余开发者可以在如下链接中进行参考:

> - [BiscuitOS linux 5.0 i386 环境部署](https://biscuitos.github.io/blog/Linux-5.0-i386-Usermanu
al/)
>
> - [BiscuitOS linux 环境部署汇总](https://biscuitos.github.io/blog/Kernel_Build/)

--------------------------------------------

#### <span id="C1001">实践部署</span>

开发环境部署完毕之后，接下来是使用 BiscuitOS 代码库中的代码，这里以一个 i386 架构的 Demo 做为介绍，其余架构与之类似:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] BiscuitOS Demo Code  --->
          [*] Kernel Demo Code on BiscuitOS  --->

make
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-kernel-0.0.1/
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000477.png)

开发者使用如下命令部署所需的环境，正如上图，选择 "Kernel Demo Code on  BiscuitOS" 选项，保存并退出配置，接着使用 make 进行部署，部署成功之后，进入部署的目录, 接着获取 Demo 源码使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-kernel-0.0.1/
make download
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000478.png)

通过上面的命令之后，BiscuitOS 会自动部署一个 Demo 程序，其中 BiscuitOS-kernel-0.0.1 目录下的 main.c 函数就是源码的位置，同级的 Makefile 就是源码编译描述。Demo 中的 main.c 源码很简单，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000479.png)

Demo 源码很简单，就是在 "device_initcall" 阶段调用 "BiscuitOS_init()" 函数，并打印 "Hello Biscui
tOS on  kernel." 字符串。开发者使用如下命令进行源码编译，并在 BiscuitOS 上运行:

{% highlight bash %}
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000480.png)

从内核启动 log 中看到打印的 "Hello BiscuitOS on  kernel", 至此 Demo 程序演示完毕.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

#### HKC 计划实践 -- 应用程序

> - [实践准备](#C2000)
>
> - [实践部署](#C2001)
>
> - [实践执行](#C2002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C2000">实践准备</span>

BiscuitOS 支持多种架构、多种内核版本的实践，开发者可以根据需要自行选择，这里以 i386 架构的 linxu
 5.0 为例子进行讲解，其余开发者可以在如下链接中进行参考:

> - [BiscuitOS linux 5.0 i386 环境部署](https://biscuitos.github.io/blog/Linux-5.0-i386-Usermanu
al/)
>
> - [BiscuitOS linux 环境部署汇总](https://biscuitos.github.io/blog/Kernel_Build/)

--------------------------------------------

#### <span id="C2001">实践部署</span>

开发环境部署完毕之后，接下来是使用 BiscuitOS 代码库中的代码，这里以一个 i386 架构的 Demo 做为介绍，其余架构与之类似:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] BiscuitOS Demo Code  --->
          [*] Application Demo Code on BiscuitOS  --->
          [*] Application Project Demo on BiscuitOS  --->
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000477.png)

开发者使用如下命令部署所需的环境，正如上图，选择 "Application Demo Code on BiscuitOS" 选项和 "Application Project Demo on BiscuitOS"，保存并退出配置，"Application Demo Code on BiscuitOS" 选项是一个简单的应用程序 Demo，使用 make 进行部署，部署成功之后，进入部署的目录, 接着获取 Demo 源码使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-Application-0.0.1/
make download
tree
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000484.png)

通过上面的命令之后，BiscuitOS 会自动部署一个 Demo 程序，其中 BiscuitOS-Application-0.0.1 目录下的 main.c 函数就是源码的位置，同级的 Makefile 就是源码编译描述。Demo 中的 main.c 源码很简单，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000485.png)

Demo 源码很简单，就是应用程序运行的时候打印 "Hello Application Demo on BiscuitOS" 字符串。开发者使用如下命令进行源码编译，并在 BiscuitOS 上运行:

{% highlight bash %}
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000486.png)

BiscuitOS 启动之后，运行应用程序后看到打印相应的字符串, 至此 Demo 程序演示完毕。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000001"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### register_shrinker/unregister_shrinker

register_shrinker()/unregister_shrinker() 函数用于向 SLAB 维护的 shrinker_list 链表上注册 "struct shrinker" 节点，当系统调用 drop_slab() 函数收缩 SLAB 内存的时候，系统就会遍历到 register_shrinker() 注册的函数进行数据的统计.

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过，在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] MMU Shrink  --->
        [*] register_shrink/unregister_shrink  --->
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C1)

###### 通用例程

{% highlight c %}
/*
 * BiscuitOS Kernel BiscuitOS Code
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

/* Shrink interface */
#include <linux/mm.h>
#include <linux/shrinker.h>

extern void drop_slab(void);
unsigned long BiscuitOS_free_pages = 0x1000;
unsigned long BiscuitOS_used_pages = 0x200;

static unsigned long
mmu_shrink_count_bs(struct shrinker *shrink, struct shrink_control *sc)
{
        printk("BiscuitOS Count...\n\n\n");
        return BiscuitOS_used_pages;
}

static unsigned long
mmu_shrink_scan_bs(struct shrinker *shrink, struct shrink_control *sc)
{
        printk("BiscuitOS Scan... \n\n\n");
        return BiscuitOS_free_pages;
}

static struct shrinker mmu_shrinker_bs = {
        .count_objects = mmu_shrink_count_bs,
        .scan_objects  = mmu_shrink_scan_bs,
        .seeks = DEFAULT_SEEKS * 10,
};

/* Shrink */
static int __init BiscuitOS_shrink_init(void)
{
        register_shrinker(&mmu_shrinker_bs);

        return 0;
}

/* kernel entry on initcall */
static int __init BiscuitOS_init(void)
{
        printk("Hello BiscuitOS on kernel.\n");

        drop_slab();

        /* unregister */
        unregister_shrinker(&mmu_shrinker_bs);
        return 0;
}

fs_initcall(BiscuitOS_shrink_init);
device_initcall(BiscuitOS_init);
{% endhighlight %}

内核启动过程中打印如下:

{% highlight bash %}
Block layer SCSI generic (bsg) driver version 0.4 loaded (major 251)
io scheduler mq-deadline registered
io scheduler kyber registered
Hello BiscuitOS on kernel.
BiscuitOS Count...


input: Power Button as /devices/LNXSYSTM:00/LNXPWRBN:00/input/input0
ACPI: Power Button [PWRF]
Serial: 8250/16550 driver, 4 ports, IRQ sharing enabled
00:05: ttyS0 at I/O 0x3f8 (irq = 4, base_baud = 115200) is a 16550A
Non-volatile memory driver v1.3
Linux agpgart interface v0.103
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Hello BiscuitOS Application

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

功能介绍

###### BiscuitOS 配置

在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Assembly  --->
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C)

###### 通用例程

{% highlight bash %}

{% endhighlight %}



![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [Linux Kernel Driver Data Base](https://cateee.net/lkddb/web-lkddb/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)
