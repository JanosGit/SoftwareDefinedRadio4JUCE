function [fileContent, precision] = readMCV(fileName)
    %readMCV: Reads an MCV file. Returns the contained matrix and as the
    %number precision used in the file
    
    fileHandle = fopen(fileName);
    
    if fileHandle == -1
        warning(['File ' fileName ' could not be opened'])
        fileContent = [];
        return
    end
    
    identifier = fread(fileHandle, [1, 7], '*char');
    if strcmp(identifier, 'NTLABMC') ~= 1  
        warning(['File ' fileName ' is invalid'])
        fileContent = [];
        return
    end
    
    isComplex = fread(fileHandle, 1, 'ubit1');
    isDouble  = fread(fileHandle, 1, 'ubit1');
    
    if isDouble
        precision = 'double';
    else
        precision = 'float';
    end
    
    numCols = fread(fileHandle, 1, 'int64');
    numRows = fread(fileHandle, 1, 'int64');
    
    if isComplex
        fileContentInterleaved = fread(fileHandle, [2 * numCols, numRows], precision);
        fileContent = fileContentInterleaved(1:2:end, :) + 1i * fileContentInterleaved(2:2:end, :);
    else
        fileContent = fread(fileHandle, [numCols, numRows], precision);
    end
    
    fclose(fileHandle);
    
    if ~isequal(size(fileContent), [numCols numRows])
        error('Corrupted file')
    end
    
    
end

