#
# spec file for package prismlauncher
#
# Copyright (c) 2024 SUSE LLC
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://bugs.opensuse.org/
#

%bcond_with qt6

# Change this variables if you want to use custom keys
# Leave blank if you want to build PrismLauncher without MSA id or curseforge api key
%define msa_id default
%define curseforge_key default

%if %{with qt6}
%global qt_major 6
%global min_qt_version 6
%else
%global qt_major 5
%global min_qt_version 5.12
%endif

%global _name   FreesmLauncher
Name:           freesmlauncher
Version:        9.0
Release:        0
Summary:        An open source launcher for Minecraft
License:        GPL-3.0-only
Group:          Amusements/Games/Other
URL:            https://freesmlauncher.windstone.space/
Source:         %{name}-%{version}.tar.xz
BuildRequires:  cmake >= 3.15
BuildRequires:  extra-cmake-modules
BuildRequires:  gcc-c++
BuildRequires:  hicolor-icon-theme
BuildRequires:  java-devel
BuildRequires:  cmake(Qt%{qt_major}Concurrent) >= %{min_qt_version}
BuildRequires:  cmake(Qt%{qt_major}Core) >= %{min_qt_version}
BuildRequires:  cmake(Qt%{qt_major}Gui) >= %{min_qt_version}
BuildRequires:  cmake(Qt%{qt_major}Network) >= %{min_qt_version}
BuildRequires:  cmake(Qt%{qt_major}Test) >= %{min_qt_version}
BuildRequires:  cmake(Qt%{qt_major}Widgets) >= %{min_qt_version}
BuildRequires:  cmake(Qt%{qt_major}Xml) >= %{min_qt_version}
%if %{with qt6}
BuildRequires:  cmake(Qt6Core5Compat)
%endif
BuildRequires:  pkgconfig(scdoc)
BuildRequires:  zlib-devel
# PrismLauncher requires QuaZip >= 1.3
%if 0%{?suse_version} >= 1550
BuildRequires:  cmake(QuaZip-Qt%{qt_major})
%endif
BuildRequires:  strip-nondeterminism
Recommends:     java-openjdk-headless
# xrandr needed for LWJGL [2.9.2, 3) https://github.com/LWJGL/lwjgl/issues/128
Recommends:     xrandr

%description
A custom launcher for Minecraft that allows you to easily manage
multiple installations of Minecraft at once. (Fork of PrismLauncher)

%prep
%setup -q

# Do not set RPATH and timestamp
sed -i -e "s|\$ORIGIN/||" -e "s|\${TODAY}|unknown|" CMakeLists.txt

%build
%cmake \
    -DLauncher_QT_VERSION_MAJOR="%{qt_major}" \
    -DLauncher_BUILD_PLATFORM="openSUSE" \
    %if "%{msa_id}" != "default"
    -DLauncher_MSA_CLIENT_ID="%{msa_id}" \
    %endif
    %if "%{curseforge_key}" != "default"
    -DLauncher_CURSEFORGE_API_KEY="%{curseforge_key}" \
    %endif

%cmake_build

%install
%cmake_install

# rpm can't strip binary automatically so strip the binary here
strip -s %{buildroot}%{_bindir}/%{name}

# strip timestamps from jar files to avoid rebuilds
strip-nondeterminism %{buildroot}%{_datadir}/%{name}/{NewLaunch,JavaCheck}.jar

%files
%doc README.md
%license LICENSE COPYING.md
%dir %{_datadir}/%{name}/
%{_bindir}/%{name}
%{_datadir}/%{name}/NewLaunch.jar
%{_datadir}/%{name}/JavaCheck.jar
%{_datadir}/applications/org.%{name}.%{_name}.desktop
%{_datadir}/metainfo/org.%{name}.%{_name}.metainfo.xml
%{_datadir}/mime/packages/modrinth-mrpack-mime.xml
%{_datadir}/man/man?/%{name}.*
%{_datadir}/icons/hicolor/scalable/apps/org.%{name}.%{_name}.svg

%changelog
