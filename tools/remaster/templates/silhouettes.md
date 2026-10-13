---
template: silhouettes
title: Equipment silhouettes
pack_class: silhouettes
ids:
  - silhouette/*
count: 159
scale: 4
background: transparent
recolour: ink
references:
  - silhouette/000
  - silhouette/011
  - silhouette/049
  - silhouette/100
rules:
  mask_tolerance: 0.5
  neutral: true
  chroma_tolerance: 16
---
# Equipment silhouettes

## What it is

`silhouette/<nnn>`: the 159 side-view line drawings of the equipment types (tanks, assault guns, artillery,
aircraft, infantry weapons, trucks) shown in the equipment panel, 192x80, a one-bit outline drawing the game draws
in a pen colour over the panel.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x).

## Style references

The classic drawing ({{source}}) is the vehicle; the class references {{references}} fix one technical line-drawing
style: the same line weight, side view, scale and ground line for every type.

## Constraints

- Transparent background, white or grey lines only (the pen colour is the game's), no fill unless the classic fills.
- The same vehicle, side view, facing and size in the panel as the classic; one line weight for the whole class.
- No text, no designation labels.

## Prompt

Technical side-view line drawing of a World War II {{use}} for a 1990s strategy game, remastered at high
resolution: exactly the vehicle, facing, proportions and placement of the reference {{source}} ({{id}}), clean white
lines on a transparent background, one even line weight matching the references {{references}}, no fill, no text.
Output {{target_width}}x{{target_height}} pixels.
