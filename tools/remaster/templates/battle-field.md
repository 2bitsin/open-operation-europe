---
template: battle-field
title: Battle field objects, blasts and borders
pack_class: battle
ids:
  - battle-terrain/*
  - battle-blast/*
  - battle-border/*
count: 200
scale: 4
background: transparent
recolour: hour
references:
  - battle-terrain/summer/02
  - battle-terrain/summer/06
  - battle-terrain/summer/10
  - battle-terrain/snow/62
  - battle-terrain/snow/72
  - battle-blast/large/1
  - battle-border/ne-sw
rules:
  mask_tolerance: 0.2
  daylight: true
---
# Battle field objects

## What it is

The tactical battle field is an isometric grid of 40x26 cells. `battle-terrain/<season>/<nn>` (95 per season,
summer and snow) are the objects drawn on it: buildings, churches, factories, forests, hills, rivers, roads and
bridges. A large object spans several cells and is cut into cell-sized parts ("the part over the diagonal cell"), so
neighbouring parts must join. `battle-blast/large|small/<frame>` are the 4-frame hit and miss explosions;
`battle-border/nw-se|ne-sw` the diagonal edge of the field.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x; 2x
and 8x are the alternatives, one scale for the whole pack).

## Style references

The classic object being replaced ({{source}}) and, for a multi-cell object, its neighbouring parts; the class
references {{references}}.

## Constraints

- Transparent background: alpha 0 where the classic shows the ground, the same coverage within a tolerance.
- Isometric, the classic cell's diamond; a part of a larger object joins its neighbours at the cell edges. Generate a
  multi-cell building as a whole and cut it into the cells it spans, rather than part by part.
- Daytime, neutral light (the game tints dawn, dusk and night by colour modulation); snow objects are the summer
  objects under snow, the same shapes. Light from the top left everywhere, one line weight.
- Blast frames: one explosion in four stages, the same centre; transparent around it.
- No text, no people unless the classic shows them.

## Prompt

Isometric battle field object for a 1990s World War II strategy game, remastered as clean high-resolution pixel art:
{{use}}, exactly the shape, footprint and position in the cell of the reference {{source}} ({{id}}). Transparent
background where the reference shows the ground, evenly lit daylight, light from the top left, no text. Match the
style references {{references}}. Output {{target_width}}x{{target_height}} pixels.
