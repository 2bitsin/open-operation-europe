---
template: ui-frames
title: Screen frame, panels and dialogs
pack_class: [dialogs, pictures]
ids:
  - frame/*
  - dialog/*
count: 15
scale: 4
background: opaque
recolour: none
references:
  - frame/top-bar
  - frame/map-frame
  - frame/message-panels
  - dialog/number-dialog
  - dialog/supply-panel
  - dialog/yes
rules: {}
---
# Screen frame, panels and dialogs

## What it is

The game's chrome: `frame/top-bar`, `frame/map-frame`, `frame/message-panels` (the turn screen around the map),
`frame/strip-<n>` (the six scenario title strips, with the operation's name in capitals on camouflage),
`dialog/number-dialog`, `dialog/supply-panel` (riveted grey metal panels with black wells where the game draws
numbers and pictures) and the YES / NO buttons, released and pressed. The game draws text, numbers and pictures
into the black wells at fixed pixel positions.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x; 2x
the alternative; one scale for the whole pack).

## Style references

The classic panel ({{source}}) is the exact layout; the class references {{references}} fix the material (riveted
gunmetal, bevels, camouflage) for the whole set.

## Constraints

- Opaque. Every well, button, arrow and edge stays at the classic position and size to the game pixel: the game draws
  into those rectangles. The wells stay flat black.
- One bevel direction (light from the top left) and one rivet and bevel size across the class.
- Text: the strips' operation names and YES / NO keep their exact words, in a period stencil or typewriter face;
  pressed buttons are the released ones pushed in. No other text.

## Prompt

User-interface panel for a 1990s World War II strategy game, remastered as high-resolution riveted gunmetal and
camouflage: exactly the layout of the reference {{source}} ({{id}}, {{use}}), every black well, button and edge in
the same place and size, the wells flat black. Bevels lit from the top left. Keep only the reference's own lettering.
Match the style references {{references}}. Output {{target_width}}x{{target_height}} pixels.
