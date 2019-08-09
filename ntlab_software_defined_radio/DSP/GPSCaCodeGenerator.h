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

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <algorithm>

namespace ntlab
{
    /**
     * A simple class to generate GPS CA Codes. Inspired by this implementation:
     * https://www.beechwood.eu/ca-code-gps-generator/
     *
     * The output will always be all 1023 values of the code in a bipolar fashion, e.g. the binary values are
     * represented as 1 and -1 so that it can be used directly for a BPSK modulation
     */
    class GPSCaCodeGenerator
    {
    public:
        /** Make sure the array you want to fill can hold 1023 values */
        static const int destArraySize = 1023;

        /**
         * Fills destArray with the desired code. caCodeIdx can be any value from 0 to 36, if you pass in a value out of
         * those bounds fals will be returned, true otherwise. Make sure the array you want to fill can hold 1023 values.
         */
        template <typename T>
        bool computeCaCode (T* destArray, const int caCodeIdx)
        {
            if (!init (caCodeIdx))
                return false;

            for (int i = 0; i < destArraySize; ++i)
            {
                int8_t g1 = updateG1();
                int8_t g2 = updateG2();
                destArray[i] = ((g1 + g2) % 2) ? T(1) : T(-1);
            }

            return true;
        }

        /**
         * Fills destArray with the desired code. caCodeIdx can be any value from 0 to 36, if you pass in a value out of
         * those bounds fals will be returned, true otherwise.
         */
        template <typename T>
        bool computeCaCode (std::array<T, destArraySize>& destArray, const int caCodeIdx)
        {
            return computeCaCode (destArray.data(), caCodeIdx);
        }

    private:
        static const int gArraySize = 10;
        std::array<int8_t, gArraySize> g1, g2;

        int phase1 = -1;
        int phase2 = -1;

        int8_t updateG1()
        {
            int8_t newVal = (g1[2] + g1[9]) % 2;
            int8_t ret = g1[9];

            // Backup
            std::array<int8_t, gArraySize> temp = g1;

            //Update
            g1[0] = newVal;
            for(int i  = 1; i < gArraySize; ++i)
                g1[i] = temp[i-1];

            return ret;
        };

        int8_t updateG2()
        {
            int8_t newVal = (g2[1] + g2[2] + g2[5] + g2[7] + g2[8] + g2[9]) % 2;

            // Backup
            std::array<int8_t, gArraySize> temp = g2;

            //Update
            g2[0] = newVal;
            for(int i  = 1; i < gArraySize; ++i)
                g2[i] = temp[i-1];

            //From the Old Values
            return ((temp[phase1 - 1] + temp[phase2 - 1]) % 2);
        }

        bool init (int caCodeIdx)
        {
            switch (caCodeIdx){
                case 0:  phase1 = 2; phase2 = 6;  break;
                case 1:  phase1 = 3; phase2 = 7;  break;
                case 2:  phase1 = 4; phase2 = 8;  break;
                case 3:  phase1 = 5; phase2 = 9;  break;
                case 4:  phase1 = 1; phase2 = 9;  break;

                case 5:  phase1 = 5; phase2 = 9;  break;
                case 6:  phase1 = 1; phase2 = 8;  break;
                case 7:  phase1 = 2; phase2 = 9;  break;
                case 8:  phase1 = 3; phase2 = 10; break;
                case 9:  phase1 = 2; phase2 = 3;  break;

                case 10: phase1 = 3; phase2 = 4;  break;
                case 11: phase1 = 5; phase2 = 6;  break;
                case 12: phase1 = 6; phase2 = 7;  break;
                case 13: phase1 = 7; phase2 = 8;  break;
                case 14: phase1 = 8; phase2 = 9;  break;

                case 15: phase1 = 9; phase2 = 10; break;
                case 16: phase1 = 1; phase2 = 4;  break;
                case 17: phase1 = 2; phase2 = 5;  break;
                case 18: phase1 = 3; phase2 = 6;  break;
                case 19: phase1 = 4; phase2 = 7;  break;

                case 20: phase1 = 5; phase2 = 8;  break;
                case 21: phase1 = 6; phase2 = 9;  break;
                case 22: phase1 = 1; phase2 = 3;  break;
                case 23: phase1 = 4; phase2 = 6;  break;
                case 24: phase1 = 5; phase2 = 7;  break;

                case 25: phase1 = 6; phase2 = 8;  break;
                case 26: phase1 = 7; phase2 = 9;  break;
                case 27: phase1 = 8; phase2 = 10; break;
                case 28: phase1 = 1; phase2 = 6;  break;
                case 29: phase1 = 2; phase2 = 7;  break;

                case 30: phase1 = 3; phase2 = 8;  break;
                case 31: phase1 = 4; phase2 = 9;  break;
                case 32: phase1 = 5; phase2 = 10; break;
                case 33: phase1 = 4; phase2 = 10; break;
                case 34: phase1 = 1; phase2 = 7;  break;

                case 35: phase1 = 2; phase2 = 8;  break;
                case 36: phase1 = 4; phase2 = 10; break;

                // you passed in an invalid ca code idx
                default: jassertfalse; return false;
            }

            // init shift registers with ones
            std::fill (g1.begin(), g1.end(), 1);
            std::fill (g2.begin(), g2.end(), 1);

            return true;
        }
    };
}