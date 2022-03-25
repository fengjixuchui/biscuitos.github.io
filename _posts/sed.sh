#!/bin/bash

#sed -i "s/https:\/\/raw\.githubusercontent\.com\/EmulateSpace\/PictureSet\/master/https:\/\/gitee\.com\/BiscuitOS_team\/PictureSet\/raw\/Gitee/g" `grep "https:\/\/raw\.githubusercontent\.com\/EmulateSpace\/PictureSet\/master" -rl ./`
sed -i "s/https:\/\/gitee\.com\/BiscuitOS_team\/PictureSet\/raw\/Gitee/\/assets\/PDB/g" `grep "https:\/\/gitee\.com\/BiscuitOS_team\/PictureSet\/raw\/Gitee" -rl ./`
