#!/bin/bash
rpmdev-setuptree

tar czvf ~/rpmbuild/SOURCES/agiros-loong-travodds_1.0.0.orig.tar.gz \
	--transform 's,^,agiros-loong-travodds-1.0.0/,' \
	cmake include src thirdparty CMakeLists.txt

cp ./rpm/travodds.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/travodds.spec
mkdir -p bin
find ~/rpmbuild/RPMS/ -type f -name "*.rpm" -print0 | xargs -0 cp -t ./bin/