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
R"(

kernel void oscillatorFillNextSampleBufferComplex (global float2*      restrict bufferToFill,
                                                   global const int*   restrict channelList,
                                                   global const float* restrict angle,
                                                   global const float* restrict angleDelta,
                                                   global const float* restrict amplitude)
{
    const int channel = get_global_id (0);
    const int sample  = get_global_id (1);
    
    global float2* channelPtr = bufferToFill + channelList[channel];
    const float currentAngle  = angle[channel] + angleDelta[channel] * sample;
    const float currentAmpl   = amplitude[channel];
    
    channelPtr[sample].x = native_cos (currentAngle) * currentAmpl;
    channelPtr[sample].y = native_sin (currentAngle) * currentAmpl;
};

kernel void oscillatorFillNextSampleBufferReal (global float*       restrict bufferToFill,
                                                global const int*   restrict channelList,
                                                global const float* restrict angle,
                                                global const float* restrict angleDelta,
                                                global const float* restrict amplitude)
{
    const int channel = get_global_id (0);
    const int sample  = get_global_id (1);
    
    global float* channelPtr = bufferToFill + channelList[channel];
    const float currentAngle = angle[channel] + angleDelta[channel] * sample;
    const float currentAmpl  = amplitude[channel];
    
    channelPtr[sample] = native_sin (currentAngle) * currentAmpl;
};
)"
