/* desklamp_lighting.pov 1.0
 * Persistence of Vision Raytracer scene description file
 * A proposed POV-Ray Object Collection Demo.
 *
 * Demonstrates lighting output of macro desklamp.inc::Lamp_Lighting().
 *
 * Copyright (C) 2022 Richard Callwood III.  Some rights reserved.
 * This file is licensed under the terms of the CC-LGPL
 * a.k.a. the GNU Lesser General Public License version 2.1.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Please
 * visit https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html for
 * the text of the GNU Lesser General Public License version 2.1.
 *
 * Vers  Date         Notes
 * ----  ----         -----
 *       2021-Mar-24  Created.
 * 1.0   2022-???-??  Uploaded.
 */
#version max (3.5, min (3.8, version));

#include "colors.inc"
#include "desklamp.inc"

global_settings { assumed_gamma 1 }

#default { finish { ambient 0 diffuse 1 } }

#declare Lamp_Lumen = 0.01;
#declare Lamp_Scale = LAMP_METER;
#declare NCOLORS = 6;
#declare BRIGHTNESS = 750;

#declare Height = NCOLORS * 2 * image_height / image_width;
camera
{ orthographic
  location <NCOLORS, Height / 2, -2>
  right NCOLORS * 2 * x
  up Height * y
}

plane
{ -z, 0
  pigment { rgb 1 }
}

//--------------------- ANNOTATIONS ------------------------

#macro Annotate (Msg, Y)
  text
  { ttf "cyrvetic" Msg 1, 0
    translate -y
    scale 16 * Height / image_height
    translate <0.1, Height * Y, -1>
    pigment { rgb 0.7 }
    finish { #if (version < 3.7) ambient #else emission #end 1 }
  }
#end
Annotate
( concat
  ( "Colored c_Light, white c_Bulb; Brightness = ", str (BRIGHTNESS, 0, 1)
  ),
  1
)
Annotate
( concat
  ( "Colored c_Light, white c_Bulb; Brightness = ", str (-BRIGHTNESS, 0, 1)
  ),
  3/4
)
Annotate
( concat
  ( "White c_Light, colored c_Bulb; Brightness = ", str (BRIGHTNESS, 0, 1)
  ),
  2/4
)
Annotate
( concat
  ( "White c_Light, colored c_Bulb; Brightness = ", str (-BRIGHTNESS, 0, 1)
  ),
  1/4
)

//------------------------ LIGHTS --------------------------

#macro Test_light (Color, Brightness, Fade, uv_Posn)
  light_source
  { <uv_Posn.x, uv_Posn.y, -1>, color Color * Brightness
    fade_distance Fade / Lamp_Scale
    fade_power 2
    spotlight point_at <uv_Posn.x, uv_Posn.y, 0> radius 22.5 falloff 45
  }
#end

#declare Colors = array[NCOLORS]
{ <-3, 0.9, 1>, <15, 0.9, 1>, <45, 0.9, 1>,
  <135, 0.9, 0.5>, <232.5, 0.9, 1>, <270, 0.9, 1>,
}

#declare c_Light = rgb 0;
#declare Brightness = 0;
#declare Fade = 0;
#declare Surface = rgb 0;

#declare I = 0;
#while (I < NCOLORS)
  #declare C = CHSV2RGB (Colors[I]);
  Lamp_Lighting
  ( "", C, rgb 1, 3, BRIGHTNESS, c_Light, Brightness, Fade, Surface
  )
  Test_light (c_Light, Brightness, Fade, <2 * I + 1, 7/8 * Height>)
  Lamp_Lighting
  ( "", C, rgb 1, 3, -BRIGHTNESS, c_Light, Brightness, Fade, Surface
  )
  Test_light (c_Light, Brightness, Fade, <2 * I + 1, 5/8 * Height>)
  Lamp_Lighting
  ( "", rgb 1, C, 3, BRIGHTNESS, c_Light, Brightness, Fade, Surface
  )
  Test_light (c_Light, Brightness, Fade, <2 * I + 1, 3/8 * Height>)
  Lamp_Lighting
  ( "", rgb 1, C, 3, -BRIGHTNESS, c_Light, Brightness, Fade, Surface
  )
  Test_light (c_Light, Brightness, Fade, <2 * I + 1, 1/8 * Height>)
  #declare I = I + 1;
#end

// end of desklamp_lighting.pov
