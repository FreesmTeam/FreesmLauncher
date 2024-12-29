# Maintainer: windstone
# Maintainer: hand7s
# Contributor: sophrtemin
# Contributor: foxy_chipher

pkgname=freesmlaucnher
pkgver=9.0.2
pkgrel=2
pkgdesc="Minecraft launcher with ability to manage multiple instances."
arch=('i686' 'x86_64' 'aarch64')
url="https://github.com/FreesmTeam/FreesmLauncher"
license=('GPL-3.0-only AND LGPL-3.0-or-later AND LGPL-2.0-or-later AND Apache-2.0 AND MIT AND BSD-2-Clause AND BSD-3-Clause AND LicenseRef-Batch AND OFL-1.1')
depends=('java-runtime=17' 'qt6-networkauth' 'libgl' 'qt6-base' 'qt6-5compat' 'qt6-svg' 'qt6-imageformats' 'zlib' 'hicolor-icon-theme' 'quazip-qt6' 'tomlplusplus' 'cmark')
makedepends=('cmake' 'extra-cmake-modules' 'git' 'jdk17-openjdk' 'scdoc' 'ghc-filesystem' 'gamemode')
optdepends=('glfw: to use system GLFW libraries'
            'openal: to use system OpenAL libraries'
            'visualvm: Profiling support'
            'xorg-xrandr: for older minecraft versions'
            'java-runtime=8: for older minecraft versions'
            'flite: minecraft voice narration')
source=("https://github.com/FreesmTeam/FreesmLauncher/releases/download/${pkgver}/FreesmLauncher-develop.tar.gz")
sha256sums=('SKIP')

prepare() {
  cd "FreesmLauncher-develop"

}

build() {
  cd "FreesmLauncher-develop"

  export PATH="/usr/lib/jvm/java-17-openjdk/bin:$PATH"

  cmake -DCMAKE_BUILD_TYPE= \
    -DCMAKE_INSTALL_PREFIX="/usr" \
    -DLauncher_BUILD_PLATFORM="archlinux" \
    -DLauncher_QT_VERSION_MAJOR="6" \
    -Bbuild -S.
  cmake --build build
}

check() {
  cd "FreesmLauncher-develop/build"
  ctest .
}

package() {
  cd "FreesmLauncher-develop/build"
  DESTDIR="${pkgdir}" cmake --install .
}
