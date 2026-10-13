---
template: unit-icons
title: Battle unit icons and their side variants
pack_class: battle
ids:
  - battle-unit/*
count: 88
scale: 4
background: transparent
recolour: side
references:
  - battle-unit/class-00/facing-0
  - battle-unit/class-04/facing-1
  - battle-unit/class-04/facing-1/side-0
  - battle-unit/class-06/facing-0
  - battle-unit/class-09/facing-0
variant_text:
  base: the blue side (blue, light blue and steel grey)
  side-0: the red side (red, orange and yellow)
rules:
  mask_tolerance: 0.2
  variant_tolerance: 0.02
---
# Battle unit icons

## What it is

The units on the tactical battle field: `battle-unit/class-<cc>/facing-<f>` for 11 unit classes (tanks, assault
guns, artillery, anti-tank guns, half-tracks, infantry, trucks ...) in 4 facings each, drawn in isometric
projection on a 40x26 battle cell with a green ground shadow. The plain id is the blue side; the `side-0` variant
(`battle-unit/class-<cc>/facing-<f>/side-0`) is the same icon in the other side's red and yellow. 44 assets, 44
variants.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x;
alternatives 2x or 8x, the whole pack shares one scale). The icon sits where the classic one sits in the cell: the
same footprint, the same ground shadow position.

## Style references

The classic icon being replaced ({{source}}), the same unit's other facings (the four must read as one vehicle
turned), and the class references {{references}}.

## Constraints

- Transparent background: alpha 0 wherever the classic icon shows the battle field, the silhouette at the classic
  footprint (the validator compares the coverage within a tolerance).
- Isometric, the same projection and facing as the classic; light from the top left; one line weight.
- Side variants are separate generations from one neutral base: generate the vehicle once in neutral grey (not in
  the pack), then make the plain id (blue side) and the `side-0` id (red and yellow side) as two recolours of that
  base. Only the side colours change: both variants have exactly the same silhouette and detail (the validator
  requires the same coverage within 2 %).
- No text, no numbers, no flags or national insignia beyond what the classic shows.

## Prompt

Isometric World War II battle unit icon for a 1990s strategy game, remastered as clean high-resolution pixel art:
{{use}}, the unit {{id}}, same vehicle type, facing and footprint as the reference {{source}}. Transparent background,
a soft green ground shadow under the unit as in the reference, light from the top left. Side colours: {{variant}}; otherwise identical to the
neutral base. No text, no border.
Match the style references {{references}}. Output {{target_width}}x{{target_height}} pixels.
