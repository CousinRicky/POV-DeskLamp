/* desklamp_spectral.inc version 3.0-alpha.20260918  2026-Sep-18
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
 * 3.0    20??-???-??  Completed.
 *
 * Implementation note: due to POV-Ray namespace scope leakage, local variables
 * and function parameters (other than x, y, and z) *must* be prefixed.
 */
/* This file references the following identifiers that are defined by libraries
 * Lightsys IV or SpectralRender, which are not in the Object Collection:
 *   C_Average
 *   C_Metal
 *   C_RGB
 *   C_Spectral
 *   CMF_xyz
 *   D_CC_A4
 *   D_CC_B4
 *   D_CC_F4
 *   EXT_Metal
 *   IOR_K_Iron
 *   IOR_Metal
 *   IOR_N_Iron
 *   MetalIndex
 *   SpectralMode
 *   WavelengthFromIndex
 *   WavelengthIndex
 *   Value_0
 *   xyz2xyY
 */
#ifndef (DeskLamp_Spectral_Inc_Temp) #declare DeskLamp_Spectral_Inc_Temp = version;
#version max (3.7, min (3.8, version)); // Minimum POV version is 3.7.

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
  ( "DeskLamp version mismatch: desklamp.inc v", lamp__s_Version,
    "; desklamp_spectral.inc v", Lamp__Version_s (DESKLAMP__SPECTRAL_VERSION, "")
  )
#end

//----------------------------------------------------------

#declare Lamp_c_Hardware = C_RGB (0.756, 0.873, 0.773);
#declare Lamp_c_Metal = C_Metal (IOR_N_Iron, IOR_K_Iron);
#declare Lamp_c_Padding = C_Spectral (D_CC_F4);
#declare Lamp_c_Switch_Neg = C_Average (D_CC_F4, 1, Value_0, 1);
#declare Lamp_c_Switch_Pos = C_Average (D_CC_A4, 2, D_CC_B4, 1);
#declare Lamp_i_Metal = interior { IOR_Metal (IOR_N_Iron, IOR_K_Iron) }

//----------------------------------------------------------

#macro Lamp_Metal_m (Occlusion)
  material
  { Lamp_Metal_t (Occlusion)
    interior { Lamp_i_Metal }
  }
#end

//----------------------------------------------------------

#macro Lamp_Metal_t (Occlusion)
  Lamp_SRMetal_t (Occlusion, IOR_N_Iron, IOR_K_Iron, 1)
#end

//----------------------------------------------------------

// Adapted from spectral_metals.inc::M_Spectral_Metal()
#macro Lamp_SRMetal_t (Occlusion, N, K, Reflectance)
  texture
  { pigment { C_Metal (N, K) }
    finish
    { Lamp_Finish_Base (0.5 - Reflectance * 0.45, Occlusion)
      reflection
      { #if (SpectralMode)
          MetalIndex (N, K, WavelengthIndex) * 0.9, 0.95
        #else
          0.9, 0.95 metallic
        #end
        fresnel on
      }
      conserve_energy
      brilliance EXT_Metal (N, K)
      metallic
      #if (!Lamp_Reflect) specular albedo 0.9 metallic roughness 0.001 #end
    }
  }
#end

//========================= SPECTRAL MODE ============================

#if (SpectralMode)

  #macro Lamp_SRBrightness (Brightness, sr_Light, sr_Filter)
  ( #if (Brightness < 0)
      Brightness * sr_Light [WavelengthIndex] / Lamp_SRLuminance (sr_Light)
    #else
      -Brightness * sr_Light [WavelengthIndex]
      / (Lamp_SRLuminance (sr_Light) * Lamp_SRLuminance (sr_Filter))
    #end
  )
  #end

  //--------------------------------------------------------

  #macro Lamp_SRFilter (sr_Color, sr_Filter, Scale)
    #local lamp_Out = array[36]
    #for (W, 0, 35)
      #local lamp_Out[W] = sr_Color[W] * sr_Filter[W] * Scale;
    #end
    lamp_Out
  #end

  //--------------------------------------------------------

  #macro Lamp_SRLuminance (sr_Color)
  ( #local lamp_Acc = <0, 0, 0>;
    #for (lamp_I, 0, 35)
      #local lamp_Acc = lamp_Acc + CMF_xyz (WavelengthFromIndex (lamp_I)) * sr_Color [lamp_I];
    #end
    // 10 is the band resolution in nm; / 100 is for unit conversion:
    (xyz2xyY (lamp_Acc * 10 / 100)).z
  )
  #end

  #declare Lamp_c_Weights = rgb 1/3; // probably irrelevant or useless

#else //==================== PREVIEW MODE ============================

  #macro Lamp_SRBrightness (Brightness, sr_Light, sr_Filter)
    Brightness
  #end

  //--------------------------------------------------------

  #macro Lamp_SRFilter (sr_Color, sr_Filter, Scale)
    (sr_Color * sr_Filter * Scale)
  #end

  //--------------------------------------------------------

  #macro Lamp_SRLuminance (sr_Color)
    Lamp_Luminance (sr_Color)
  #end

#end

//----------------------------------------------------------

#version DeskLamp_Spectral_Inc_Temp;
#end
// end of desklamp_spectral.inc
