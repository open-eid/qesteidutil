# package does not work in F26 because openssl-devel is 1.1 version
# but Qt has been built against OpenSSL 1.0 version, so qesteidutil will crash
# Therefore this spec file is for Fedora >= 27 only

Name:           qesteidutil
Version:        3.12.10
Release:        1%{?dist}
Summary:        Estonian ID card utility
License:        LGPLv2+
URL:            https://github.com/open-eid/qesteidutil
Source0:        %{url}/releases/download/v%{version}/%{name}-%{version}.tar.gz

Source1:        config.json
Source2:        config.pub
Source3:        config.rsa
Source4:        config.qrc

Patch0:         sandbox-compilation.patch

BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(Qt5Designer)
BuildRequires:  pkgconfig(libpcsclite) >= 1.7
BuildRequires:  libappstream-glib
BuildRequires:  qtsingleapplication-qt5-devel
Requires:       hicolor-icon-theme
Requires:       opensc%{?_isa}
Requires:       pcsc-lite-ccid%{?_isa}

%description
QEsteidUtil is an application for managing Estonian ID Card. In an
user-friendly interface it is possible to change and unlock PINs, examine
detailed information about personal data file on the card, extract and view
certificates, set up mobile ID, and configure @eesti.ee email.

%prep
%setup -q

cp %{S:1} common/
cp %{S:2} common/
cp %{S:3} common/
cp %{S:4} common/

# Remove bundled qtsingleapplication to make sure it isn't used
rm -rf qtsingleapplication
# COPING is not needed as it is the license for the removed qtsingleapplication
# bundle. See https://bugzilla.redhat.com/show_bug.cgi?id=1519323#c2
rm -rf COPING

%build

%{cmake} . -DBREAKPAD=FALSE

%make_build

%install
%make_install

desktop-file-validate %{buildroot}/%{_datadir}/applications/qesteidutil.desktop

appstream-util validate-relax --nonet %{buildroot}/%{_datadir}/appdata/%{name}.appdata.xml

%post
touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%doc AUTHORS CONTRIBUTING.md README.md RELEASE-NOTES.md
%license LICENSE.LGPL
%{_bindir}/qesteidutil
%{_datadir}/applications/qesteidutil.desktop
%{_datadir}/icons/hicolor/*/apps/qesteidutil.png
%{_datadir}/appdata/*.appdata.xml
%{_mandir}/man1/qesteidutil.1*

%changelog
* Thu Nov 30 2017 Germano Massullo <germano.massullo@gmail.com> - 3.12.10-1
- forced %{cmake} . -DBREAKPAD=FALSE because breakpad does not compile with recent compilers. Source: Raul Metsma (upstream) on IRC chat, which showed me also https://github.com/open-eid/qesteidutil/commit/efdfe4c5521f68f206569e71e292a664bb9f46aa
- adjusted doc file names
- removed OpenSSL 1.1 patch because no longer necessary
- replaced make %{?_smp_mflags} with %make_build (see package review #1519323)
- replaced make install DESTDIR=%{buildroot} with %make_install (see package review #1519323)
- removed line %clean and rm -rf %{buildroot} (see package review #1519323)
- license file attached to %license macro, instead of %doc macro (see package review #1519323)
- Replaced BuildRequires: openssl-devel with BuildRequires: pkgconfig(openssl) (see package review #1519323)
- Replaced BuildRequires: qt5-qttools with BuildRequires: pkgconfig(Qt5Designer) (see package review #1519323)
- Replaced BuildRequires: libpcsclite-devel >= 1.7 with BuildRequires: pkgconfig(libpcsclite) >= 1.7 (see package review #1519323)

* Mon May 15 2017 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.12.4-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_27_Mass_Rebuild

* Fri Feb 24 2017 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.4-3
- Add supprt to openssl v1.1 via openssl_v1.1.patch
- Manually update config.rsa, config.pub and config.json

* Sat Feb 11 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.12.4-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Tue Jan 10 2017 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.4-1
- New upstream release
- Update config.json, config.rsa and config.pub manually, since koji does not
  support networking while building

* Wed Nov 23 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.3-4
- Fixed sandbox-compilation.patch - really include those config files.

* Wed Nov 23 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.3-3
- Fix typo sandbox-compilation.path -> sandbox-compilation.patch

* Wed Nov 23 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.3-2
- Add config.json, config.rsa, config.pub, config.qrc and a patch that ties them
  all together. They are needed because koji does not allow networking during
  build time.

* Sun Jul 10 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.3-1
- New upstream release

* Sun Mar 20 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.2-1
- New upstream release

* Sat Mar 05 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.1-1
- New upstream release

* Tue Feb 02 2016 Mihkel Vain <mihkel@fedoraproject.org> - 3.12.0-1
- New upstream release

* Thu Jun 18 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.10.1-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Fri May  1 2015 Mihkel Vain <mihkel@fedoraproject.org> - 3.10.1-4
- Remove bundled qtsingleapplication (#1195076)

* Sun Apr 19 2015 Mihkel Vain <mihkel@fedoraproject.org> - 3.10.1-3
- Rebuild for gcc5

* Fri Apr 10 2015 Mihkel Vain <mihkel@fedoraproject.org> - 3.10.1-2
- Require pcsc-lite-ccid (#1210639)

* Sat Mar 28 2015 Mihkel Vain <mihkel@fedoraproject.org> - 3.10.1-1
- New upstream release
- Project moved to github
- Use bundled qtsingleapplication since Fedora system version is not
  ready fro QT5 yet. Will use it when it is ready

* Sun Feb 22 2015 Till Maas <opensource@till.name> - 3.8.0.1106-8
- Remove bundled qtsingleapplication

* Tue Nov 18 2014 Dan Horák <dan[at]danny.cz> - 3.8.0.1106-7
- enable internal crash handler only on supported arches

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.8.0.1106-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Sat Jul  5 2014 Mihkel Vain <mihkel@fedoraproject.org> - 3.8.0.1106-5
- New build for updated appdata file (add screenshots)

* Sat Jul  5 2014 Mihkel Vain <mihkel@fedoraproject.org> - 3.8.0.1106-4
- Add appdata support

* Sun Jun 08 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.8.0.1106-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Wed Apr 30 2014 Mihkel Vain <mihkel@fedoraproject.org> - 3.8.0.1106-2
- Add desktop-file-validate to install section
- Change minidump.cc permissions in prep
- Use cmake macro

* Wed Apr 30 2014 Mihkel Vain <mihkel@fedoraproject.org> - 3.8.0.1106-1
- First package based on new source code from RIA

* Sun Aug 04 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.3.1-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.3.1-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.3.1-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.3.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Wed Oct 12 2011 Kalev Lember <kalevlember@gmail.com> - 0.3.1-1
- Update to 0.3.1
- Dropped upstreamed patch

* Fri Apr 15 2011 Kalev Lember <kalev@smartlink.ee> - 0.3.0-5
- Rebuilt for lib11 0.2.8 soname bump

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.3.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Wed Oct 20 2010 Kalev Lember <kalev@smartlink.ee> - 0.3.0-3
- Require hicolor-icon-theme

* Thu Oct 07 2010 Kalev Lember <kalev@smartlink.ee> - 0.3.0-2
- Backported patch to use system qtsingleapplication (#641042)

* Thu Oct 07 2010 Kalev Lember <kalev@smartlink.ee> - 0.3.0-1
- Update to 0.3.0

* Tue May 11 2010 Kalev Lember <kalev@smartlink.ee> - 0.2.0-0.19.svn2685
- BR qt4-webkit-devel (F14+)

* Mon Mar 29 2010 Kalev Lember <kalev@smartlink.ee> - 0.2.0-0.18.svn2685
- Spec file clean up
- Added AUTHORS and COPYING docs
- Cleaned up nightly build changelog entries

* Fri Feb 26 2010 Kalev Lember <kalev@smartlink.ee> - 0.2.0-0.14.svn2499
- rebuilt with Qt 4.6

* Thu Jan 21 2010 Kalev Lember <kalev@smartlink.ee> - 0.2.0-0.11.svn2455
- rebuilt with new libp11

* Sun Jun 14 2009 Kalev Lember <kalev@smartlink.ee> - 0.2.0-0.1.svn714
- Initial RPM release.
