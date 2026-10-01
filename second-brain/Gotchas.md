---
tags: [gotcha, engineering]
---

# Gotchas

## Other landmines

- **Lan Mouse pointer locks must cover layer-shell surfaces.** In
  `external/dwl/dwl.c`, `motionnotify()` previously enforced locks only for
  normal `Client` windows. Lan Mouse uses an edge layer surface, so reversing
  mouse direction could leave the host's capture strip and return control
  prematurely, even after traveling far into the guest. Enforce a focused
  surface's locked-pointer constraint before resolving a normal window;
  continue sending relative motion and retain existing window confinement.
  Build verified against wlroots 0.20; user confirmed the runtime fix works.

- **Layout switching must include virtual keyboards.** Lan Mouse's incoming
  keyboard has its own `KeyboardGroup`; changing only `kb_group` leaves remote
  typing on the old layout. Track keyboard groups through creation/destruction
  and apply `setxkbrules()` to all group keymaps and virtual device keymaps.
  Layout cycling uses the same path, and the previously active seat keyboard
  is restored after updates. This changes the receiving machine's layout,
  not the sending machine's. Build verified; remote Alt+A / Alt+R retest pending.

Related: [[Home]].
