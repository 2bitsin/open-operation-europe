---
template: portraits
title: Commander portraits
pack_class: portraits
ids:
  - portrait/*
count: 120
scale: 4
background: opaque
recolour: none
references:
  - portrait/000
  - portrait/040
  - portrait/080
  - portrait/118
rules: {}
---
# Commander portraits

## What it is

`portrait/<nnn>`: the 120 head-and-shoulders portraits of the commanders (German, British, American, Soviet, French
officers and generals of 1940..1945), 64x80, framed, in the 8 picture colours of the day palette. Each is a real
historical figure the game names; the remaster keeps the likeness, uniform, rank insignia and pose of the classic.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x: 256x320;
2x is 128x160, 8x is beyond any window).

## Style references

The classic portrait ({{source}}) is the subject; the class references {{references}} fix the style: the same
painterly finish, framing (head and shoulders, eyes on the same line), background treatment and light for all 120.

## Constraints

- Opaque, fills the frame; the same head size and eye line as the classic so the panels that frame it still fit.
- Light from the top left; the same background tone across the class.
- Period-correct uniform and insignia as the classic shows them; no text, no name plate.

## Prompt

Head-and-shoulders portrait of a World War II commander for a 1990s strategy game, remastered as a detailed painted
portrait in the style of the references {{references}}: the same person, pose, uniform, rank insignia and framing as
the reference {{source}} ({{id}}). Opaque, fills the frame, light from the top left, no text. Output
{{target_width}}x{{target_height}} pixels.
