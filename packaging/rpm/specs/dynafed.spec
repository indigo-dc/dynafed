%if %{?fedora}%{!?fedora:0} >= 17 || %{?rhel}%{!?rhel:0} >= 7
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include
%else
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include/boost148 -DBOOST_LIBRARYDIR=%{_libdir}/boost148
%endif

%if %{?fedora}%{!?fedora:0} >= 17 || %{?rhel}%{!?rhel:0} >= 7
%global systemd 1
%else
%global systemd 0
%endif




Name:				dynafed
Version:			1.4.0
Release:			2%{?dist}
Summary:			Ultra-scalable dynamic system for federating HTTP-based storage resources
Group:				Applications/Internet
License:			ASL 2.0
URL:				https://svnweb.cern.ch/trac/lcgdm/wiki
# svn export http://svn.cern.ch/guest/lcgdm/ugr/trunk ugr
Source0:			http://grid-deployment.web.cern.ch/grid-deployment/dms/lcgutil/tar/%{name}/%{name}-%{version}.tar.gz
BuildRoot:			%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%if %{?fedora}%{!?fedora:0} >= 17 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:			boost-devel >= 1.48.0
%else
BuildRequires:			boost148-devel >= 1.48.0
%endif

## Provides: libugrconnector.so.1.2.2()(64bit)

BuildRequires:		cmake
BuildRequires:		dmlite-devel >= 1.11.0
BuildRequires:		dmlite-private-devel  >= 1.11.0
BuildRequires:		davix-devel >= 0.6.2
BuildRequires:		lfc-devel >= 1.8.8
BuildRequires:		gfal2-devel
BuildRequires:		GeoIP-devel
BuildRequires:		libmaxminddb-devel
BuildRequires:		libmemcached-devel
BuildRequires:		openssl-devel
BuildRequires:		protobuf-devel
BuildRequires:		python
BuildRequires:		python-devel

%if %systemd
# possible deps to configure the journal for practical logging
%else
Requires:		rsyslog
%endif


%description
The Dynafed project provides a dynamic, scalable HTTP resource
federation mechanism for distributed storage systems.
It supports backends exposing HTTP, WebDAV, S3, Azure as
access protocols. In the S3 and Azure case it hides the secret
keys and exploits pre-signed URLs.
The default deployment style is accessible by any HTTP/Webdav
compatible client. The core components can be used to design
frontends based on other protocols.

%package private-devel
Summary:			Development files for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release}
Requires:			pkgconfig

%description private-devel
Headers files for %{name}'s plugin development.

%package http-plugin
Summary:			Http and WebDav plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release}
Requires:			davix-libs >= 0.5.1
Provides:                       %{name}-dav-plugin = %{version}-%{release}

%description http-plugin
Plugin for the WebDav based storage system for %{name}

%package lfc-plugin
Summary:			Logical File catalog (LFC) plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release}
Requires:			gfal2-plugin-lfc%{?_isa}

%description lfc-plugin
Plugin for the Logical File catalog system for %{name}

%package dmlite-plugin
Summary:                        dmlite plugin for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}
Requires:                       dmlite-libs%{?_isa} >= 1.11.0

%description dmlite-plugin
Plugin for using dmlite for %{name}

%package dmlite-frontend
Summary:                        dmlite plugin for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}
Requires:                       %{_libdir}/httpd/modules/mod_lcgdm_dav.so
Requires:                       lcgdm-dav-server
Requires:                       dmlite-libs%{?_isa} >= 1.11.0
%if %systemd == 0
Requires:                       mod_proxy_fcgi
%endif
Requires:                       php-fpm
Requires:                       php-pecl-memcache

%description dmlite-frontend
Webdav frontend for %{name} using dmlite and lcgdm-dav



%clean
rm -rf %{buildroot};
make clean

%prep
%setup -q

%build
%if %systemd
%cmake \
-DDOC_INSTALL_DIR=%{_docdir}/%{name}-%{version} \
-DAPACHE_SITES_INSTALL_DIR=%{_sysconfdir}/httpd/conf.d \
-DOUT_OF_SOURCE_CHECK=FALSE \
-DRSYSLOG_SUPPORT=FALSE \
-DLOGROTATE_SUPPORT=FALSE \
%{boost_cmake_flags} \
.
make
%else
%cmake \
-DDOC_INSTALL_DIR=%{_docdir}/%{name}-%{version} \
-DAPACHE_SITES_INSTALL_DIR=%{_sysconfdir}/httpd/conf.d \
-DOUT_OF_SOURCE_CHECK=FALSE \
-DRSYSLOG_SUPPORT=TRUE \
-DLOGROTATE_SUPPORT=TRUE \
%{boost_cmake_flags} \
.
make
%endif

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%post
/sbin/ldconfig
/sbin/service rsyslog condrestart || true
## conf file plugin path transition
## sed -i 's@/usr/lib\([0-9]*\)/ugr@/usr/lib\1/dynafed@g' /etc/ugr.conf || true

%postun
/sbin/ldconfig

%post dmlite-frontend
/sbin/ldconfig
/sbin/service rsyslog condrestart || true
/sbin/service httpd condrestart  || true

%files
%defattr (-,root,root)
%{_libdir}/libugrconnector.so.*
%{_libdir}/ugr/libugrgeoplugin_geoip.so
%{_libdir}/ugr/libugrgeoplugin_mmdb.so
%{_libdir}/ugr/libugrnoloopplugin.so
%{_libdir}/ugr/libugrauthplugin_python*.so
%config(noreplace) %{_sysconfdir}/ugr/ugr.conf
%config(noreplace) %{_sysconfdir}/ugr/conf.d/*
%if %systemd
# possible config to configure the journal for practical logging
%else
%config(noreplace) %{_sysconfdir}/rsyslog.d/*
%config(noreplace) %{_sysconfdir}/logrotate.d/*
%endif
%doc RELEASE-NOTES
%doc doc/whitepaper/Doc_DynaFeds.pdf

%files private-devel
%defattr (-,root,root)
%{_libdir}/libugrconnector.so
%dir %{_includedir}/ugr
%{_includedir}/ugr/*
%{_libdir}/pkgconfig/*

%files http-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_dav.so
%{_libdir}/ugr/libugrlocplugin_http.so
%{_libdir}/ugr/libugrlocplugin_s3.so
%{_libdir}/ugr/libugrlocplugin_azure.so
%{_libdir}/ugr/libugrlocplugin_davrucio.so

%files lfc-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_lfc.so


%files dmlite-plugin
%defattr (-,root,root)
%attr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_dmliteclient.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientORA.conf
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientMY.conf


%files dmlite-frontend
%defattr (-,root,root)
%{_libdir}/ugr/libugrdmlite.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmlite.conf
%config(noreplace) %{_sysconfdir}/httpd/conf.d/zlcgdm-ugr-dav.conf
/var/www/html/dashboard/*

%changelog
* Wed Sept 26, 2018 Fabrizio Furano <furano at cern.ch> - 1.4.0
 - now requires dmlite >= 1.11
* Thu May 18 2016 Fabrizio Furano <furano at cern.ch> - 1.2.1-1
 - Little packaging fixes for inclusion into EPEL
* Fri Jun 01 2012 Adrien Devresse <adevress at cern.ch> - 0.0.2-0.1-2012052812snap
 - initial draft
