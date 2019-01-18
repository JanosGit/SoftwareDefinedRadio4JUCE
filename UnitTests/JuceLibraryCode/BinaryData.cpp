/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

namespace BinaryData
{

//================== readMCV.m ==================
static const unsigned char temp_binary_data_0[] =
"function [fileContent, precision] = readMCV(fileName)\r\n"
"    %readMCV: Reads an MCV file. Returns the contained matrix and as the\r\n"
"    %number precision used in the file\r\n"
"    \r\n"
"    fileHandle = fopen(fileName);\r\n"
"    \r\n"
"    if fileHandle == -1\r\n"
"        warning(['File ' fileName ' could not be opened'])\r\n"
"        fileContent = [];\r\n"
"        return\r\n"
"    end\r\n"
"    \r\n"
"    identifier = fread(fileHandle, [1, 7], '*char');\r\n"
"    if strcmp(identifier, 'NTLABMC') ~= 1  \r\n"
"        warning(['File ' fileName ' is invalid'])\r\n"
"        fileContent = [];\r\n"
"        return\r\n"
"    end\r\n"
"    \r\n"
"    isComplex = fread(fileHandle, 1, 'ubit1');\r\n"
"    isDouble  = fread(fileHandle, 1, 'ubit1');\r\n"
"    \r\n"
"    if isDouble\r\n"
"        precision = 'double';\r\n"
"    else\r\n"
"        precision = 'float';\r\n"
"    end\r\n"
"    \r\n"
"    numCols = fread(fileHandle, 1, 'int64');\r\n"
"    numRows = fread(fileHandle, 1, 'int64');\r\n"
"    \r\n"
"    if isComplex\r\n"
"        fileContentInterleaved = fread(fileHandle, [numCols, numRows * 2], precision);\r\n"
"        fileContent = fileContentInterleaved(:, 1:2:end) + 1i * fileContentInterleaved(:, 2:2:end);\r\n"
"    else\r\n"
"        fileContent = fread(fileHandle, [numCols, numRows], precision);\r\n"
"    end\r\n"
"    \r\n"
"    fclose(fileHandle);\r\n"
"    \r\n"
"    if ~isequal(size(fileContent), [numCols numRows])\r\n"
"        error('Corrupted file')\r\n"
"    end\r\n"
"    \r\n"
"    \r\n"
"end\r\n"
"\r\n";

const char* readMCV_m = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;
    if (resourceNameUTF8 != 0)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0xbc8e0998:  numBytes = 1322; return readMCV_m;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "readMCV_m"
};

const char* originalFilenames[] =
{
    "readMCV.m"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
    {
        if (namedResourceList[i] == resourceNameUTF8)
            return originalFilenames[i];
    }

    return nullptr;
}

}
