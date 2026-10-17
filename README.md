# <img src="resources/lantern.svg" width="48" height="48" alt=""> Lantern

A lightweight IP scanner for [Omarchy](https://omarchy.org/), built with C++ and Qt Quick.

## Features

- Scan an IPv4 address, inclusive range, or CIDR subnet.
- Discover live hosts with ICMP ping and optional common TCP port checks.
- Resolve hostnames using reverse DNS where available.
- Show ping response times and open TCP ports as results arrive.
- Hide dead IPs, search results, and filter by host status.
- Sort columns, copy host details, and export all results to CSV.
- Stop a scan while preserving completed results.
- Follow the active Omarchy theme, including live changes.

## Install

Build from source using the instructions below. The repository includes an automated
release workflow for x86_64 and ARM64 Arch packages. Once a release is available,
download the package matching your architecture from
[Releases](https://github.com/seth-reee/Lantern/releases) and install it with:

```bash
sudo pacman -U ./lantern-VERSION-1-x86_64.pkg.tar.zst
```

Use the `aarch64` package on ARM64. Install `qt6-wayland` for native Wayland support.
Launch **Lantern** from the application menu or run `lantern`.

## Use

Enter one address (`192.168.1.1`), an inclusive range (`192.168.1.1-192.168.1.254`), or CIDR (`192.168.1.0/24`). CIDR excludes network/broadcast addresses except /31 and /32. Maximum 4,096 targets. Local /24 uses the first active non-loopback IPv4 interface, limited to /24 for a manageable default; choose the intended range on machines with multiple interfaces or VPNs.

Scan sends one ICMP echo per address using iputils. Optional common-port detection tries TCP connections to 23 ports: 21, 22, 23, 25, 53, 80, 110, 139, 143, 443, 445, 587, 631, 993, 995, 1433, 3306, 3389, 5432, 5900, 8000, 8080, 8443. It checks whether connections open; it does not fingerprint services or scan UDP. The timeout applies to each probe; host concurrency is configurable from 1–64 (default 32).

- **Live:** a ping reply or at least one open TCP port.
- **No reply:** no ping response and no open tested ports; a host might still be online behind a firewall.
- **Error:** ping could not execute or reported an operational error, and TCP did not establish liveness. Select the host to see the error.
- **Cancelled:** not completed when stopped. Partial results remain available.

Reverse DNS is attempted for live hosts with a two-second deadline. A blank name means no reverse DNS name was obtained; not every device publishes one. Ping latency is shown only when ICMP responds. MAC/vendor discovery, NetBIOS/mDNS names, service fingerprinting, and continuous monitoring are not implemented.

Enable **Hide dead IPs** to hide hosts with **No reply** status while retaining them in scan totals and CSV exports. Filter by IP/name/port/status, click column headings to sort, right-click to copy a host, or double-click to copy its IP. Ctrl+F focuses search; Escape stops a running scan. CSV export includes **all** rows, including cancelled/error rows, regardless of view filters, with proper quoting and atomic writes.

## Build

Dependencies: `base-devel`, `cmake`, `ninja`, `qt6-base`, `qt6-declarative`, and
`iputils`. Requires Qt 6.8+, CMake 3.24+, and a C++20 compiler.

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/lantern
```

For a user installation:

```bash
cmake --install build --prefix "$HOME/.local"
```

To build the Arch package from a published release, run `makepkg -s` in
`packaging/`. The recipe downloads and verifies the pinned source archive.

Theme colors load from `$XDG_STATE_HOME/omarchy/current/theme/colors.toml`
(default `~/.local/state`) and refresh within two seconds. A fallback palette
supports other desktops.

Tests cover range validation, scan lifecycle, cancellation, filtering, CSV export,
error handling, and populated Qt Quick results. A loopback TCP integration test is
skipped if the environment blocks local sockets. An offscreen startup check is
also included. These checks do not establish real LAN or ARM desktop behavior.

## License

Lantern is made by [seth-reee](https://github.com/seth-reee) and is available under
the [MIT License](LICENSE).

## Automated releases

Every push to `master` prepares the next patch version, builds native x86_64 and
ARM64 Arch packages, and publishes a GitHub release after both architecture jobs
pass. The workflow updates the version and pinned source checksum, tags the tested
commit, and verifies uploaded asset checksums before publishing. An explicitly
increased, untagged version is respected.

Pull requests build development packages without publishing. Manual runs on
`master` also release; matching version tags build and publish that version.
Published releases are verified on retries and never replaced.

CI runs the scanner and UI tests, checks package metadata and ELF architecture,
and runs the packaged executable offscreen. CI does not establish real ARM
desktop, Wayland, hardware GPU, or live LAN behavior.
