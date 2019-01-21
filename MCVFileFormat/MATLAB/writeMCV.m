function writeMCV(fileName, matrixToWrite, desiredPrecision)
    %writeMCV: Writes a matrix as MCV file

    if nargin == 2
        desiredPrecision = 'double';
    end
    
    if isreal(matrixToWrite)
        isComplex = 0;
    else
        isComplex = 1;
    end
    
    switch desiredPrecision
        case {'float', 'float32', 'single'}
            isDouble = 0;
        case {'double', 'float64'}
            isDouble = 1;
        otherwise
            error ('Invalid precision type')
    end
    
    fileHandle = fopen(fileName, 'w+');

    if fileHandle == -1
        warning(['File ' fileName ' could not be created'])
        return
    end
    
    fwrite(fileHandle, 'NTLABMC', '*char');
    fwrite(fileHandle, isComplex, 'ubit1');
    fwrite(fileHandle, isDouble,  'ubit1');
    
    [numCols, numRows] = size(matrixToWrite);
    fwrite(fileHandle, numCols, 'int64');
    fwrite(fileHandle, numRows, 'int64');
    
    if isComplex
        matrixToWriteInterleaved = zeros(2 * numCols, numRows);
        matrixToWriteInterleaved(1:2:end, :) = real(matrixToWrite);
        matrixToWriteInterleaved(2:2:end, :) = imag(matrixToWrite);
        fwrite(fileHandle, matrixToWriteInterleaved, desiredPrecision);
    else
        fwrite(fileHandle, matrixToWrite, desiredPrecision);
    end
   
    fclose(fileHandle);
end