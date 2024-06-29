/* desklamp.pov 2.0.1
 * Persistence of Vision Raytracer scene description file
 * A proposed POV-Ray Object Collection demo
 *
 * Demonstrates use of DeskLamp.
 *
 * Copyright (C) 2022, 2024 Richard Callwood III.  Some rights reserved.
 * This file is licensed under the terms of the GNU-LGPL
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
 * Vers.  Date         Notes
 * -----  ----         -----
 *        2021-Dec-16  Started.
 * 1.0    2022-Sep-06  Completed and uploaded.
 *        2024-Jan-15  A finish highlight is corrected.
 * 2.0    2024-Jun-04  One of the hooded lamps is replaced with a flat panel
 *                     lamp.
 * 2.0.1  2024-Jun-28  No change.
 */
// +W800 +H600 +A0.1 +R5
// +W1600 +H1200 +A0.1
#version max (3.5, min (3.8, version));

#ifndef (Lamp_Radiosity) #declare Lamp_Radiosity = yes; #end
#ifndef (Draft) #declare Draft = 2; #end
// Draft = 0: point light; spiral normal on flexible neck
// Draft = 1: low quality area light; spiral normal on flexible neck
// Draft = 2: high quality area light; flexible neck is an actual spiral

#include "colors.inc"
#include "desklamp.inc"

#declare Lamp_Scale = LAMP_FOOT;
#declare Lamp_Lumen = 0.01;
#declare Lamp_Diffuse = 0.75;
#declare Lamp_c_Ambient = rgb (Lamp_Radiosity? 0: <8.8, 7.2, 6.4> * Lamp_Lumen);
// We must set a default finish before #including woods.inc.  POV-Ray's default
// diffuse is assumed:
#default { finish { ambient Lamp_c_Ambient * 0.6 / Lamp_Diffuse diffuse 0.6 } }
#include "woods.inc"
// Now set our scene's default finish:
#default { finish { ambient Lamp_c_Ambient diffuse Lamp_Diffuse } }

#if (version < 3.7)
  #if (Lamp_Radiosity) #declare Lamp_Max_Sample = 15; #end
#else
  #declare RAD_REGULAR = 500;
  #declare RAD_IMPORTANT = 10000;
  #default { radiosity { importance RAD_REGULAR / RAD_IMPORTANT } }
#end

global_settings
{ assumed_gamma 1
  max_trace_level 15
  #if (Lamp_Radiosity)
    radiosity
    { error_bound 0.5
      recursion_limit 2
      #if (version < 3.7)
        count 400
        max_sample Lamp_Max_Sample
        pretrace_end 0.01
        pretrace_start 0.08
      #else
        count RAD_IMPORTANT, RAD_IMPORTANT * 17
        nearest_count 10
        pretrace_end 2 / image_width
        pretrace_start 64 / image_width
      #end
    }
  #end
}

#declare RROOM = 6;
#declare WTABLE = 5.0;
#declare DTABLE = 2.5;
#declare HTABLE = 2.5;
#declare HLAMP = 1.5;
camera
{ location <0, HTABLE + HLAMP, 1>
  look_at <0, HTABLE + HLAMP / 2, RROOM - DTABLE + 1>
  right 4/3 * x
  up y
  angle 45
}

#declare t_Gloss = texture
{ pigment { rgbf 1 }
  finish
  { reflection { 1 fresnel } conserve_energy
    specular 6.08464 roughness 0.001
  }
}

#declare t_Low_gloss = texture
{ pigment { rgbf 1 }
  finish
  { reflection { 0.25 fresnel } conserve_energy
    specular 0.155
    roughness 0.01
  }
}

#declare i_Gloss = interior { ior 1.49 }

//======================== THE LAMPS ===========================

// All these examples use gamma conversion for the lamp texture,
// but linear color for the light and bulb colors.

#switch (Draft)
  #case (0)
    #declare Soft = 0;
    #declare Quality = 1;
    #break
  #case (1)
    #declare Soft = <1, 5, 0, 0>;
    #declare Quality = 1;
    #break
  #case (2)
    #declare Soft = <1, 17, 2, 0>;
    #declare Quality = 3;
    #break
#end

// Hooded lamp with American scaling, aim point, white bulb,
// wattage proxy, binary switch, & split texture:
#declare t_Red = texture
{ pigment
  { object
    { plane { y, 0 }
      pigment
      { radial color_map
        { [0.5 Lamp_Color (Scarlet)]
          [0.5 Lamp_Color (MediumVioletRed)]
        }
        frequency 6
        rotate 15 * y
      }
      pigment { Lamp_Color (Scarlet) }
    }
  }
}
texture { t_Gloss }
object
{ Lamp_Flexneck
  ( HLAMP * LAMP_FOOT, <-1.15, HTABLE, RROOM - DTABLE + 1.3>, y,
    <0, HTABLE, RROOM - DTABLE + 0.5>, on, rgb <1.0000, 0.5574, 0.2422>,
    Lamp_fn_Watts_to_Lumens (40), t_Red, Lamp_Bulb_A19,
    rgb 1, Soft, off, <Quality, 0>
  )
  interior { i_Gloss }
}

// Hooded lamp with international scaling, aim angle, colored
// bulb, binary switch, & split texture; switched off:
#declare t_Green = texture
{ pigment
  { object
    { plane { y, 0 }
      pigment
      { radial color_map
        { [0.5 Lamp_Colour (SeaGreen)]
          [0.5 Lamp_Colour (YellowGreen)]
        }
        frequency 6
        rotate 15 * y
      }
      pigment { Lamp_Colour (SeaGreen) }
    }
  }
}
texture { t_Gloss }
object
{ Lamp_Flexneck
  ( 45, <0.25, HTABLE, RROOM - 0.9>, y,
    <0, HTABLE, RROOM - 2, -20>, off, rgb 1,
    -450, t_Green, Lamp_Bulb_A60,
    CHSV2RGB (<45, 0.9, 1>), Soft, off, <Quality, 0>
  )
  interior { i_Gloss }
}

// Flat panel lamp with international scaling, aim angle,
// colored bulb, & dimmer dial:
#declare t_Blue = texture
{ pigment
  { radial color_map
    { [0.5 Lamp_Colour (SlateBlue)]
      [0.5 Lamp_Colour (DarkSlateBlue)]
    }
    frequency 3
  }
}
texture { t_Low_gloss }
object
{ Lamp_Flexneck_Rectangular
  ( 45, 12.5, <1, HTABLE, RROOM - DTABLE + 1>, y,
    <0, HTABLE, RROOM - DTABLE + 1, -15>, 1, rgb 1,
    -300, 2, t_Blue,
    CHSV2RGB (<210, 0.8, 1>), 10, Soft, off, 1
  )
  interior { i_Gloss }
}
//======================= ROOM & TABLE =========================

box
{ -<RROOM, 0, RROOM>, <RROOM, 8, RROOM> hollow
  pigment { rgb 1 }
}

box
{ <-WTABLE / 2, HTABLE - 1/8, RROOM>, <WTABLE / 2, HTABLE, RROOM - DTABLE>
  texture { T_Wood22 scale 0.5 rotate 90 * y translate HTABLE * y }
  texture { t_Low_gloss }
  interior { i_Gloss }
}

//====================== SHEET OF PAPER ========================

#declare WPAPER = 8.5;
#declare HPAPER = 11;
#declare VMARGIN = 1;
#declare HMARGIN = 0.8;
#declare PAPER_THIN = 0.004; // 2" per ream
#declare LARGE = 72 / 72;
#declare REGULAR = 36 / 72;
#declare s_Heading = "Lorem Ipsum"
#declare NLINES = 13;
#declare s_Lines = array[NLINES]
{ "Dolor sit amet, consectetuer",
  "adipiscing elit. Aenean commodo",
  "ligula eget dolor. Aenean massa.",
  "Cum sociis natoque penatibus et",
  "magnis dis parturient montes,",
  "nascetur ridiculus mus. Donec",
  "quam felis, ultricies nec,",
  "pellentesque eu, pretium quis,",
  "sem. Nulla consequat massa quis",
  "enim. Donec pede justo, fringilla",
  "vel, aliquet nec, vulputate eget,",
  "arcu. In enim justo, rhoncus ut,",
  "imperdiet a, venenatis vitae, justo.",
}

#declare Heading = text
{ ttf "timrom" s_Heading PAPER_THIN, 0
  scale LARGE
}
#declare hLarge = max_extent (Heading).y;
#declare hRegular = hLarge * REGULAR / LARGE;

union
{ object
  { RE_Box (-PAPER_THIN * z, <WPAPER, HPAPER, 0>, PAPER_THIN / 2, no)
    pigment { rgb 0.96 }
    finish { ambient Lamp_c_Ambient / Lamp_Diffuse diffuse 1 }
  }
 // Note: actual text objects render A LOT faster than an object pigment.
  union
  { #declare yHeading = HPAPER - VMARGIN - hLarge;
    object
    { Center_Object (Heading, x)
      translate <WPAPER / 2, yHeading, 0>
    }
    #declare yLine = yHeading - REGULAR * 2;
    #declare L = 0;
    #while (L < NLINES)
      text
      { ttf "timrom" s_Lines[L] PAPER_THIN, 0
        scale REGULAR
        translate <HMARGIN, yLine, 0>
      }
      #declare yLine = yLine - REGULAR * 1.2;
      #declare L = L + 1;
    #end
    pigment { rgb 0.02 }
    finish { ambient Lamp_c_Ambient / Lamp_Diffuse diffuse 1 }
    translate -PAPER_THIN * 1.1 * z
  }
  translate -WPAPER / 2 * x
  scale 1/12
  rotate 90 * x
  translate <0, HTABLE, RROOM - DTABLE + 1/12>
  #if (version >= 3.7)
    radiosity { importance sqrt (RAD_REGULAR / RAD_IMPORTANT) }
  #end
}

// end of desklamp.pov
