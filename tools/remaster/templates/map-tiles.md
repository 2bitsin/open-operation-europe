---
template: map-tiles
title: Map tiles
pack_class: tiles
ids:
  - view-tile/*
  - overview-tile/*
count: 1152
scale: 4
background: opaque
recolour: hour
references:
  - view-tile/map-0/00
  - view-tile/map-1/18
  - view-tile/map-1/41
  - view-tile/map-1/52
  - overview-tile/map-0/00
rules:
  grid: true
  grid_margin: 0
  daylight: true
---
# Map tiles

## What it is

The strategic map is a grid of square terrain tiles: `view-tile/map-<scenario>/<nn>` are the 24x24 tiles of the
zoomed map view (96 per scenario map, maps 0..5), `overview-tile/map-<scenario>/<nn>` the 8x8 tiles of the whole-map
overview (the same 96 terrains, reduced). Plains, forest, hills, towns, roads, rivers, coast and sea, as seen from
straight above, in dithered 16-colour EGA. Tiles abut each other with no border: the map is the tiles side by side.
Some tiles are plain black (unused or off-map).

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x). The pack has one
scale: 4x is the recommendation (96x96 and 32x32 here). 2x keeps the pixel-art feel and a quarter of the size; 8x
gives nothing a 4K window shows. Every tile is exactly that size, opaque, filling its cell to the edge.

## Style references

Show the generator the classic tile being replaced ({{source}}) and the class references, upscaled
nearest-neighbour: {{references}}. Keep the classic tile's terrain, layout and where each feature meets the edge.

## Constraints

- Opaque: no transparency at all, the tile fills the whole square.
- Seamless: what crosses an edge (a road, a river, a coastline, a forest border) leaves at the same place and in the
  same width as in the classic tile, so neighbours still join. Same palette families across every tile of a map.
- Daytime, neutral light: the game tints the image for dawn, dusk and night by colour modulation, so the generator
  draws only the daylight image, evenly lit, no shadows that imply a time of day, no fog, no vignette.
- Light from the top left on every tile of the class; one line weight for every outline.
- No text, no labels, no unit markers, no grid lines.
- Overview tiles are the view tiles reduced: generate the view tile first and derive the overview tile from it (or
  generate at the same style, much simpler detail).

## Prompt

Top-down terrain tile for a 1990s World War II strategy game map, remastered as high-resolution painted pixel art.
Reproduce exactly the terrain and layout of the reference tile {{source}} ({{use}}): every road, river, coastline
and forest edge leaves the square at the same place. Square, opaque, fills the frame edge to edge, seamless with its
neighbours. Evenly lit daylight, light from the top left, no shadows suggesting a time of day, no text, no grid, no
border. Muted period map palette matching the style references {{references}}. Output {{target_width}}x{{target_height}}
pixels.
