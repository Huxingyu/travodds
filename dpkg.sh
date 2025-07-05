#!/bin/bash
# 生成新版本号
NEW_VERSION="1.0.0-1"

# 替换 changelog 第一行的版本号
sed -i "1s/(.*)/($NEW_VERSION)/" debian/changelog
# 构建新包
dpkg-buildpackage -us -uc -b -tc
