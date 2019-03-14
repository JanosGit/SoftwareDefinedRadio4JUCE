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

#include <ntlab_software_defined_radio/HardwareDevices/MCVFileEngine/MCVFileEngine.h>
#include "CovarianceMatrix.h"

#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS

class CovarianceMatrixUnitTests : public juce::UnitTest
{
public:
    CovarianceMatrixUnitTests() : juce::UnitTest ("CovarianceMatrix test"), covarianceMatrix (150000, numChannels) {}

    void runTest() override
    {
        auto random = getRandom();
        float phaseShifts[numChannels];

        beginTest ("Create Oscillator instance");

        ntlab::Oscillator oscillator (numChannels);
        oscillator.setSampleRate (1e6);
        oscillator.setFrequencyHz (0.5e6, ntlab::SDRIOEngine::allChannels);
        for (int chan = 0; chan < numChannels; ++chan)
        {
            phaseShifts[chan] = juce::jmap (random.nextFloat(), 0.0f, juce::MathConstants<float>::twoPi);
            oscillator.setPhaseShift (phaseShifts[chan], chan);
        }

        beginTest ("Assign covariance matrix callback");

        int numCallbacks = 0;
        covarianceMatrix.matrixReadyCallback = [&phaseShifts, &numCallbacks, this] (Eigen::MatrixXcf mat)
        {
            for (int chan = 0; chan < numChannels; ++chan)
                expect (mat(chan, chan).imag() == 0.0f);

            for (int chan = 1; chan < numChannels; ++chan)
            {
                const float maxDifference = 0.00017f;

                float phaseShiftDiff = phaseShifts[0] - phaseShifts[chan] - std::arg(mat(0, chan));
                while (phaseShiftDiff > (juce::MathConstants<float>::twoPi - maxDifference))
                    phaseShiftDiff -= juce::MathConstants<float>::twoPi;
                while (phaseShiftDiff < (-juce::MathConstants<float>::twoPi + maxDifference))
                    phaseShiftDiff += juce::MathConstants<float>::twoPi;

                expectLessOrEqual (phaseShiftDiff, maxDifference);
            }
            ++numCallbacks;
        };

        beginTest ("Compute covariance matrix");

        ntlab::SampleBufferComplex<float> inputBuffer (numChannels, 1004);
        while (numCallbacks < 5)
        {
            oscillator.fillNextSampleBuffer (inputBuffer);
            covarianceMatrix.processNextSampleBlock (inputBuffer);
        }
    }

private:
    ntlab::CovarianceMatrix<float> covarianceMatrix;

    static const int numChannels = 5;
};

static CovarianceMatrixUnitTests covarianceMatrixUnitTests;

#endif