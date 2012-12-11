%global checkout_tag 111212111454

%if 0%{?el5}
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include/boost141 -DBOOST_LIBRARYDIR=%{_libdir}/boost141
%else
%global boost_cmake_flags -DBOOST_INCLUDEDIR=/usr/include
%endif

Name:				ugr
Version:			1.0.1
Release:			0.1.%{checkout_tag}%{?dist}
Summary:			Ugr dynamic storage federation system
Group:				Applications/Internet
License:			ASL 2.0
URL:				https://svnweb.cern.ch/trac/lcgdm/wiki
# svn export http://svn.cern.ch/guest/lcgdm/ugr/trunk ugr
Source0:			http://grid-deployment.web.cern.ch/grid-deployment/dms/lcgutil/tar/%{name}/%{name}-%{version}-%{checkout_tag}.tar.gz 
BuildRoot:			%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%if 0%{?el5}
BuildRequires:		boost141-devel
%else
BuildRequires:		boost-devel
%endif
BuildRequires:		cmake
BuildRequires:		dmlite-devel
BuildRequires:		davix-devel >= 0.0.9
BuildRequires:		GeoIP-devel
BuildRequires:		glibmm24-devel
BuildRequires:		libmemcached-devel
BuildRequires:		lfc-devel
BuildRequires:		protobuf-devel


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

%package dav-plugin
Summary:			Http and WebDav plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release} 

%description dav-plugin
Plugin for the WebDav based storage system for %{name}

%package dmlite-plugin
Summary:                        dmlite plugin for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}

%description dmlite-plugin
Plugin for using dmlite for %{name}


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
%{boost_cmake_flags} \
.
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%post
/sbin/ldconfig


%postun
/sbin/ldconfig


%files
%defattr (-,root,root)
%{_libdir}/libugrconnector.so.*
%{_libdir}/ugr/libugrgeoplugin_geoip.so
%config(noreplace) %{_sysconfdir}/ugr.conf
%doc RELEASE-NOTES

%files devel
%defattr (-,root,root)
%{_libdir}/libugrconnector.so
%dir %{_includedir}/ugr
%{_includedir}/ugr/*
%{_libdir}/pkgconfig/*

%files dav-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_dav.so
%{_libdir}/ugr/libugrlocplugin_http.so



%files dmlite-plugin
%defattr (-,root,root)
%{_libdir}/ugr/libugrlocplugin_dmliteclient.so
%{_libdir}/ugr/libugrdmlite.so
%config(noreplace) %{_sysconfdir}/ugr/ugrdmlite.conf
%config(noreplace) %{_sysconfdir}/httpd/conf.d/zlcgdm-ugr-dav.conf
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientORA.conf
%config(noreplace) %{_sysconfdir}/ugr/ugrdmliteclientMY.conf

%changelog
* Fri Jun 01 2012 Adrien Devresse <adevress at cern.ch> - 0.0.2-0.1-2012052812snap
 - initial draft

