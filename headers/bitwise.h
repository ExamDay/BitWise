#include <bitset>
#include <cassert>
#include <fstream>
#include <iostream>

using namespace std;

#define usint unsigned short int
#define ullint unsigned long long int
#define byte unsigned char

namespace bitwise {
bool readBit(int selector, byte data);
int readFirst(int n, byte data);
int readLast(int n, byte data);
size_t decodedLength(byte *pattern, int patternLength, int data_length);
size_t encodedLength(byte *pattern, int patternLength, int data_length);
usint *bitDecode_usint(byte *pattern, size_t patternLength, byte *data, size_t data_length);
byte *bitEncode(byte *pattern, size_t patternLength, usint *data, size_t data_length);
class ibitStream;
class obitStream;
} // namespace bitwise

bool bitwise::readBit(int n, byte data) {
    /**
     * get value of n'th bit indexed from the left. (most to least significant)
     */
    assert(0 <= n && n <= 7);
    byte sMask = 1;
    sMask = sMask << (7 - n);
    byte result = data & sMask;
    return (bool)result;
}

int bitwise::readFirst(int n, byte data) {
    /**
     * get integer value of the first n bits (indexed from the left) of a byte interpreted in
     * isolation. (eg: after they have been right justified and padded with zeroes on the left).
     */
    assert(0 <= n && n <= 7);
    data = data >> (7 - n);
    // bitset<8> x(data);
    // cout << "data in binary: " << x << " (last 8 bits)" << endl;
    return (int)data;
}

int bitwise::readLast(int n, byte data) {
    /**
     * get integer value of the last n bits (indexed from the right) of a byte interpreted in
     * isolation. (eg: after every bit to the left of the selection has been unset).
     */
    assert(0 <= n && n <= 7);
    byte sMask = 255;
    sMask = sMask >> (7 - n);
    data = data & sMask;
    // bitset<8> x(data);
    // cout << "data in binary: " << x << " (last 8 bits)" << endl;
    return (int)data;
}

size_t bitwise::decodedLength(byte *pattern, int patternLength, int data_length) {
    /**
     * find how many tiles (bitstrings with sub-byte length precision) of particular bitcounts
     * as declared in the pattern array can be laid in sequence into an array of unsigned chars of
     * byte-length "data-length".
     */
    usint patternSum = 0;
    for (usint i = 0; i < patternLength; i++) {
        patternSum += pattern[i];
    };
    // find how many full patterns fit into data array.
    int fullPatterns = (data_length * 8) / patternSum;
    // find how many bits are left in data after all full patterns have been read.
    usint tailBits = (data_length * 8) % patternSum;
    // find number of tiles in pattern that can fit into tail bits.
    usint tailTiles = 0;
    usint bitsLeft = tailBits;
    for (usint i = 0; i < tailBits; i++) {
        if (pattern[i] > bitsLeft) {
            break;
        } else {
            tailTiles += 1;
            bitsLeft -= pattern[i];
        };
    };
    // return total number of tiles of pattern that fit into a byte-array of data_length.
    return (fullPatterns * patternLength) + tailTiles;
}

usint *bitwise::bitDecode_usint(byte *pattern, size_t patternLength, byte *data,
                                size_t data_length) {
    // step through bytes of data and extract tiles from these bytes with bit-level precision
    size_t totalTiles = decodedLength(pattern, patternLength, data_length);
    usint tile;
    usint leftover = 0;
    int readHead = 0;
    usint datum = 0;
    usint *decoded = new usint[totalTiles];
    usint bits;
    byte buffer;
    byte buflen = 8; // buflen reflects how man bits of a byte we are interested in.
    for (int i = 0; i < totalTiles; i++) {
        tile = pattern[i % patternLength];
        datum = 0;
        bits = tile;
        for (readHead; bits > 0; readHead++) {
            buffer = data[readHead];
            buflen = 8;
            if (leftover > 0) { // eliminate all but the leftovers from the buffer
                buffer = buffer << (8 - leftover); // unset all bits left of leftover
                buffer = buffer >> (8 - leftover);
                buflen = leftover; // only intrested in the bits that are leftunchanged.
                leftover = 0;
            };
            if (bits < buflen) {
                leftover = buflen - bits;
                readHead -= 1;
                datum = datum << bits;
                datum = datum | ((usint)buffer >> leftover);
                bits -= bits; // signal that all bits of tile have been read
            } else {
                datum = datum << buflen;
                datum = datum | (usint)buffer;
                bits -= buflen;
            };
        };
        decoded[i] = datum;
    };
    return decoded;
}

size_t bitwise::encodedLength(byte *pattern, int patternLength, int data_length) {
    /**
     * find how many bytes the output of bitEncode will contain when given the same information.
     */
    // find how many elements are left in data after the maximum number of full patterns have been
    // encoded.
    usint tailElements = data_length % patternLength;
    // cout << "tailElements: " << tailElements << endl;
    // find how many bits are in a complete pattern of encoded data.
    usint patternSum = 0;
    for (usint i = 0; i < patternLength; i++) {
        patternSum += pattern[i];
    };
    // cout << "patternSum: " << patternSum << endl;
    // find number of bits in encoded tailElements.
    int tailBits = 0;
    for (int i = 0; i < tailElements; i++) {
        tailBits += pattern[i];
    }
    // cout << "tailBits: " << tailBits << endl;
    // find how many full patterns fit into data array.
    size_t fullPatterns = data_length / patternLength;
    // return total number of bytes that the output of encode will contain when given the
    // same information.
    // cout << "fullPatterns: " << fullPatterns << endl;
    size_t totalBits = (fullPatterns * patternSum) + tailBits;
    // cout << "totalBits: " << totalBits << endl;
    size_t totalBytes = (totalBits / 8) + (totalBits % 8 != 0); // count bytes and round up.
    // cout << "totalBytes: " << totalBytes << endl;
    return totalBytes;
}

byte *bitwise::bitEncode(byte *pattern, size_t patternLength, usint *data, size_t data_length) {
    size_t totalBytes = encodedLength(pattern, patternLength, data_length);
    byte *encoded = new byte[totalBytes];
    usint tile;
    byte surplus = 8;
    byte datum = 0;
    usint tBitsLeft;
    usint buffer;
    int writeHead = 0;
    for (int i = 0; i < data_length; i++) {
        tile = pattern[i % patternLength];
        tBitsLeft = tile;
        while (tBitsLeft > 0) {
            buffer = data[i];                    // get usint value of current data to be encoded.
            buffer = buffer << (16 - tBitsLeft); // push off all bits to left of interest.
            buffer = buffer >> (16 - surplus);   // position remaining bits for merge with datum.
            datum = datum | (byte)buffer;        // merge buffer into datum.
            if (tBitsLeft < surplus) {
                // this means case like:
                // surplus = 5
                // tBitsLeft = 3
                //  datum = [XXX_,____] <-- then we are given a datum like this
                // buffer = [____,_XXX][XXXX,XXXX] <-- and a buffer looking like this
                // buffer = [XXX_,____][____,____] <-- we left shift to truncate the buffer
                // buffer = [____,____][___X,XX__] <-- and right shift to position like so
                // datum = [XXXX,XX__] <-- then merge buffer with datum and continue
                surplus -= tBitsLeft;
                tBitsLeft = 0;
            } else if (tBitsLeft >= surplus) {
                // this means case like:
                // surplus = 3
                // tBitsLeft = 7
                //  datum = [XXXX,X___] <-- then we are given a datum like this
                // buffer = [____,_XXX][XXXX,XXXX] <-- and a buffer looking like this
                // buffer = [XXXX,XXX_][____,____] <-- we left shift to truncate the buffer
                // buffer = [____,____][____,_XXX] <-- and right shift to position like so
                // datum = [xxxx,xxxx] <-- then merge buffer with datum and continue
                // ^ in this case the datum will be full when we are done and written
                // to the output buffer "encoded".
                tBitsLeft -= surplus;
                surplus = 0;
            };
            if (surplus == 0) {
                encoded[writeHead] = datum;
                // cout << (int)datum << " ";
                datum = 0;
                surplus = 8;
                writeHead++;
            };
        };
    };
    encoded[writeHead] = datum;
    // cout << endl;
    return encoded;
};

class bitwise::ibitStream {
  private:
    fstream f;
    ios::openmode mode = ios::in | ios::binary;
    byte *inputData = new byte[1];
    size_t fileSize = 0;

  public:
    size_t outLength = 0;
    ibitStream(string filename) { f.open(filename, mode); }
    ~ibitStream() {
        delete[] inputData;
        f.close();
    };

    void close() { ibitStream::~ibitStream(); }

    void seek(size_t n, ios_base::seekdir sd) { f.seekg(n, sd); }

    size_t tell() { return f.tellg(); }

    usint *read(streamsize n, byte *pattern, int patternLength) {
        /*
         * Buffers n bytes from the stream, decodes them according to the bitdepth pattern specified
         * in pattern, and returns the decoded data as an array of unsigned short integers.
         * pattern is an array of single byte long integers that specify the number of bits to read
         * for each element.
         *
         * -- BEWARE: see limitations of bitwise::bitDecode functions.
         *
         * pattern is always an array of bytes because this array could be very long (potentially
         * longer than the number of bytes in the file being read) so space efficiency is important.
         * Furthermore, there is no c++ datastructure requiring more than 255 bits per value that
         * s could be an array of, so one byte per element is more than sufficient.
         * (long doubles are only 96 bits each).
         * Pattern is circularly indexed, so this continues from the first element in the pattern
         * if end of pattern is reached mid-encoding.
         */
        delete[] inputData;
        outLength = decodedLength(pattern, patternLength, n);
        inputData = new byte[n];
        f.read((char *)inputData, n);
        return bitDecode_usint(pattern, patternLength, inputData, n);
    }

    void readTo(char *s, streamsize n, byte *pattern, int patternLength) {
        /*
         * Like ibitStream.read() but stores the decoded data in the array pointed to by s.
         * Like the standard fstream.read(), s need not be a char array but it must be passed as
         * one as it will be filled byte-by-byte regardless of underlying datastructure.
         * NOTE: the size of s required to store the decoded data will almost certainly NOT be n!
         * n is simply the number of bytes to buffer from the file for decoding, but because we are
         * reading that buffered data bitwise it is possible to return many data points from each
         * individual byte. To find the required length of s use bitwise::decodedLength.
         */
        delete[] inputData;
        outLength = decodedLength(pattern, patternLength, n);
        inputData = new byte[n];
        f.read((char *)inputData, n);
        s = bitDecode_usint(pattern, patternLength, inputData, n);
    }
    size_t getFileSize() {
        // get number of bytes in file:
        if
            !(fileSize) {
                f.seekg(0, ios::end);
                fileSize = f.tellg();
                f.seekg(0, ios::beg);
            }
        return fileSize;
    }
};

class bitwise::obitStream {
  private:
    fstream f;
    ios::openmode mode = ios::out | ios::binary;
    size_t fileSize = 0;

  public:
    size_t outLength = 0;
    obitStream(string filename) { f.open(filename, mode); }
    ~obitStream() { f.close(); }

    void close() { obitStream::~obitStream(); }

    void seek(size_t n, ios_base::seekdir sd) { f.seekg(n, sd); }

    size_t tell() { return f.tellg(); }

    void write(usint *data, size_t dataLength, byte *pattern, int patternLength) {
        /*
         * Encoded data according to bitdepths in pattern and writes the encoding to a file.
         *
         * -- BEWARE: see limitations of bitwise::bitEncode function.
         *
         */
        outLength = encodedLength(pattern, patternLength, dataLength);
        f.write((char *)bitEncode(pattern, patternLength, data, dataLength), outLength);
    }

    size_t getFileSize() {
        /* get number of bytes in file */
        if
            !(fileSize) {
                f.seekg(0, ios::end);
                fileSize = f.tellg();
                f.seekg(0, ios::beg);
            }
        return fileSize;
    }
};
