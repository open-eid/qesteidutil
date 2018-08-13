# ID-card utility

![European Regional Development Fund][ERDF-link]

* License: [LGPL 2.1][license-link]
* &copy; Estonian Information System Authority
* [Architecture of ID-software][open-eid-github-link]

| Linux | Windows | Coverity |
| --- | --- | --- |
| [![Linux Build Status][travis-img]][travis-link] | [![Windows Build Status][appveyor-img]][appveyor-link] | [![Coverity Scan Build Status][coverity-img]][coverity-link] |

  - [Building](#building)
    - [Linux](#linux)
    - [OSX](#osx)
    - [Windows](#windows)
  - [Support](#support)

## Building

### Linux

**1. Install dependencies**
```
# Ubuntu
sudo apt-get install cmake qttools5-dev qttools5-dev-tools libpcsclite-dev libssl-dev
# Fedora
sudo dnf install cmake desktop-file-utils openssl-devel qt5-qttools-devel pcsc-lite-devel libappstream-glib-devel qtsingleapplication-qt5-devel
```

**2. Clone the source**
```
git clone --recurse-submodules git@github.com:open-eid/qesteidutil.git
## OR git < 2.13
git clone --recursive git@github.com:open-eid/qesteidutil.git
cd qesteidutil
```

**3. Configure**
```
mkdir build
cd build
cmake ..
```

**4. Build**

```
make
```

**5. Install**

```
sudo make install
```

**6. Execute**

```
/usr/local/bin/qesteidutil
```

### OSX

**1. Install dependencies from**
- [XCode]
- [CMake]
- [Qt]

Since Qt 5.6 default SSL backend is SecureTransport and this project depends openssl.  
See how to build [OSX Qt from source](#building-osx-qt-from-source)

**2. Clone the source**
```
git clone --recurse-submodules git@github.com:open-eid/qesteidutil.git
## OR git < 2.13
git clone --recursive git@github.com:open-eid/qesteidutil.git
cd qesteidutil
```

**3. Configure**
```
mkdir build
cd build
cmake -DQt5_DIR="~/Qt/5.5/clang_64/lib/cmake/Qt5" ..
```

**4. Build**
```
make
```

**5. Install**
```
sudo make install
```

**6. Execute**
```
open /usr/local/bin/qesteidutil.app
```

#### Building OSX Qt from source

```
brew install openssl

curl -O -L http://download.qt.io/official_releases/qt/5.9/5.9.3/submodules/qtbase-opensource-src-5.9.3.tar.xz
tar xf qtbase-opensource-src-5.9.3.tar.xz
cd qtbase-opensource-src-5.9.3
./configure -prefix /Developer/Qt-5.9.3 -opensource -nomake tests -nomake examples -no-securetransport -openssl-runtime -confirm-license -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib
make
sudo make install
cd ..
rm -rf qtbase-opensource-src-5.9.3

curl -O -L http://download.qt.io/official_releases/qt/5.9/5.9.3/submodules/qttools-opensource-src-5.9.3.tar.xz
tar xf qttools-opensource-src-5.9.3.tar.xz
cd qttools-opensource-src-5.9.3
/Developer/Qt-5.9.3/bin/qmake
make
sudo make install
cd ..
rm -rf qttools-opensource-src-5.9.3
```

### Windows

**1. Install dependencies from**

- [Visual Studio Community][vs-link]
- [CMake]
- [Qt]

**2. Clone the source**
```
git clone --recurse-submodules git@github.com:open-eid/qesteidutil.git
## OR git < 2.13
git clone --recursive git@github.com:open-eid/qesteidutil.git
cd qesteidutil
```

**3. Configure**
```
mkdir build
cd build
cmake -G"NMAKE Makefiles" -DQt5_DIR="C:\Qt\5.9\msvc2015\lib\cmake\Qt5" ..
```

**4. Build**
```
nmake
```

**6. Execute**
```
qesteidutil.exe
```

## Support

Official builds are provided through official distribution point [installer.id.ee]. If you want support, you need to be using official builds. Contact for assistance by email [abi@id.ee] or [www.id.ee].

Source code is provided on "as is" terms with no warranty (see license for more information). Do not file Github issues with generic support requests.

<!-- links -->
[ERDF-link]: https://github.com/e-gov/RIHA-Frontend/raw/master/logo/EU/EU.png "European Regional Development Fund - DO NOT REMOVE THIS IMAGE BEFORE 05.03.2020"
[license-link]: LICENSE.LGPL
[open-eid-github-link]: http://open-eid.github.io
[www.id.ee]: https://www.id.ee/
[installer.id.ee]: https://installer.id.ee
[abi@id.ee]: mailto:abi@id.ee
[XCode]: https://itunes.apple.com/app/xcode/id497799835?mt=12
[CMake]: https://cmake.org/install/
[Qt]: https://doc.qt.io/qt-5.6/osx.html
[vs-link]: https://www.visualstudio.com/downloads/
[travis-img]: https://travis-ci.org/open-eid/qesteidutil.svg?branch=master
[travis-link]: https://travis-ci.org/open-eid/qesteidutil

[appveyor-img]: https://ci.appveyor.com/api/projects/status/github/open-eid/qesteidutil?branch=master&svg=true
[appveyor-link]: https://ci.appveyor.com/project/open-eid/qesteidutil

[coverity-img]: https://scan.coverity.com/projects/737/badge.svg
[coverity-link]: https://scan.coverity.com/projects/737
