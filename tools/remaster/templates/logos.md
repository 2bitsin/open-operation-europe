---
template: logos
title: Logos and captions
pack_class: logos
ids:
  - logo/*
count: 4
scale: 4
background: transparent
recolour: ink
references:
  - logo/koei
  - logo/presents
rules:
  mask_tolerance: 0.3
  neutral: true
  chroma_tolerance: 16
---
# Logos and captions

## What it is

`logo/koei`, `logo/presents` (KOEI PRESENTS), `logo/copyright-1992`, `logo/copyright-1994`: one-bit lettering the
title and the opening draw in a pen colour.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x).

## Style references

The classic lettering ({{source}}) and {{references}}.

## Constraints

- Transparent background, white or grey only (the pen colour is the game's).
- The exact words, layout and placement of the classic. These are trademarks and legal notices: redraw the
  lettering faithfully (tracing or vectorising the classic is the safer route), never reinterpret it.

## Prompt

The lettering of the reference {{source}} ({{id}}), redrawn at high resolution exactly as it is: the same words,
letter shapes, layout and placement, clean white on a transparent background, no colour, no effects. Output
{{target_width}}x{{target_height}} pixels.
