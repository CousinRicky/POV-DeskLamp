/* desklamp_spectral.pov version 3.0-alpha.20260905
 * Persistence of Vision Raytracer scene description file
 * A proposed POV-Ray Object Collection demo
 *
 * Demonstrates use of DeskLamp with LILYsoft SpectralRender.
 * Download SpectralRender at:
 *   https://www.lilysoft.org/CGI/SR/Spectral%20Render.htm
 * Download modified SpectralRender files for gamut mapping at:
 *   https://github.com/CousinRicky/POV-SpectralRender-mods
 * Download Lightsys IV at:
 *   http://www.ignorancia.org/index.php?page=lightsys
 *     or
 *   https://news.povray.org/64cffd99%40news.povray.org
 *
 * Copyright (C) 2026 Richard Callwood III.  Some rights reserved.
 * This file is licensed under the terms of the GNU-LGPL
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Please
 * visit https://www.gnu.org/licenses/lgpl-3.0.html for the text
 * of the GNU Lesser General Public License version 3.
 *
 * Vers.  Date         Notes
 * -----  ----         -----
 * 3.0    2026-???-??  Adapted from desklamp.pov
 */
// Preview:
//   +W800 +H600 +A Declare=Preview=1
// Pass 1:
//   +W1600 +H1200 +A +AM1 +R3 +FE +KI1 +KF36 +KFI38 +KFF73
// Pass 2:
//   +W800 +H600 +A -J +AM1 +R2
// Before running pass 2, make sure ALL of the #declare FName lines in
// SpectralComposer.pov are commented out.
//
// After running pass 2, you may delete the integratelight_scene??.exr files.
#version max (3.7, min (3.8, version));

#ifndef (Preview) #declare Preview = no; #end

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%% CREATE THE SCENE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#if (clock_on | Preview)

#ifndef (Lamp_Radiosity) #declare Lamp_Radiosity = yes; #end
#ifndef (Draft) #declare Draft = 2; #end
// Draft = 0: point light; spiral normal on flexible neck
// Draft = 1: low quality area light; spiral normal on flexible neck
// Draft = 2: high quality area light; flexible neck is an actual spiral

//#include "colors.inc"
#include "desklamp.inc"
#include "spectral.inc"
#include "desklamp_spectral.inc"

#declare Lamp_Scale = LAMP_FOOT;
#declare Lamp_Lumen = 0.01;//0.005;//
#declare Lamp_Max_Sample = 15;
#declare Lamp_Diffuse = 1;
//#declare Lamp_c_Ambient = rgb (Lamp_Radiosity? 0: <8.8, 7.2, 6.4> * Lamp_Lumen); //@@ RECALCULATE THE AMBIENT!
#declare Lamp_c_Ambient = rgb 0;
#declare RAD_REGULAR = 400;//500;
#declare RAD_IMPORTANT = 2000;//10000;
#default
{ finish { ambient Lamp_c_Ambient diffuse Lamp_Diffuse }
  radiosity { importance RAD_REGULAR / RAD_IMPORTANT }
}

global_settings
{ assumed_gamma 1
  max_trace_level 15
  #if (Lamp_Radiosity)
    radiosity
    { error_bound 0.5
      recursion_limit 2
      //count RAD_IMPORTANT, RAD_IMPORTANT * 17
      count 400, 6472
      max_sample Lamp_Max_Sample
      nearest_count 10
      pretrace_end 2 / image_width
      pretrace_start 64 / image_width
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

// Technical note: for glossy finishes, I use a layered texture rather than
// M_Spectral_Shiny(), because the latter uses the obsolete pre-finish-level
// Fresnel model.

#declare t_Gloss = texture
{ pigment { rgbf 1 }
  finish
  { reflection { 1 fresnel } conserve_energy
    specular albedo 1 roughness 0.001
  }
}

#declare t_Low_gloss = texture
{ pigment { rgbf 1 }
  finish
  { reflection { 0.25 fresnel } conserve_energy
    specular albedo 0.25 roughness 0.01
  }
}

//#declare i_Gloss = interior { ior 1.49 } //@@ IOR_Acryl
#declare i_Gloss = interior { IOR_Spectral (IOR_Acryl) }

//======================== THE LAMPS ===========================

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

// Hooded lamp with American scaling, aim point, colored bulb,
// wattage proxy, binary switch, & split texture:
#declare t_Red = texture
{ pigment
  { object
    { plane { y, 0 }
      pigment
      { radial color_map
        { [0.5 C_Spectral (D_CC_C3)]
          [0.5 C_Spectral (D_CC_E3)]
        }
        frequency 6
        rotate 15 * y
      }
      pigment { C_Spectral (D_CC_C3) }
    }
  }
}
texture { t_Gloss }
object
{ Lamp_Flexneck
  ( HLAMP * LAMP_FOOT, <-1.15, HTABLE, RROOM - DTABLE + 1.3>, y,
    <0, HTABLE, RROOM - DTABLE + 0.5>, on, SpectralEmission (E_D50),
    Lamp_Spectral_Bright (-Lamp_fn_Watts_to_Lumens (40), E_D50, D_CC_D3),
    t_Red, Lamp_Bulb_A19, C_Spectral (D_CC_D3), Soft, off, <Quality, 0>
  )
  interior { i_Gloss }
}

// Hooded lamp with international scaling, aim angle, colored
// bulb, binary switch, & split texture; switched off:
#declare c_SeaGreen = C_Average (D_CC_B3, 1, D_CC_F3, 1);
#declare t_Green = texture
{ pigment
  { object
    { plane { y, 0 }
      pigment
      { radial color_map
        { [0.5 c_SeaGreen]
          [0.5 C_Spectral (D_CC_E2)]
        }
        frequency 6
        rotate 15 * y
      }
      pigment { c_SeaGreen }
    }
  }
}
texture { t_Gloss }
object
{ Lamp_Flexneck
  ( 45, <0.25, HTABLE, RROOM - 0.9>, y,
    <0, HTABLE, RROOM - 2, -20>, off, SpectralEmission (E_D50),
    Lamp_Spectral_Bright (-450, E_D50, D_CC_D3),
    t_Green, Lamp_Bulb_A60, C_Spectral (D_CC_D3), Soft, off, <Quality, 0>
  )
  interior { i_Gloss }
}

// Flat panel lamp with international scaling, aim angle,
// white bulb, & dimmer dial:
#declare t_Blue = texture
{ pigment
  { radial color_map
    { [0.5 C_Average (D_CC_F3, 2, D_CC_A3, 1)]
      [0.5 C_Spectral (D_CC_A3)]
    }
    frequency 3
  }
}
texture { t_Low_gloss }
object
{ Lamp_Flexneck_Rectangular
  ( 45, 12.5, <1, HTABLE, RROOM - DTABLE + 1>, y,
    <0, HTABLE, RROOM - DTABLE + 1, -15>, 1, SpectralEmission (E_D93),
    Lamp_Spectral_Bright (200, E_D93, Value_1),
    2, t_Blue, rgb 1, 10, Soft, off, 1
  )
  interior { i_Gloss }
}
//======================= ROOM & TABLE =========================

box
{ -<RROOM, 0, RROOM>, <RROOM, 8, RROOM> hollow
  pigment { C_Average (D_CC_A4, 1, D_CC_B4, 1) }
}

#declare d_Darkish = D_Average (D_CC_A1, 9, D_CC_D1, 1);
#declare c_Dark_wood = C_Average (d_Darkish, 9, Value_0, 1);
#declare c_Light_wood = C_Average (D_CC_A1, 9, D_CC_B1, 1);
#declare p_Wood = pigment
{ wood ramp_wave color_map
  { [0.00 c_Light_wood]
    [0.55 c_Dark_wood]
    [0.65 c_Light_wood]
    [0.80 c_Dark_wood]
    [0.95 c_Dark_wood]
    [1.00 c_Light_wood]
  }
  scale 1 / <1, 3, 1>
  warp { turbulence <1, 1, 0> * 0.5 omega 0.4 }
  scale <1, 3, 3> / 20
  rotate 3
}

box
{ <-WTABLE / 2, HTABLE - 1/8, RROOM>, <WTABLE / 2, HTABLE, RROOM - DTABLE>
  texture { pigment { p_Wood rotate 90 * y translate HTABLE * y } }
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
    pigment { C_Average (D_CC_A4, 1, Value_1, 1) }
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
    pigment { C_Average (D_CC_F4, 5, Value_0, 3) }
    translate -PAPER_THIN * 1.1 * z
  }
  translate -WPAPER / 2 * x
  scale 1/12
  rotate 90 * x
  translate <0, HTABLE, RROOM - DTABLE + 1/12>
  radiosity { importance sqrt (RAD_REGULAR / RAD_IMPORTANT) }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%% ASSEMBLE THE FRAMES %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#else

  #declare FName = "desklamp_spectral"
  #ifndef (Whitepoint) #declare Whitepoint = 5003; #end
  #if (file_exists ("SpectralComposer.inc"))
    // from https://github.com/CousinRicky/POV-SpectralRender-mods
    #declare GamutMap = 4; // luminance-based
    #include "SpectralComposer.inc"
  #else
    // from original SpectralRender
    // IMPORTANT:
    // Make sure ALL of the #declare FName lines in SpectralComposer.pov
    // are commented out, or the above #declare FName will be overridden!
    // For proper white balance, also comment out the #declare Whitepoint line.
    #include "SpectralComposer.pov"
  #end

#end

// end of desklamp_spectral.pov
