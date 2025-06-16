# 基本信息
Name:           travodds
Version:        1.0.0
Release:        1%{?dist}
Summary:        AGI Data Distribution Service Library

License:        BSD
URL:            https://github.com/your/travodds
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
Contains shared libraries for AGI DDS implementation.

%description devel
Provides headers and CMake config files for AGI DDS development.


%define Installprefix /usr
# 构建流程
%prep
%setup -q

%build
mkdir -p build
cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=%{Installprefix} \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DAPPEND_PROJECT_NAME_TO_INSTALL=ON
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}

# 确保目标目录存在
install -d %{buildroot}%{Installprefix}/lib/travodds

# 添加ld.so.conf.d配置
install -Dm644 /dev/null %{buildroot}/etc/ld.so.conf.d/travodds.conf
echo "%{Installprefix}/lib/travodds" > %{buildroot}/etc/ld.so.conf.d/travodds.conf

# 创建符号链接（开发包使用）
# ln -sf libtravodds.so.1 %{buildroot}%{installprefix}/lib/travodds/libtravodds.so

# 主包文件列表（运行时库）
%files
%{Installprefix}/lib/travodds/libtravodds.so.*
/etc/ld.so.conf.d/travodds.conf

# 开发包文件列表（头文件、符号链接等）
%files devel
%{Installprefix}/include/travodds/
%{Installprefix}/lib/travodds/libtravodds.so
%{Installprefix}/share/travodds/cmake/

%post
/sbin/ldconfig

%postun
/sbin/ldconfig
