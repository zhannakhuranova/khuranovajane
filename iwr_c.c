#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <math.h>

//---------------------first packege-------------------------------------

unsigned char buffer_n[1];
int byteVec[32768];
int byteBuffer[32768];

int idX = 0;

int byteBufferLength = 0;
unsigned long int word[4] = {1, 256, 65563, 16777216};

//---------------------second packege-------------------------------------
int BYTE_VEC_ACC_MAX_SIZE = 32768; //  pow(2,15) = 32768
int MMWDEMO_UART_MSG_DETECTED_POINTS = 1;
int MMWDEMO_UART_MSG_RANGE_PROFILE = 2;
int maxBufferSize = 32768; // pow(2,15)]
int OBJ_STRUCT_SIZE_BYTES = 12;
int magicOK = 0; // Checks if magic number has been read
int dataOK = 0;  // Checks if the data has been read correctly
int frameNumber = 0;
int tlv_type = 0;

char magic[] = {2, 1, 4, 3, 6, 5, 8, 7};
int wordsecond[] = {1, 256}; // pow(2, 8)}

int byteCount = 4000;

unsigned long int totalPacketLen;
unsigned long int magicNumber[8];
unsigned long int version, platform, timeCpuCycles, numDetectedObj, numTLVs, subFrameNumber;

//---------------------third packege-------------------------------------

float *rangeIdxs;  // unsigned long int
float *dopplerIdx; // np.zeros(tlv_numObj,dtype = 'int16')
float *peakVal;
float *x;
float *y;
float *z;

unsigned long int tlv_xyzQFormat, tlv_numObj;
int shiftSize;

FILE *inFile;
int state = 0;
int save_flag = 1;

unsigned char readbuf; // Buffer used to read data.
int readbuflen = 0;    // Number of bytes to read.
int readbytes = 0;     // Final number of bytes successfully read by all the calls of ReadFile().
int nbbytes = 0;       // Number of bytes successfully read by each call of ReadFile().

HANDLE hComm2;
//--------------------------main-------------------------------------

int main()
{
    unsigned char stopWord[11] = {'s', 'e', 'n', 's', 'o', 'r', 'S', 't', 'o', 'p', '\n'};

    long unsigned int bt, bt2;
    char data[3];
    char dataR[3];
    HANDLE hComm1;

    hComm1 = CreateFileA("\\\\.\\COM3",                // port name
                         GENERIC_READ | GENERIC_WRITE, // Read/Write
                         0,                            // No Sharing
                         NULL,                         // No Security
                         OPEN_EXISTING,                // Open existing port only
                         0,                            // Non Overlapped I/O
                         NULL);                        // Null for Comm Devices

    if (hComm1 == INVALID_HANDLE_VALUE)
        printf("Error in opening serial port");
    else
        printf("opening serial port successful");

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hComm1, &dcbSerialParams))
    {
        // could not get the state of the comport
    }
    printf("\nbaud rate = %d\n", dcbSerialParams.BaudRate);

    dcbSerialParams.BaudRate = 115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hComm1, &dcbSerialParams))
    {
        // analyse error
        printf("Error 2\n");
    }

    hComm2 = CreateFileA("\\\\.\\COM4",                // port name
                         GENERIC_READ | GENERIC_WRITE, // Read/Write
                         0,                            // No Sharing
                         NULL,                         // No Security
                         OPEN_EXISTING,                // Open existing port only
                         0,                            // Non Overlapped I/O
                         NULL);                        // Null for Comm Devices

    if (hComm2 == INVALID_HANDLE_VALUE)
        printf("Error in opening serial port");
    else
        printf("opening serial port successful");
    DCB dcbSerialParams2 = {0};
    dcbSerialParams2.DCBlength = sizeof(dcbSerialParams2);
    if (!GetCommState(hComm2, &dcbSerialParams2))
    {
        return 1;
        // could not get the state of the comport
    }
    printf("\nbaud rate = %d\n", dcbSerialParams2.BaudRate);
    printf("\nbaud rate = %d\n", dcbSerialParams2.fRtsControl);

    dcbSerialParams2.BaudRate = 921600;
    dcbSerialParams2.ByteSize = 8;
    dcbSerialParams2.StopBits = ONESTOPBIT;
    dcbSerialParams2.Parity = NOPARITY;
    if (!SetCommState(hComm2, &dcbSerialParams2))
    {
        // analyse error
        printf("Error 2\n");
    }

    Sleep(21);
    WriteFile(hComm1,   // Handle to the Serial port
              stopWord, // Data to be written to the port
              11,       // No of bytes to write
              &bt,      // Bytes written
              NULL);
    Sleep(21);

    PurgeComm(hComm2, PURGE_RXCLEAR);

    Sleep(500);
    PurgeComm(hComm2, PURGE_RXCLEAR);
    unsigned char buff[100];
    unsigned char readbuff[1];
    unsigned char *buffer;
    int counter = 0;
    char string[100];
    char c;

    DWORD read;
    FILE *file;
    file = fopen("1.cfg", "r");

    if (file)
    {

        while (((c = getc(file)) != EOF))
        {

            buff[counter] = c;

            counter++;
            if (c == '\n')
            {

                buffer = (unsigned char *)malloc(counter * sizeof(unsigned char));
                for (int i = 0; i < counter; i++)
                {
                    buffer[i] = buff[i];
                }
                Sleep(1);
                WriteFile(hComm1,  // Handle to the Serial port
                          buffer,  // Data to be written to the port
                          counter, // No of bytes to write
                          &bt,     // Bytes written
                          NULL);

                // printf("%s", buffer);
                free(buffer);

                counter = 0;
            }
        }
        printf("Config transfered!\n");
        fclose(file);
    }

    // Now let's read some bytes from the port

    readbuflen = 10; // We expect for examle 10 bytes
    readbytes = 0;
    nbbytes = 0;
    char buffer2[255];
    unsigned char final_buffer[10000];
    // Here we read byte by byte and we end the loop when we receive a 0 caracter

    int counter1 = 0;
    while (1)
    {
        counter1++;
        printf("\nIteration num = %d\n ", counter1);
        if (readAndParseData16xx() == -1)
        {
            printf("\nEOF\n ");

            printf("\nIteration num = %d\n ", counter1);
            break;
        }

        printf("\n --------------- New circle is ---------------\n ");
    } // read until the a specian caracter arrives
    printf("\nDone\n");

    CloseHandle(hComm1); // Closing the Serial Port
    CloseHandle(hComm2); // Closing the Serial Port
    return 0;
}

int readAndParseData16xx()
{

    if ((state == 0) && (save_flag == 1))
    {

        for (size_t i = 0; i <= byteCount; i++)

        {

            if (ReadFile(hComm2, &readbuf, 1, (LPDWORD)&nbbytes, NULL)) // 1 is the number
                                                                        // of bytes that we read
            {
                buffer_n[0] = readbuf;
                if (nbbytes == 0)
                {
                    state = 1;
                    break;
                    printf("ReadFile() timed out\n");
                    return -1;
                }
                buffer_n[0] = readbuf;
                byteBuffer[i] = readbuf;
                byteVec[i] = readbuf;

                readbytes += nbbytes; // here nbbytes is alwayes equal to 1
                // printf("%X ", byteBuffer[i]); // print the read caracters on the screen
            }
            else
            {
                printf("\nReaded bytes %d\n", readbytes);
                printf("ReadFile() failed\n");
                return -1;
            }
        }
    }

    if (((byteBufferLength + byteCount) < maxBufferSize) && (state == 0))
    {
        for (int i = 0; i < byteCount; i++)
        {
            byteBuffer[i + byteBufferLength] = byteVec[i];
            // printf(" byteVec is %X ", byteVec[i]);
        }
        byteBufferLength = byteBufferLength + byteCount;
        save_flag = 1;
    }
    else
    {
        save_flag = 0;
    }
    printf("\n ");
    printf("byteBufferLength + byteCount < maxBufferSize , byteBufferLength  is - %i \n", byteBufferLength);
    //индексы magicword пишем в startIdx
    int startIdx[10000];
    int lengtstartIdx = 0;
    if (byteBufferLength > 16)
    {
        for (int i = 0; i < byteBufferLength - 8; i++)
        {
            if ((byteBuffer[i + 0] == 2) && (byteBuffer[i + 1] == 1) && (byteBuffer[i + 2] == 4) && (byteBuffer[i + 3] == 3) && (byteBuffer[i + 4] == 6) && (byteBuffer[i + 5] == 5) && (byteBuffer[i + 6] == 8) && (byteBuffer[i + 7] == 7))
            {
                startIdx[lengtstartIdx] = i;

                printf("startIdx is %i \n", startIdx[lengtstartIdx]);
                lengtstartIdx += 1;
            }
        }
    }

    if ((startIdx[0] > 0) && (startIdx[0] < byteBufferLength))
    {
        int cut1 = byteBufferLength - startIdx[0];
        for (int i = 0, j = startIdx[0]; j < byteBufferLength; i++, j++)
        {
            byteBuffer[i] = byteBuffer[j];
        }
        for (int i = cut1; i < byteBufferLength; i++)
        {
            byteBuffer[i] = 0;
        }
        byteBufferLength = byteBufferLength - startIdx[0];
    }

    // printf("startIdx[0] > 0 && (startIdx[0] < byteBufferLength , byteBufferLength  is - %i \n", byteBufferLength);
    if (byteBufferLength < 0)
    {
        byteBufferLength = 0;
    }

    totalPacketLen = word[0] * byteBuffer[12] + word[1] * byteBuffer[12 + 1] + word[2] * byteBuffer[12 + 2] + word[3] * byteBuffer[12 + 3]; //може 11 попробувати

    if ((byteBufferLength >= totalPacketLen) && (byteBufferLength != 0)) // Check that all the packet has been read
    {
        magicOK = 1;
    }
    else
    {
        return 1;
    }

    if (magicOK > 0)
    {
        idX = 0;
        // word array to convert 4 bytes to a 32 bit number

        for (int i = 0; i <= 8; i++)
        {
            magicNumber[i] = byteBuffer[i]; // malloc?
        }
        idX = idX + 8;
        /*version and platform convert to 16 in python*/
        version = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;
        totalPacketLen = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;
        platform = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3]; // x - Число в шестнадцатеричной системе счисления (буквы в нижнем регистре).
        idX = idX + 4;
        frameNumber = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;
        timeCpuCycles = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;
        numDetectedObj = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;
        numTLVs = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;
        subFrameNumber = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
        idX = idX + 4;

        // printf("numTLVs is %i", numTLVs);

        for (int tlvIdx = 0; tlvIdx <= numTLVs; tlvIdx++)
        {
            tlv_type = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1] + word[2] * byteBuffer[idX + 2] + word[3] * byteBuffer[idX + 3];
            idX += 8;

            if (tlv_type == MMWDEMO_UART_MSG_DETECTED_POINTS)
            {

                tlv_numObj = 1 * byteBuffer[idX] + 256 * byteBuffer[idX + 1]; // множим множим , конвертим 4 байта в 16

                idX += 2;
                tlv_xyzQFormat = pow(2, word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1]);
                idX += 2;

                // Initialize the arrays

                rangeIdxs = (float *)malloc(tlv_numObj * sizeof(float));
                dopplerIdx = (float *)malloc(tlv_numObj * sizeof(float));
                peakVal = (float *)malloc(tlv_numObj * sizeof(float));
                x = (float *)malloc(tlv_numObj * sizeof(float));
                y = (float *)malloc(tlv_numObj * sizeof(float));
                z = (float *)malloc(tlv_numObj * sizeof(float));
                printf("\n\nNum obj  %d \n", tlv_numObj);
                // tlv_numObj = ?
                for (int i = 0; i < tlv_numObj; i++)
                {

                    //откуда начинается idX ?
                    // int idX=5;
                    rangeIdxs[i] = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1];
                    idX += 2;
                    dopplerIdx[i] = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1];
                    // np.matmul(byteBuffer[idX:idX+2],word)
                    idX += 2;
                    peakVal[i] = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1];
                    idX += 2;
                    x[i] = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1];
                    idX += 2;
                    y[i] = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1];
                    idX += 2;
                    z[i] = word[0] * byteBuffer[idX] + word[1] * byteBuffer[idX + 1];
                    idX += 2;
                    // printf("%f ", peakVal[i]);
                    //  x[i] = x[i] / tlv_xyzQFormat;
                    //  y[i] = y[i] / tlv_xyzQFormat;
                    //  z[i] = z[i] / tlv_xyzQFormat;
                    //  printf(" [y] is %f ", y[i]);
                }
                // printf("\n Idx=%d\n", idX);

                // Make the necessary corrections and calculate the rest of the data
                // rangeVal = rangeIdx * configParameters["rangeIdxToMeters"]
                //  dopplerIdx[dopplerIdx > (configParameters["numDopplerBins"]/2 - 1)]  = dopplerIdx[dopplerIdx > (configParameters["numDopplerBins"]/2 - 1)] - 65535
                // dopplerVal = dopplerIdx *  configParameters["dopplerResolutionMps"]
            }
        }

        if ((idX > 0) && (byteBufferLength > idX))
        {
            shiftSize = totalPacketLen;

            int cut2 = byteBufferLength - shiftSize;
            for (int i = 0, j = shiftSize; j < byteBufferLength; i++, j++)
            {
                byteBuffer[i] = byteBuffer[j];
                // printf("\n ololoshka, byteBuffer is %i", byteBuffer[i]); //----------------------------------
            }
            for (int i = cut2; i < byteBufferLength; i++)
            {
                byteBuffer[i] = 0;
            }
            byteBufferLength = byteBufferLength - shiftSize;

            // Check that there are no errors with the buffer length
            if (byteBufferLength < 0)
            {
                byteBufferLength = 0;
            }
        }
    }
    return 0;
}