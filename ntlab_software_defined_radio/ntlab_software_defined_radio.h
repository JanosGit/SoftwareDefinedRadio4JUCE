/*
This file is part of SoftwareDefinedRadio4JUCE.

SoftwareDefinedRadio4JUCE is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

SoftwareDefinedRadio4JUCE is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SoftwareDefinedRadio4JUCE. If not, see <http://www.gnu.org/licenses/>.
*/

 /*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.txt file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 ntlab_software_defined_radio
  vendor:             Janos Buttgereit
  version:            1.0.0
  name:               Software Defined Radio
  description:        Classes to use JUCE for software defined radio applications, optionally OpenCL accelerated
  website:
  license:            GPL v3
  minimumCppStandard: 17

  dependencies:       juce_dsp, juce_data_structures

  OSXFrameworks:      OpenCL

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

#pragma once

//==============================================================================
/** Config: NTLAB_NO_SIMD
    Per default AVX2 instructions are used on x86-64 architecture and NEON instructions
    are used on ARM architecture. Enabling this flag generates non SIMD-optimized code
    instead. You might want to enable it if your target architecture does not support
    those instruction sets
*/
#ifndef NTLAB_NO_SIMD
#define NTLAB_NO_SIMD 0
#endif

/** Config: NTLAB_INCLUDE_EIGEN
    Some SDR DSP operations are based on heavy matrix calculations. To implement some
    of those features the Eigen Linear Algebra library was included as a submodule. You
    can choose to not download this submodule and therefore not enable this flag to exclude
    all features that rely on Eigen. Make sure that the Eigen license suits to your needs
    when including Eigen sources in your project.
 */
#ifndef NTLAB_INCLUDE_EIGEN
#define NTLAB_INCLUDE_EIGEN 0
#endif

/** Config: NTLAB_WARN_ABOUT_MISSING_UHD_GAIN_ELEMENTS
    If enabled, a message to stderr will be printed in case that an expected gain element
    could not be found in your hardware setup. SoftwareDefinedRadio4JUCE tries to abstract
    the device behaviour as good as possible, however we don't know each variety of Ettus
    devices out there so we are not sure if the current code covers all edge cases. This
    warning helps us to spot unhandled cases and add a string matching case to handle those
    cases.
    Disable this option if you are bothered by false errors.
 */
#ifndef NTLAB_WARN_ABOUT_MISSING_UHD_GAIN_ELEMENTS
#define NTLAB_WARN_ABOUT_MISSING_UHD_GAIN_ELEMENTS 1
#endif

/** Config: NTLAB_USE_CL_DSP
    This enables some DSP classes to use OpenCL for DSP processing. Some constructors might chang
    if this option is enabled
 */
#ifndef NTLAB_USE_CL_DSP
#define NTLAB_USE_CL_DSP 0
#endif

/** Config: NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
    This enables the SDRIOEngine classes to use OpenCL Buffers for the processing callback
 */
#ifndef NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
#define NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK 0
#endif

#if !JUCE_IOS
#include "HardwareDevices/EttusEngine/UHDReplacement.h"
#endif
#include "HardwareDevices/EttusEngine/UHDEngine.h"
#include "HardwareDevices/MCVFileEngine/MCVFileEngine.h"
#include "HardwareDevices/SDRIODeviceCallback.h"
#include "HardwareDevices/SDRIOEngine.h"
#include "HardwareDevices/SDRIODeviceManger.h"

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
#include "GUI/UHDConfigComponent.h"
#include "GUI/MCVConfigComponent.h"
#endif

#include "DSP/Oscillator.h"

#if NTLAB_INCLUDE_EIGEN
#include "Matrix/Eigen/Eigen/Dense"
#include "Matrix/CovarianceMatrix.h"
#endif

#if NTLAB_USE_CL_DSP
#include "OpenCL2/cl2WithVersionChecks.h"
#include "OpenCL2/clArray.h"
#include "Matrix/CLCovarianceMatrix.h"
#endif

#include "MCVFileFormat/MCVHeader.h"
#include "MCVFileFormat/MCVWriter.h"
#include "MCVFileFormat/MCVReader.h"



#include "SampleBuffers/VectorOperations.h"
#include "SampleBuffers/SampleBuffers.h"

#include "Threading/RealtimeSetterThreadWithFIFO.h"

#include "UnitTestHelpers/UnitTestHelpers.h"