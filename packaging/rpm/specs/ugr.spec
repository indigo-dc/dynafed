%define checkout_tag 2012052812snap

Name:				ugr
Version:			0.0.2
Release:			0.1.%{checkout_tag}%{?dist}
Summary:			Ugr dynamic storage federation system
Group:				Applications/Internet
License:			ASL 2.0
URL:				https://svnweb.cern.ch/trac/lcgdm/wiki
# svn export http://svn.cern.ch/guest/lcgdm/ugr/trunk ugr
Source0:			http://grid-deployment.web.cern.ch/grid-deployment/dms/lcgutil/tar/%{name}/%{name}-%{version}-%{checkout_tag}.tar.gz 
BuildRoot:			%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:		cmake
BuildRequires:		doxygen
BuildRequires:		lfc-devel
BuildRequires:		dmlite-devel
BuildRequires:		dmlite-utils
BuildRequires:		davix-devel
BuildRequires:		boost-devel
BuildRequires:		GeoIP-devel
BuildRequires:		protobuf-devel
BuildRequires:		protobuf
BuildRequires:		libmemcached-devel
BuildRequires:		libmemcached

%description
Ugr provides a powerfull, fast and scalable dynamic storage federation 
system for grid and clouds.
It is able to federate several distributed storage systems 
in one consistent namespace in a transparent manner for the client side.
Ugr is accessible by any HTTP/Webdav compatible client.

%package devel
Summary:			Development files for %{name}
Group:				Applications/Internet
Requires:			%{name}-core%{?_isa} = %{version}-%{release} 
Requires:			pkgconfig

%description devel
development files for %{name}

%package doc
Summary:			Documentation for %{name}
Group:				Applications/Internet
Requires:			%{name}-core%{?_isa} = %{version}-%{release} 

%description doc
documentation, Doxygen and examples of %{name} .

%package dav-plugin
Summary:			WebDav plugin for %{name}
Group:				Applications/Internet
Requires:			%{name}-core%{?_isa} = %{version}-%{release} 

%description dav-plugin
Plugin for the WebDav based stroage system

%clean
rm -rf %{buildroot};
make clean

%prep
%setup -q

%build
mkdir build
cd build
%cmake \
-DDOC_INSTALL_DIR=%{_docdir}/%{name}-%{version} \
-DBOOST_INCLUDEDIR=/usr/include/boost141 \
-DBOOST_LIBRARYDIR=/usr/lib64/boost141 \
-DDMLITE_LIBRARY=/usr/lib64/libdmlite.so \
-DDMLITECOMMON_LIBRARY=/usr/lib64/libdmlitecommon.so \
../
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
cd build
make DESTDIR=%{buildroot} install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr (-,root,root)
%{_libdir}/libugrconnector.so.*
%{_docdir}/%{name}-%{version}/RELEASE-NOTES

%files devel
%defattr (-,root,root)
%{_libdir}/libugrconnector.so
%{_includedir}/ugr/*
%{_libdir}/pkgconfig/*

%files dav-plugin
%defattr (-,root,root)
%{_libdir}/libugrlocplugin_dav.so

%files doc
%defattr (-,root,root)
%{_docdir}/%{name}-%{version}/html/*

%changelog
* Fri Jun 01 2012 Adrien Devresse <adevress at cern.ch> - 0.0.2-0.1-2012052812snap
 - initial draft
