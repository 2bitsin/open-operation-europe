---
template: opening
title: The opening's pictures
pack_class: pictures
ids:
  - opening/*
count: 30
scale: 4
background: opaque
recolour: scene
references:
  - opening/03
  - opening/04
  - opening/09
  - opening/16
  - opening/21
rules: {}
---
# The opening's pictures

## What it is

`opening/<nn>`: the 30 pictures of the animated opening (OPEN.EXE): the credits lettering (A KOU SHIBUSAWA
PRODUCTION, WORLD WAR SERIES, "from its" ...), a Tiger tank head on, a saluting officer, aircraft, battle scenes,
each in its own scene palette (`open/<nnn>`) that the opening fades and cycles.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x).

## Style references

The classic picture in its scene palette ({{source}}, palette {{palette}}) and the class references
{{references}}: monochrome-leaning, duotone scenes (sepia, steel blue), dramatic.

## Constraints

- Opaque; the same composition and placement, the picture is positioned and animated by the opening's script.
- The opening's palette animation becomes colour modulation and fades: draw the picture fully lit in the colours of
  its scene palette, no baked fade.
- Lettering pictures keep their exact words and layout in a period face; no other text.

## Prompt

Picture from the animated opening of a 1990s World War II strategy game, remastered as a detailed dramatic
illustration: the same subject, composition and colour scheme as the reference {{source}} ({{id}}, palette
{{palette}}), fully lit, no dithering, no added text. Match the references {{references}}. Output
{{target_width}}x{{target_height}} pixels.
