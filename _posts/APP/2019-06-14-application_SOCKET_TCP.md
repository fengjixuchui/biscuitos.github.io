---
layout: post
title:  "Socket-TCP 部署到应用程序中"
date:   2019-06-12 05:30:30 +0800
categories: [HW]
excerpt: Socket-TCP 部署到应用程序中().
tags:
  - App
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

> [Github: Socket: TCP](https://github.com/BiscuitOS/HardStack/tree/master/CodeSegment/socket/TCP)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [Socket: TCP 部署方法](#A0)
>
> - [Socket: TCP 使用方法](#A1)
>
>   - [Socket: TCP 端程序](#B00)
>
>   - [Socket: TCP 客户端程序](#B01)
>
>   - [Socket：TCP 编译方法](#B02)
>
>   - [Socket: TCP 运行情况](#B03)
>
> - [附录](#附录)

-----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

## Socket: TCP 部署方法

BiscuitOS 开源项目提供了一套用户空间使用的 Socket: TCP，开发者只要按照使用步骤就可以
轻松将 Socket: TCP 部署到开发者自己的项目中。具体步骤如下：

##### 获取 Socket: TCP

开发者首先获得 Socket: TCP server 和 client 端的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/CodeSegment/socket/TCP/tcp_client.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/CodeSegment/socket/TCP/tcp_server.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/CodeSegment/socket/TCP/Makefile
{% endhighlight %}

通过上面的命令可以获得 Socket: TCP 的源代码，其中 tcp_client.c 文件为客户端
端代码，tcp_server.c 文件为服务器端代码。Makefile 用于编译生成可运行文件。

------------------------------

<span id="A1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## Socket: TCP 使用方法

> - [Socket: TCP 端程序](#B00)
>
> - [Socket: TCP 客户端程序](#B01)
>
> - [Socket：TCP 编译方法](#B02)
>
> - [Socket: TCP 运行情况](#B03)

#### <span id="B00">Socket: TCP 端程序</span>

Socket:TCP 服务器端程序，如下, 值得注意的是，开发者在使用服务器端程序的
时候，socket 绑定 IP 应该为服务器是在的 IP。

{% highlight c %}
/*
 * Socket Server (TCP).
 *
 * (C) 2019.06.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

/* Define IP */
#define SOCKET_IP	"127.0.0.1"

/* Define PORT */
#ifdef CONFIG_PORT
#define SOCKET_PORT	CONFIG_PORT
#else
#define SOCKET_PORT	8890
#endif

int main(void)
{
	struct sockaddr_in server_address, client_address;
	int server_sockfd, client_sockfd;
	int server_len, client_len;
	char recvbuf[1024];

	/* TCP socket */
	server_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	/* Configureation IP and PORT */
	server_address.sin_addr.s_addr = inet_addr(SOCKET_IP);
	server_address.sin_port = htons(SOCKET_PORT);
	server_len = sizeof(server_address);

	/* Bind */
	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	/* listen */
	listen(server_sockfd, SOMAXCONN);
	client_len=sizeof(client_address);
	client_sockfd=accept(server_sockfd,
		(struct sockaddr *)&client_address, (socklen_t *)&client_len);

	while(1) {
		memset(recvbuf,0,sizeof(recvbuf));
		read(client_sockfd, recvbuf, sizeof(recvbuf));
		fputs(recvbuf,stdout);
		write(client_sockfd, recvbuf, sizeof(recvbuf));
	}

	close(server_sockfd);
	close(client_sockfd);
	return 0;
}
{% endhighlight %}

-----------------------------------------------

#### <span id="B01">Socket:TCP 客户端端程序</span>

Socket:TCP 客户端端程序，如下，开发者在搭建客户端程序的时候，Socket 绑定的 IP
应该为服务器所在的 IP，即程序中 SOCKET_IP 宏定义的 IP。

{% highlight c %}
/*
 * Socket Client (TCP).
 *
 * (C) 2019.06.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

/* Define IP */
#define SOCKET_IP	"127.0.0.1"

/* Define Port */
#ifdef CONFIG_PORT
#define SOCKET_PORT	CONFIG_PORT
#else
#define SOCKET_PORT	8890
#endif

/* Buffer size */
#define MES_BUFSZ	10240

int main(void)
{
	struct sockaddr_in server_address;
	char sendbuf[MES_BUFSZ] = {0};
	char recvbuf[MES_BUFSZ] = {0};
	int server_len;
	int sock;

	/* TCP: SOCK_STREAM */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	/* Configuration IP and Port */
	server_address.sin_addr.s_addr = inet_addr(SOCKET_IP);
	server_address.sin_port = htons(SOCKET_PORT);

	/* Bind */
	connect(sock, (struct sockaddr*)&server_address,
						sizeof(server_address));

	while(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL) {
		/* Send message */
		write(sock, sendbuf, sizeof(sendbuf));
		/* Write message */
		read(sock, recvbuf, sizeof(recvbuf));

		/* Prepare for next */
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));
	}

	close(sock);
	return 0;
}
{% endhighlight %}

-----------------------------------------------

#### <span id="B02">Socket：TCP 编译方法</span>

开发者在获得 Socket: TCP 的源码之后，参照 Makefile 编译到自己的项目中，例如：

{% highlight ruby %}
# SPDX-License-Identifier: GPL-2.0
CC=gcc

CFLAGS = -I./

# Config

## PORT
CFLAGS += -DCONFIG_PORT=8860

all: client server

client: tcp_client.c
	@$(CC) $^ $(CFLAGS) -o $@

server: tcp_server.c
	@$(CC) $^ $(CFLAGS) -o $@

clean:
	@rm -rf *.o client server > /dev/null
{% endhighlight %}

例如在上面的 Makefile 脚本中，CONFIG_PORT 宏用于定义 TCP 使用的端口，分别将
客户端和服务器端编译。

-----------------------------------------------

#### <span id="B03">Socket: TCP 运行情况</span>

将编译生成的两个可指定文件分别在两个 IP 中运行，运行情况如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000133.png)

上图为客户端程序，只要在里面输入字符串，相应的服务器端程序也会收到并
显示字符串。如下图

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000132.png)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
