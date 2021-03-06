# eCap: The Code in the Middle (http://www.e-cap.org/)
Name: clearos-ecap-adapter
Version: 2.1
Release: 3%{dist}
Vendor: The Measurement Factory
License: GPL
Group: System Environment/Libraries
Packager: ClearFoundation
Source: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}
Summary: ClearOS eCAP Adapter Library
BuildRequires: autoconf >= 2.63
BuildRequires: automake
BuildRequires: libtool
BuildRequires: expat-devel
BuildRequires: libecap-devel >= 1.0.0
Requires: squid >= 7:3.3.8-11
Requires: app-web-proxy-core >= 1:1.6.2-1
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
The ClearOS eCAP Adapter can be configured to add custom headers to outbound HTTP requests through Squid.

# Build
%prep
%setup -q
./autogen.sh
%{configure}

%build
make %{?_smp_mflags}

# Install
%install
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.a
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.la
mkdir -p -m 755 $RPM_BUILD_ROOT%{_sysconfdir}/clearos
cp clearos-ecap-adapter.conf $RPM_BUILD_ROOT%{_sysconfdir}/clearos/ecap-adapter.conf
mkdir -p -m 755 $RPM_BUILD_ROOT%{_sysconfdir}/squid
cp squid_ecap.conf $RPM_BUILD_ROOT%{_sysconfdir}/squid

# Clean-up
%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

# Post install
%post
/sbin/ldconfig

# Post uninstall
%postun
/sbin/ldconfig

# Files
%files
%defattr(-,root,root)
%{_libdir}/libclearos-ecap-adapter.so*
%config(noreplace) %attr(0640,root,squid) %{_sysconfdir}/clearos/ecap-adapter.conf
%{_sysconfdir}/squid/squid_ecap.conf

# vi: expandtab shiftwidth=4 softtabstop=4 tabstop=4
