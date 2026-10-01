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
  Build verified against wlroots 0.20; two-machine runtime retest pending.

Related: [[Home]].
