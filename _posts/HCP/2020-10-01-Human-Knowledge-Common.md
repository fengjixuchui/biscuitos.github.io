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

> - [HKC 计划介绍](#A)
>
> - [HKC 计划使用](#B)
>
> - [HKC 计划实践](#C)
>
> - [HKC 贡献者名单](#D)

<span id="H000016"></span>
> - Assembly
>
>   - X86/X85_64 Assembly
>
>     - [RDMSR](#H000002)
>
>     - [RDTSC](#H000011)
>
>     - [WRMSR](#H000003)
>
> - Bitmap
>
>   - [Emulate PID Allocating and Releasing](#H000014)
>
>   - [LC-trie Protocol](#H000015)
>
> - CPUMASK
>
>   - [kernel cpumask Demo](#H000005)
>
> - MMU Shrinker
>
>   - [kernel slab shrink Demo](#H000001)
>
> - Notifier mechanism
>
>   - [kernel notifier Demo](#H000004)
>
>   - [Reboot notifier](#H000008)
>
>   - [MMU notifier](#H000012)
>
> - Platform Device Driver
>
>   - [Platform Simple Device Driver](#H000013)
>
> - Power manager
>
>   - [Trigger resume notifier](#H000010)
>
>   - [Trigger suspend notifier](#H000009)
>
> - SMP
>
>   - [on_each_cpu](#H000007)
>
>   - [smp_call_function_single](#H000006)
>
> - [附录](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000V.jpg)

#### HKC 计划介绍

使用过 BiscuitOS 的开发者应该使用过 BiscuitOS 快速开发框架，开发者只需通过图形化的界面勾选所需的项目，那么 BiscuitOS 可以自动部署对应的项目，并可以在 BiscuitOS 上快速执行项目。设想一下，如果提供一个足够充足的 BiscuitOS 项目库，开发者在开发过程中需要进行某些功能模块的验证或使用，只需简单的配置一下 BiscuitOS，那么 BiscuitOS 就会快速搭建所需项目的开发环境，并支持快速跨平台验证，这样将大大提高一线工程师、学生研究学者、极客爱好者开发进度，因此这里提出了面向程序员的 "人类知识共同体" 计划。

"人类知识共同体" 计划面向每个开发者，提供一套完整的生态逻辑，开发者可以提供自己的 "独立代码" 到 BiscuitOS 代码库里，也可自由使用 biscuitOS 代码库里面的代码，相辅相成，共同促进生态的可持续发展.因此本文为开发者介绍加入和使用 "人类知识共同体" 计划。如果开发者需要使用 "人类知识共同体" 计划，请参考如下:

> - [HKC 计划实践](#C)

如果开发者想向 "人类知识共同体" 计划提供 "独立代码"，请参考:

> - [HKC 计划使用](#B)

项目捐赠，捐赠资金将捐赠给我的一位患白血病的小学同学. (下图是微信支付和支付宝支付)

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
        [*] kernel slab shrink Demo  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/kernel-slab-shrink-base-0.0.1
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

在例子中，BiscuitOS_shrink_init() 会在 "fs_initcall" 阶段调用，而 BiscuitOS_init() 函数则在 "device_initcall" 阶段才调用，因此 BiscuitOS_shrink_init() 函数先执行，而 BiscuitOS_init() 函数后执行。

在 BiscuitOS_shrink_init() 函数中先调用 register_shrinker() 函数向 slab shrink 消息链上注册监听，只要系统 slab 发出 shrink 消息，那么就会调用到 mmu_shrinker_bs 注册的接口函数。在 BiscuitOS_init() 函数中，主动通过 drop_slab() 函数发出 slab shrink 的消息，那么内核启动过程中打印如下:

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

从内核启动的信息来看，当调用 drop_slab 的时候，mmu_shrinker_bs 接收到了消息，并调用 mmu_shrink_count_bs() 函数执行相关的操作.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000002"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### RDMSR

RDMSR 指令用于读取一个 64 bit 的 MSR 寄存器. RDMSR 指令通过向 ECX 寄存器写入指定 MSR 寄存器的地址，RDMSR 指令便会将指定 MSR 寄存器的 64 位值存储到 EDX:EAX, 其中 EDX 寄存器存储指定 MSR 寄存器的高 32 bit 的值，而 EAX 寄存器则存储指定 MSR 寄存器的低 32 bit 的值。

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过，在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Assembly  --->
        [*] X86/i386/X64 Assembly  --->
            [*] RDMSR  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/rdmsr-x86-0.0.1/
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * RDMSR [X86/i386/X86_64] -- Read from MSR register.
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

/*
 * Both i386 and x86_64 returns 64-bit value in edx:eax, but gcc's "A"
 * constraint has different meanings. For i386, "A" means exactly
 * edx:eax, while for x86_64 it doesn't mean rdx:rax or edx:eax. Instead
 * it means rax *or* rdx.
 */
#ifdef CONFIG_X86_64
#define DECLARE_ARGS(val, low, high)		unsigned low, high
#define EAX_EDX_VAL(val, low, high)		((low) | ((u64)(high) << 32))
#define EAX_EDX_ARGS(val, low, high)		"a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high)		"=a" (low), "=d" (high)
#else
#define DECLARE_ARGS(val, low, high)		unsigned long long val
#define EAX_EDX_VAL(val, low, high)		(val)
#define EAX_EDX_ARGS(val, low, high)		"A" (val)
#define EAX_EDX_RET(val, low, high)		"=A" (val)
#endif

static inline unsigned long long native_read_msr_bs(unsigned int msr)
{
	DECLARE_ARGS(val, low, high);

	asm volatile("rdmsr"
		    : EAX_EDX_RET(val, low, high)
		    : "c" (msr));
	return EAX_EDX_VAL(val, low, high);
}

#define rdmsr_bs(msr, low, high)			\
do {							\
	u64 __val = native_read_msr_bs((msr));		\
	(void)((low) = (u32)__val);			\
	(void)((high) = (u32)(__val >> 32));		\
} while (0)

#define MSR_IA32_SYSENTER_CS		0x00000174

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
	unsigned int host_cs, junk;

	/* RDMSR */
	rdmsr_bs(MSR_IA32_SYSENTER_CS, host_cs, junk);
	printk("MSR_IA32_SYSENTER_CS: %#lx-%lx\n", host_cs, junk);

	return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

上面的例子很简单，就是通过内嵌汇编的方式在内核模块中调用 RDMSR 指令。值得注意的是，在 X86_64 内嵌汇编中，GCC "A" 标志不代表 "EDX:EAX" 的组合，而在 X86 内嵌汇编中，GCC "A" 标志表示 "EDX:EAX" 的组合，因此使用时应该做区分. 程序运行时的结果如下:

{% highlight bash %}
cd /lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # ls
rdmsr-x86-0.0.1.ko
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # insmod rdmsr-x86-0.0.1.ko 
rdmsr_x86_0.0.1: loading out-of-tree module taints kernel.
MSR_IA32_SYSENTER_CS: 0x60-0
/lib/modules/5.0.0/extra #
{% endhighlight %}

更多 RDMSR 请参考 "Intel Development Manual", 更多 MSR 寄存器请参考手册 "Volume 4: Model-Specific Registers: Chapter 2 Model-Specific Registers (MSRs)"

> [Intel Development Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/Intel/Intel-IA32_DevelopmentManual.pdf)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000003"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### WRMSR

WRMSR 指令用于将一个 64bit 的值写入指定的 MSR 寄存器中。WRMSR 指令通过将指定寄存器的地址写入 ECX 寄存器中，然后将写入值的高 32bit 写入 EDX 寄存器，然后将写入值的低 32bit 写入 EAX 寄存器中，最后调用 "WRMSR" 指令进行写入操作。

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Assembly  --->
        [*] X86/i386/X64 Assembly  --->
            [*] WRMSR  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/wrmsr-x86-0.0.1/
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight bash %}
/*
 * WRMSR [X86/X86_64] -- Write to Model Specific Register
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define IA32_PQR_ASSOC		0x0c8f

static inline void wrmsr_bs(unsigned msr, unsigned low, unsigned high)
{
	asm volatile ("wrmsr"
		      :
		      : "c" (msr), "a" (low), "d" (high)
		      : "memory");
}

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
	printk("Hello modules on BiscuitOS\n");

	/* WRMSR */
	wrmsr_bs(IA32_PQR_ASSOC, 0, 0);

	return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

上面的例子很简单，就是通过内嵌汇编的方式将 MSR 寄存器的值通过 "c" 写入到 ECX 寄存器中，然后通过 "a" 将 64bit 值的低 32bit 写入到 EAX 寄存器，通过 "d" 将 64bit 值写入到 EDX 寄存器。最终使用 "memory" 标记，以此告诉编译器不要将用到的变量缓存到寄存器，因为这段代码可能会用到内存变量，而这些内存变量会以不可预知的方式发生改变，因此 GCC 插入必要的代码先将缓存到寄存器的变量值写回内存，如果后面又访问这些变量，需要重新访问内存. 上面的实例在 BiscuitOS 运行的结果如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
wrmsr-x86-0.0.1.ko
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # insmod wrmsr-x86-0.0.1.ko 
wrmsr_x86_0.0.1: loading out-of-tree module taints kernel.
Hello modules on BiscuitOS
general protection fault: 0000 [#1] SMP
CPU: 1 PID: 1142 Comm: insmod Tainted: G           O      5.0.0 #61
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.12.0-0-ga698c8995f-p4
EIP: BiscuitOS_init+0x1b/0x1000 [wrmsr_x86_0.0.1]
Code: Bad RIP value.
EAX: 00000000 EBX: e0a5c000 ECX: 00000c8f EDX: 00000000
ESI: e0a5f000 EDI: cb59c0e0 EBP: cd3c9df8 ESP: cd3c9df4
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00010246
CR0: 80050033 CR2: e0a5eff1 CR3: 0ef54000 CR4: 001406d0
Call Trace:
 do_one_initcall+0x42/0x191
 ? call_usermodehelper_exec+0x87/0x170
 ? free_pcp_prepare+0x4f/0xa0
 ? _cond_resched+0x17/0x40
 ? kmem_cache_alloc_trace+0x35/0x160
 ? do_init_module+0x21/0x1e4
 do_init_module+0x50/0x1e4
 load_module+0x1d46/0x2280
 ? map_vm_area+0x2c/0x40
 sys_init_module+0x10d/0x150
 do_fast_syscall_32+0x7a/0x1c0
 entry_SYSENTER_32+0x6b/0xbe
EIP: 0xb7f3b871
Code: 8b 98 58 cd ff ff 85 d2 89 c8 74 02 89 0a 5b 5d c3 8b 04 24 c3 8b 14 24 c3 8b 1c6
EAX: ffffffda EBX: 09ab6390 ECX: 00000aec EDX: 0820dbd6
ESI: 00000003 EDI: 00000000 EBP: bf8b064c ESP: bf8b02e0
DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b EFLAGS: 00000246
Modules linked in: wrmsr_x86_0.0.1(O+)
---[ end trace f75e95a374ac04ef ]---
EIP: BiscuitOS_init+0x1b/0x1000 [wrmsr_x86_0.0.1]
Code: Bad RIP value.
EAX: 00000000 EBX: e0a5c000 ECX: 00000c8f EDX: 00000000
ESI: e0a5f000 EDI: cb59c0e0 EBP: cd3c9df8 ESP: dd06049c
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00010246
CR0: 80050033 CR2: e0a5eff1 CR3: 0ef54000 CR4: 001406d0
insmod (1142) used greatest stack depth: 6600 bytes left
Segmentation fault
/lib/modules/5.0.0/extra # 
{% endhighlight %}

更多 WRMSR 请参考 "Intel Development Manual", 更多 MSR 寄存器请参考手册 "Volume 4: Model-Specific Registers: Chapter 2 Model-Specific Registers (MSRs)"

> [Intel Development Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/Intel/Intel-IA32_DevelopmentManual.pdf)

----------------------------------

<span id="H000004"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Kernel notifier Demo

本节用于介绍一个在内核中使用通知链的例子, 内核内核通知机制可以在不同子系统之间进行通知，本例子给出一个最简单的例子实现。


###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Notifier mechanism on Kernel  --->
        [*] Kernel notifier Base Demo  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/kernel-notifier-base-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Kernel notifier
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define BISCUITOS_EVENT_A		0x01
#define BISCUITOS_EVENT_B		0x02
#define BISCUITOS_EVENT_C		0x03

static RAW_NOTIFIER_HEAD(BiscuitOS_chain);

int BiscuitOS_notifier_event(struct notifier_block *nb, 
					unsigned long event, void *v)
{
	switch (event) {
	case BISCUITOS_EVENT_A:
		printk("BiscuitOS notifier event A [%s]\n", (char *)v);
		break;
	case BISCUITOS_EVENT_B:
		printk("BiscuitOS notifier event B [%s]\n", (char *)v);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block BiscuitOS_notifier = {
	.notifier_call = BiscuitOS_notifier_event,
};

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
	printk("Hello modules on BiscuitOS\n");

	/* Register notifier */
	raw_notifier_chain_register(&BiscuitOS_chain, &BiscuitOS_notifier);

	/* Notifier */
	raw_notifier_call_chain(&BiscuitOS_chain, 
					BISCUITOS_EVENT_B, "BiscuitOS");
	raw_notifier_call_chain(&BiscuitOS_chain,
					BISCUITOS_EVENT_A, "Buddy");
	raw_notifier_call_chain(&BiscuitOS_chain,
					BISCUITOS_EVENT_C, "Memory");

	return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
	/* Unregister notifier */
	raw_notifier_chain_unregister(&BiscuitOS_chain, &BiscuitOS_notifier);
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在例子中，函数通过调用 "raw_notifier_chain_register()" 函数注册了一条名为 "BiscuitOS_chain" 的通知链，当接收到通知之后对应的处理函数通过 BiscuitOS_notifier 对应的数据进行处理，其最终对应 "BiscuitOS_notifier_event()" 函数，当接收到通知之后，内核调用该函数，函数内核对不同的消息进行处理，这里定义了两个消息: "BISCUITOS_EVENT_A" 和 "BISCUITOS_EVENT_B", 当 BiscuitOS_chain 消息链接受到其中一条信息都会进行相应的处理，而对于没有定义的消息，那么则忽略。

内核可以在其他子系统通过调用 "raw_notifier_call_chain()" 向指定消息链发消息，例如本例子中，通过调用该函数一共发送三个信息，并传入指定的内容. 通过上面的代码逻辑构建了一个内核最简单的消息通知机制. 本例子在 BiscuitOS 中运行的结果如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # ls
kernel-notifier-base-0.0.1.ko
/lib/modules/5.0.0/extra # insmod kernel-notifier-base-0.0.1.ko 
kernel_notifier_base_0.0.1: loading out-of-tree module taints kernel.
Hello modules on BiscuitOS
BiscuitOS notifier event B [BiscuitOS]
BiscuitOS notifier event A [Buddy]
/lib/modules/5.0.0/extra #
{% endhighlight %}

在 BiscuitOS 运行模块之后，可以看到 "BiscuitOS_chain" 消息链接受到了 "BISCUITOS_EVENT_A" 和 "BISCUITOS_EVENT_B" 消息，并打印伴随消息传递过来的内容，对于 "BISCUITOS_EVENT_C" 则选择忽略.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000005"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Kernel cpumask Demo

CPUMASK 机制用于标记 CPU 的使用情况，在 SMP 系统中，系统在维护某个功能时需要统计指定 CPU 使用某个功能的记录，或者用于标记指定的 CPU 以便得到一个白名单或者黑名单，以此可以通过 MASK 隔离指定的 CPU。内核提供了一套 CPUMASK 机制来实现各种场景下的功能。

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] CPUMASK mechanism on Kernel  --->
        [*] Kernel cpumask Base Demo  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/kernel-cpumask-base-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * CPU mask mechanism
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>

/* CPUmask */
#include <linux/cpumask.h>

/* declear cpumask */
static cpumask_var_t BiscuitOS_cpumask;

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
	int cpu = raw_smp_processor_id();

	printk("BiscuitOS current cpu %d\n", cpu);
	/* alloc cpumask */
	zalloc_cpumask_var(&BiscuitOS_cpumask, GFP_KERNEL);

	/* test and set */
	if (!cpumask_test_cpu(cpu, BiscuitOS_cpumask)) {
		printk("CPUMASK set cpu %d\n", cpu);
		cpumask_set_cpu(cpu, BiscuitOS_cpumask);
	}

	/* test and clear */
	if (cpumask_test_cpu(cpu, BiscuitOS_cpumask)) {
		printk("CPUMASK clear cpu %d\n", cpu);
		cpumask_clear_cpu(cpu, BiscuitOS_cpumask);
	}

	return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
	/* free cpumask */
	free_cpumask_var(BiscuitOS_cpumask);
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在例子中，首先定义了一个 cpumask_var_t 的变量 BiscuitOS_cpumask, 然后调用 zalloc_cpumask_alloc() 函数为其分配相应的内存，分配完毕之后该 CPUMASK 就可以使用了。这里调用 raw_smp_processor_id() 函数获得当前 CPU ID，然后判断该 CPU 是否已经在 BiscuitOS_cpumask 中置位，如果没有那么调用 cpumask_set_cpu() 将该 CPU 在 BiscuitOS_cpumask 中置位，这样做的目的是以后可以检测针对特定 CPU 的白名单/黑名单. 例子中在检测到对应的 CPU 置位之后，调用 cpumask_clear_cpu() 函数将 CPU 从 BiscuitOS_cpumask 中移除，这样做的目的是以后可以将 CPU 从白名单/黑名单中除名. 该实例在 SMP 4 core 的情况下，BiscuitOS 中运行的情况如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
kernel-cpumask-base-0.0.1.ko
/lib/modules/5.0.0/extra # insmod kernel-cpumask-base-0.0.1.ko 
kernel_cpumask_base_0.0.1: loading out-of-tree module taints kernel.
BiscuitOS current cpu 0
CPUMASK set cpu 0
CPUMASK clear cpu 0
/lib/modules/5.0.0/extra #
{% endhighlight %}

从运行的结果可以看出，当前 CPU ID 是 0，第一次检测的时候，CPU 没有在 BiscuitOS_cpumask 中置位，那么将其置位。第二次检测的时候，CPU 已经在 BiscuitOS_cpumask 中置位，那么将其清零.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000006"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### smp_call_function_single

smp_call_function_single() 函数用于在指定的 CPU 上运行指定的函数，其函数原型如下:

{% highlight c %}
/*
 * smp_call_function_single - Run a function on a specific CPU
 * @func: The function to run. This must be fast and non-blocking.
 * @info: An arbitrary pointer to pass to the function.
 * @wait: If true, wait until function has completed on other CPUs.
 *
 * Returns 0 on success, else a negative status code.
 */
int smp_call_function_single(int cpu, smp_call_func_t func, void *info, int wait)
{% endhighlight %}

cpu 指定了需要运行的 CPU ID，func 参数则是指向需要执行的函数，info 指向需要向执行函数传入的数据，wait 用于指明是否等待原 CPU 执行完毕之后再去其他 CPU 执行。

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] SMP  --->
        [*] smp_call_function_single  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/smp_call_function_single-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Run a function on a specific CPU
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

void BiscuitOS_smp(void *rtn)
{
	int cpu = raw_smp_processor_id();

	printk("SMP running on CPU%d - Default CPU%d\n", cpu, (int)rtn);
}

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
	int cpu = 0;
	int current_cpu = raw_smp_processor_id();

	/* Call function on specific CPU */
	smp_call_function_single(cpu, BiscuitOS_smp, (void *)current_cpu, 0);

	printk("Hello modules on BiscuitOS\n");

	return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在例子中，函数调用 smp_call_function_single() 函数在指定的 CPU 0 上运行 BiscuitOS_smp() 函数，并向 BiscuitOS_smp() 函数传递了原始 CPU ID。其在 BiscuitOS 上运行的结果如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
smp_call_function_single-0.0.1.ko
/lib/modules/5.0.0/extra # insmod smp_call_function_single-0.0.1.ko 
smp_call_function_single_0.0.1: loading out-of-tree module taints kernel.
SMP running on CPU0 - Default CPU1
Hello modules on BiscuitOS
/lib/modules/5.0.0/extra #
{% endhighlight %}

函数运行在 CPU 1 上，然后调用 smp_call_function_single() 函数让 BiscuitOS_smp() 函数运行在 CPU 0 上.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000007"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### on_each_cpu

on_each_cpu() 函数用于将指定函数在没有 CPU 上运行，其函数原型如下:

{% highlight c %}
/*
 * Call a function on all processors.  May be used during early boot while
 * early_boot_irqs_disabled is set.  Use local_irq_save/restore() instead
 * of local_irq_disable/enable().
 */ 
int on_each_cpu(void (*func) (void *info), void *info, int wait)
{% endhighlight %}

func 为指定在每个 CPU 上运行的函数，info 为传入调用函数的数据，wait 参数用于指定是否等待当前 CPU 任务执行完毕之后在执行.

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] SMP  --->
        [*] on_each_cpu  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/on_each_cpu-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Call a function on all processors
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

void BiscuitOS_smp(void *info)
{
        int cpu = raw_smp_processor_id();

        printk("Current CPU%d Default CPU%d\n", cpu, (int)info);
}

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
        int cpu = raw_smp_processor_id();

        /* Call function on all processor */
        on_each_cpu(BiscuitOS_smp, (void *)cpu, 0);

        printk("Hello modules on BiscuitOS\n");

        return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在本例子中，函数定义了一个函数 BiscuitOS_smp，并调用 on_each_cpu() 函数让 BiscuitOS_smp() 函数在所有的 CPU 上运行。其在 BiscuitOS 上运行的情况如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
on_each_cpu-0.0.1.ko
/lib/modules/5.0.0/extra # insmod on_each_cpu-0.0.1.ko 
on_each_cpu_0.0.1: loading out-of-tree module taints kernel.
Current CPU1 Default CPU1
Current CPU0 Default CPU1
Hello modules on BiscuitOS
/lib/modules/5.0.0/extra #
{% endhighlight %}

从运行的结果可以看出 BiscuitOS_smp() 函数在所有的 CPU 上都运行了一次.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000008"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### reboot-notifier

Reboot notifier 用于当系统 reboot 的时候为注册 register_reboot_notifier() 的函数发送 REBOOT 消息。当监听者收到 REBOOT 消息之后就执行对应的函数. Reboot notifier 机制提供了 register_reboot_notifier()/unregister_reboot_notifier() 函数用于注册和撤销 REBOOT 监听事件.

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Notifier mechanism on Kernel  --->
        [*] Reboot notifier  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/reboot-notifier-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Reboot notifier
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* reboot notifier */
#include <linux/reboot.h>

static int BiscuitOS_reboot_notifier(struct notifier_block *notifier,
                                                unsigned long val, void *v)
{
        printk("Trigger reboot on BiscuitOS.\n");
        return 0;
}

static struct notifier_block BiscuitOS_reboot = {
        .notifier_call = BiscuitOS_reboot_notifier,
        .priority = 0,
};

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
        printk("Hello modules on BiscuitOS\n");

        /* Register reboot notifier */
        register_reboot_notifier(&BiscuitOS_reboot);

        return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
        unregister_reboot_notifier(&BiscuitOS_reboot);
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在本例子中，调用 register_reboot_notifier() 函数注册了一个监听事件，当系统发生 REBOOT 事件之后，BiscuitOS_reboot_notifier() 函数就会被执行。当模块卸载的时候，调用 unregister_reboot_notifier() 函数撤销监听事件. 其在 BiscuitOS 上执行情况如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
reboot-notifier-0.0.1.ko
/lib/modules/5.0.0/extra # insmod reboot-notifier-0.0.1.ko 
reboot_notifier_0.0.1: loading out-of-tree module taints kernel.
Hello modules on BiscuitOS

~ # 
~ # reboot
~ # umount: devtmpfs busy - remounted read-only
EXT4-fs (ram0): re-mounted. Opts: (null)
The system is going down NOW!
Sent SIGTERM totelnetd (1129) used greatest stack depth: 6172 bytes left
 all processes
Sent SIGKILL to all processes
Requesting system reboot
sh (1131) used greatest stack depth: 6144 bytes left
Unregister pv shared memory for cpu 1
Unregister pv shared memory for cpu 0
Trigger reboot on BiscuitOS.
reboot: Restarting system
reboot: machine restart
{% endhighlight %}

在 BiscuitOS 上安装上模块之后，执行 reboot 命令，系统在准备 reboot 过程中调用了 BiscuitOS_reboot_notifier() 函数，并打印了字符串 "Trigger reboot on BiscuitOS.".

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000009"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Trigger suspend notifier

内核 syscore 子系统提供了 register_syscore_ops() 函数，用于向系统的 suspend 注册监听事件，当系统进行 SUSPEND 状态，那么调用指定的函数处理 suspend 消息.

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Power Manager  --->
        [*] Trigger Suspend Demo  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/suspend-base-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Syscore suspend/resume
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/syscore_ops.h>

static int BiscuitOS_suspend(void)
{
        printk("Trigger suspend on BiscuitOS\n");
        return 0;
}

static void BiscuitOS_resume(void)
{
        printk("Trigger resume on BiscuitOS.\n");
}

static struct syscore_ops BiscuitOS_syscore = {
        .suspend = BiscuitOS_suspend,
        .resume  = BiscuitOS_resume,
};

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
        printk("Hello modules on BiscuitOS\n");

        register_syscore_ops(&BiscuitOS_syscore);    

        return 0;
}

/* Module exit entry */ 
static void __exit BiscuitOS_exit(void)
{
        unregister_syscore_ops(&BiscuitOS_syscore);
}   

module_init(BiscuitOS_init); 
module_exit(BiscuitOS_exit);
    
MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在本例子中，驱动通过调用 register_syscore_ops() 注册了 SUSPEND/RESUME 的监听事件 BiscuitOS_syscore, 并提供两个接口，当系统进行 SUSPEND 状态，那么 BiscuitOS_suspend() 函数则会被调用，如果系统从 SUSPEND 状态恢复为 RESUME 状态，那么 BiscuitOS_resume() 函数则会别调用. 当驱动卸载的时候，会调用 unregister_syscore_ops() 函数撤销监听事件. 本例子在 BiscuitOS 上运行的情况如下:

{% highlight bash %}
~ # 
~ # cd /lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
reboot-notifier-0.0.1.ko  suspend-base-0.0.1.ko
/lib/modules/5.0.0/extra # insmod suspend-base-0.0.1.ko
suspend_base_0.0.1: loading out-of-tree module taints kernel.
Hello modules on BiscuitOS
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # cd /sys/power/
/sys/power # echo disk > state
PM: hibernation entry
PM: Syncing filesystems ... 
PM: done.
Freezing user space processes ... (elapsed 0.001 seconds) done.
OOM killer disabled.
PM: Marking nosave pages: [mem 0x00000000-0x00000fff]
PM: Marking nosave pages: [mem 0x0009f000-0x000fffff]
PM: Basic memory bitmaps created
PM: Preallocating image memory... done (allocated 47593 pages)
PM: Allocated 190372 kbytes in 0.01 seconds (19037.20 MB/s)
Freezing remaining freezable tasks ... (elapsed 0.001 seconds) done.
printk: Suspending console(s) (use no_console_suspend to debug)
ACPI: Preparing to enter system sleep state S4
PM: Saving platform NVS memory
Disabling non-boot CPUs ...
Unregister pv shared memory for cpu 1
smpboot: CPU 1 is now offline
Trigger suspend on BiscuitOS
PM: Creating hibernation image:
PM: Need to copy 47276 pages
PM: Normal pages needed: 47276 + 1024, available pages: 83650
PM: Hibernation image created (47276 pages copied)
kvm-clock: cpu 0, msr 1ba6b001, primary cpu clock, resume
PM: Restoring platform NVS memory
Trigger resume on BiscuitOS.
Enabling non-boot CPUs ...
x86: Booting SMP configuration:
smpboot: Booting Node 0 Processor 1 APIC 0x1
Initializing CPU#1
kvm-clock: cpu 1, msr 1ba6b041, secondary cpu clock
KVM setup async PF for cpu 1
kvm-stealtime: cpu 1, msr 1f78fa40
 cache: parent cpu1 should not be sleeping
CPU1 is up
ACPI: Waking up from system sleep state S4
PM: Cannot find swap device, try swapon -a
PM: Cannot get swap writer
PM: Basic memory bitmaps freed
OOM killer enabled.
Restarting tasks ... done.
PM: hibernation exit
sh: write error: No such device
/sys/power # ata2.01: NODEV after polling detection
e1000: eth0 NIC Link is Up 1000 Mbps Full Duplex, Flow Control: RX
{% endhighlight %}

当向系统安装完驱动之后，在 /sys/power/ 目录下执行如下命令:

{% highlight bash %}
echo disk > /sys/power/state
{% endhighlight %}

执行上面的操作之后，将运行状态数据存到硬盘, 然后关机, 唤醒最慢。以上动作便会触发系统进入 SUSPEND 状态，此时触发驱动的 BiscuitOS_suspend() 函数。当有数据写入磁盘时候，系统又由 SUSPEND 状态进入 RESUME 状态，此时触发驱动的 BiscuitOS_resume() 函数。系统支持的 4 中休眠设置，如下:

{% highlight bash %}
1. freeze  冻结 I/O 设备, 将它们置于低功耗状态,使处理器进入空闲状态, 唤醒最快, 耗电比其它 standby、mem、disk 方式高
2. standby 除了冻结 I/O 设备外,还会暂停系统,唤醒较快,耗电比其它 mem、disk 方式高
3. mem     将运行状态数据存到内存, 并关闭外设, 进入等待模式, 唤醒较慢, 耗电比 disk 方式高
4. disk    将运行状态数据存到硬盘, 然后关机, 唤醒最慢
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000010"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Trigger resume notifier

内核 syscore 子系统提供了 register_syscore_ops() 函数，用于向系统的 suspend 注册监听事件，当系统进行 SUSPEND 状态，然后从 SUSPEND 中唤醒为 RESUME 状态，那么调用指定的函数处理 resume 消息.

###### BiscuitOS 配置

本实例已经在 Linux 5.0 i386 架构上验证通过, 在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Power Manager  --->
        [*] Trigger Resume Demo  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/resume-base-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Syscore suspend/resume
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/syscore_ops.h>

static int BiscuitOS_suspend(void)
{
        printk("Trigger suspend on BiscuitOS\n");
        return 0;
}

static void BiscuitOS_resume(void)
{
        printk("Trigger resume on BiscuitOS.\n");
}

static struct syscore_ops BiscuitOS_syscore = {
        .suspend = BiscuitOS_suspend,
        .resume  = BiscuitOS_resume,
};

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
        printk("Hello modules on BiscuitOS\n");

        register_syscore_ops(&BiscuitOS_syscore);    

        return 0;
}

/* Module exit entry */ 
static void __exit BiscuitOS_exit(void)
{
        unregister_syscore_ops(&BiscuitOS_syscore);
}   

module_init(BiscuitOS_init); 
module_exit(BiscuitOS_exit);
    
MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在本例子中，驱动通过调用 register_syscore_ops() 注册了 SUSPEND/RESUME 的监听事件 BiscuitOS_syscore, 并提供两个接口，当系统进行 SUSPEND 状态，那么 BiscuitOS_suspend() 函数则会被调用，如果系统从 SUSPEND 状态恢复为 RESUME 状态，那么 BiscuitOS_resume() 函数则会别调用. 当驱动卸载的时候，会调用 unregister_syscore_ops() 函数撤销监听事件. 本例子在 BiscuitOS 上运行的情况如下:

{% highlight bash %}
~ # 
~ # cd /lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
reboot-notifier-0.0.1.ko  suspend-base-0.0.1.ko
/lib/modules/5.0.0/extra # insmod suspend-base-0.0.1.ko
suspend_base_0.0.1: loading out-of-tree module taints kernel.
Hello modules on BiscuitOS
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # cd /sys/power/
/sys/power # echo disk > state
PM: hibernation entry
PM: Syncing filesystems ... 
PM: done.
Freezing user space processes ... (elapsed 0.001 seconds) done.
OOM killer disabled.
PM: Marking nosave pages: [mem 0x00000000-0x00000fff]
PM: Marking nosave pages: [mem 0x0009f000-0x000fffff]
PM: Basic memory bitmaps created
PM: Preallocating image memory... done (allocated 47593 pages)
PM: Allocated 190372 kbytes in 0.01 seconds (19037.20 MB/s)
Freezing remaining freezable tasks ... (elapsed 0.001 seconds) done.
printk: Suspending console(s) (use no_console_suspend to debug)
ACPI: Preparing to enter system sleep state S4
PM: Saving platform NVS memory
Disabling non-boot CPUs ...
Unregister pv shared memory for cpu 1
smpboot: CPU 1 is now offline
Trigger suspend on BiscuitOS
PM: Creating hibernation image:
PM: Need to copy 47276 pages
PM: Normal pages needed: 47276 + 1024, available pages: 83650
PM: Hibernation image created (47276 pages copied)
kvm-clock: cpu 0, msr 1ba6b001, primary cpu clock, resume
PM: Restoring platform NVS memory
Trigger resume on BiscuitOS.
Enabling non-boot CPUs ...
x86: Booting SMP configuration:
smpboot: Booting Node 0 Processor 1 APIC 0x1
Initializing CPU#1
kvm-clock: cpu 1, msr 1ba6b041, secondary cpu clock
KVM setup async PF for cpu 1
kvm-stealtime: cpu 1, msr 1f78fa40
 cache: parent cpu1 should not be sleeping
CPU1 is up
ACPI: Waking up from system sleep state S4
PM: Cannot find swap device, try swapon -a
PM: Cannot get swap writer
PM: Basic memory bitmaps freed
OOM killer enabled.
Restarting tasks ... done.
PM: hibernation exit
sh: write error: No such device
/sys/power # ata2.01: NODEV after polling detection
e1000: eth0 NIC Link is Up 1000 Mbps Full Duplex, Flow Control: RX
{% endhighlight %}

当向系统安装完驱动之后，在 /sys/power/ 目录下执行如下命令:

{% highlight bash %}
echo disk > /sys/power/state
{% endhighlight %}

执行上面的操作之后，将运行状态数据存到硬盘, 然后关机, 唤醒最慢。以上动作便会触发系统进入 SUSPEND 状态，此时触发驱动的 BiscuitOS_suspend() 函数。当有数据写入磁盘时候，系统又由 SUSPEND 状态进入 RESUME 状态，此时触发驱动的 BiscuitOS_resume() 函数。系统支持的 4 中休眠设置，如下:

{% highlight bash %}
1. freeze  冻结 I/O 设备, 将它们置于低功耗状态,使处理器进入空闲状态, 唤醒最快, 耗电比其它 standby、mem、disk 方式高
2. standby 除了冻结 I/O 设备外,还会暂停系统,唤醒较快,耗电比其它 mem、disk 方式高
3. mem     将运行状态数据存到内存, 并关闭外设, 进入等待模式, 唤醒较慢, 耗电比 disk 方式高
4. disk    将运行状态数据存到硬盘, 然后关机, 唤醒最慢
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000011"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### RDTSC

RDTSC 指令是 Intel X86 提供的一个用于从 "Time-Stamp Counter" MSR 寄存器中读取当前系统的时间戳. 从 Pentium 开始，很多 80x86 微处理器引入了 TSC 寄存器，它的每个时钟信息 (CLK, CLK 是微处理器中一条用于接收外部振荡器的时钟信号输入线) 到来时加一. 操作系统可以使用 TSC 寄存器来计算 CPU 主频，比如微处理器的主频是 1MHz，那么 TSC 就会在 1s 内增加 1000000。除了计算 CPU 的主频外，还可以通过 TSC 来测试微处理器其他处理单元的运算能力，具体请参考:

> - [Using the RDTSC Instruction for Performance Monitoring](https://www.ccsl.carleton.ca/~jamuir/rdtscpm1.pdf)

RDTSC 指令执行读取寄存器时，处理器将 "Time-Stamp Counter" MSR 寄存器的高 32bit 的值存储在 EDX 寄存器，而低 32bit 值存储在 EAX 寄存器中。

###### BiscuitOS 配置

在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Assembly  --->
        [*] X86/i386/X64 Assembly  --->
            [*] RDTSC  --->
            [*] RDTSC on Userspace  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/rdtsc-x86-0.0.1
BiscuitOS/output/linux-XXX-YYY/package/rdtsc-app-x86-0.0.1
{% endhighlight %}

具体实践办法请参考:

> [HKC 计划 BiscuitOS 实践框架介绍 (rdtsc-x86-0.0.1)](#C0)
>
> [HKC 计划 BiscuitOS 实践框架介绍 (rdtsc-app-x86-0.0.1)](#C2)

###### 通用例程(内核篇)

{% highlight c %}
/*
 * RDTSC - Read Time-Stamp Counter [x86]
 *
 * (C) 2020.10.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* mdelay */
#include <linux/delay.h>

/*
 * both i386 and x86_64 returns 64-bit value in edx:eax, but gcc's "A"
 * constraint has different meanings. For i386, "A" means exactly
 * edx:eax, while for x86_64 it doesn't mean rdx:rax or edx:eax. Instead,
 * it means rax *or* rdx.
 */
#ifdef CONFIG_X86_64
#define DECLARE_ARGS(val, low, high)    unsigned low, high
#define EAX_EDX_VAL(val, low, high)     ((low) | ((u64)(high) << 32))
#define EAX_EDX_ARGS(val, low, high)    "a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high)     "=a" (low), "=d" (high)
#else
#define DECLARE_ARGS(val, low, high)    unsigned long long val
#define EAX_EDX_VAL(val, low, high)     (val)
#define EAX_EDX_ARGS(val, low, high)    "A" (val)
#define EAX_EDX_RET(val, low, high)     "=A" (val)
#endif

static __always_inline unsigned long long __native_read_tsc(void)
{
        DECLARE_ARGS(val, low, high);

        asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));

        return EAX_EDX_VAL(val, low, high);
}

#define rdtscl(low)     ((low) = (u32)__native_read_tsc())
#define rdtscll(val)    ((val) = __native_read_tsc())

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
        u64 tsc_u64;
        u32 tsc_low;
        u64 tsc_a, tsc_b, result, mod;

        /* Read TSC 64bit contents */
        rdtscll(tsc_u64);
        printk("TSC 64bit: %#llx\n", tsc_u64);
        /* Read TSC low-order 32bit contents */
        rdtscl(tsc_low);
        printk("TSC low-order 32bit: %#x\n", tsc_low);

        /* CPU frequency */
        rdtscll(tsc_a);
        mdelay(1000);
        rdtscll(tsc_b);
        result = tsc_b - tsc_a;
        /* 64bit div */
        mod = do_div(result, 1 * 1024 * 1024);

        printk("CPU %d.%d MHz\n", result, mod);
        printk("Hello modules on BiscuitOS\n");

        return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Common Device driver on BiscuitOS");
{% endhighlight %}

在本例子中，使用内嵌汇编定义了 RDTSC 指令的两个接口函数 "rdtscll()" 和 "rdtscl()". "rdtscl()" 函数只读取 "Time-stamp Counter" 寄存器的低 32bit 值，而 "rdtscll()" 可以读取 64bit 的值。例子中还通过计算 1s 内 "Time-stamp Counter" 的变化，以此计算 CPU 的频率，但这里使用 do_div() 函数用于处理 64bit 的除法。本例子在 BiscuitOS 中运行的结果如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
rdtsc-x86-0.0.1.ko
/lib/modules/5.0.0/extra # insmod rdtsc-x86-0.0.1.ko 
rdtsc_x86_0.0.1: loading out-of-tree module taints kernel.
TSC 64bit: 0xbe7b729e4
TSC low-order 32bit: 0xe7cb5b2d
CPU 3141.0 MHz
Hello modules on BiscuitOS
/lib/modules/5.0.0/extra # 
/lib/modules/5.0.0/extra # cat /proc/cpuinfo | grep model
model		: 60
model name	: Intel(R) Core(TM) i5-4590 CPU @ 3.30GHz
model		: 60
model name	: Intel(R) Core(TM) i5-4590 CPU @ 3.30GHz
/lib/modules/5.0.0/extra #
{% endhighlight %}

加载模块之后，计算出 CPU 的频率是 3141 MHz, 与系统提供的 3.30GHz 有一点相差，但属于正常情况, 因此计算的数值有效.

###### 通用例程(应用程序)

{% highlight c %}
/*
 * RDTSC -- Read Time-Stamp Counter on userspace [x86]
 *
 * (C) 2020.02.02 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
 * both i386 and x86_64 returns 64-bit value in edx:eax, but gcc's "A"
 * constraint has different meanings. For i386, "A" means exactly
 * edx:eax, while for x86_64 it doesn't mean rdx:rax or edx:eax. Instead,
 * it means rax *or* rdx.
 */
#ifdef CONFIG_X86_64
#define DECLARE_ARGS(val, low, high)    unsigned low, high
#define EAX_EDX_VAL(val, low, high)     ((low) | ((u64)(high) << 32))
#define EAX_EDX_ARGS(val, low, high)    "a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high)     "=a" (low), "=d" (high)
#else
#define DECLARE_ARGS(val, low, high)    unsigned long long val
#define EAX_EDX_VAL(val, low, high)     (val)
#define EAX_EDX_ARGS(val, low, high)    "A" (val)
#define EAX_EDX_RET(val, low, high)     "=A" (val)
#endif

static __always_inline unsigned long long __native_read_tsc(void)
{
        DECLARE_ARGS(val, low, high);

        asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));

        return EAX_EDX_VAL(val, low, high);
}

#define rdtscl(low)     ((low) = (u32)__native_read_tsc())
#define rdtscll(val)    ((val) = __native_read_tsc())

typedef unsigned long long u64;
typedef unsigned int u32;

int main()
{
        u64 tsc_u64;
        u32 tsc_low;
        u64 tsc_a, tsc_b;

        /* Read TSC 64bit contents */
        rdtscll(tsc_u64);
        printf("TSC 64bit: %#llx\n", tsc_u64);
        /* Read TSC low-order 32bit contents */
        rdtscl(tsc_low);
        printf("TSC low-order 32bit: %#x\n", tsc_low);

        /* CPU frequency */
        rdtscll(tsc_a);
        sleep(1);
        rdtscll(tsc_b);

        printf("CPU %lld MHz\n", (tsc_b - tsc_a) / 1000000);
        printf("Hello Application Demo on BiscuitOS.\n");
        return 0;
}
{% endhighlight %}

在本例子中，在用户空间定义两个接口用于从 "Time-Stamp Counter" 寄存器中读取当前的时间戳，rdtscl() 函数用于读取 "Time-Stamp Counter" 寄存器的低 32bit 的值，而 rdtscll() 函数则可以读取 "Time-Stamp Counter" 寄存器的 64bit 值。在程序中还通过延时 1s 来计算 CPU 的频率。本例子在 BiscuitOS 的运行情况如下:

{% highlight bash %}
~ # 
~ # rdtsc-app-x86-0.0.1 
TSC 64bit: 0x670b3d3e8
TSC low-order 32bit: 0x70c2ce2d
CPU 3293 MHz
Hello Application Demo on BiscuitOS.
~ # cat /proc/cpuinfo | grep model
model		: 60
model name	: Intel(R) Core(TM) i5-4590 CPU @ 3.30GHz
model		: 60
model name	: Intel(R) Core(TM) i5-4590 CPU @ 3.30GHz
~ # 
{% endhighlight %}

从计算的结果来看非常接近 CPUINFO 提供的主频，因此计算有效.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000012"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### MMU notifier

MMU notifier 机制用于当进程修改页表、或者让页表无效时，用于通知进程的内部这些消息。该机制通过提供很多接口: 

* mmu_notifier_register()/mmu_notifier_unregister() 函数注册事件。
* mmu_notifier_invalidate_range_start()/mmu_notifier_invalidate_range_end() 当进程内某段虚拟地址的页表无效时候，可以调用这些函数进行通知
* mmu_notifier_clear_flush_young()/mmu_notifier_clear_young()/mmu_notifier_test_young() 当某些页被修改，可以调用这些函数进行通知
* mmu_notifier_change_pte()  当进程的 PTE 页表改变时，可以调用该函数进行通知。

###### BiscuitOS 配置

在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Notifier mechanism on Kernel  --->
        -*-   MMU notifier (Kernel Portion)  --->
        [*]   MMU notifier (Userspace Portion)  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/MMU-notifier-0.0.1
BiscuitOS/output/linux-XXX-YYY/package/MMU-userspace-notifier-0.0.1
{% endhighlight %}

具体实践办法请参考:

> [HKC 计划 BiscuitOS 实践框架介绍 (MMU-notifier-0.0.1)](#C0)
>
> [HKC 计划 BiscuitOS 实践框架介绍 (MMU-userspace-notifier-0.0.1)](#C2)

###### 通用例程 (内核部分)

{% highlight c %}
/*
 * MMU notifier on BiscuitOS
 *
 * (C) 2020.10.06 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/sched/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

/* MMU notifier */
#include <linux/mmu_notifier.h>
/* Current */
#include <linux/sched.h>

/* DD Platform Name */
#define DEV_NAME                "BiscuitOS"

static struct mmu_notifier BiscuitOS_notifier;
static struct mmu_notifier_range BiscuitOS_range;

static void BiscuitOS_mmu_release(struct mmu_notifier *mn,
                                                struct mm_struct *mm)
{
        printk("BiscuitOS notifier: release\n");
}

static int BiscuitOS_mmu_clear_flush_young(struct mmu_notifier *mn,
                struct mm_struct *mm, unsigned long start, unsigned long end)
{
        printk("BiscuitOS notifier: clear_flush_young\n");
        return 0;
}

static int BiscuitOS_mmu_clear_young(struct mmu_notifier *mn,
                struct mm_struct *mm, unsigned long start, unsigned long end)
{
        printk("BiscuitOS notifier: clear_young\n");
        return 0;
}

static int BiscuitOS_mmu_test_young(struct mmu_notifier *mn,
                        struct mm_struct *mm, unsigned long address)
{   
        printk("BiscuitOS notifier: test_young\n");
        return 0;
}

static void BiscuitOS_mmu_change_pte(struct mmu_notifier *mn,
                struct mm_struct *mm, unsigned long address, pte_t pte)
{
        printk("BiscuitOS notifier: change_pte\n");
}

static int BiscuitOS_mmu_invalidate_range_start(struct mmu_notifier *mn,
                                const struct mmu_notifier_range *range)
{
        printk("BiscuitOS notifier: invalidate_range_start.\n");
        return 0;
}

static void BiscuitOS_mmu_invalidate_range_end(struct mmu_notifier *mn,
                                const struct mmu_notifier_range *range)
{
        printk("BiscuitOS notifier: invalidate_range_end.\n");
}

static void BiscuitOS_mmu_invalidate_range(struct mmu_notifier *mn,
                struct mm_struct *mm, unsigned long start, unsigned long end)
{
        printk("BiscuitOS notifier: invalidate_range.\n");
}

static const struct mmu_notifier_ops BiscuitOS_mmu_notifer_ops = {
        .release     = BiscuitOS_mmu_release,
        .clear_young = BiscuitOS_mmu_clear_young,
        .test_young  = BiscuitOS_mmu_test_young,
        .change_pte  = BiscuitOS_mmu_change_pte,
        .clear_flush_young = BiscuitOS_mmu_clear_flush_young,
        .invalidate_range  = BiscuitOS_mmu_invalidate_range,
        .invalidate_range_start = BiscuitOS_mmu_invalidate_range_start,
        .invalidate_range_end   = BiscuitOS_mmu_invalidate_range_end,
};

static int BiscuitOS_mmap(struct file *filp, struct vm_area_struct *vma)
{
        struct mm_struct *mm = filp->private_data;
        pte_t pte;

        /* Trigger invalidate range [range, start, end] */
        mmu_notifier_range_init(&BiscuitOS_range, mm,
                vma->vm_start & PAGE_MASK, vma->vm_end & PAGE_MASK);
        mmu_notifier_invalidate_range_start(&BiscuitOS_range);
        mmu_notifier_invalidate_range_end(&BiscuitOS_range);

        /* Trigger clear_flush_young */
        mmu_notifier_clear_flush_young(mm, vma->vm_start, vma->vm_end);

        /* Trigger clear_young */
        mmu_notifier_clear_young(mm, vma->vm_start, vma->vm_end);

        /* Trigger test_young */
        mmu_notifier_test_young(mm, vma->vm_start);

        /* Trigger change pte */
        mmu_notifier_change_pte(mm, vma->vm_start, pte);

        /* Trigger realease */
        mmu_notifier_release(mm);

        return 0;
}

static int BiscuitOS_open(struct inode *inode, struct file *file)
{
        struct mm_struct *mm = get_task_mm(current);

        file->private_data = mm;
        /* mmu notifier initialize */
        BiscuitOS_notifier.ops = &BiscuitOS_mmu_notifer_ops;
        /* mmu notifier register */
        mmu_notifier_register(&BiscuitOS_notifier, mm);

        return 0;
}

static int BiscuitOS_release(struct inode *inode, struct file *file)
{
        mmu_notifier_unregister(&BiscuitOS_notifier, current->mm);
        return 0;
}

/* file operations */
static struct file_operations BiscuitOS_fops = {
        .owner          = THIS_MODULE,
        .open           = BiscuitOS_open,
        .mmap           = BiscuitOS_mmap,
        .release        = BiscuitOS_release,
};

/* Misc device driver */
static struct miscdevice BiscuitOS_drv = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = DEV_NAME,
        .fops   = &BiscuitOS_fops,
};

/* Module initialize entry */
static void __init BiscuitOS_init(void)
{
        /* Register Misc device */
        misc_register(&BiscuitOS_drv);

        printk("Hello modules on BiscuitOS\n");
}

device_initcall(BiscuitOS_init);
{% endhighlight %}

在本例子中，例子通过 MISC 机制在用于空间创建了 "/dev/BiscuitOS" 节点供用户空间部分的程序使用。用户空间在调用 open() 函数时，例子就会调用 mmu_notifier_register() 注册通知事件，当用户空间调用 close() 函数时，例子就会调用 mmu_notifier_unregister() 释放事件。当用户空间调用 mmap() 的时候，传递了一段虚拟内存地址到该例子的 BiscuitOS_mmap() 函数，此时在该函数中主动触发了以下几个通知:

* mmu_notifier_invalidate_range_start/mmu_notifier_invalidate_range_end 用于触发 BiscuitOS_mmu_invalidate_range_start/BiscuitOS_mmu_invalidate_range_end/BiscuitOS_mmu_invalidate_range 三个函数。
* mmu_notifier_clear_flush_young 触发 BiscuitOS_mmu_clear_flush_young 函数.
* mmu_notifier_clear_young 触发 BiscuitOS_mmu_clear_young 函数.
* mmu_notifier_test_young 触发 BiscuitOS_mmu_test_young 函数.
* mmu_notifier_change_pte 触发 BiscuitOS_mmu_change_pte 函数.
* mmu_notifier_release 触发 BiscuitOS_mmu_release 函数.

###### 通用例程 (用户空间部分)

{% highlight c %}
/*
 * BiscuitOS MMU notifier on Userspace
 *
 * (C) 2020.10.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

/* PATH for device */
#define DEV_PATH                "/dev/BiscuitOS"

int main()
{
        char *memory;
        int fd;

        /* open device */
        fd = open(DEV_PATH, O_RDWR);
        if (fd < 0) {
                printf("ERROR: Can't open %s\n", DEV_PATH);
                return -1;
        }

        /* mmap */
        memory = mmap(NULL, 0x100000, PROT_READ | PROT_WRITE,
                                                MAP_SHARED, fd, 0);
        if (!memory) {
                printf("ERROR: mmap faied.\n");
                goto out;
        }

        /* un-mmap */
        munmap(memory, 0x100000);

        /* Normal ending */
        close(fd);
        return 0;
out:
        close(fd);
        return -1;
}
{% endhighlight %}

在本例子用户空间部分的程序，首先通过 open() 函数打开了 "/dev/BiscuitOS" 节点，然后调用调用 mmap() 函数进行映射操作，映射完毕之后再通过 munmap() 函数接触映射。函数最后调用 close() 关闭节点。本例子纯粹是为内核部分的代码创造独立进程的条件. 两个部分在 BiscuitOS 中运行的情况如下:

{% highlight bash %}
~ # MMU-userspace-notifier-0.0.1 
BiscuitOS notifier: invalidate_range_start.
BiscuitOS notifier: invalidate_range.
BiscuitOS notifier: invalidate_range_end.
BiscuitOS notifier: clear_flush_young
BiscuitOS notifier: clear_young
BiscuitOS notifier: test_young
BiscuitOS notifier: change_pte
BiscuitOS notifier: release
~ #
{% endhighlight %}

从运行的结果可以看出，指定的消息已经传递成功. 开发者可以利用该机制进行页表操作时候通知进程内的其他功能模块.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000013"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Platform Simple Device Driver

这是一个最简单的 Platform 驱动，驱动实现向系统注册一个 Platform 驱动.

###### BiscuitOS 配置

在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Platofrm: Device Driver and Application  --->
        [*] Platform Simple Module  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/platform_simple_module-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Simple Platform Device Driver
 *
 * (C) 2019.10.01 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

/* LDD Platform Name */
#define DEV_NAME        "BiscuitOS"

static struct platform_device *pdev;

/* Probe: (LDD) Initialize Device */
static int BiscuitOS_probe(struct platform_device *pdev)
{
        /* Device Probe Procedure */
        printk("BiscuitOS Porbeing...\n");

        return 0;
}

/* Remove: (LDD) Remove Device (Module) */
static int BiscuitOS_remove(struct platform_device *pdev)
{
        /* Device Remove Procedure */
        printk("BiscuitOS Removing...\n");

        return 0;
}

/* Platform Driver Information */
static struct platform_driver BiscuitOS_driver = {
        .probe    = BiscuitOS_probe,
        .remove   = BiscuitOS_remove,
        .driver = {
                .owner  = THIS_MODULE,
                .name   = DEV_NAME,
        },
};

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
        int ret;

        ret = platform_driver_register(&BiscuitOS_driver);
        if (ret) {
                printk("Error: Platform driver register.\n");
                return -EBUSY;
        }

        pdev = platform_device_register_simple(DEV_NAME, 1, NULL, 0);
        if (IS_ERR(pdev)) {
                printk("Error: Platform device register\n");
                return PTR_ERR(pdev);
        }
        return 0;
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
        platform_device_unregister(pdev);
        platform_driver_unregister(&BiscuitOS_driver);
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("Simple Platform Device Driver");
{% endhighlight %}

在本例子中，通过调用 platform_driver_register/platform_device_register_simple 注册了一个 platform 的设备和驱动，其中驱动通过 BiscuitOS_driver 数据进行描述，在驱动描述中提供了驱动的 probe 和 remove 两个方法。当驱动 KO 被加载的时候，系统 Platform 总线就会在查找并调用该驱动的 probe 函数，当驱动卸载的时候，Platform 总线就会调用该驱动的 remove 函数。本实例在 BiscuitOS 的运行情况如下:

{% highlight bash %}
cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
platform_simple_module-0.0.1.ko
/lib/modules/5.0.0/extra # insmod platform_simple_module-0.0.1.ko 
platform_simple_module_0.0.1: loading out-of-tree module taints kernel.
BiscuitOS Porbeing...
/lib/modules/5.0.0/extra # 
/sys/bus/platform # cd /sys/bus/platform/devices/BiscuitOS.1/
/sys/devices/platform/BiscuitOS.1 # 
/sys/devices/platform/BiscuitOS.1 # ls
driver           modalias         subsystem
driver_override  power            uevent
/sys/devices/platform/BiscuitOS.1 #
{% endhighlight %}

当驱动加载的时候，驱动的 probe 函数就会被调用，并且在 /sys/bus/platform/devices 目录下创建了 BiscuitOS.1 的节点，该节点下还包含了与 BiscuitOS.1 设备相关的信息.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000014"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Emulate PID Allocating and Releasing

用 4096 位模拟进程的 pid (0~4095), 创建两个线程，一个线程用随机数，选择 0~4095 中的一位进行清零操作，以此模拟进程的退出; 另外一个线程查找 0~4094 位，发现 0 位置位 1，模拟 pid 的申请。

> 代码共享者: Meijusan <20741602@qq.com>

###### BiscuitOS 配置

在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Bitmap  --->
        [*] Emulate PID Allocating and Releasing  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/emulate-pid-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight c %}
/*
 * Bitmap: Emulate PID allocating and Releasing on BiscuitOS
 *
 * (C) 2020.10.11  Meijusan <20741602@qq.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include<linux/slab.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/gfp.h>
#include <linux/pid.h>
#include <linux/delay.h>
#include <linux/random.h>

#define PID_BITMAP_MAX 4096

/*
 * pid range 0~4096,this is a example
 */

static DECLARE_BITMAP( pid_bitmap_mask, PID_BITMAP_MAX);
static DEFINE_MUTEX(pid_bitmap_lock);

int pid_produce_thread(void *arg)
{
        while (!kthread_should_stop())  {
                int pid;

                mutex_lock(&pid_bitmap_lock);
                if( bitmap_full(pid_bitmap_mask, PID_BITMAP_MAX) ) {
                        mutex_unlock(&pid_bitmap_lock);
                        msleep(1000);
                        continue;
                }

                pid = find_first_zero_bit(pid_bitmap_mask, PID_BITMAP_MAX);
                if ( pid >= PID_BITMAP_MAX) {
                        printk("bitmap is full\n");
                        mutex_unlock(&pid_bitmap_lock);
                        msleep(1000);
                        continue;
                }
                printk("threadid %d create  pid: %d \n",current->pid,  pid);
                set_bit(pid, pid_bitmap_mask);
                mutex_unlock(&pid_bitmap_lock);
        }
        return 0;
}

static int pid_consume_thread(void *dummy)
{
        while (!kthread_should_stop()) {

                /*range 0 ~PID_BITMAP_MAX*/
                int pid = prandom_u32()%(PID_BITMAP_MAX + 1) ;

                mutex_lock(&pid_bitmap_lock);
                clear_bit(pid, pid_bitmap_mask);
                mutex_unlock(&pid_bitmap_lock);
                printk("threadid %d remove  pid: %d \n",current->pid,  pid);
                msleep(1000);
        }
        return 0;
}

static struct task_struct *pid_produce_task = NULL;
static struct task_struct *pid_consume_task = NULL;


/* Module initialize entry */
static int __init pid_bitmap_demo_init(void)
{
        /* default all pid already exist */
        bitmap_fill(pid_bitmap_mask, PID_BITMAP_MAX);

        pid_produce_task = kthread_run(pid_produce_thread, NULL, "pidconsujme");
        if (IS_ERR(pid_produce_task)) {
                int err = PTR_ERR(pid_produce_task);
                printk("failed to start the kauditd thread (%d)\n", err);
        }

        pid_consume_task = kthread_run(pid_consume_thread, NULL, "testbitmap");
        if (IS_ERR(pid_consume_task)) {
                int err = PTR_ERR(pid_consume_task);
                printk("failed to start the kauditd thread (%d)\n", err);
        }

        return 0;
}

/* Module exit entry */
static void __exit pid_bitmap_demo_exit(void)
{
        if(pid_produce_task)
                kthread_stop(pid_produce_task);
        if(pid_consume_task)
                kthread_stop(pid_consume_task);
}

module_init(pid_bitmap_demo_init);
module_exit(pid_bitmap_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Meijusan <20741602@qq.com>");
MODULE_DESCRIPTION("Emulate PID allocating and Releasing on BiscuitOS");
{% endhighlight %}

独立代码在 BiscuitOS 中运行的结果如下:

{% highlight bash %}
~ # cd lrandom: fast init done
~ # cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
emulate-pid-0.0.1.ko
/lib/modules/5.0.0/extra # insmod emulate-pid-0.0.1.ko 
emulate_pid_0.0.1: loading out-of-tree module taints kernel.
threadid 1140 remove  pid: 521 
/lib/modules/5.0.0/extra # threadid 1138 create  pid: 521 
threadid 1140 remove  pid: 4073 
threadid 1140 remove  pid: 3235 
threadid 1138 create  pid: 3235 
threadid 1138 create  pid: 4073 
threadid 1140 remove  pid: 960 
threadid 1138 create  pid: 960 
threadid 1140 remove  pid: 1575 
threadid 1138 create  pid: 1575 
threadid 1140 remove  pid: 286 
threadid 1138 create  pid: 286 
threadid 1140 remove  pid: 2280 
threadid 1138 create  pid: 2280 
threadid 1140 remove  pid: 4061 
threadid 1140 remove  pid: 435 
threadid 1138 create  pid: 435 
threadid 1138 create  pid: 4061 
threadid 1140 remove  pid: 2294
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H000015"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### LC-trie Protocol

###### 功能说明

{% highlight bash %}
1.1 为了方便实验和更好的理解 trie, 本模块从路由 LC-trie 协议, 去除了路由
    规则, trie 的扩展/压缩等复杂模块简化而成
1.2 主要完成一个 32bits 的字典树 (网络搜索参考字典树的逻辑及下文的 "实例图解")
1.3 本模块功能包括: 插入节点, 移除节点和节点查找,节点可以附带具体业务对象数据
1.4 本模块包含基础插入/移除和查找 api, 以及一个使用实例

2. 调试环境信息: linux 5.0.0
3. 运行输出信息: 为了便于理解, 本模块在 init 函数中执行实例程序,并输出人性化
   的调试信息,如下 [运行输出信息]
4. 代码说明

   bits32_trie_new.c:    节点的创建, 内存分配, 本例分配内存统一使用
                         kmalloc/kfree, 如需优化,自行修改
   bits32_trie_debug.c:  调试信息输出, 针对trie结构体进行逐层打印
   bits32_trie_insert.c: 节点插入代码
   bits32_trie_lookup.c: 节点查找代码
   bits32_trie_remove.c: 节点删除代码
   bits32_trie.c:        模块初始化代码及测试实例
{% endhighlight %}

> 代码贡献者: Shaobin <shaobin.huang@kernelworker.net>

###### BiscuitOS 配置

在 BiscuitOS 中使用配置如下:

{% highlight bash %}
[*] Package  --->
    [*] Bitmap  --->
        [*] LC-trie Protocol  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/LC-trie-0.0.1
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C0)

###### 通用例程

{% highlight bash %}
.
├── LC-trie-0.0.1
│   ├── bits32_trie.c
│   ├── bits32_trie_debug.c
│   ├── bits32_trie_debug.h
│   ├── bits32_trie.h
│   ├── bits32_trie_insert.c
│   ├── bits32_trie_insert.h
│   ├── bits32_trie_lookup.c
│   ├── bits32_trie_lookup.h
│   ├── bits32_trie_new.c
│   ├── bits32_trie_new.h
│   ├── bits32_trie_remove.c
│   ├── bits32_trie_remove.h
│   ├── Kconfig
│   ├── Makefile
│   └── readme.txt
├── Makefile
└── README.md

1 directory, 17 files
{% endhighlight %}

该代码在 BiscuitOS 上运行的情况如下:

{% highlight bash %}
~ # cd lib/modules/5.0.0/extra/
/lib/modules/5.0.0/extra # ls
C-trie-0.0.1.ko
/lib/modules/5.0.0/extra # insmod C-trie-0.0.1.ko 
C_trie_0.0.1: loading out-of-tree module taints kernel.
bits32_trie_init:[16 - 4]
~~~~~~~~  start insert  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bits32_trie_init insert leaf[0x12345678] data[0x0]
bits32_trie_init insert leaf[0x87654321] data[0x1]
bits32_trie_init insert leaf[0x11111111] data[0x2]
bits32_trie_init insert leaf[0x22222222] data[0x3]
~~~~~~~~   end insert   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
--------------------------------------------------------------
~~~~~~~~   now let's look the trie view ~~~~~~~~~~~~~~~~~
tnode[0x0]:pos[0x1f]bits[0x1]cindex[0x0]
- tnode[0x0]:pos[0x1d]bits[0x1]cindex[0x0]
- - tnode[0x10000000]:pos[0x19]bits[0x1]cindex[0x0]
- - - leaf[0x11111111]:pos[0x0]bits[0x0]data[0x2]cindex[0x0]
- - - leaf[0x12345678]:pos[0x0]bits[0x0]data[0x0]cindex[0x1]
- - leaf[0x22222222]:pos[0x0]bits[0x0]data[0x3]cindex[0x1]
- leaf[0x87654321]:pos[0x0]bits[0x0]data[0x1]cindex[0x1]
~~~~~~~~   trie view end, it's as what you think??  ~~~~~
--------------------------------------------------------------
~~~~~~~~   test lookup  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bits32_trie_init lookup[0x11111111]index[0x2]
bits32_trie_init it seems ok, found leaf->key[0x11111111]
~~~~~~~~  test remove  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bits32_trie_init remove[0x11111111]index[0x2]
bits32_trie_init it seems ok, found no leaf->key[0x11111111] after del
bits32_trie_init remove[12345678]index[0]
bits32_trie_init it seems ok, found no leaf->key[0x12345678] after del
~~~~~~~~  now let's check the trie if it same as what you think  ~~~~
tnode[0x0]:pos[0x1f]bits[0x1]cindex[0x0]
- tnode[0x0]:pos[0x1d]bits[0x1]cindex[0x0]
- - leaf[0x22222222]:pos[0x0]bits[0x0]data[0x3]cindex[0x1]
- leaf[0x87654321]:pos[0x0]bits[0x0]data[0x1]cindex[0x1]
bits32_trie_init finish
insmod (1137) used greatest stack depth: 6300 bytes left
/lib/modules/5.0.0/extra # 
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000571.png)

{% highlight bash %}
             bit31  ->                      bit0
0x12345678   00010010 00110100 01010110 01111000
0x87654321   10000111 01100101 01000011 00100001
0x11111111   00010001 00010001 00010001 00010001
0x22222222   00100010 00100010 00100010 00100010


                       tp
                       |
                       v
              0x12345678
(00010010 00110100 01010110 01111000)

                                           tp
                                           |
                                           v
                       ------------------ 0x0 --------------------
                       |                                         |
                       v                                         v
              0x12345678                                0x87654321
(00010010 00110100 01010110 01111000)    (10000111 01100101 01000011 00100001)


                                                               tp
                                                               |
                                                               v
                                           ------------------ 0x0 --------------------
                                           |                                         |
                                           v                                         v
                                  0x10000000                                0x87654321
                    (00010000 00000000 00000000 00000000)    (10000111 01100101 01000011 00100001)
                                           |
                                           v
                       -------------------- ----------------------
                       |                                         |
                       v                                         v
              0x12345678                                0x11111111
(00010010 00110100 01010110 01111000)    (00010001 00010001 00010001 00010001)
                                                                                    tp
                                                                                    |
                                                                                    v
                                                                ------------------ 0x0 --------------------
                                                                |                                         |
                                                                v                                         v
                                                               0x0                                   0x87654321
                                                  (00000000 00000000 00000000 00000000)       (10000111 01100101 01000011 00100001)
                                                                |
                                                                v
                                           --------------------  ---------------------
                                           |                                         |
                                           v                                         v
                                     0x10000000                                 0x22222222
                    (00000001 00000000 00000000 00000000)    (00100010 00100010 00100010 00100010)
                                           |
                                           v
                       -------------------- ----------------------
                       |                                         |
                       v                                         v
                   0x11111111                                0x12345678
     (00010001 00010001 00010001 00010001)    (00010010 00110100 01010110 01111000)
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

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/
{% endhighlight %}

具体实践办法请参考:

> - [HKC 计划 BiscuitOS 实践框架介绍](#C)

###### 通用例程

{% highlight c %}

{% endhighlight %}


{% highlight bash %}

{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### HKC 贡献者名单

> BuddyZhang1 <buddy.zhang@aliyun.com>
>
> Meijusan <20741602@qq.com>
>
> Shaobin <shaobin.huang@kernelworker.net>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)
