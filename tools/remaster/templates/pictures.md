---
template: pictures
title: Event, outcome, title and epilogue pictures
pack_class: pictures
ids:
  - event/*
  - outcome/*
  - title/*
  - epilogue/*
count: 56
scale: 4
background: opaque
recolour: none
references:
  - event/00
  - event/04
  - event/19
  - outcome/axis-wins
  - title/scenario-1
  - epilogue/1
rules: {}
---
# Pictures

## What it is

The full-colour illustrations: `event/<nn>` (32 pictures shown when an event happens during a campaign, 256x168),
`outcome/*` (the six medals, 96x144, and the four victory and defeat screens, 480x240), `title/*` (the title and
scenario-selection screen, 640x400, and the six scenario vignettes, 256x128) and `epilogue/<n>` (the ending's seven
pictures, up to 640x368, including the END lettering). Painted in 1994 in 16-colour EGA with heavy dithering.

## Size and scale

Classic {{width}}x{{height}}; the remaster is {{target_width}}x{{target_height}} ({{scale}}x, recommended 4x; the
largest, 640x400, becomes 2560x1600). 2x halves every side; one scale for the whole pack.

## Style references

The classic picture ({{source}}) is the composition to keep; the class references {{references}} fix one painterly
illustration style for the class: the same brushwork, saturation and light.

## Constraints

- Opaque, the same composition, horizon, figures and vehicles in the same places: dialogs and text are drawn over
  some of them at fixed positions.
- Light from the top left unless the scene's own light source (a fire, a sunset) says otherwise.
- Lettering only where the classic has it (`epilogue/6`, END; the title screen's logo), kept as the same words in the
  same place; no other text, no signatures or watermarks.
- Period-correct vehicles, uniforms and aircraft for 1940..1945.

## Prompt

Illustration for a 1990s World War II strategy game, remastered as a detailed painted picture: the same scene,
composition, figures, vehicles and light as the reference {{source}} ({{id}}: {{use}}), no dithering, period-correct
equipment. Opaque, fills the frame, no added text, no border. Match the painting style of the references
{{references}}. Output {{target_width}}x{{target_height}} pixels.
