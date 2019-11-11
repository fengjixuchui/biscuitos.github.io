---
layout: post
title:  "Linux 5.x arm32 Usermanual"
date:   2019-05-06 14:45:30 +0800
categories: [Build]
excerpt: Linux 5.x arm32 Usermanual.
tags:
  - Linux
---

> [GitHub: BiscuitOS](https://github.com/BiscuitOS/BiscuitOS)
>
> Email: BuddyZhang1 <Buddy.zhang@aliyun.com>

{% highlight bash %}
sudo apt-get install mr
sudo apt install debhelper
{% endhighlight %}

{% highlight bash %}
git clone https://salsa.debian.org/installer-team/d-i.git
cd d-i
./scripts/git-setup
mr -p checkout
cd d-i/installer
dpkg-checkbuilddeps

---> Install package


{% endhighlight %}
