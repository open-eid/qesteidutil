# ID-card utility

<!-- European Regional Development Fund - DO NOT REMOVE THIS IMAGE BEFORE 05.03.2020 -->
![European Regional Development Fund](https://github.com/e-gov/RIHA-Frontend/raw/master/logo/EU/EU.png)

* License: [LGPL 2.1](LICENSE.LGPL)
* &copy; Estonian Information System Authority
* [Architecture of ID-software](https://open-eid.github.io/)

[![Linux Build Status](https://travis-ci.org/open-eid/qesteidutil.svg?branch=master)](https://travis-ci.org/open-eid/qesteidutil)
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/open-eid/qesteidutil?branch=master&svg=true)](https://ci.appveyor.com/project/open-eid/qesteidutil)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/737/badge.svg)](https://scan.coverity.com/projects/737)


  - [Building](#building)
    - [Linux](#linux)
    - [OSX](#osx)
    - [Windows](#windows)
  - [Support](#support)

NB! This repository is not maintained and supported any more. Use [Digidoc4 client](https://github.com/open-eid/DigiDoc4-Client) for alternative solution.

## Building

### Linux

**1. Install dependencies**

    # Ubuntu
    sudo apt-get install cmake qttools5-dev qttools5-dev-tools libpcsclite-dev libssl-dev
    # Fedora
    sudo dnf install cmake desktop-file-utils openssl-devel qt5-qttools-devel pcsc-lite-devel libappstream-glib-devel qtsingleapplication-qt5-devel

**2. Clone the source**

    git clone --recurse-submodules git@github.com:open-eid/qesteidutil.git
    ## OR git < 2.13
    git clone --recursive git@github.com:open-eid/qesteidutil.git
    cd qesteidutil

**3. Configure**

    mkdir build
    cd build
    cmake ..

**4. Build**

    make

**5. Install**

    sudo make install

**6. Execute**

    /usr/local/bin/qesteidutil

### OSX

**1. Install dependencies from**
- [XCode](https://itunes.apple.com/app/xcode/id497799835?mt=12)
- [CMake](https://cmake.org/install/)
- [Qt](https://doc.qt.io/qt-5.9/osx.html)

Since Qt 5.6 default SSL backend is SecureTransport and this project depends openssl.  
See how to build [OSX Qt from source](#building-osx-qt-from-source)

**2. Clone the source**

    git clone --recurse-submodules git@github.com:open-eid/qesteidutil.git
    ## OR git < 2.13
    git clone --recursive git@github.com:open-eid/qesteidutil.git
    cd qesteidutil

**3. Configure**

    mkdir build
    cd build
    cmake -DQt5_DIR="~/Qt/5.9/clang_64/lib/cmake/Qt5" ..

**4. Build**

    make

**5. Install**

    sudo make install

**6. Execute**

    open /usr/local/bin/qesteidutil.app

#### Building OSX Qt from source

    brew install openssl

    curl -O -L http://download.qt.io/official_releases/qt/5.9/5.9.6/submodules/qtbase-opensource-src-5.9.6.tar.xz
    tar xf qtbase-opensource-src-5.9.6.tar.xz
    cd qtbase-opensource-src-5.9.6
    ./configure -prefix /Developer/Qt-5.9.6 -opensource -nomake tests -nomake examples -no-securetransport -openssl-runtime -confirm-license -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib
    make
    sudo make install
    cd ..
    rm -rf qtbase-opensource-src-5.9.6

    curl -O -L http://download.qt.io/official_releases/qt/5.9/5.9.6/submodules/qttools-opensource-src-5.9.6.tar.xz
    tar xf qttools-opensource-src-5.9.6.tar.xz
    cd qttools-opensource-src-5.9.6
    /Developer/Qt-5.9.6/bin/qmake
    make
    sudo make install
    cd ..
    rm -rf qttools-opensource-src-5.9.6

### Windows

**1. Install dependencies from**

- [Visual Studio Community](https://www.visualstudio.com/downloads/)
- [CMake](https://cmake.org/install/)
- [Qt](https://doc.qt.io/qt-5.9/windows-support.html)

**2. Clone the source**

    git clone --recurse-submodules git@github.com:open-eid/qesteidutil.git
    ## OR git < 2.13
    git clone --recursive git@github.com:open-eid/qesteidutil.git
    cd qesteidutil

**3. Configure**

    mkdir build
    cd build
    cmake -G"NMAKE Makefiles" -DQt5_DIR="C:\Qt\5.9\msvc2015\lib\cmake\Qt5" ..

**4. Build**

    nmake

**6. Execute**

    qesteidutil.exe

## Support

Official builds are provided through official distribution point [installer.id.ee](https://installer.id.ee). If you want support, you need to be using official builds. Contact for assistance by email [abi@id.ee](mailto:abi@id.ee) or [www.id.ee](https://www.id.ee/).

Source code is provided on "as is" terms with no warranty (see license for more information). Do not file Github issues with generic support requests.
