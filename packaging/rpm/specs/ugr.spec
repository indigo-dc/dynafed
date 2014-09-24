%if 0%{?el5}
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include/boost141 -DBOOST_LIBRARYDIR=%{_libdir}/boost141
%else
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include
%endif

Name:				ugr
Version:			1.0.7
Release:			19%{?dist}
Summary:			The Dynamic Federations - Ultra-scalable storage federation system
Group:				Applications/Internet
License:			ASL 2.0
URL:				https://svnweb.cern.ch/trac/lcgdm/wiki
# svn export http://svn.cern.ch/guest/lcgdm/ugr/trunk ugr
Source0:			http://grid-deployment.web.cern.ch/grid-deployment/dms/lcgutil/tar/%{name}/%{name}-%{version}.tar.gz 
BuildRoot:			%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%if 0%{?el5}
BuildRequires:		boost141-devel
%else
BuildRequires:		boost-devel
%endif
BuildRequires:		cmake
BuildRequires:		dmlite-devel >= 0.7.0
BuildRequires:		dmlite-private-devel  >= 0.7.0
BuildRequires:		davix-devel >= 0.2.11
BuildRequires:          gfal2-devel
BuildRequires:		GeoIP-devel
BuildRequires:		libmemcached-devel
BuildRequires:		protobuf-devel

Requires:               rsyslog


%description
Ugr provides a powerfull, fast and scalable dynamic storage federation 
system for grid and clouds.
It is able to federate several distributed storage systems 
in one consistent namespace in a transparent manner for the client side.
Ugr is accessible by any HTTP/Webdav compatible client.

%package devel
Summary:			Development files for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release} 
Requires:			pkgconfig

%description devel
development files for %{name}

%package http-plugin
Summary:			Http and WebDav plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release} 
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
# transition dependency
Requires:                       %{name}-dmlite-frontend

%description dmlite-plugin
Plugin for using dmlite for %{name}

%package dmlite-frontend
Summary:                        dmlite plugin for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}
Requires:                       %{_libdir}/httpd/modules/mod_lcgdm_dav.so
Requires:                       dmlite-libs%{?_isa} >= 0.7.0

%description dmlite-frontend
Webdav frontend for %{name} using dmlite and lcgdm-dav



%clean
rm -rf %{buildroot};
make clean

%prep
%setup -q

%build
%cmake \
-DDOC_INSTALL_DIR=%{_docdir}/%{name}-%{version} \
-DAPACHE_SITES_INSTALL_DIR=%{_sysconfdir}/httpd/conf.d \
-DOUT_OF_SOURCE_CHECK=FALSE \
-DRSYSLOG_SUPPORT=TRUE \
-DLOGROTATE_SUPPORT=TRUE \
%{boost_cmake_flags} \
.
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%post
/sbin/ldconfig
/sbin/service rsyslog condrestart || true

%postun
/sbin/ldconfig

%post dmlite-frontend
/sbin/ldconfig
/sbin/service rsyslog condrestart || true

%files
%defattr (-,root,root)
%{_libdir}/libugrconnector.so.*
%{_libdir}/ugr/libugrgeoplugin_geoip.so
%{_libdir}/ugr/libugrnoloopplugin.so
%config(noreplace) %{_sysconfdir}/ugr.conf
%config(noreplace) %{_sysconfdir}/rsyslog.d/*
%config(noreplace) %{_sysconfdir}/logrotate.d/*
%doc RELEASE-NOTES

%files devel
%defattr (-,root,root)
%{_libdir}/libugrconnector.so
%dir %{_includedir}/ugr
%{_includedir}/ugr/*
%{_libdir}/pkgconfig/*

%files http-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_dav.so
%{_libdir}/ugr/libugrlocplugin_http.so


%files lfc-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_lfc.so


%files dmlite-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_dmliteclient.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientORA.conf
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientMY.conf


%files dmlite-frontend
%{_libdir}/ugr/libugrdmlite.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmlite.conf
%config(noreplace) %{_sysconfdir}/httpd/conf.d/zlcgdm-ugr-dav.conf


%changelog
* Fri Jun 01 2012 Adrien Devresse <adevress at cern.ch> - 0.0.2-0.1-2012052812snap
 - initial draft

