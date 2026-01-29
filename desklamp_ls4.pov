/* desklamp_ls4.pov 2.??
 * Persistence of Vision Raytracer scene description file
 * A proposed POV-Ray Object Collection demo
 *
 * Demonstrates use of DeskLamp with Lightsys IV.
 * Download Lightsys IV at:
 *   http://www.ignorancia.org/index.php?page=lightsys
 *     or
 *   https://news.povray.org/64cffd99%40news.povray.org
 *
 * Copyright (C) 2022 - 20?? Richard Callwood III.  Some rights reserved.
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
 *        2022-Aug-24  Started.
 * 1.0    2022-Sep-06  Completed and uploaded.
 * 2.0    2024-Jan-15  A finish highlight is corrected.
 *        2024-Dec-22  The Lightsys URL is updated in the header comments.
 * 2.??   20??-???-??  The license is upgraded to LGPL 3.
 */
// +W600 +H800 +A0.1 +AM2
#version max (3.5, min (3.8, version));

#ifndef (Lamp_Radiosity) #declare Lamp_Radiosity = yes; #end
#ifndef (Draft) #declare Draft = 2; #end

#include "CIE.inc"
#include "colors.inc"
#include "desklamp.inc"
#include "lightsys.inc"
#include "roundedge.inc"

// Set scale in DeskLamp, set lumens in Lightsys, and get them to agree:
#declare Lamp_Scale = LAMP_FOOT;
Lamp_Set_Lightsys()
#declare Lightsys_Brightness = 0.15;
Lamp_Get_Lightsys()

#declare Lamp_Diffuse = 0.75;
#declare Lamp_c_Ambient = rgb (Lamp_Radiosity? 0: <0.315, 0.285, 0.275>);
// We must set a default finish before #including woods.inc.  POV-Ray's default
// diffuse is assumed:
#default { finish { ambient Lamp_c_Ambient * 0.6 / Lamp_Diffuse diffuse 0.6 } }
#include "woods.inc"
// Now set our scene's default finish:
#default { finish { ambient Lamp_c_Ambient diffuse Lamp_Diffuse } }

#declare Lamp_Max_Sample = 15;

global_settings
{ assumed_gamma 1
  max_trace_level 15
  #if (Lamp_Radiosity)
    radiosity
    { error_bound 0.5
      max_sample Lamp_Max_Sample
      recursion_limit 2
      #if (version < 3.7)
        count 400
        pretrace_end 0.01
        pretrace_start 0.08
      #else
        count 400, 64721
        pretrace_end 2 / image_height
        pretrace_start 64 / image_height
      #end
    }
  #end
}

#declare RROOM = 6;
#declare HROOM = 8;
#declare WTABLE = 5.0;
#declare DTABLE = 2.5;
#declare HTABLE = 2.5;
#declare HLAMP = 1.5;
#declare ZLIGHT = 0;
#declare DLIGHT = 1;
#declare HLIGHT = 0.001;

#declare CAM = <0, 5, 1 - RROOM>;
#declare LOWER = <0, HTABLE, RROOM - DTABLE>;
#declare UPPER = <0, HROOM, ZLIGHT>;
camera
{ location CAM
  look_at CAM + vnormalize (LOWER - CAM) + vnormalize (UPPER - CAM)
  right 3/4 * x
  up y
  angle 39
}

#switch (Draft)
  #case (0) #declare Soft = <0, 0, 0, 0>; #break
  #case (1) #declare Soft = <1, 5, 0, 0>; #break
  #case (2) #declare Soft = <1, 17, 1, 0>; #break
#end

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

//======================== DESK LAMP ===========================

#declare t_Red = texture { pigment { Lamp_Color (Scarlet) } }
texture { t_Gloss }

object
{ Lamp_Flexneck
  ( 45, <-1.15, HTABLE, RROOM - DTABLE + 1.3>, y,
    <0, HTABLE, RROOM - DTABLE + 0.5>, on, rgb <1.0000, 0.5574, 0.2422>,
    800, t_Red, Lamp_Bulb_A60,
    rgb 1, Soft, off, <1, 0>
  )
  interior { i_Gloss }
}

//============= CENTRAL LIGHT USING LIGHTSYS IV ================

CIE_ColorSystem (sRGB_ColSys)
CIE_ColorSystemWhitepoint (sRGB_ColSys, Illuminant_D50)
#declare lAxis = Lamp_fn_Area_Adjust (DLIGHT, max (Soft.y, 2));
#declare COSADJ = 4 * (pi/2); // Maintain lumens with cosine falloff
#declare c3_Main = Daylight (5003);
#declare MainLm = 1600;
#declare LS4toSqMeter = Lamp_Lumen / Lightsys_Brightness;
#declare MainSqMeter = pow (Lamp_Scale / 100 * DLIGHT, 2);
//#debug concat ("LS4toSqMeter = ", Lamp__str (LS4toSqMeter, 4), "\n")
//#debug concat ("MainSqMeter = ", Lamp__str (MainSqMeter, 4), "\n")

union
{ Light (c3_Main, MainLm * COSADJ, lAxis * x, lAxis * z, Soft.y, Soft.y, true)
  box
  { -<1, 0, 1>, 1 scale <DLIGHT/2, HLIGHT, DLIGHT/2>
    // The surface brightness isn't exactly what I think it should be,
    // compared to the lamp hood interior, but it's fairly close.
    pigment { rgb Light_Color (c3_Main, MainLm) * LS4toSqMeter / MainSqMeter }
    #if (version >= 3.7)
      finish { ambient 0 diffuse 0 emission 1 }
      no_radiosity
    #else
      finish { ambient 1 diffuse 0 }
    #end
    no_shadow
  }
  translate <0, HROOM - HLIGHT, 0>
}

//=========================== ROOM =============================

box
{ -<RROOM, 0, RROOM>, <RROOM, HROOM, RROOM> hollow
  pigment { rgb 1 }
}

//========================== TABLE =============================

#declare t_Table = texture { T_Wood22 scale 0.5 }
texture { t_Low_gloss }

#declare INSET = 1/4;
#declare UNDER = HTABLE - 1/8;
#declare Table_leg = cone
{ 0, 1/16, UNDER * y, 1/12
  texture { t_Table rotate 90 * x }
}
union
{ RE_Box_y
  ( <-WTABLE/2, UNDER, RROOM>, <WTABLE/2, HTABLE, RROOM - DTABLE>,
    1/12, no
  )
  object { Table_leg translate <WTABLE/2 - INSET, 0, RROOM - INSET> }
  object { Table_leg translate <INSET - WTABLE/2, 0, RROOM - INSET> }
  object { Table_leg translate <WTABLE/2 - INSET, 0, RROOM - DTABLE + INSET> }
  object { Table_leg translate <INSET - WTABLE/2, 0, RROOM - DTABLE + INSET> }
  texture { t_Table rotate 90 * y translate HTABLE * y }
  interior { i_Gloss }
}

//====================== SHEET OF PAPER ========================

#declare WPAPER = 8.5;
#declare HPAPER = 11;
#declare PAPER_THIN = 0.004; // 2" per ream

object
{ RE_Box (0, <WPAPER, PAPER_THIN, HPAPER>, PAPER_THIN / 2, no)
  pigment { rgb 0.96 }
  finish { ambient Lamp_c_Ambient / Lamp_Diffuse diffuse 1 }
  translate -WPAPER / 2 * x
  scale 1/12
  translate <0, HTABLE, RROOM - DTABLE + 1/12>
}

//===================== WALL DECORATIONS =======================
// just to make the scene look more interesting

#macro Frame (Length)
  intersection
  { prism
    { -1, 1, 5, <0, 0>, <0, -0.25>, <1.5, -1>, <1.5, 0>, <0, 0>
      scale <1/12, Length/2 + 1/8, 1/12>
    }
    plane { <-1, 1, 0>, 0 translate Length/2 * y }
    plane { <-1, -1, 0>, 0 translate -Length/2 * y }
    texture { T_Wood6 rotate <90, 90, 20> scale 0.1 }
    texture { t_Gloss }
  }
#end

#macro Painting (s_Image, Width, Height)
  union
  { box
    { 0, 1
      texture { pigment { image_map { png s_Image interpolate 2 } } }
      texture { t_Low_gloss }
      translate <-0.5, -0.5, -1>
      scale <Width, Height, 0.001>
    }
    object { Frame (Height) translate Width/2 * x }
    object { Frame (Height) translate Width/2 * x rotate 180 * z }
    object { Frame (Width) translate Height/2 * x rotate 90 * z }
    object { Frame (Width) translate Height/2 * x rotate -90 * z }
    interior { i_Gloss }
  }
#end

#declare H1 = 15/8;
#declare W2 = 2.4;
#declare YCTR = 5.5;
object
{ Painting ("bumpmap_.png", 3, H1) // POV-Ray should find this automatically
  rotate 180 * z
  translate <-1.5, YCTR, RROOM>
}

object
{ Painting ("mtmandj.png", W2, 1.5) // POV-Ray should find this automatically
  rotate 90 * z
  translate <2.1, YCTR - (W2 - H1) / 2, RROOM>
}

// end of desklamp_ls4.pov
