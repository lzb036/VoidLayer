---
version: alpha
name: VoidLayer UI
source: "VoltAgent/awesome-design-md: design-md/linear.app"
baseStyle: Linear
description: "A Linear-inspired dark desktop utility interface for a Windows-only window opacity controller. The UI should feel precise, compact, technical, and calm: near-black canvas, charcoal panels, hairline borders, lavender-blue accent, dense controls, and product UI surfaces as the main visual language."

colors:
  primary: "#5e6ad2"
  on-primary: "#ffffff"
  primary-hover: "#828fff"
  primary-focus: "#5e69d1"
  ink: "#f7f8f8"
  ink-muted: "#d0d6e0"
  ink-subtle: "#8a8f98"
  ink-tertiary: "#62666d"
  canvas: "#010102"
  surface-1: "#0f1011"
  surface-2: "#141516"
  surface-3: "#18191a"
  surface-4: "#191a1b"
  hairline: "#23252a"
  hairline-strong: "#34343a"
  hairline-tertiary: "#3e3e44"
  semantic-success: "#27a644"
  semantic-warning: "#d8a657"
  semantic-danger: "#ef6b73"
  overlay: "#000000"

typography:
  display:
    fontFamily: "Segoe UI Variable, Segoe UI, system-ui, sans-serif"
    fontSize: 32px
    fontWeight: 600
    lineHeight: 1.15
    letterSpacing: 0
  headline:
    fontFamily: "Segoe UI Variable, Segoe UI, system-ui, sans-serif"
    fontSize: 24px
    fontWeight: 600
    lineHeight: 1.2
    letterSpacing: 0
  title:
    fontFamily: "Segoe UI Variable, Segoe UI, system-ui, sans-serif"
    fontSize: 18px
    fontWeight: 600
    lineHeight: 1.3
    letterSpacing: 0
  body:
    fontFamily: "Segoe UI Variable, Segoe UI, system-ui, sans-serif"
    fontSize: 14px
    fontWeight: 400
    lineHeight: 1.5
    letterSpacing: 0
  caption:
    fontFamily: "Segoe UI Variable, Segoe UI, system-ui, sans-serif"
    fontSize: 12px
    fontWeight: 400
    lineHeight: 1.4
    letterSpacing: 0
  button:
    fontFamily: "Segoe UI Variable, Segoe UI, system-ui, sans-serif"
    fontSize: 13px
    fontWeight: 500
    lineHeight: 1.2
    letterSpacing: 0
  mono:
    fontFamily: "Cascadia Mono, Consolas, ui-monospace, monospace"
    fontSize: 12px
    fontWeight: 400
    lineHeight: 1.5
    letterSpacing: 0

rounded:
  xs: 4px
  sm: 6px
  md: 8px
  lg: 12px
  xl: 16px
  pill: 9999px

spacing:
  xxs: 4px
  xs: 8px
  sm: 12px
  md: 16px
  lg: 24px
  xl: 32px
  xxl: 48px

components:
  app-shell:
    backgroundColor: "{colors.canvas}"
    textColor: "{colors.ink}"
    typography: "{typography.body}"
  title-bar:
    backgroundColor: "{colors.canvas}"
    textColor: "{colors.ink-muted}"
    typography: "{typography.caption}"
    height: 40px
  sidebar:
    backgroundColor: "{colors.surface-1}"
    textColor: "{colors.ink-muted}"
    rounded: "{rounded.md}"
    borderColor: "{colors.hairline}"
  panel:
    backgroundColor: "{colors.surface-1}"
    textColor: "{colors.ink}"
    rounded: "{rounded.lg}"
    borderColor: "{colors.hairline}"
    padding: 16px
  panel-raised:
    backgroundColor: "{colors.surface-2}"
    textColor: "{colors.ink}"
    rounded: "{rounded.lg}"
    borderColor: "{colors.hairline-strong}"
    padding: 16px
  button-primary:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.on-primary}"
    typography: "{typography.button}"
    rounded: "{rounded.md}"
    padding: 8px 12px
  button-secondary:
    backgroundColor: "{colors.surface-2}"
    textColor: "{colors.ink}"
    typography: "{typography.button}"
    rounded: "{rounded.md}"
    padding: 8px 12px
  icon-button:
    backgroundColor: "transparent"
    textColor: "{colors.ink-muted}"
    rounded: "{rounded.md}"
    size: 32px
  text-input:
    backgroundColor: "{colors.surface-2}"
    textColor: "{colors.ink}"
    typography: "{typography.body}"
    rounded: "{rounded.md}"
    padding: 8px 10px
  slider:
    trackColor: "{colors.surface-3}"
    fillColor: "{colors.primary}"
    thumbColor: "{colors.ink}"
    height: 4px
  toggle:
    backgroundColor: "{colors.surface-3}"
    selectedColor: "{colors.primary}"
    rounded: "{rounded.pill}"
  status-badge:
    backgroundColor: "{colors.surface-2}"
    textColor: "{colors.ink-muted}"
    typography: "{typography.caption}"
    rounded: "{rounded.pill}"
    padding: 2px 8px
---

## Direction

VoidLayer should use the `linear.app` design language from `awesome-design-md` as its base, adapted for a Windows-only desktop utility. The product should feel like a professional control surface for power users: compact, keyboard-friendly, precise, and quiet.

This is not a marketing site style. Avoid oversized hero sections, decorative gradients, floating promo cards, or large empty editorial layouts. The first screen should be the usable opacity-control workspace.

## Why Linear Fits

- The near-black canvas and charcoal panels match a desktop utility that may run beside other windows for long sessions.
- The sparse lavender-blue accent works well for active opacity, focus states, selected windows, and primary actions.
- Hairline borders and surface levels create hierarchy without bulky shadows.
- Dense product UI panels fit the core workflows: active windows, opacity sliders, presets, rules, hotkeys, and tray behavior.

## Product Adaptation

Use these major surfaces:

- **Active Window Panel**: show the currently selected foreground window, process name, live opacity value, and quick actions.
- **Window List**: dense table/list of visible windows with per-window opacity sliders, lock pins, and restore controls.
- **Preset Rules**: saved rules by app/process/window title, using compact rows and status badges.
- **Hotkeys**: editable shortcuts for increase/decrease/reset/toggle transparency.
- **System Tray Settings**: startup behavior, minimize-to-tray, overlay indicator, and default opacity.

## Visual Rules

- Use `{colors.canvas}` as the app background and `{colors.surface-1}` / `{colors.surface-2}` for panels.
- Use `{colors.primary}` only for primary actions, selected state, focus rings, and active slider fill.
- Keep buttons at `{rounded.md}` and panels at `{rounded.lg}`; do not over-round desktop controls.
- Prefer compact controls over large cards. Toolbars should use icon buttons with tooltips.
- Use Segoe UI Variable / Segoe UI as the primary Windows typeface.
- Keep all letter spacing at `0` for implementation.
- Avoid gradients, decorative blobs, colorful illustrations, and marketing-style page sections.

## Layout Rules

- Default to a two-pane app shell: sidebar/navigation on the left, control workspace on the right.
- Keep the title bar slim and native-feeling; window controls should remain predictable.
- Use stable dimensions for sliders, toolbar buttons, badges, and window rows so the UI does not shift.
- Desktop minimum target size should support a compact utility window, roughly `900x620`.
- Settings dialogs should be functional panels, not full marketing pages.

## Interaction Rules

- Sliders must show exact percentage values and support keyboard adjustment.
- Every destructive or global action needs a clear confirmation or undo path.
- Hotkey capture should visibly show listening/focused state.
- Tray/minimize behavior should be explicit and reversible.
- Active and pinned windows must be visually distinguishable without relying only on color.

## Implementation Notes

- If a frontend framework is introduced later, translate the tokens above into CSS variables or theme tokens first.
- If using Windows-native UI, map these tokens to the platform styling layer rather than forcing web-specific layout ideas.
- Preserve existing functionality over visual purity when there is a conflict.
- Before finishing UI work, verify desktop and narrow layouts and check that labels do not overflow controls.
