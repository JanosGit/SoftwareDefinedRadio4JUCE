# MCV File format

The MCV file format is designed to exchange matrix-like complex valued floating point data. This can also be multichannel sample buffers, where the channels are represented as columns and the samples are the lines.

## Structure

| Byte Offset | Meaning                      | Content              |
| ----------- |------------------------------| ---------------------|
| 0           | Unique Identifier            | 'NTLABMC' (7 chars)  |
| 7           | Flags                        | Bit 0: 0 = real values, 1 = complex values <br> Bit 1: 0 = single precision, 1 = double precision <br> Bit 2 - 7: Reserved|
| 8           | Number of columns / Channels | signed long ( 8 Byte)|
| 16          | Number of rows / Samples     | signed long ( 8 Byte)|
| 24          | Payload                      | Col[0]-Line[0]; Col[1]-Line[0]; ... Col[0]-Line[1]; Col[1]-Line[1]; ... |

## MATLAB Support

In the MATLAB subfolder you will find MCV read- and write functions for MATLAB