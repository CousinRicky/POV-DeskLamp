/* desklamp.inc version 1.0
 * Persistence of Vision Raytracer include file
 * A proposed POV-Ray Object Collection module
 *
 * A flexible gooseneck desk lamp with a built-in light source.
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
 *       2021-Jan-11  Started.
 * 1.0   2022-???-??  Completed and tested.
 *
 * Implementation note: due to POV-Ray namespace scope leakage, local variables
 * and function parameters (other than x, y, and z) *must* be prefixed.
 */
#ifndef (DeskLamp_Inc_Temp) #declare DeskLamp_Inc_Temp = version;
#version max (3.5, min (3.8, version)); // Bracket the POV version.

#declare DESKLAMP_VERSION = 1.0;
#ifdef (View_POV_Include_Stack)
  #debug "#include \"desklamp.inc\" //version 1.0.beta-20220830\n"
  #debug concat ("  #version ", str (version, 0, 2), ";\n")
#end

#include "functions.inc"
#include "math.inc"
#include "roundedge.inc"
#include "transforms.inc"

//=============== PUBLIC PARAMETERS AND REFERENCE IDENTIFIERS ==================

#ifndef (Lamp_371Patch) #declare Lamp_371Patch = false; #end
#ifndef (Lamp_Debug) #declare Lamp_Debug = false; #end
#ifndef (Lamp_Diffuse) #declare Lamp_Diffuse = 0.6; #end
#ifndef (Lamp_Fade) #declare Lamp_Fade = 0.3; #end
#ifndef (Lamp_Lumen) #declare Lamp_Lumen = 0.01; #end
#ifndef (Lamp_Max_Sample) #declare Lamp_Max_Sample = 0; #end
#ifndef (Lamp_Radiosity) #declare Lamp_Radiosity = false; #end
#ifndef (Lamp_Reflect) #declare Lamp_Reflect = false; #end
#ifndef (Lamp_Scale) #declare Lamp_Scale = 1; #end
#ifndef (Lamp_c_Weights) #declare Lamp_c_Weights = rgb <0.2126, 0.7152, 0.0722>; #end
#ifndef (Lamp_c_Ambient)
  #declare Lamp_c_Ambient = rgb (version >= 3.8 | Lamp_Radiosity? 0: 0.1);
#end

#declare LAMP_CM = 1;
#declare LAMP_FOOT = 30.48;
#declare LAMP_INCH = 2.54;
#declare LAMP_METER = 100;
#declare LAMP_METRE = LAMP_METER;
#declare LAMP_YARD = 91.44;

#declare LAMP_HBULB = 8.6;

// Helix slope map
#declare Lamp_sm_Coil = slope_map
{ [0.0 <0.67, 0.00>]
  [1/3 <0.42, -0.58>]
  [1/3 <0.42, 3>]
  [0.47 <1, 0>]
  [1.00 <1, 0>]
}

//============================= PRIVATE FEATURES ===============================
// These tools are not intended be referenced directly by the user.

// If nonzero, lamp shoots a narrow beam; its value controls the brightness
#ifndef (lamp__Dbg_aim) #declare lamp__Dbg_aim = no; #end
// #debug attenuation?
#ifndef (lamp__Dbg_fade) #declare lamp__Dbg_fade = no; #end
// If nonzero, hood is cutaway
#ifndef (lamp__Dbg_hood) #declare lamp__Dbg_hood = 0; #end
// Turns off ambient manipulation and radiosity artifact mitigation
#ifndef (lamp__Dbg_hood_ctrl) #declare lamp__Dbg_hood_ctrl = no; #end

// Flags to avoid multiple warnings of global situations
#declare lamp__Warned_max = no;
#declare lamp__Warned_ambient = no;

// Permanent magic numbers
#declare LAMP__DLUMENEQLUX = 100 / sqrt (4 * pi); // 1 lumen = 1 lux, in cm
#declare LAMP__RNECK = 0.635;           // radius of flex-neck
#declare LAMP__W2L = ln (8) / ln (3);   // for wattage conversion curve fit
#declare LAMP__WHITE = 0.9;             // albedo of hood interior

// Flex-neck function magic numbers
#local LAMP_COUNT = 3;                  // thread count per cm
#local LAMP_2COUNT = LAMP_COUNT * 2;
#local LAMP_OFFSET = 0.5 / LAMP_COUNT;  // inner vs outer coil offset
#local LAMP_rMINOR = 0.111;             // isosurface coils minor radius
#local LAMP_rMINORN = 0.5 / LAMP_COUNT; // faked normal minor radius
#local LAMP_ROUTER = LAMP__RNECK - LAMP_rMINOR; // outer coil radius
#local LAMP_RINNER = 0.468;             // inner coil radius

// Return some vector perpendicular to its argument
#macro Lamp__any_perp (V)
( #if (V.y = 0 & V.z = 0)
    -y
  #else
    vcross (V, z)
  #end
)
#end

// Converts a color to a string of so many significant digits;
// filter and transmit are ignored:
#macro Lamp__cstr (Color, Sig)
  #local lamp_High = max (VMax (Color), VMax (-Color));
  concat
  ( "rgb <",
    vstr
    ( 3, Color, ", ", 0,
      #if (lamp_High = 0) Sig - 1
      #else max (Sig - floor (log (lamp_High)) - 1, 0)
      #end
    ),
    ">"
  )
#end

// The function used by macro Lamp_Flex_Angle().
//   x = the aim point's x coordinate
//   y = the aim point's y coordinate
//   z = the flex angle
//   lamp_P0_Flex  = length of the flexible part of neck
//   lamp_P1_Rigid = length of the rigid uppermost part of neck
#declare Lamp__fn_Flex = function (x, y, z, lamp_P0_Flex, lamp_P1_Rigid)
{ select
  ( -abs (z),
    lamp_P1_Rigid - y * cos (z) + (x + lamp_P0_Flex / z) * sin (z),
    lamp_P1_Rigid - y + lamp_P0_Flex // limit as z --> 0
  )
}

// Check availability of features.  The retracted 3.71 is considered for the
// sake of unofficial patches based on POV-Ray 3.7.1 (e.g., UberPOV 1.37.1.1).
#macro Lamp__has_Fresnel() // finish-level Fresnel
  (version >= 3.71)
#end
#macro Lamp__has_invsq() // power light fading
  (#if (Lamp_371Patch) version >= 3.71 #else version >= 3.8 #end)
#end

// Fatal error in lighting parameters
#macro Lamp__light_error (s_Caller, c_Light, c_Bulb, Brightness, s_Message)
  #local lamp_L = color c_Light;
  #local lamp_B = color c_Bulb;
  #warning concat
  ( s_Caller, "(): c_Light = ", Lamp__cstr (lamp_L, 4),
    ", c_Bulb = ", Lamp__cstr (lamp_B, 4),
    ", Brightness = ", Lamp__str (Brightness, 4), "\n"
  )
  #error concat (s_Caller, "(): ", s_Message)
#end

// Converts a number to a string of so many significant digits:
#macro Lamp__str (Number, Sig)
  #if (Number = 0)
    str (0, 1, Sig - 1)
  #else
    str (Number, 1, max (Sig - floor (log (abs (Number))) - 1, 0))
  #end
#end

// Converts a 3-D vector to a string of so many significant digits:
#macro Lamp__vstr (V, Sig)
  #local lamp_High = max (VMax (V), VMax (-V));
  concat
  ( "<",
    vstr
    ( 3 + Lamp_Has4D(V), V, ", ", 0,
      #if (lamp_High = 0) Sig - 1
      #else max (Sig - floor (log (lamp_High)) - 1, 0)
      #end
    ),
    ">"
  )
#end

//---------------------- HOOD PARTS ------------------------

// Permanent magic numbers
#declare LAMP__R1HOOD = LAMP_INCH * 1.25; // radius of rear of hood
#declare LAMP__R2HOOD = LAMP_INCH * 2.5; // radius of front of hood
#declare LAMP__HTOP = LAMP_INCH * 0.75; // length of top part of neck
#declare LAMP__DHOOD = LAMP_INCH * 5;  // depth of hood from neck to front
#declare LAMP__THIN = 0.08;           // sheet metal thickness
#declare LAMP__ZSOCKET = -1.3;        // rear of socket
#declare LAMP__Z0 = -LAMP_INCH * 2;   // rear of hood
#declare LAMP__Z1 = -LAMP_INCH * 5/3; // rear of cylindrical part of hood
#declare LAMP__Z3 = LAMP_INCH * 1/4;  // front of cylindrical part of hood

// Map the hood depth to the range 0...1
#declare LAMP__HOOD_WAVEXP = 2;
#declare Lamp__fn_Hood_map = function (z)
{ pow ((z - LAMP__Z0) / (LAMP__DHOOD - LAMP__Z0), LAMP__HOOD_WAVEXP)
}

// Ephemeral magic numbers
#local LAMP_NHOLES = 12;                // number of vent holes
#local LAMP_ASECT = 360 / LAMP_NHOLES;  // angular spacing of vent holes
#local LAMP_RHOLE = 0.4;                // radius of ends of vent holes
#local LAMP_THINNER = LAMP__THIN * 0.1; // thickness of white inner hood
#local LAMP_Z2 = -LAMP_INCH * 3/8;      // front of vent holes

// Hood shell, inset from a full-sized hood
//  InsOuter - inset of exterior
//  InsInner - inset of interior
#macro Lamp__make_hood (InsOuter, InsInner)
  #local lamp_RoRear = LAMP__R1HOOD - InsOuter;
  #local lamp_RiRear = LAMP__R1HOOD - InsInner;
  #local lamp_RoFront = LAMP__R2HOOD - InsOuter;
  #local lamp_RiFront = LAMP__R2HOOD - InsInner;
  // vent hole demensions excess:
  #local lamp_Z1x = LAMP__Z1 - 0.01;
  #local lamp_Z3x = LAMP__Z3 + 0.01;
  #local lamp_Yx = -lamp_RoRear - 0.01;
  #local lamp_Yix = -lamp_RiRear * cos (radians (LAMP_ASECT));
  // To minimize render time, each ventilation hole is differenced separately:
  #local Lamp_hood_sect = difference
  { cylinder { LAMP__Z1 * z, LAMP__Z3 * z, lamp_RoRear }
    union
    { box // bounding and clipping
      { <0, -lamp_RoRear, lamp_Z1x>,
        < sin (radians (LAMP_ASECT)) * lamp_RoRear,
          -cos (radians (LAMP_ASECT)) * lamp_RiRear,
          lamp_Z3x
        >
        inverse
      }
      cylinder { lamp_Z1x * z, lamp_Z3x * z, lamp_RiRear }
      cylinder
      { lamp_Yix * y, lamp_Yx * y, LAMP_RHOLE
        translate (LAMP_Z2 - LAMP_RHOLE) * z
        rotate LAMP_ASECT / 2 * z
      }
      cylinder
      { lamp_Yix * y, lamp_Yx * y, LAMP_RHOLE
        translate (LAMP_Z2 - LAMP_RHOLE * 3) * z
        rotate LAMP_ASECT / 2 * z
      }
      box
      { <-LAMP_RHOLE, lamp_Yx, LAMP_Z2 - LAMP_RHOLE>
        <LAMP_RHOLE, lamp_Yix, LAMP_Z2 - LAMP_RHOLE * 3>
        rotate LAMP_ASECT / 2 * z
      }
    }
  }
  union
  {//----- rear -----
    #local lamp_dRear = LAMP__Z1 - LAMP__Z0;
    difference
    { sphere { 0, 1 scale <lamp_RoRear, lamp_RoRear, lamp_dRear - InsOuter> }
      sphere { 0, 1 scale <lamp_RiRear, lamp_RiRear, lamp_dRear - InsInner> }
      plane { -z, 0 }
      translate LAMP__Z1 * z
    }
   //----- cylindrical section with ventilation holes -----
    #local lamp_A = 0;
    #while (lamp_A < 359)
      object { Lamp_hood_sect rotate lamp_A * z }
      #local lamp_A = lamp_A + LAMP_ASECT;
    #end
   //----- front -----
    // blob field sizes:
    #local lamp_FoRear = lamp_RoRear + 0.5;
    #local lamp_FiRear = lamp_RiRear + 0.5;
    #local lamp_FoFront = lamp_RoFront + 0.3;
    #local lamp_FiFront = lamp_RiFront + 0.3;
    // blob sphere z scale: given the above field sizes, this brings the
    // blobbed joint very close to the cylindrical section of hood (LAMP__Z3):
    #local lamp_dFront = LAMP__DHOOD + 0.4;
    // For the blob cylinders below, the exact length is unimportant; they just
    // need to be long enough to blob the spheroid and also connect smoothly
    // with the cylindrical section of the hood.
    intersection
    { blob
      { sphere
        { 0, lamp_FoFront / lamp_RoFront,
          RE_fn_Blob_strength (lamp_RoFront, lamp_FoFront)
          scale <lamp_RoFront, lamp_RoFront, lamp_dFront - InsOuter>
          translate LAMP__DHOOD * z
        }
        cylinder
        { LAMP__DHOOD * z, min (LAMP__DHOOD - lamp_dFront, LAMP__Z3) * z, lamp_FoRear
          RE_fn_Blob_strength (lamp_RoRear, lamp_FoRear)
        }
      }
      blob
      { sphere
        { 0, lamp_FiFront / lamp_RiFront, RE_fn_Blob_strength (lamp_RiFront, lamp_FiFront)
          scale <lamp_RiFront, lamp_RiFront, lamp_dFront - InsInner>
          translate LAMP__DHOOD * z
        }
        cylinder
        { LAMP__DHOOD * z, min (LAMP__DHOOD - lamp_dFront, LAMP__Z3) * z, lamp_FiRear
          RE_fn_Blob_strength (lamp_RiRear, lamp_FiRear)
        }
        inverse
      }
      cylinder { LAMP__Z3 * z, LAMP__DHOOD * z, lamp_RoFront }
    }
  }
#end

// Turnkey hood layers
#declare Lamp__hood = Lamp__make_hood (0, LAMP__THIN - LAMP_THINNER)
// A separate interior is needed to avoid radiosity artifacts:
#declare Lamp__hood_interior = Lamp__make_hood (LAMP__THIN - LAMP_THINNER, LAMP__THIN)

// Debugging
#if (lamp__Dbg_hood)
  #declare Lamp__hood = intersection
  { object { Lamp__hood }
    plane { x * lamp__Dbg_hood, 0 }
  }
  #declare Lamp__hood_interior = intersection
  { object { Lamp__hood_interior }
    plane { x * lamp__Dbg_hood, 0 }
  }
#end

// Rudimentary 3/4" hex nut
#local LAMP_ = 1 / sqrt(3);
#declare Lamp__nut_rudi = prism
{ 0, 1/3, 7, <2 * LAMP_, 0>, <LAMP_, -1>, <-LAMP_, -1>,
  <-2 * LAMP_, 0>, <-LAMP_, 1>, <LAMP_, 1>, <2 * LAMP_, 0>
  scale LAMP_INCH * 3/8
}

// Electrical lead from neck to socket
#declare Lamp__wire =  sphere_sweep
{ cubic_spline 9,
  <0.65, 0, LAMP__ZSOCKET + 1>, 0.16,
  <0.65, 0, LAMP__ZSOCKET>, 0.16,
  <0.85, -0.4, LAMP__ZSOCKET * 1.3>, 0.16,
  <1.20, -1.0, LAMP__ZSOCKET * 1.3>, 0.16,
  <1.35, -1.5, LAMP__ZSOCKET>, 0.16,
  <1.05, -1.8, LAMP__ZSOCKET * 0.6>, 0.16,
  <0.5, -2.05, LAMP__ZSOCKET * 0.2>, 0.16,
  <0.2, -2.4, 0>, 0.16,
  <0.2, -LAMP__R1HOOD, 0>, 0.2
}

//============================= PUBLIC FUNCTIONS ===============================

#declare Lamp_fn_Area_Adjust = function (x, y) { x * (y - 1) / y }

//----------------------------------------------------------

#declare Lamp_fn_Coil_Normal = function (x, y, z)
{ -LAMP_2COUNT * f_helix1 (-x, y, z, 1, pi * LAMP_2COUNT, LAMP_rMINORN, LAMP__RNECK, 1, 1, 0)
}

//----------------------------------------------------------

#declare Lamp_fn_Isocoil = function (x, y, z)
{ min
  ( f_helix1 (-x, y, z, 1, pi * LAMP_2COUNT, LAMP_rMINOR, LAMP_ROUTER, 1, 0.2, 0),
    f_helix1 (-x, y + LAMP_OFFSET, z, 1, pi * LAMP_2COUNT, LAMP_rMINOR, LAMP_RINNER, 1, 1, 0)
  )
}

//----------------------------------------------------------

#declare Lamp_fn_Watts_to_Lumens = function (x)
{ 20 * x - 400 + select (x - 60, pow ((60 - x) / 20, LAMP__W2L) * 50, 0)
}

//============================== PUBLIC MACROS =================================

#macro Lamp_Bulb_Material (c_Light, c_Bulb, Glossy)
  #local lamp_L = color c_Light filter 0 transmit 0;
  #local LAMP_DIFFUSE = 0.75; // affects pigment, but not glow intensity
  #local LAMP_N = 1.523; // glass IOR
  // Fresnel+conserve_energy compromises the surface glow brightness,
  // so calculate Fresnel to compensate:
  #local lamp_Fresnel = (Glossy? pow ((1 - LAMP_N) / (1 + LAMP_N), 2): 0);
  material
  { texture
    { pigment { color c_Bulb filter 0 transmit 0 }
      finish
      { #if (Lamp__has_Fresnel() & Glossy)
          fresnel 1
          reflection { 0 1 } conserve_energy
          #if (!Lamp_Reflect) specular albedo 1 roughness 0.001 #end
        #end
        diffuse LAMP_DIFFUSE
        ambient LAMP_DIFFUSE * Lamp_c_Ambient / Lamp_Diffuse
        #if (version < 3.7) + #else emission #end lamp_L / (1 - lamp_Fresnel)
      }
    }
    #if (!Lamp__has_Fresnel() & Glossy)
      texture
      { pigment { rgbf 1 }
        finish
        { reflection { 0 1 fresnel } conserve_energy
          #if (!Lamp_Reflect) specular 6.67 roughness 0.001 #end
        }
      }
    #end
    interior { ior LAMP_N }
  }
#end

//----------------------------------------------------------

#macro Lamp_Check (s_Caller, Value, s_Name, Min, Max)
  #if (Value < Min | Value > Max)
    #error concat
    ( s_Caller, "(): ", s_Name, " was set to ", str (Value, 0, 3),
      ". It must be within ", str (Min, 0, 0), " to ", str (Max, 0, 0), "."
    )
  #end
#end

//----------------------------------------------------------

#macro Lamp_Coil_Normal (Length, Flex, t_Texture)
  Lamp_Check ("Lamp_Coil_Normal", Flex, "Flex", -180, 180)
  #if (Flex = 0)
    cylinder
    { 0, Length * y, LAMP__RNECK
      texture
      { t_Texture
        normal
        { function { Lamp_fn_Coil_Normal (x, y, z) } 1
          slope_map { Lamp_sm_Coil }
        }
      }
    }
  #else
    #local lamp_Flex = radians (Flex);
    #local lamp_RoC = abs (Length / lamp_Flex);
    #local lamp_More = lamp_RoC + LAMP__RNECK;
    #local lamp_Less = lamp_RoC - LAMP__RNECK;
    #local lamp_Sin = sin (lamp_Flex);
    #local lamp_Cos = cos (lamp_Flex);
    intersection
    { torus { lamp_RoC, LAMP__RNECK rotate 90 * z }
      #if (mod (Flex, 90) != 0) plane { y, 0 rotate Flex * x } #end
      #if (Flex > 0)
        box
        { <-LAMP__RNECK, 0, -lamp_More>,
          #if (Flex < 90)
            <LAMP__RNECK, lamp_More * lamp_Sin, -lamp_Less * lamp_Cos>
          #else
            <LAMP__RNECK, lamp_More, -lamp_More * lamp_Cos>
          #end
        }
      #else
        box
        { <-LAMP__RNECK, 0, lamp_More>,
          #if (Flex > -90)
            <LAMP__RNECK, -lamp_More * lamp_Sin, lamp_Less * lamp_Cos>
          #else
            <LAMP__RNECK, lamp_More, lamp_More * lamp_Cos>
          #end
        }
      #end
      texture
      { t_Texture
        normal
        { function
          { Lamp_fn_Coil_Normal
            ( x,
              #if (Flex > 0) atan2 (-y, -z) #else atan2 (y, z) #end * lamp_RoC,
              sqrt (y*y + z*z) - lamp_RoC
            )
          } 1
          slope_map { Lamp_sm_Coil }
        }
      }
      translate #if (Flex < 0) - #end lamp_RoC * z
    }
  #end
#end

//----------------------------------------------------------

#macro Lamp_Color (Color)
  #local lamp_C = color Color;
  #if (VMin (lamp_C) < 0 | VMax (lamp_C) > 1)
    #warning concat
    ( "Lamp_Color(): red, green, and blue should be from 0 to 1, but <"
      vstr (5, lamp_C, ", ", 0, 4), "> was passed in."
    )
  #end
  // See "Stock colors and assumed_gamma 1 in POV-Ray 3.6" (2020-Oct-12) in
  // povray.binaries.images for why the formulas differ by POV version.
  // https://news.povray.org/5f83d3cb%40news.povray.org
  #if (version < 3.7)
    #local lamp_Out = rgbft
    < pow (lamp_C.red, 2.2), pow (lamp_C.green, 2.2), pow (lamp_C.blue, 2.2),
      lamp_C.filter, lamp_C.transmit
    >;
  #else
    #local lamp_Out = srgbft lamp_C;
  #end
  lamp_Out
#end

#macro Lamp_Colour (Colour)
  Lamp_Color (Colour)
#end

//----------------------------------------------------------

#macro Lamp_Finish_Base (Diffuse, Occlusion)
  diffuse Diffuse
  ambient color Diffuse * Lamp_c_Ambient / Lamp_Diffuse * (lamp__Dbg_hood_ctrl? 1: Occlusion)
    filter 0 transmit 0
  #if (version >= 3.7) emission 0 #end
#end

//----------------------------------------------------------

#macro Lamp_Flex_Angle (lFlex, lRigid, uv_Aim)
( #local lamp_Aim = uv_Aim + <0,0>;
  #local lamp_Aim = <uv_Aim.x, uv_Aim.y, 0>;
  #local lamp_hFull = lFlex + lRigid;
  #if (lamp_Aim.x < 0)
    4
  #else
    #local LAMP_EPSILON = 1e-6;
    #local lamp_X90 = 2 * lFlex / pi;
   // We need the highest root (max extension or minimum flex) that is <= pi/2.
   // First, find an interval by stepping backwards.
   // Step in degrees, not radians, to avoid numerical fuzz.
    #local lamp_I = 90;
    #local LAMP_STEP = 10;
    #local lamp_Y = Lamp__fn_Flex (lamp_Aim.x, lamp_Aim.y, pi / 2, lFlex, lRigid);
    // Technical note: the circumlocution !(lamp_Y <= 0) triggers POV-Ray's
    // epsilon; a simple lamp_Y > 0 can miss a valid root.
    #while (lamp_I > -180 & !(lamp_Y <= 0))
      #local lamp_Y =
        Lamp__fn_Flex (lamp_Aim.x, lamp_Aim.y, radians (lamp_I - LAMP_STEP), lFlex, lRigid);
      #local lamp_I = lamp_I - LAMP_STEP;
    #end
    #if (lamp_I = 90 | !(lamp_Y <= 0)) // no suitable interval found
      4
    #else #if (lamp_Y = 0) // lower end of interval is a root
      radians (lamp_I)
    #else
      #local lamp_X1 = radians (lamp_I);
      #local lamp_X2 = radians (lamp_I + LAMP_STEP);
     // Get some sort of root solver working: although the
     // bisection method is slow, it's simple and reliable.
      #local lamp_Done = no;
      #while (!lamp_Done)
        #local lamp_Angle = (lamp_X1 + lamp_X2) / 2;
        #local lamp_Y = Lamp__fn_Flex (lamp_Aim.x, lamp_Aim.y, lamp_Angle, lFlex, lRigid);
        #if (lamp_Y = 0 | lamp_X2 - lamp_X1 < LAMP_EPSILON)
          #local lamp_Done = true;
        #else
          #if (Lamp__fn_Flex (lamp_Aim.x, lamp_Aim.y, lamp_X1, lFlex, lRigid) * lamp_Y < 0)
            #local lamp_X2 = lamp_Angle;
          #else
            #local lamp_X1 = lamp_Angle;
          #end
        #end
      #end
      lamp_Angle
    #end
    #end
  #end
)
#end

//----------------------------------------------------------

#macro Lamp_Flexneck
( Height, pv_Base, v_Up, v_Aim, Status, c_Light, Brightness, t_Fixture,
  Bulb, c_Bulb, v4_Soft, Use_photons, v_Flags
)
 // Parameter checks and preparations
  Lamp_Warnings()
  #local lamp_This = "Lamp_Flexneck"
  #local LAMP_MIN_HT = 30;
  #if (Height < LAMP_MIN_HT)
    #error concat
    ( lamp_This, "(): Height is set to ", Lamp__str (Height, 6),
      "; it must be ", str (LAMP_MIN_HT, 0, 0), " or more."
    )
  #end
  #local lamp_v_Aim = v_Aim + <0,0,0>;      // Be sure it's not a scalar...
  #local lamp_4D = Lamp_Has4D (lamp_v_Aim); // ... before testing for .t
  #local lamp_L = color c_Light;
  #local lamp_B = color c_Bulb;
  #local lamp_v_Soft = v4_Soft + <0,0,0,0>;
  #if (lamp_v_Soft.x & lamp_v_Soft.y < 2)
    #error concat
    ( lamp_This, "(): v4_Soft.y is set to ", Lamp__str (lamp_v_Soft.y, 6),
      "; it must be 2 or more."
    )
  #end
  #local lamp_v_Flags = v_Flags + <0,0,0,0>;
 // Local magic numbers, cm
  #local LAMP_HPAD = 0.2;
  #local LAMP_HBASE = LAMP_INCH * 1.5;
  #local LAMP_RBASE = LAMP_INCH * 3;
  #local LAMP_ZOFS = -LAMP_INCH;
  #local LAMP_RSTEM = LAMP_INCH * 9/32;
  #local LAMP_RSOCKET = 1.6;
  #local LAMP_RWASHER = LAMP_INCH * 3/8;
 // Derived dimensions, cm
  // Flatten the hood/stem contact point so hood doesn't look unstably balanced:
  #local lamp_Flatten = LAMP__R1HOOD * (1 - cos (asin (LAMP_RSTEM / LAMP__R1HOOD)));
  // Length of flexible portion of neck
  #local lamp_lFlex = min
  ( LAMP_INCH * 8, // no longer than 8 inches
    Height - (LAMP__R1HOOD - lamp_Flatten) - LAMP_HBASE - 2 * LAMP__HTOP
  );
  // Length of lower rigid portion of neck
  #local lamp_hBottom =
    Height - (LAMP__R1HOOD - lamp_Flatten) - LAMP__HTOP - lamp_lFlex - LAMP_HBASE;
 // Determine rotation for lamp orientation
  #local lamp_Wrn_up = no;
  #local lamp_Wrn_aim = no;
  #local lamp_x_Position = Lamp_Orient_Trans (pv_Base, v_Up, v_Aim, lamp_Wrn_up, lamp_Wrn_aim)
  #local lamp_pv_Aim_rotated =
    Lamp_Scale * vtransform (lamp_v_Aim, transform { lamp_x_Position inverse });
  #if (lamp_Wrn_up)
    #warning concat
    ( lamp_This, "(): the null vector was used for v_Up. The y vector is substituted."
    )
  #end
  #if (lamp_Wrn_aim)
    #warning concat
    ( "pv_Base = ", Lamp__vstr (pv_Base + <0,0,0>, 4),
      ", v_Up = ", Lamp__vstr (v_Up + <0,0,0>, 4),
      ", v_Aim = ", Lamp__vstr (lamp_v_Aim, 4)
    )
    #warning concat
    ( lamp_This,
      "(): v_Aim is directly above or below the lamp; the lamp's direction is indeterminate."
    )
  #end
 // Determine flex of neck
  #if (lamp_4D)
    Lamp_Check (lamp_This, lamp_v_Aim.t, "v_Aim.t", -180, 180)
    #local lamp_Flexd = -lamp_v_Aim.t;
    #local lamp_Flex = radians (lamp_Flexd);
  #else
    #local lamp_Flex = -Lamp_Flex_Angle
    ( lamp_lFlex, LAMP__R1HOOD + LAMP__HTOP - lamp_Flatten,
      <lamp_pv_Aim_rotated.z - LAMP_ZOFS, lamp_pv_Aim_rotated.y - lamp_hBottom - LAMP_HBASE>
    );
    #if (lamp_Flex > pi)
      #warning concat
      ( "Height = ", Lamp__str (Height, 4),
        ", pv_Base = ", Lamp__vstr (pv_Base + <0,0,0>, 4),
        ", v_Up = ", Lamp__vstr (v_Up + <0,0,0>, 4),
        ", v_Aim = ", Lamp__vstr (lamp_v_Aim, 4)
      )
      #error concat (lamp_This, "(): The lamp cannot be aimed at the given point.")
    #end
    #local lamp_Flexd = degrees (lamp_Flex);
  #end
  //
  #if (lamp_Flex != 0)
    #local lamp_sRoC = lamp_lFlex / lamp_Flex; // signed radius of curvature
  #end
  #local lamp_x_Light = transform
  { translate (LAMP__R1HOOD - lamp_Flatten + LAMP__HTOP) * y
    #if (lamp_Flex = 0)
      translate lamp_lFlex * y
    #else
      translate -lamp_sRoC * z
      rotate lamp_Flexd * x
      translate lamp_sRoC * z
    #end
    translate <0, LAMP_HBASE + lamp_hBottom, LAMP_ZOFS>
    scale 1 / Lamp_Scale
    transform { lamp_x_Position }
  }
 // Determine lighting
  #local lamp_IsLit = (Status != 0 & Brightness != 0 & !VZero (lamp_L));
  #local lamp_c_Light = rgb 0;
  #local lamp_Brightness = 0;
  #local lamp_Fade = 0;
  #local lamp_c_Surface = rgb 0;
  #local lamp_Rad = rgb 0;
  #if (lamp_IsLit)
    #local lamp_Dims = max_extent (Bulb) - min_extent (Bulb);
    #local lamp_dBulb = (lamp_Dims.x + lamp_Dims.z) / 2; // bulb diameter
    #local lamp_rBulb = lamp_dBulb / 2;
    Lamp_Lighting
    ( lamp_This, lamp_L, lamp_B, lamp_rBulb, Brightness * Status,
      lamp_c_Light, lamp_Brightness, lamp_Fade, lamp_c_Surface
    )
    // Estimate light reflected from hood interior:
    #local lamp_rInt = LAMP__R2HOOD - LAMP__THIN;
    #local lamp_Refl_area = pow (lamp_rInt / lamp_rBulb, 2) - 1; // hood area compared to bulb
    #local lamp_Refl_flux = pow (lamp_rBulb / lamp_rInt, 2) * LAMP__WHITE; // inv sq reduction
    #local LAMP_HOOD_CONTRIB = 0.47; // determined from test renders
    #local lamp_Refl = lamp_Refl_area * lamp_Refl_flux * lamp_Brightness * (1 + LAMP_HOOD_CONTRIB);
    #if (Lamp_Radiosity)
      #if (Lamp_Max_Sample > 0) // make up for reduced radiosity
        #local lamp_Refl = lamp_Refl
          * max (1 - Lamp_Max_Sample / VMax (lamp_c_Light * lamp_Refl), 0);
      #else
        #local lamp_Refl = 0;
      #end
    #else // ambient only: estimate radiosity within hood interior:
      #local lamp_Refl_portion = 1 - pow (lamp_rBulb / lamp_rInt, 2);
      #local lamp_Rad = lamp_c_Surface * lamp_Refl_portion;
    #end
  #end // lamp_IsLit
  // Halve the surface brightnesses of the bulb and hood interior for 3.5/3.6,
  // since radiosity cannot be suppressed in these versions.
  // In practice, the halved luminance won't be noticed in non-HDRI renders.
  #local lamp_Rad36 =
    (Lamp_Radiosity & lamp_IsLit & version < 3.7 & !lamp__Dbg_hood_ctrl? 0.5: 1);

 // Textures
 // Note: the Occlusion and c_Glow arguments passed to Lamp_Lit_Finish() from
 // this macro were determined by visual inspection of radiosity renders with
 // an A19 bulb.
  #local lamp_t_Exterior = texture { finish { Lamp_Finish_Base (1, 1) } }
  #local lamp_t_Hood_hardware = texture
  { pigment { rgb <0.53, 0.51, 0.49> }
    finish
    { Lamp_Lit_Finish (0.5, 0.5, lamp_Rad * 0.12)
      brilliance 5/3
      specular 1.59 metallic roughness 0.01
    }
  }
  #local lamp_t_Metal = texture
  { pigment { rgb <0.53, 0.51, 0.49> }
    #if (mod (lamp_v_Flags.x, 2) = 1)
      finish
      { Lamp_Finish_Base (0.05, 1)
        reflection { 0.9 metallic }
        #if (!Lamp_Reflect) specular 107 metallic roughness 0.001 #end
      }
    #else // e.g., for radiosity pass 1
      finish
      { Lamp_Finish_Base (0.5, 1)
        brilliance 5/3
        specular 1.59 metallic roughness 0.01
      }
    #end
  }

 // Vary the interior ambients to give the impression of radiosity.
  #local LAMP_ZBLOB = 2.1; // eyeballed hood curve reversal
  #local lamp_T = texture { pigment { rgb LAMP__WHITE * lamp_Rad36 } }
  #local lamp_t_Hood = texture
  { gradient z poly_wave LAMP__HOOD_WAVEXP texture_map
    { [ Lamp__fn_Hood_map (LAMP__Z1)
        lamp_T finish { Lamp_Lit_Finish (1, 0.5, lamp_Rad * 0.055) }
      ]
      [ Lamp__fn_Hood_map (LAMP__Z3)
        lamp_T finish { Lamp_Lit_Finish (1, 1/3, lamp_Rad * 0.12) }
      ]
      [ Lamp__fn_Hood_map (LAMP_ZBLOB)
        lamp_T finish { Lamp_Lit_Finish (1, 0.5, lamp_Rad * 0.3) }
      ]
      [ 1
        lamp_T finish { Lamp_Lit_Finish (1, 1, lamp_Rad * 0.25) }
      ]
    }
    scale LAMP__DHOOD - LAMP__Z0
    translate LAMP__Z0 * z
  }
  #local lamp_T = texture { pigment { rgb 0.75 } }
  #local lamp_t_Supports = texture
  { gradient z texture_map
    { [0.0 lamp_T finish { Lamp_Lit_Finish (1, 0.35, lamp_Rad * 0.075) }]
      [0.6 lamp_T finish { Lamp_Lit_Finish (1, 0.45, lamp_Rad * 0.25) }]
      [1.0 lamp_T finish { Lamp_Lit_Finish (1, 0.60, lamp_Rad * 0.3) }]
    }
  }

 // Parts
  #local Lamp_interior_assy = union
  { object
    { Lamp__hood_interior
      texture { lamp_t_Hood }
    }
    difference // socket
    { cylinder
      { 0, z, LAMP_RSOCKET
        texture { lamp_t_Supports scale 1.002 translate -0.001 * z }
        scale <1, 1, LAMP__DHOOD - LAMP_HBULB - LAMP__ZSOCKET>
        translate LAMP__ZSOCKET * z
      }
      superellipsoid
      { <1, 0.5>
        scale <1.3, 1.3, 2.6>
        translate (LAMP__DHOOD - LAMP_HBULB) * z
        texture { lamp_t_Hood_hardware }
      }
      // An area light is required to illuminate the outside of the
      // socket.  Although area_light only works in areas of cast shadow,
      // double_illuminate has the effect of forcing self-shadowing.
      double_illuminate
    }
    // other doohickeys
    #local lamp_yWasher =
      LAMP__THIN - sqrt (pow (LAMP__R1HOOD - LAMP__THIN, 2) - pow (LAMP_RWASHER, 2));
    union
    { intersection // washer
      { box
        { <-LAMP_RWASHER, -LAMP__R1HOOD, LAMP__ZSOCKET>,
          <LAMP_RWASHER, lamp_yWasher, LAMP_RWASHER>
        }
        cylinder
        { 1.01 * LAMP__ZSOCKET * z, 1.01 * LAMP__R1HOOD * z,
          LAMP__R1HOOD - LAMP__THIN
        }
      }
      box // socket support
      { <-LAMP_RWASHER, lamp_yWasher, LAMP__ZSOCKET>,
        <LAMP_RWASHER, 0, LAMP__ZSOCKET + 0.16>
      }
      texture
      { lamp_t_Supports
        scale <1, 1, LAMP_RWASHER - LAMP__ZSOCKET + 0.002>
        translate (LAMP__ZSOCKET - 0.001) * z
      }
    }
    object
    { Lamp__wire
      texture
      { pigment { rgb 0.015 }
        finish { Lamp_Lit_Finish (1, 0.5, lamp_Rad * 0.15) }
      }
    }
    object
    { Lamp__wire
      texture
      { pigment { rgb 0.9 }
        finish { Lamp_Lit_Finish (1, 0.5, lamp_Rad * 0.15) }
      }
      scale <-1, 1, 1>
    }
    union
    { object { Lamp__nut_rudi } // nut
      torus // hollow bolt at end of neck
      { 0.56, 0.08
        scale <1, 2, 1>
        translate max_extent (Lamp__nut_rudi) * y
      }
      texture { lamp_t_Hood_hardware }
      translate lamp_yWasher * y
    }
  } // Lamp_interior_assy

 // final assembly
  union
  {
    union // hood assembly
    { object
      { Lamp__hood
        texture
        { t_Fixture
          scale Lamp_Scale
          rotate -90 * x
          translate (LAMP__DHOOD + 1e-6) * z
        }
      }
      object
      { Lamp_interior_assy
        #if (lamp_IsLit & version >= 3.7 & !lamp__Dbg_hood_ctrl)
          radiosity { importance 1 }
        #end
      }
      object // bulb
      { Bulb
        rotate 90 * x
        translate (LAMP__DHOOD - LAMP_HBULB) * z
        Lamp_Bulb_Material (lamp_c_Surface * lamp_Rad36, lamp_B, mod (lamp_v_Flags.x, 2))
        #if (lamp_IsLit)
          no_shadow
          #if (!Lamp_Reflect) no_reflection #end
          #if (version >= 3.7) no_radiosity #end
        #end
      }
      // If no_radiosity is set on the bulb, then create an invisible mask to
      // block the radiosity behind it.
      #if (version >= 3.7 & Lamp_Radiosity & lamp_IsLit)
        object
        { Bulb
          rotate 90 * x
          translate (LAMP__DHOOD - LAMP_HBULB) * z
          Lamp_Bulb_Material (0, lamp_B, mod (lamp_v_Flags.x, 2))
          no_shadow
          no_image
        }
      #end
      #if (lamp_IsLit & lamp__Dbg_aim) // narrow beam for debugging
        light_source
        { LAMP__DHOOD * z, lamp_c_Light * lamp__Dbg_aim
          cylinder point_at LAMP__DHOOD * 2 * z
          radius 1 / Lamp_Scale falloff 1.5 / Lamp_Scale
        }
      #end
      object // top of neck
      { RE_Cylinder (0, -LAMP__HTOP * y, LAMP_RSTEM, 0.04, no)
        translate (lamp_Flatten - LAMP__R1HOOD) * y
        texture { lamp_t_Metal }
      }
      transform { lamp_x_Light }
    } // hood assembly

    union // remainder of fixture
    {
      object // flexible part of neck
      { #if (lamp_v_Flags.x >= 2)
          Lamp_Isocoil (lamp_lFlex, lamp_Flexd)
          texture { lamp_t_Metal }
        #else
          Lamp_Coil_Normal (lamp_lFlex, lamp_Flexd, lamp_t_Metal)
        #end
        translate <0, LAMP_HBASE + lamp_hBottom, LAMP_ZOFS>
      }
      union // rigid stem at the bottom of the neck
      { RE_Cylinder (0, LAMP_INCH / 16 * y, LAMP_INCH * 3/8, 0.04, no)
        RE_Cylinder_end (lamp_hBottom * y, LAMP_INCH / 16 * y, LAMP_RSTEM, 0.04, no)
        object
        { RE_Round_join (LAMP_RSTEM, 0.04)
          translate LAMP_INCH / 16 * y
        }
        translate <0, LAMP_HBASE, LAMP_ZOFS>
        texture { lamp_t_Metal }
      }
      object // switch
      { Lamp_Switch (Status)
        scale <1, 1, -1>
        translate <0, LAMP_HBASE + 0.16, LAMP_RBASE - LAMP_INCH * 1.5>
      }
      object // base
      { RE_Cylinder_end (LAMP_HBASE * y, LAMP_HPAD * y, LAMP_RBASE, 1.27, no)
        texture { t_Fixture scale Lamp_Scale translate LAMP_HBASE * y }
      }
      cylinder // rubber pad
      { 0, LAMP_HPAD * y, LAMP_RBASE
        texture
        { lamp_t_Exterior
          pigment { rgb 0.03 }
          normal { quilted -1 scale 0.15 translate (LAMP_HPAD - 0.15) / 2 }
        }
      }
      scale 1 / Lamp_Scale
      transform { lamp_x_Position }
    } // remainder of fixture (apart from hood assembly)

   // light source and optional verbosity
    #if (lamp_IsLit)
      // For what reason I cannot discern, failure to reference lamp_c_Light prior
      // to passing it to Lamp_Source() sometimes causes v3.6 to zero it out.
      #local lamp_Dummy = lamp_c_Light;
      Lamp_Source
      ( "Lamp_Flexneck",
        (LAMP__DHOOD - LAMP_HBULB + max_extent (Bulb).y - lamp_rBulb) * z,
        lamp_c_Light * lamp_Brightness, lamp_Fade,
        lamp_v_Soft * <lamp_dBulb, 1, 1, 1>, Use_photons, "ovoid white",
        lamp_rBulb - max_extent (Bulb).y + LAMP_HBULB, lamp_x_Light
      )
      #if (lamp_Refl > 0) // hood diffuse reflection
        Lamp_Source
        ( "Lamp_Flexneck", LAMP__DHOOD * z,
          lamp_c_Light * lamp_Refl, lamp_Fade,
          lamp_v_Soft * <lamp_rInt * 2, 1, 1, 1>, Use_photons, "",
          0, lamp_x_Light
        )
      #end
    #end // light source & optional verbosity
  }
#end // Lamp_Flexneck

//----------------------------------------------------------

#macro Lamp_Has4D (v_Arg)
( #local lamp_Test1 = (v_Arg + 1) + <0,0,0,0>;
  #local lamp_Test2 = (v_Arg + 2) + <0,0,0,0>;
  lamp_Test1.t != lamp_Test2.t
)
#end

//----------------------------------------------------------

#macro Lamp_Isocoil (Length, Flex)
  Lamp_Check ("Lamp_Isocoil", Flex, "Flex", -180, 180)
  #if (Flex = 0)
    isosurface
    { function { Lamp_fn_Isocoil (x, y, z) }
      contained_by
      { box { <-LAMP__RNECK, 0, -LAMP__RNECK>, <LAMP__RNECK, Length, LAMP__RNECK> }
      }
      max_gradient 1.1
    }
  #else
    #local lamp_Flex = radians (Flex);
    #local lamp_RoC = abs (Length / lamp_Flex);
    #local lamp_More = lamp_RoC + LAMP__RNECK;
    #local lamp_Less = lamp_RoC - LAMP__RNECK;
    #local lamp_Sin = sin (lamp_Flex);
    #local lamp_Cos = cos (lamp_Flex);
    #if (Flex > 0)
      isosurface
      { function
        { max
          ( Lamp_fn_Isocoil (x, atan2 (-y, -z) * lamp_RoC, sqrt (y*y + z*z) - lamp_RoC),
            y * lamp_Cos + z * lamp_Sin
          )
        }
        contained_by
        { box
          { <-LAMP__RNECK, 0, -lamp_More>,
            #if (Flex < 90)
              <LAMP__RNECK, lamp_More * lamp_Sin, -lamp_Less * lamp_Cos>
            #else
              <LAMP__RNECK, lamp_More, -lamp_More * lamp_Cos>
            #end
          }
        }
        max_gradient 1.1
        translate lamp_RoC * z
      }
    #else
      isosurface
      { function
        { max
          ( Lamp_fn_Isocoil (x, atan2 (y, z) * lamp_RoC, sqrt (y*y + z*z) - lamp_RoC),
            y * lamp_Cos + z * lamp_Sin
          )
        }
        contained_by
        { box
          { <-LAMP__RNECK, 0, lamp_More>,
            #if (Flex > -90)
              <LAMP__RNECK, -lamp_More * lamp_Sin, lamp_Less * lamp_Cos>
            #else
              <LAMP__RNECK, lamp_More, lamp_More * lamp_Cos>
            #end
          }
        }
        max_gradient 1.1
        translate -lamp_RoC * z
      }
    #end
  #end
#end

//----------------------------------------------------------

#macro Lamp_Lighting
( s_Caller, c_Light, c_Bulb, rBulb, Brightness,
  c_Light_out, Light_brightness_out, Fade_distance_out, c_Surface_out
)
  #local lamp_L = color c_Light;
  #local lamp_B = color c_Bulb;
 // Light color:
  #if (Brightness = 0)
    #declare lamp_C = rgb 0;
  #else
    #if (Brightness > 0) // calculate lumens after filtering
      #local lamp_Gray = Lamp_Luminance (lamp_L * lamp_B);
      #if (lamp_Gray = 0)
        Lamp__light_error
        ( s_Caller, lamp_L, lamp_, Brightness,
          concat
          ( "the light's filtered color (", Lamp__cstr (lamp_L * lamp_B, 4),
            ") is black. Lumens cannot be achieved."
          )
        )
      #end
    #else // calculate lumens before filtering
      #local lamp_Gray = Lamp_Luminance (lamp_L);
      #if (lamp_Gray = 0)
        Lamp__light_error
        ( s_Caller, c_Light, c_Bulb, Brightness,
          concat
          ( "the light's color (", Lamp__cstr (lamp_L, 4),
            ") is black. Lumens cannot be achieved."
          )
        )
      #end
    #end
    #local lamp_C = abs (Brightness) * Lamp_Lumen * lamp_L / lamp_Gray;
  #end
  #declare c_Light_out = lamp_C * lamp_B;
 // Dimensions:
  #if (Lamp__has_invsq()) // use true 1/sq. fade
    #if (lamp__Dbg_fade) #debug "Lamp_Lighting // true inverse square\n" #end
    #declare Fade_distance_out = 0;
    #declare Light_brightness_out = pow (LAMP__DLUMENEQLUX / Lamp_Scale, 2);
    #local lamp_Surface = pow (LAMP__DLUMENEQLUX / rBulb, 2);
  #else // use old POV-Ray attenuation
    #if (lamp__Dbg_fade) #debug "Lamp_Lighting // attenuation approximation\n" #end
    #local Fade_distance_out = Lamp_Fade; // arbitrarily tiny
    // Inverse of POV's attenuation formula:
    #declare Light_brightness_out = (1 + pow (LAMP__DLUMENEQLUX / Fade_distance_out, 2)) / 2;
    // POV's attenuation formula:
    #local lamp_Surface = Light_brightness_out * 2 / (1 + pow (rBulb / Fade_distance_out, 2));
  #end
  #declare c_Surface_out = lamp_C * lamp_Surface;
  #if (lamp__Dbg_fade)
    #debug concat ("( \"", s_Caller, "\", ", Lamp__cstr (lamp_L, 4), ",\n")
    #debug concat
    ( "  ", Lamp__cstr (lamp_B, 4), ", ", Lamp__str (rBulb, 3), ", ", Lamp__str (Brightness, 4), ",\n"
    )
    #debug concat
    ( "  ", Lamp__cstr (c_Light_out, 4), ", ", Lamp__str (Light_brightness_out, 4), ", ",
      Lamp__str (Fade_distance_out, 4), ", ", Lamp__cstr (c_Surface_out, 4), "\n)\n"
    )
  #end
#end

//----------------------------------------------------------

#macro Lamp_Lit_Finish (Diffuse, Occlusion, c_Glow)
  diffuse Diffuse
  #if (lamp__Dbg_hood_ctrl)
    ambient color Diffuse * Lamp_c_Ambient / Lamp_Diffuse
  #else
    ambient color Diffuse * Lamp_c_Ambient / Lamp_Diffuse * Occlusion + c_Glow
      filter 0 transmit 0
  #end
  #if (version >= 3.7) emission 0 #end
#end

//----------------------------------------------------------

#macro Lamp_Luminance (Color)
( #local lamp_G = color Color * Lamp_c_Weights;
  lamp_G.red + lamp_G.green + lamp_G.blue
)
#end

//----------------------------------------------------------

#macro Lamp_Orient_Trans (pv_Base, v_Up, pv_Aim, Warning_up, Warning_aim)
  #local lamp_v_Up = v_Up + <0,0,0>;
  #local lamp_v_Up = <lamp_v_Up.x, lamp_v_Up.y, lamp_v_Up.z>;
  #local lamp_v_Dir = pv_Aim - pv_Base;

  #declare Warning_up = VZero (lamp_v_Up);
  #if (Warning_up) #local lamp_v_Up = y;
  #else #local lamp_v_Up = vnormalize (lamp_v_Up);
  #end

  #local lamp_v_Plane =
    vcross (lamp_v_Up, <lamp_v_Dir.x, lamp_v_Dir.y, lamp_v_Dir.z>);
  #declare Warning_aim = VZero (lamp_v_Plane);
  #if (Warning_aim)
    #local lamp_v_Plane = Lamp__any_perp (lamp_v_Up);
    #if (lamp__Dbg_aim)
      #debug concat ("lamp_v_Plane set to <", Lamp__vstr (lamp_v_Plane, 4), "\n")
    #end
  #end

  #local lamp_x_Plane = Reorient_Trans (x, lamp_v_Plane)
  #local lamp_v_inPlane = vtransform (y, lamp_x_Plane);
  #local lamp_aFinal = acos (vdot (lamp_v_Up, lamp_v_inPlane));
  #local lamp_x_Rotn = transform
  { rotate degrees (lamp_aFinal) * x
    transform { lamp_x_Plane }
  }
  #local lamp_Sanity = vtransform (y, lamp_x_Rotn);
  #if (lamp__Dbg_aim)
    #debug concat ("v_Dir = ", Lamp__vstr (lamp_v_Dir, 6), "\n")
    #debug concat ("v_Plane = ", Lamp__vstr (lamp_v_Plane, 6), "\n")
    #debug concat ("v_inPlane = ", Lamp__vstr (lamp_v_inPlane, 6), "\n")
    #debug concat ("aFinal = ", str (degrees (lamp_aFinal), 0, 6), " degrees\n")
    #debug concat ("v_Up   = ", Lamp__vstr (lamp_v_Up, 6), "\n")
    #debug concat ("Sanity = ", Lamp__vstr (lamp_Sanity, 6), "\n")
  #end
  #if (!VEq (lamp_v_Up, lamp_Sanity))
    #local lamp_x_Rotn = transform
    { rotate -degrees (lamp_aFinal) * x
      transform { lamp_x_Plane }
    }
    #if (lamp__Dbg_aim)
      #debug concat ("Sanity = <", Lamp__vstr (vtransform (y, lamp_x_Rotn), 6), " on 2nd try\n")
    #end
  #end
  #local lamp_x_Final = transform
  { lamp_x_Rotn
    translate pv_Base
  }
  lamp_x_Final
#end

//----------------------------------------------------------

#macro Lamp_Source (s_Caller, pv_Location, c_Light, Fade, v4_Soft, Photons, s_Hood, zHood, Xform)
  #local lamp_Spotlight = (strlen (s_Hood) = 0);
  #local lamp_v_Soft = <0,0,0,0> + v4_Soft;
  #local lamp_pv_Final = vtransform (<0,0,0> + pv_Location, Xform);
  #local lamp_v_Aim = vtransform (z * 100 + pv_Location, Xform);
  #if (lamp_v_Soft.x != 0)
    #local lamp_Xlate = vtransform (<0,0,0>, Xform);
    #local lamp_dAreaCm = Lamp_fn_Area_Adjust (lamp_v_Soft.x, lamp_v_Soft.y);
    #local lamp_dArea = lamp_dAreaCm / Lamp_Scale;
    #local lamp_v_xArea = vtransform (lamp_dAreaCm * x, Xform) - lamp_Xlate;
    #local lamp_v_yArea = vtransform (lamp_dAreaCm * y, Xform) - lamp_Xlate;
  #end
  light_source
  { lamp_pv_Final, color c_Light
    fade_distance Fade / Lamp_Scale
    fade_power 2
    #if (lamp_Spotlight)
      spotlight point_at lamp_v_Aim
      radius -90 falloff 90 tightness 1 // cosine falloff
    #end
    #if (lamp_v_Soft.x != 0)
      area_light
      #if (lamp_Spotlight)
        lamp_v_xArea, lamp_v_yArea, lamp_v_Soft.y, lamp_v_Soft.y
      #else
        lamp_dArea * x, lamp_dArea * y, lamp_v_Soft.y, lamp_v_Soft.y
        orient
      #end
      circular jitter adaptive lamp_v_Soft.z
      #if (lamp_v_Soft.t)
        #if (version >= 3.7)
          area_illumination
        // Spotlights are assumed to be paired with a main light,
        // so to avoid a double warning, do not warn for spotlight.
        #else #if (!lamp_Spotlight)
          #warning concat
          ( s_Caller, ":() area_illumination is not available in POV-Ray ",
            str (version, 0, 2), "."
          )
        #end
        #end
      #end
    #end
    #if (!Use_photons)
      photons { reflection off refraction off }
    #end
  }
 //----- Write transformed SDL to the debug stream -----
  #if (Lamp_Debug)
   //----- light source -----
    #debug concat
    ( "light_source // ",
      #if (!lamp_Spotlight)
        "main light source\n"
      #else #if (Lamp_Radiosity)
        "radiosity makeup for max_sample ", Lamp__str (Lamp_Max_Sample, 4), "\n"
      #else
        "radiosity replacement\n"
      #end
      #end
    )
    #debug concat
    ( "{ ", Lamp__vstr (lamp_pv_Final, 6),
      ", ", Lamp__cstr (color c_Light, 4), "\n"
    )
    #debug concat
    ( "  fade_distance ", Lamp__str (Fade / Lamp_Scale, 4), "\n  fade_power 2\n"
    )
    #if (lamp_Spotlight) // spotlight
      #debug concat ("  spotlight point_at ", Lamp__vstr (lamp_v_Aim, 6), "\n")
      #debug "  radius -90 falloff 90 tightness 1\n"
    #end
    #if (lamp_v_Soft.x != 0)
      #debug "  area_light "
      #if (lamp_Spotlight) // directed area light
        #debug concat
        ( Lamp__vstr (lamp_v_xArea, 6), ", ",
          Lamp__vstr (lamp_v_yArea, 6), ", "
        )
      #else // isotropic area light
        #debug concat
        ( Lamp__str (lamp_dArea, 4), " * x, ",
          Lamp__str (lamp_dArea, 4), " * y, "
        )
      #end
      #debug concat (str (lamp_v_Soft.y, 0, 0), ", ", str (lamp_v_Soft.y, 0, 0), "\n")
      #debug concat
      ( "  circular jitter adaptive ", str (floor (lamp_v_Soft.z), 0, 0),
        #if (!lamp_Spotlight) " orient", #end
        "\n"
      )
      #if (lamp_v_Soft.t & version >= 3.7)
        #debug "  area_illumination\n"
      #end
    #end
    #if (!Photons)
      #debug "  photons { reflection off refraction off }\n"
    #end
    #debug "}\n"
   //----- rudimentary hood -----
    #switch (0)
      #case (strcmp (s_Hood, ""))
        #break
      #case (strcmp (s_Hood, "ovoid white"))
        #debug "sphere // rudimentary hood\n"
        #debug concat
        ( "{ 0, ", Lamp__str ((LAMP__R2HOOD - LAMP__THIN) / Lamp_Scale, 4),
          " scale <1, 1, ",
          str ((LAMP__DHOOD + 0.4 - LAMP__THIN) / (LAMP__R2HOOD - LAMP__THIN), 0, 3),
          "> hollow\n"
        )
        #debug "  clipped_by { plane { z, 0 } }\n"
        #debug concat ("  translate ", Lamp__str (zHood / Lamp_Scale, 4), " * z\n")
        #debug concat
        ( "  Reorient_Trans (z, ", Lamp__vstr (lamp_v_Aim - lamp_pv_Final, 6), ")\n"
        )
        #debug concat ("  translate ", Lamp__vstr (lamp_pv_Final, 6), "\n")
        #debug concat ("  pigment { rgb ", str (LAMP__WHITE, 0, 2), " }\n")
        #debug concat
        ( "  finish { ambient ", Lamp__cstr (color Lamp_c_Ambient / Lamp_Diffuse, 4),
          " diffuse 1",
          #if (version >= 3.7) " emission 0", #end
          " }\n"
        )
        #if (version >= 3.7 & !VZero (color c_Light))
          #debug "  radiosity { importance 1 }\n"
        #end
        #debug "}\n"
        #break
      #else
        #warning concat (s_Caller, "(): Lamp_Source(): unknown hood type \"", s_Hood, "\"")
    #end
  #end
#end

//----------------------------------------------------------

#macro Lamp_Switch (State)
  #local LAMP_ROTN = 45;
  #local LAMP_R = 1.2;
  #local LAMP_rEDGE = 0.075;
  #local LAMP_RRIM = 1.5;
  #local lamp_RCutout = LAMP_R * tan (radians ((180 - LAMP_ROTN) / 2));
  #local lamp_Switch_blob = LAMP_rEDGE / LAMP_R;
  #local lamp_Rim_blob = LAMP_rEDGE / (LAMP_R + LAMP_RRIM) * 2;
  union
  { isosurface
    { function
      { sqrt
        (  RE_fn_Blob2
           ( f_superellipsoid (z / LAMP_R, y / LAMP_R, x / LAMP_R, 1, 0.6),
             lamp_Switch_blob
           )
         + RE_fn_Blob2
           ( f_sphere (0, y - lamp_RCutout, z - LAMP_R, lamp_RCutout),
             LAMP_rEDGE
           )
        ) - 1
      }
      contained_by
      { box { -LAMP_R, <LAMP_R, LAMP_R * sin (radians (LAMP_ROTN)), LAMP_R> }
      }
      max_gradient 0.90 / lamp_Switch_blob
      rotate -State * LAMP_ROTN * x
    }
    isosurface
    { function
      { sqrt
        (  RE_fn_Blob2 (-y, LAMP_rEDGE)
         + RE_fn_Blob2
           ( -max
             ( -f_superellipsoid (z / LAMP_RRIM, 0, x / LAMP_RRIM, 1, 0.65),
               f_superellipsoid (z / LAMP_R, 0, x / LAMP_R, 1, 0.6)
             ),
             lamp_Rim_blob
           )
        ) - 1
      }
      contained_by
      { box { -<LAMP_RRIM, 1, LAMP_RRIM>, <LAMP_RRIM, 0, LAMP_RRIM> }
      }
      max_gradient 0.79 / lamp_Rim_blob
    }
    pigment { rgb 0.015 }
    finish
    { Lamp_Finish_Base (1, 1)
      specular 0.215 roughness 0.03
    }
  }
#end

//----------------------------------------------------------

#macro Lamp_Warnings()
 // In this version of DeskLamp, all global warnings involve pre-3.7 radiosity
  #if (version < 3.7 & Lamp_Radiosity)
    #if (!VZero (Lamp_c_Ambient) & !lamp__Warned_ambient)
      #warning concat
      ( "For best results with radiosity in POV-Ray ", str (version, 0, 2),
        ", Lamp_c_Ambient and the #default ambient should both be set to zero."
      )
      #declare lamp__Warned_ambient = yes;
    #end
    #if (Lamp_Max_Sample <= 0 & !lamp__Warned_max)
      #warning concat
      ( "To reduce radiosity artifacts in POV-Ray ", str (version, 0, 2),
        ", set radiosity { max_sample } and Lamp_Max_Sample to a positive number."
      )
      #declare lamp__Warned_max = yes;
    #end
  #end
#end

//------------------ LIGHTSYS INTERFACE --------------------

// DeskLamp attenuation at 1 meter:
#local lamp_InvSq = 1 / (4 * pi);
// Plugging Light_Fading() and NormLct in lightsys.inc
// into the POV-Ray attenuation formula, at 1 meter:
#local lamp_LS4 = 2 / (sqrt(3) * (1 + pow (100 / (2 * pi), 2)));
#declare LAMP__LS4 = lamp_InvSq / lamp_LS4;
// DeskLamp and Lightsys IV have the same base unit of length:
#declare LAMP__LS4_SCALE = 1;

//--------------------------------------

#macro Lamp_Get_Lightsys()
  #declare Lamp_Lumen =
    #ifdef (Lightsys_Brightness) Lightsys_Brightness / LAMP__LS4;
    #else 1 / LAMP__LS4; // Lightsys brightness defaults to 1.
    #end
  #declare Lamp_Scale =
    #ifdef (Lightsys_Scene_Scale) LAMP__LS4_SCALE / Lightsys_Scene_Scale;
    #else LAMP__LS4_SCALE; // Lightsys scale defaults to 1.
    #end
#end

//--------------------------------------

#macro Lamp_Set_Lightsys()
  #declare Lightsys_Brightness = Lamp_Lumen * LAMP__LS4;
  #declare Lightsys_Scene_Scale = LAMP__LS4_SCALE / Lamp_Scale;
#end

//============================ PUBLIC LIGHT BULBS ==============================

#local LAMP_A60RNECK = 1.6;
#local lamp_A60join = LAMP_HBULB - 12 + 3 * LAMP_A60RNECK;
#declare Lamp_Bulb_A60 = merge
{ sphere { (LAMP_HBULB - 3) * y, 3 }
  difference
  { cylinder { 0, 3 * (2.4 - LAMP_A60RNECK) * y, 2.4 }
    torus { 12 - 4 * LAMP_A60RNECK, 5 * (2.4 - LAMP_A60RNECK) }
    translate lamp_A60join * y
  }
  object
  { RE_Cylinder_end (0, (lamp_A60join + 0.01) / 2 * y, LAMP_A60RNECK, LAMP_A60RNECK - 1.3, yes)
    scale <1, 2, 1>
  }
}

#declare Lamp_Bulb_A19 = Lamp_Bulb_A60

//----------------------------------------------------------

#version DeskLamp_Inc_Temp;
#end
// end of desklamp.inc
