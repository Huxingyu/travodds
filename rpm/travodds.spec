# 基本信息
Name:           agiros-loong-travodds
Version:        1.1.0
Release:        1%{?dist}
Summary:        Travo Data Distribution Service Library

License:        Mulan PSL v2
URL:            https://gitee.com/agiros/travodds
Source0:        %{name}_%{version}.orig.tar.gz

# 构建依赖
BuildRequires:  cmake >= 3.10
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig

# 运行时依赖
Requires:       glibc >= 2.17

# 分开发包
%package devel
Summary:        Development files for %{name}
Requires:       %{name} = %{version}-%{release}
Requires:       pkgconfig
Requires:       cmake

%description
Contains shared libraries for TravoDDS implementation.

%description devel
Provides headers and CMake config files for TravoDDS development.


%define Installprefix /opt/agiros/loong
# 构建流程
%prep
%autosetup -p1

%build
mkdir -p build
cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=%{Installprefix} \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo 
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}

# 确保目标目录存在
install -d %{buildroot}%{Installprefix}/lib

# 创建符号链接（开发包使用）
# ln -sf libtravodds.so.1 %{buildroot}%{installprefix}/lib/libtravodds.so

# 主包文件列表（运行时库）
%files
%{Installprefix}/lib/libtravodds.so.*

# 开发包文件列表（头文件、符号链接等）
%files devel
%{Installprefix}/include/travodds/
%{Installprefix}/lib/libtravodds.so
%{Installprefix}/share/travodds/cmake/
