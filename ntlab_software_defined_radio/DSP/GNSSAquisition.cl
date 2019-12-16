R"(
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

#define FFT_LENGTH 16384
#define HALF_FFT_LENGTH 8192
#define FFT_ORDER 14

#define FREQ_SPACING_HZ 500
#define NUM_FREQ_STEPS 28
#define MIN_FREQ (((-NUM_FREQ_STEPS / 2) * FREQ_SPACING_HZ) + (FREQ_SPACING_HZ / 2))

#define F_SAMPLE_HZ 16367600
#define T_SAMPLE_SEC 6.109631222659401e-08f

// seems to be missing on some apple opencl implementations...
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Multiplies the complex vector a and the complex conjugated values of vector b and puts the result into dest. a, b and
 * dest must not‚ overlap. All vectors must be of the size FFT_LENGTH.
 */
void complexConjVectorMultiply (global float2* restrict dest, const global float2* restrict a, const global float2* restrict b);

/** Multiplies a and b and returns the result */
float2 complexMultiply     (const float2 a, const float2 b);

/** Multiplies a with the complex conjugated value of b and returns the result */
float2 complexConjMultiply (const float2 a, const float2 b);

/** Mixes down the input samples. Needed once per aquisition */
//__attribute__((reqd_work_group_size(NUM_FREQ_STEPS, 1, 1)))
kernel void gnssMixInput (const global float2* restrict inSamples, global float2* restrict inSamplesMixed, const global float2* restrict twiddleTable)
{
    const int freqOffsetIdx = get_global_id (0);
    const int freqOffsetHz  = MIN_FREQ + freqOffsetIdx * FREQ_SPACING_HZ;
    
    // Using the output value column for this frequency as the temporary working buffer the whole way through this work item. Therefore the buffer passed
    // in must be twice as big as needed (as it's real valued but will hold complex valued intermediate results)
    // Could be a bottle neck because it's global memory
    global float2*  workingBuffer = inSamplesMixed + freqOffsetIdx * FFT_LENGTH;
    
    // First step: Mix input signal with a oscillator value according to the frequency offset of this work item
    for (int i = 0; i < FFT_LENGTH; ++i)
    {
        const float arg = 2 * M_PI * freqOffsetHz * i * T_SAMPLE_SEC;
        const float2 oscValue = (float2)(native_cos (arg), native_sin (arg));
        
        workingBuffer[i] = complexMultiply (inSamples[i], oscValue);
    }
    
    // Second step: Perform a DIF FFT that outputs a bit-reversed ordered spectrum. Not very optimized at this time.
    global float2* currentTwiddles = twiddleTable;
    for (int stage = FFT_ORDER ; stage > 0; --stage)
    {
        const int sizeThisStage = 1 << stage;
        const int halfSizeThisStage = sizeThisStage / 2;
        const int numButterfliesThisStage = FFT_LENGTH / sizeThisStage;
        
        global float2* upperWorkingBuffer = workingBuffer;
        
        for (int butterfly = 0; butterfly < numButterfliesThisStage; ++butterfly)
        {
            global float2* lowerWorkingBuffer = upperWorkingBuffer + halfSizeThisStage;
            
            for (int i = 0; i < halfSizeThisStage; ++i)
            {
                float2 tmp = upperWorkingBuffer[i] + lowerWorkingBuffer[i];
                lowerWorkingBuffer[i] = complexMultiply (upperWorkingBuffer[i] - lowerWorkingBuffer[i], currentTwiddles[i]);
                upperWorkingBuffer[i] = tmp;
            }
            
            upperWorkingBuffer += sizeThisStage;
        }
        
        currentTwiddles += halfSizeThisStage;
    }
}

/** Correlates mixed signal with the CA code. Needed once per CA code */
//__attribute__((reqd_work_group_size(NUM_FREQ_STEPS, 1, 1)))
kernel void gnssAcquisition (const global float2* restrict inSamplesMixed,
                             const global float2* restrict caCodes,
                             global float2*       restrict intermediateResults,
                             const global float2* restrict twiddleTable,
                             const int                     caCodeToUse,
                             global uchar*        restrict acquisitionSpectrum,
                             global int*          restrict idxOfMax,
                             global float*        restrict valueOfMax,
                             global float*        restrict meanValues)
{
    const int freqOffsetIdx = get_global_id (0);
    
    const global float2* caCode = caCodes + caCodeToUse * FFT_LENGTH;
    
    // Using the output value column for this frequency as the temporary working buffer the whole way through this work item. Therefore the buffer passed
    // in must be twice as big as needed (as it's real valued but will hold complex valued intermediate results)
    // Could be a bottle neck because it's global memory
    global float2* workingBuffer = intermediateResults + freqOffsetIdx * FFT_LENGTH;
    global uchar*  outBuffer     = acquisitionSpectrum + freqOffsetIdx * FFT_LENGTH;
    
    const global float2* InBuffer = inSamplesMixed + freqOffsetIdx * FFT_LENGTH;
    
    // Third step: Element wise multiplication with the PRN code to perform the correlation in the frequency domain.
    // Note: it is expected that the PRN code is already shuffled in a bit reversed indexed order and that the values are complex conjugated
    complexConjVectorMultiply (workingBuffer, InBuffer, caCode);

    // Fourth step: An IFFT, implemented as DIT FFT --> can work on shuffled vector and will output a correctly ordered vector
    global float2* currentTwiddles = twiddleTable + FFT_LENGTH - 1;
    for (int stage = 1; stage <= FFT_ORDER; ++stage)
    {
        const int sizeThisStage = 1 << stage;
        const int halfSizeThisStage = sizeThisStage / 2;
        const int numButterfliesThisStage = FFT_LENGTH / sizeThisStage;
        
        // we were at the end of the table in the fft above - now step backwards through the table
        currentTwiddles -= halfSizeThisStage;
        
        global float2* upperWorkingBuffer = workingBuffer;
        global float2* lowerWorkingBuffer;
        
        for (int butterfly = 0; butterfly < numButterfliesThisStage; ++butterfly)
        {
            lowerWorkingBuffer = upperWorkingBuffer + halfSizeThisStage;
            
            for (int i = 0; i < halfSizeThisStage; ++i)
            {
                lowerWorkingBuffer[i] = complexConjMultiply (lowerWorkingBuffer[i], currentTwiddles[i]);
                float2 tmp = lowerWorkingBuffer[i] + upperWorkingBuffer[i];
                lowerWorkingBuffer[i] = upperWorkingBuffer[i] - lowerWorkingBuffer[i];
                upperWorkingBuffer[i] = tmp;
            }
            upperWorkingBuffer += sizeThisStage;
        }
    }
    
    // Fifth step: compute absolute values and scale output (maybe not neccessary at all...)
    int   maxIdx = 0;
    float maxVal = -FLT_MAX;
    float meanVal = 0;
    for (int i = 0; i < FFT_LENGTH; ++i)
    {
        float newVal = native_sqrt (workingBuffer[i].x * workingBuffer[i].x + workingBuffer[i].y * workingBuffer[i].y) / FFT_LENGTH;
        if (newVal > maxVal)
        {
            maxVal = newVal;
            maxIdx = i;
        }
        meanVal += newVal;
        
        // scale to int
        newVal -= 0.2f;
        newVal *= 85.0f;
        if (newVal > 255.0f)
            newVal = 255.0f;
        if (newVal < 0.0f)
            newVal = 0.0f;
        
        outBuffer[i] = (uchar)newVal;
    }
    
    idxOfMax[freqOffsetIdx]   = maxIdx;
    valueOfMax[freqOffsetIdx] = maxVal;
    meanValues[freqOffsetIdx] = meanVal;
}

void complexConjVectorMultiply (global float2* restrict dest, const global float2* restrict a, const global float2* restrict b)
{
    for (int i = 0; i < FFT_LENGTH; ++i)
    {
        dest[i].x = a[i].x * b[i].x + a[i].y * b[i].y;
        dest[i].y = a[i].y * b[i].x - a[i].x * b[i].y;
    }
}

float2 complexMultiply (const float2 a, const float2 b)
{
    float2 dest;
    dest.x = a.x * b.x - a.y * b.y;
    dest.y = a.x * b.y + a.y * b.x;
    return dest;
}

float2 complexConjMultiply (const float2 a, const float2 b)
{
    float2 dest;
    dest.x = a.x * b.x + a.y * b.y;
    dest.y = a.y * b.x - a.x * b.y;
    return dest;
}

)"
