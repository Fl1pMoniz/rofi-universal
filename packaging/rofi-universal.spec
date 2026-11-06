%global rofi_version %(pkg-config --modversion rofi)

Name:           rofi-universal
Version:        1.2.0
Release:        1%{?dist}
Summary:        Unified search mode for Rofi inspired by KRunner and Spotlight

License:        MIT
URL:            https://github.com/Fl1pMoniz/rofi-universal
Source0:        %{url}/releases/download/v%{version}/%{name}-%{version}.tar

BuildRequires:  gcc
BuildRequires:  meson
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(rofi) >= 1.7.6
BuildRequires:  pkgconfig(glib-2.0) >= 2.72
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(json-glib-1.0) >= 1.6

Requires:       rofi = %{rofi_version}
Requires:       json-glib
Recommends:     fd-find
Recommends:     cliphist
Recommends:     wl-clipboard
Recommends:     jq
Recommends:     sqlite
Recommends:     curl
Recommends:     wget

%description
rofi-universal is a native Rofi mode that combines applications, native
calculations, currency conversion, web search, files, clipboard history,
bookmarks, dictionary lookups, GitHub search and weather in one live search
field.
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
* Fri Nov 06 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 1.2.0-1
- Update to rofi-universal 1.2.0.
- Include the tabbed Rofi launcher and refreshed demo.
- Add Fedora binary RPM release packaging and validation workflow.
- RPMs are unsigned; verify the published SHA-256 manifests before installing.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 1.1.0-1
- Update to rofi-universal 1.1.0.
- Replace the external calculator with a native evaluator and Frankfurter
  currency conversion.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 1.0.0-1
- rofi-universal 1.0.0 stable release.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.9.0-1
- Update to rofi-universal 0.9.0 release-candidate preparation.
- Document Debian-family compatibility and provider support status.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.8.0-1
- Update to rofi-universal 0.8.0.
- Add clipboard timeout safety, query regression coverage and FileManager1 tests.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.7.0-1
- Update to rofi-universal 0.7.0.
- Add file reveal action and bounded clipboard decoding.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.6.0-1
- Update to rofi-universal 0.6.0.
- Add URL normalization, clearer result labels and benchmark target.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.5.0-1
- Update to rofi-universal 0.5.0.
- Add inline dictionary and bounded subprocess output support.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.4.0-1
- Update to rofi-universal 0.4.0.
- Include search ranking, provider result and reliability improvements.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.3.1-1
- Update to rofi-universal 0.3.1.
- Build from the verified release source archive.

* Thu Nov 05 2026 Fl1pMoniz <fl1pmoniz@users.noreply.github.com> - 0.3.0-1
- Initial RPM packaging for rofi-universal 0.3.0.
