#!/bin/bash
#mv debian debian.bak 
#mv debian-agiros-loong debian
# 生成新版本号
NEW_VERSION="1.0.0-1jammy"

# 替换 changelog 第一行的版本号
sed -i "1s/(.*)/($NEW_VERSION)/" debian/changelog
# 构建新包
dpkg-buildpackage -us -uc -b -tc

#mv debian debian-agiros-loong
#mv debian.bak debian
