%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Net-SNMP
Name            : net-snmp
Version         : 5.4.1
Release         : 1
License         : BSD and BSD-like
Vendor          : Freescale
Packager        : Michael Barkowski
Group           : Networking
URL             : http://net-snmp.sourceforge.net
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --disable-embedded-perl \
 --without-perl-modules --with-endianness=big --enable-mini-agent --with-default-snmp-version="3" --disable-des --disable-debugging \
 --with-sys-contact="who@where" --with-logfile="/var/log/snmp" --with-transports="UDP TCP" \
 --with-defaults 
make

%Install
rm -rf $RPM_BUILD_ROOT
sed -i -e "s;SUBDIRS		= snmplib  agent apps man local mibs;SUBDIRS		= snmplib;" Makefile
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
sed -i -e "s;%{_prefix};$RPM_BUILD_ROOT/%{pfx}%{_prefix};" $RPM_BUILD_ROOT/%{pfx}%{_prefix}/lib/libnetsnmp.la
sed -i -e "s;SUBDIRS		= snmplib;SUBDIRS		= agent apps man local mibs;" Makefile
find ./ -name "*.la" | xargs sed -i -e "s;%{_prefix};$RPM_BUILD_ROOT/%{pfx}%{_prefix};"
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
