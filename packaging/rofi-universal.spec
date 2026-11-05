Name:           rofi-universal
Version:        0.3.1
Release:        1%{?dist}
Summary:        Unified search mode for Rofi inspired by KRunner and Spotlight

License:        MIT
URL:            https://github.com/Fl1pMoniz/rofi-universal
Source0:        %{url}/releases/download/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  meson
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(rofi) >= 1.7.6
BuildRequires:  pkgconfig(glib-2.0) >= 2.72
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  pkgconfig(cairo)

Requires:       rofi
Recommends:     qalculate
Recommends:     fd-find
Recommends:     cliphist
Recommends:     wl-clipboard
Recommends:     jq
Recommends:     sqlite
Recommends:     curl
Recommends:     wget

%description
rofi-universal is a native Rofi mode that combines applications, calculations,
currency and unit conversions, web search, files, clipboard history, bookmarks,
dictionary lookups, GitHub search and weather in one live search field.
It is inspired by KDE KRunner and macOS Spotlight, uses ordinary command-line
tools, and does not require a background daemon or an AI service.

%prep
%autosetup

%build
%meson -Dwerror=true
%meson_build

%check
%meson_test

%install
%meson_install

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/rofi-universal
%{_libdir}/rofi/libuniversal.so
%{_datadir}/rofi-universal/config.example.ini

%changelog
* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.3.1-1
- Update to rofi-universal 0.3.1.
- Build from the verified release source archive.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.3.0-1
- Initial RPM packaging for rofi-universal 0.3.0.
