---
template: masks
title: Animation masks of the opening and the ending
pack_class: masks
ids:
  - opening-shell/*
  - opening-blast-mask/*
  - ending-aircraft-mask/*
count: 20
scale: 4
background: transparent
recolour: ink
references:
  - opening-shell/0
  - opening-blast-mask/2
  - ending-aircraft-mask/0
rules:
  mask_tolerance: 0.3
  neutral: true
  chroma_tolerance: 16
---
# Animation masks

## What it is

One-bit shapes the opening and the ending animate: `opening-shell/<n>` (an artillery shell in flight, shrinking
away in 6 frames), `opening-blast-mask/<n>` (6 frames of the bomb explosions), `ending-aircraft-mask/<n>` (8
frames of the ending's aircraft). The game draws them in a pen colour or uses them to cut a picture.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x).

## Style references

The classic frame ({{source}}), the other frames of its animation ({{animation}}) and {{references}}.

## Constraints

- Transparent background, white or grey only, the classic coverage within the tolerance: these are masks.
- One animation: every frame keeps the classic centre and motion; generate the frames together.
- No text.

## Prompt

Frame {{frame}} of the animation {{animation}} of a 1990s World War II game, a solid white shape on a transparent
background, remastered at high resolution: exactly the outline, size and position of the reference {{source}}
({{id}}), smooth edges, no colour, no shading. Match the references {{references}}. Output
{{target_width}}x{{target_height}} pixels.
