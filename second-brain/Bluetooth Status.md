---
tags: [engineering, reference]
---

# Bluetooth Status

`dwlb-status/dwlb-status.c` shows the default Bluetooth controller's radio
power beside AirPods using only the Bluetooth icon: green `#7FD88F` when
on, pink `#FF79C6` when off, and gray when unavailable (no text labels).

- Left-click runs `dwlb-status --bluetooth-toggle`.
- Reads `bluetoothctl show`; toggles with `bluetoothctl power on/off`.
- Enabling first runs `rfkill unblock bluetooth`; hardware blocks still need
  to be cleared physically. Requires `bluetoothctl`, `rfkill`, and `timeout`.
- Commands have timeouts; query/toggle failures produce a notification.
- Signal 2 refreshes Bluetooth; signal 3 refreshes AirPods after a toggle.
- External changes are picked up by polling every 3.6 seconds.
- `dwlb` ignores `SIGCHLD`, inherited by click-launched helpers. Startup
  restores its default disposition so `pclose`/`system` can collect exit
  statuses instead of falsely reporting command failures.

Build/install with `make -C dwlb-status install`, then restart the status bar.

Related: [[Gotchas]].
