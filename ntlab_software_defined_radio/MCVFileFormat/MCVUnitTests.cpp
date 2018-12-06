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

#include <juce_core/juce_core.h>
#include "MCVWriter.h"
#include "MCVReader.h"
#include "../UnitTestHelpers/UnitTestHelpers.h"

#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS
class MCVUnitTests : public juce::UnitTest
{
public:
    MCVUnitTests() : juce::UnitTest ("MCV file tests") {};

    void runTest() override
    {
        auto tempFolder = juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory);
        auto realFloatFile  = tempFolder.getChildFile ("realFloat.mcv");
        auto realDoubleFile = tempFolder.getChildFile ("realDouble.mcv");
        auto cplxFloatFile  = tempFolder.getChildFile ("cplxFloat.mcv");
        auto cplxDoubleFile = tempFolder.getChildFile ("cplxDouble.mcv");

        const int numChannels = 3;
        const int numSamples = 57;

        ntlab::SampleBufferReal<float>     realFloatSrcBuffer  (numChannels, numSamples);
        ntlab::SampleBufferReal<double>    realDoubleSrcBuffer (numChannels, numSamples);
        ntlab::SampleBufferComplex<float>  cplxFloatSrcBuffer  (numChannels, numSamples);
        ntlab::SampleBufferComplex<double> cplxDoubleSrcBuffer (numChannels, numSamples);

        auto random = getRandom();

        ntlab::UnitTestHelpers::fillSampleBuffer (realFloatSrcBuffer,  random);
        ntlab::UnitTestHelpers::fillSampleBuffer (realDoubleSrcBuffer, random);
        ntlab::UnitTestHelpers::fillSampleBuffer (cplxFloatSrcBuffer,  random);
        ntlab::UnitTestHelpers::fillSampleBuffer (cplxDoubleSrcBuffer, random);

        beginTest ("Writing MCV Files");

        ntlab::MCVWriter::writeSampleBuffer (realFloatSrcBuffer,  realFloatFile);
        ntlab::MCVWriter::writeSampleBuffer (realDoubleSrcBuffer, realDoubleFile);
        ntlab::MCVWriter::writeSampleBuffer (cplxFloatSrcBuffer,  cplxFloatFile);
        ntlab::MCVWriter::writeSampleBuffer (cplxDoubleSrcBuffer, cplxDoubleFile);

        beginTest ("Read MCV Files with matching precision");

        ntlab::MCVReader realFloatReader (realFloatFile);
        expect (realFloatReader.isValid());

        auto realFloatBufferRead = realFloatReader.createSampleBufferRealFloat();
        expect (ntlab::UnitTestHelpers::areEqualSampleBuffers (realFloatSrcBuffer, realFloatBufferRead));

        ntlab::MCVReader realDoubleReader (realDoubleFile);
        expect (realDoubleReader.isValid());

        auto realDoubleBufferRead = realDoubleReader.createSampleBufferRealDouble();
        expect (ntlab::UnitTestHelpers::areEqualSampleBuffers (realDoubleSrcBuffer, realDoubleBufferRead));

        ntlab::MCVReader cplxFloatReader (cplxFloatFile);
        expect (cplxFloatReader.isValid());

        auto cplxFloatBufferRead = cplxFloatReader.createSampleBufferComplexFloat();
        expect (ntlab::UnitTestHelpers::areEqualSampleBuffers (cplxFloatSrcBuffer, cplxFloatBufferRead));

        ntlab::MCVReader cplxDoubleReader (cplxDoubleFile);
        expect (cplxFloatReader.isValid());

        auto cplxDoubleBufferRead = cplxDoubleReader.createSampleBufferComplexDouble();
        expect (ntlab::UnitTestHelpers::areEqualSampleBuffers (cplxDoubleSrcBuffer, cplxDoubleBufferRead));

        realFloatFile.deleteFile();
        realDoubleFile.deleteFile();
        cplxFloatFile.deleteFile();
        cplxDoubleFile.deleteFile();
    }
};

static MCVUnitTests mcvUnitTests;
#endif