---
template: glyphs
title: Font glyphs
pack_class: glyphs
ids:
  - game-glyph/*
  - ending-glyph/*
  - text-glyph/*
  - digit-glyph/*
  - sideways-digit-glyph/*
  - marker-glyph/*
count: 1418
scale: 4
background: transparent
recolour: ink
references:
  - game-glyph/0000
  - game-glyph/0016
  - game-glyph/0042
  - game-glyph/0200
  - ending-glyph/000
rules:
  mask_tolerance: 0.45
  neutral: true
  chroma_tolerance: 16
  grid: true
  grid_margin: 1
---
# Font glyphs

## What it is

The game's text is drawn glyph by glyph from fixed-cell fonts: `text-glyph/<code>` (96 glyphs of 8x16, the codes
0x20..0x7F of the game's English text), `digit-glyph/<nn>` (6x6 digits and the blank that clears one),
`sideways-digit-glyph/<nn>` (the same digits turned to read downward, 8x5), `marker-glyph/<n>` (two 8x8 overview
markers), and the Chinese edition's 16x14 fonts `game-glyph/<nnnn>` (1080 glyphs: kana, kanji, Latin letters, digits
and symbols, in code order) and `ending-glyph/<nnn>` (218, the ending's text).
Each glyph is a one-bit shape the game draws in a pen colour, so the image is a white or grey shape on transparency.

## Size and scale

Cell {{width}}x{{height}}; the remaster cell is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x).
The pen advances one fixed cell per glyph: the glyph must stay inside its cell and on the same baseline.

## Style references

The classic glyph ({{source}}) is the character; the class references {{references}} fix one typeface: a clean
period-appropriate bitmap-style face at the higher resolution, one stroke weight for all.

## Constraints

- Transparent background; the glyph is neutral white or grey only (the pen colour is applied by the game), at most
  antialiased edges in grey.
- The fixed cell grid: inside the classic glyph's extent plus one game pixel, on the classic baseline, the same
  left side bearing; one stroke weight and x-height across both fonts.
- The same character: a generator redraws shapes, so every glyph is checked against its classic one in review. A
  font this regular is better made by vectorising or by an existing typeface rendered into the cells than by image
  generation; the template is for the generator anyway.

## Prompt

A single font glyph, white on a transparent background, for a 1990s game's fixed-cell bitmap font remastered at
high resolution: exactly the character of the reference {{source}} ({{id}}), the same position in the cell, the
same baseline, one even stroke weight matching the references {{references}}. No colour, no shadow, no outline.
Output {{target_width}}x{{target_height}} pixels.
