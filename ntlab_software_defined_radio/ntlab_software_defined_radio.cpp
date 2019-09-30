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

#include "HardwareDevices/SDRIODeviceManger.cpp"
#include "HardwareDevices/SDRIOEngine.cpp"

#include "HardwareDevices/EttusEngine/UHDReplacement.cpp"
#include "HardwareDevices/EttusEngine/UHDEngine.cpp"
#include "HardwareDevices/HackRFEngine/HackRFReplacement.cpp"
#include "HardwareDevices/HackRFEngine/HackRFEngine.cpp"
#include "HardwareDevices/MCVFileEngine/MCVFileEngine.cpp"

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
#include "GUI/UHDConfigComponent.cpp"
#include "GUI/MCVConfigComponent.cpp"
#include "GUI/HackRFConfigComponent.cpp"
#endif

#include "DSP/Oscillator.cpp"

#include "MCVFileFormat/MCVWriter.cpp"
#include "MCVFileFormat/MCVReader.cpp"
#include "MCVFileFormat/MCVUnitTests.cpp"

#include "SampleBuffers/VectorOperations.cpp"

#include "Matrix/CovarianceMatrix.cpp"

#include "UnitTestHelpers/UnitTestHelpers.cpp"

#include "Utilities/VarStreamOperator.cpp"

#if NTLAB_USE_CL_DSP
#include "OpenCL2/SharedCLDevice.cpp"
#endif

#include "DSP/GNSSAquisition.cpp"
#include "DSP/GNSSAquisitionTwiddleTable.cpp"