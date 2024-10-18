Name: ofono-binder-plugin-ext-qti

Version: 1.0.0
Release: 1
Summary: QTI IRadio extension ofono binder plugin
License: BSD
URL: https://gitlab.com/ubports/development/core/hybris-support/ofono-binder-plugin-ext-qti
Source: %{name}-%{version}.tar.bz2
Provides: ofono-ims-support

BuildRequires: ofono-devel
BuildRequires: pkgconfig
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libglibutil)
BuildRequires: pkgconfig(libgbinder-radio)
BuildRequires: pkgconfig(libofonobinderpluginext)
BuildRequires: libhybris-devel

%define plugin_dir %(pkg-config ofono --variable=plugindir)
%define config_dir /etc/ofono/binder.d/

%description
QTI IRadio extension for ofono binder plugin

%prep
%setup -q -n %{name}-%{version}

%build
make %{_smp_mflags} PLUGINDIR=%{plugin_dir} KEEP_SYMBOLS=1 release

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} PLUGINDIR=%{plugin_dir} install
mkdir -p %{buildroot}%{config_dir}
install -m 644 qti.conf %{buildroot}%{config_dir}

%files
%dir %{plugin_dir}
%dir %{config_dir}
%defattr(-,root,root,-)
%config %{config_dir}/qti.conf
%{plugin_dir}/qtibinderpluginext.so
