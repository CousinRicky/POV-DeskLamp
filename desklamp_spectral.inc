/* desklamp_spectral.inc version 3.0-alpha
 * Persistence of Vision Raytracer include file
 * A proposed POV-Ray Object Collection library
 *
 * An add-on to DeskLamp for rendering with LILYsoft SpectralRender.
 *
 * Copyright (C) 20?? Richard Callwood III.  Some rights reserved.
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
 * 3.0    20??-???-??  Created.
 *
 * Implementation note: due to POV-Ray namespace scope leakage, local variables
 * and function parameters (other than x, y, and z) *must* be prefixed.
 */
/* This file references the following identifiers that are defined by libraries
 * Lightsys IV or SpectralRender, which are not in the Object Collection:
 *   CMF_xyz
 *   EXT_Metal
 *   IOR_K_Iron
 *   IOR_Metal
 *   IOR_N_Iron
 *   MetalIndex
 *   SpectralMode
 *   WavelengthFromIndex
 *   WavelengthIndex
 *   xyz2xyY
 *   @@ MORE TO COME
 */
#ifndef (DeskLamp_Spectral_Inc_Temp) #declare DeskLamp_Spectral_Inc_Temp = version;
#version max (3.7, min (3.8, version)); // Bracket the POV version.

#declare DESKLAMP__SPECTRAL_VERSION = 3.0;

#ifdef (View_POV_Include_Stack)
  #debug concat ("#include \"desklamp_spectral.inc\"\n")
#end

// Explain the situation to the user before letting the scene crash:
#if (!file_exists ("CIE.inc"))
  #warning "Lightsys 4 (Lightsys IV) is a prerequisite for desklamp_spectral.inc."
#end
#include "CIE.inc"

// Do not #include spectral.inc here, because it currently has no mechanism to
// prevent multiple #includes.
#ifndef (SpectralMode)
  #error "SpectralRender file spectral.inc must be #included before #including desklamp_spectral.inc."
#end

#include "desklamp.inc"
#if (DESKLAMP__SPECTRAL_VERSION != DESKLAMP_VERSION)
  #error concat
  ( "DeskLamp version mismatch: desklamp.inc v", DESKLAMP__S_VSUFFIX,
    "; desklamp_spectral.inc v", Lamp__Version_s (DESKLAMP__SPECTRAL_VERSION, "")
  )
#end

//----------------------------------------------------------

#declare Lamp_i_Metal = interior { IOR_Metal (IOR_N_Iron, IOR_K_Iron) }

//----------------------------------------------------------

#macro Lamp_Metal_m (Occlusion)
  material
  { Lamp_Metal_t (0)
    interior { Lamp_i_Metal }
  }
#end

//----------------------------------------------------------

// Adapted from spectral_metals.inc::M_Spectral_Metal()
//@@ UNFINISHED! Pigment and highlights need to be accounted for.
#macro Lamp_Metal_t (Occlusion)
  texture
  { pigment {  }
    finish
    { ambient 0 emission 0 diffuse 0.05
      reflection
      { #if (SpectralMode)
          MetalIndex (IOR_N_Iron, IOR_K_Iron, WavelengthIndex) * 0.9, 0.95
        #else
          0.9, 0.95 metallic
        #end
        fresnel on
      }
      conserve_energy
      brilliance EXT_Metal (IOR_N_Iron, IOR_K_Iron)
      metallic
    }
  }
#end

//----------------------------------------------------------

#if (SpectralMode)

  //@@ #declare Lamp_c_Weights = rgb 1/3;

  #macro Lamp_Luminance (srColor)
  ( #local lamp_Acc = <0, 0, 0>;
    #for (lamp_I, 0, 35)
      #local lamp_Acc = lamp_Acc + CMF_xyz (WavelengthFromIndex (lamp_I)) * srColor [lamp_I];
    #end
    // 10 is the band resolution in nm; / 100 is for unit conversion:
    (xyz2xyY (lamp_Acc * 10 / 100)).z
  )
  #end

//----------------------------------------------------------

  #macro Lamp_Spectral_Bright (Brightness, srColor)
    ( Brightness * srColor [WavelengthIndex] / Lamp_Luminance (srColor) )
  #end

#else //----------------------------------------------------

  #macro Lamp_Spectral_Bright (srColor)
    1
  #end

#end

//----------------------------------------------------------

#version DeskLamp_Spectral_Inc_Temp;
#end
// end of desklamp_spectral.inc
