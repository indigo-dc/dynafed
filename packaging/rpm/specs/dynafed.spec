%if 0%{?el5}
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include/boost141 -DBOOST_LIBRARYDIR=%{_libdir}/boost141
%else
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include
%endif

Name:				dynafed
Version:			1.0.8
Release:			1%{?dist}
Summary:			Ultra-scalable dynamic system for federating HTTP-based storage resources
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
BuildRequires:		openssl-devel
BuildRequires:		protobuf-devel

Requires:               rsyslog
# name transition ugr -> dynafed
Obsoletes:              ugr < 1.0.8


%description
The Dynamic Federations project provides a dynamic, scalable HTTP resource federation mechanism for distributed storage systems.
The default deployment style is accessible by any HTTP/Webdav compatible client. The core components can be used to design frontends based on other protocols.

%package private-devel
Summary:			Development files for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release} 
Requires:			pkgconfig
# name transition ugr -> dynafed
Obsoletes:                      ugr-devel < 1.0.8

%description private-devel
Headers files for %{name}'s plugin development.

%package http-plugin
Summary:			Http and WebDav plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release} 
Provides:                       %{name}-dav-plugin = %{version}-%{release}
# name transition ugr -> dynafed
Obsoletes:                      ugr-http-plugin < 1.0.8
Obsoletes:                      ugr-dav-plugin < 1.0.8

%description http-plugin
Plugin for the WebDav based storage system for %{name}

%package lfc-plugin
Summary:			Logical File catalog (LFC) plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release}
Requires:			gfal2-plugin-lfc%{?_isa}
# name transition ugr -> dynafed
Obsoletes:                      ugr-lfc-plugin < 1.0.8

%description lfc-plugin
Plugin for the Logical File catalog system for %{name}

%package dmlite-plugin
Summary:                        dmlite plugin for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}

%description dmlite-plugin
Plugin for using dmlite for %{name}

%package dmlite-frontend
Summary:                        dmlite plugin for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}
Requires:                       %{_libdir}/httpd/modules/mod_lcgdm_dav.so
Requires:                       dmlite-libs%{?_isa} >= 0.7.0
# name transition ugr -> dynafed
Obsoletes:                      ugr-dmlite-plugin < 1.0.8
Obsoletes:                      ugr-dmlite-frontend < 1.0.8

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
## conf file plugin path transition
sed -i 's@/usr/lib\([0-9]*\)/ugr@/usr/lib\1/dynafed@g' /etc/ugr.conf || true

%postun
/sbin/ldconfig

%post dmlite-frontend
/sbin/ldconfig
/sbin/service rsyslog condrestart || true

%files
%defattr (-,root,root)
%{_libdir}/libugrconnector.so.*
%{_libdir}/dynafed/libugrgeoplugin_geoip.so
%{_libdir}/dynafed/libugrnoloopplugin.so
%config(noreplace) %{_sysconfdir}/ugr.conf
%config(noreplace) %{_sysconfdir}/rsyslog.d/*
%config(noreplace) %{_sysconfdir}/logrotate.d/*
%doc RELEASE-NOTES

%files private-devel
%defattr (-,root,root)
%{_libdir}/libugrconnector.so
%dir %{_includedir}/ugr
%{_includedir}/ugr/*
%{_libdir}/pkgconfig/*

%files http-plugin
%defattr (-,root,root)
%{_libdir}/dynafed/libugrlocplugin_dav.so
%{_libdir}/dynafed/libugrlocplugin_http.so
%{_libdir}/dynafed/libugrlocplugin_s3.so
%{_libdir}/dynafed/libugrlocplugin_davrucio.so


%files lfc-plugin
%defattr (-,root,root)
%{_libdir}/dynafed/libugrlocplugin_lfc.so


%files dmlite-plugin
%defattr (-,root,root)
%{_libdir}/dynafed/libugrlocplugin_dmliteclient.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientORA.conf
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientMY.conf


%files dmlite-frontend
%{_libdir}/dynafed/libugrdmlite.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmlite.conf
%config(noreplace) %{_sysconfdir}/httpd/conf.d/zlcgdm-ugr-dav.conf


%changelog
* Fri Jun 01 2012 Adrien Devresse <adevress at cern.ch> - 0.0.2-0.1-2012052812snap
 - initial draft
