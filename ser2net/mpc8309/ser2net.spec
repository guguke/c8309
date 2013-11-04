%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : my ser2net 2.8
Name            : ser2net
Version         : 2.8
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : xxxx
Group           : xxxx
URL             : http://xxxx
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
