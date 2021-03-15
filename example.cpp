#include "headers/bitwise.h"
#include <bitset>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <unistd.h>

using namespace std;
using namespace bitwise;

#define usint unsigned short int
#define ullint unsigned long long int
#define byte unsigned char

int main(int argc, char **argv) {
    int iflag = 0;
    int oflag = 0;
    string ivalue = "infileExample.bin";
    string ovalue = "outfileExample.bin";
    int index;
    int c;

    opterr = 0;

    while ((c = getopt(argc, argv, ":i:o:")) != -1)
        switch (c) {
        case 'i':
            ivalue = optarg;
            iflag = 1;
        case 'o':
            ovalue = optarg;
            oflag = 1;
            break;
        case '?':
            if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            return 1;
        default:
            abort();
        };
    cout << "\nBitwise Reading and Writing Demonstration:\n";
    cout << "infile: " << ivalue << endl;
    cout << "outfile: " << ovalue << endl;
    // open in file for reading:
    ifstream infile(ivalue, ios::in | ios::binary);
    // get bytes in file:
    infile.seekg(0, ios::end);
    size_t inputSize = infile.tellg();
    infile.seekg(0, ios::beg); // seek to beginning of file to begin reading data levels.

    cout << "inputSize: " << inputSize << endl;
    // display infile contents
    byte inputData[inputSize];
    infile.read((char *)inputData, inputSize);
    cout << "\ninfile contents: " << endl;
    for (int i = 0; i < inputSize; i++) {
        cout << inputData[i] << " ";
    };
    cout << endl;
    infile.close();

    // extract bit-level information from file:
    // bitdepths of each datum must be known before extraction. For example:
    // Consider an array of values: [ 1,   87   ,  16  ,       50256     ,  2 ]
    //                or in binary: [ 1, 1010111, 10000, 1100010001010000, 10 ]
    //       this has bitdepths of: [ 1,    7   ,   5  ,         16      ,  2 ] <= We need this
    // which means we will read 1 bit, then 7 bits, then 5 bits, and so on.
    // and we would find it encoded as a string of bytes padded with zeroes on the right like:
    // { 11010111, 10000111, 10100001, 00100000, 00100000 }

    // decode data
    byte bitDepths[] = {3, 2, 9};
    ibitStream ibs(ivalue);
    cout << "reading..." << endl;
    usint* decodedData = ibs.read(ibs.fileSize, bitDepths, sizeof(bitDepths));
    cout << "filesize: " << ibs.getFileSize() << endl;
    size_t decLength = ibs.outLength;
    cout << "decLength: " << decLength << endl;
    cout << "\ndecoded contents: " << endl;
    for (int i = 0; i < decLength; i++) {
        cout << (usint)decodedData[i] << " ";
    };
    cout << endl;
    ibs.close();

    // write encoded data:
    cout << "writing encoded data to outfile..." << endl;
    // initialize array for encoded bytes:
    usint outData[] = {7, 3, 483, 7, 2, 510};
    size_t dataLength = sizeof(outData) / sizeof(usint);
    // encode data and write to outfile:
    obitStream obs(ovalue);
    cout << "outData: " << endl;
    for (int i = 0; i < dataLength; i++) {
        cout << outData[i] << " ";
    }
    cout << endl;
    obs.write(outData, dataLength, bitDepths, sizeof(bitDepths));
    obs.close();
    cout << "outLength: " << obs.outLength  << " bytes" << endl;
    cout << "output: " << bitEncode(bitDepths, sizeof(bitDepths), outData, dataLength) << endl;
    cout << "\nSuccess!\n" << endl;
};
