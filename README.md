<h1 align="center">BitWise:<br>coding and file handling with bit-level precision</h1>

## Why BitWise?
Today's computers perform memory operations on a byte-by-byte basis.
This is a feature of RAM and storage architectures at the hardware level and programming languages
are written with this fact in mind. While it is very time-efficient for data to be addressed this way
, it also means that the smallest chunk of memory that can be read from, or written to, is an entire 8-bit chunk. This poses an obstacle to certain compression schemes and other programs that
demand bit-level precision. Because this limitation is baked in at the hardware level there is no
software solution for direct manipulation of individual bits. However, it is still possible to encode
information with bit-level precision (and thereby make maximum use of available storage/bandwidth) so
long as we are willing to make some small sacrifices.
## How it Works
Consider a number like 17, or in binary: 10001. This number is only 5 bits long, but
because of aforementioned hardware limitations it is padded to the left
with zeroes and stored as 00010001. In the case of a single number, this does not amount to
much wasted space and
is in fact the best we can do. However, when working with serial data this issue becomes much more
pertinent and much more tractible. In datasets composed primarily
of numbers that do not exceed some known bitdepth that is not itself a multiple of 8
(which is almost all of them) these padding bits quickly add up.

Consider the following array of integers:
```
[4, 5, 4, 5, 1, 0, 2, 3]
```

This would normally be stored as:

```
00000100, 00000101, 00000100, 00000101, 00000001, 00000000, 00000010, 00000011
```
Requiring 8 bytes. But if we happen to know that each element in the given array is at most 3 bits
long then we should be able to remember this maximum bitdepth and store the array simply as:

```
10010110, 01010010, 00010011
```
Requiring only three bytes. (padded to the <em>right</em> with zeores)

We can achieve this effect by leveraging fast logical operations to pre-form whole bytes of our
desired serial encoding, and writing
these engineered bytes to disk in keeping with hardware convention. We then reverse the process come
read time.

BitWise provides two primary tools. The first is an efficient bit-coder for packing/unpacking an
array of values to and from specified bitdepths. The second is an fstream-style, binary file handler
with coders built-in to facilitate bitwise read/write operations on storage.
## How to Compile
#### Linux
- Clone this repository, then compile the test program with:
```bash
cd /path/to/this/repository/
g++ example.cpp -o example.bin
```
- then run with:
```bash
./example.bin -i infileExample.bin -o outfileExample.bin
```
#### Windows
- Clone this repository, then compile the test program with:
```powershell
cd /path/to/this/repository/
cl example.cpp /out:example.exe
```
- then run with:
```powershell
example.exe -i infileExample.bin -o outfileExample.bin
```
#### Mac
- Clone this repository, then compile the test program with:
```zsh
cd /path/to/this/repository/
clang example.cpp -o example.bin
```
- then run with:
```bash
./example.bin -i infileExample.bin -o outfileExample.bin
```
## How to Use
### In-Memory Operations
#### Encoding
To encode an array of values to a particular bitdepth or pattern of bitdepths,
feed bitwise::bitEncoder the array of data to be encoded, and another array of single
byte long integers (unsigned chars) signifying the bitdepth at which to encode each element.

The pattern array is indexed circularly, so the coders will continue from the
first element in the pattern if the end of the pattern is reached mid-coding. A pattern element need
not be supplied for every datum to be coded, one will suffice for all of them, and repeating
patterns like "2, 3, 2, 3, 2, ..." can simply be entered as [2, 3].

Pattern is always an array of bytes because this array could be very long (potentially
longer than the number of bytes in the file being read) so space efficiency is important.
Furthermore, there is no standard c++ datastructure requiring more than 255 bits per value that
these coders could return, so one byte per element is more than sufficient.
(even long doubles are only 96 bits each)

For example:
```cpp
#include "headers/bitwise.h"
#include <iostream>

using namespace std;
using namespace bitwise;

unsigned short int data[] = {7, 3, 483, 7, 2, 510};  // data to be encoded
size_t dataLength = sizeof(data) / sizeof(unsigned short int);
unsigned char pattern[] = {3, 2, 9};  // pattern of bitdepths to encode to
size_t patternLength = sizeof(pattern);  // unsigned chars are one byte long
unsigned char encodedData[] = bitEncode(pattern, patternLength, data, dataLength);
```
- then for a quick sanity check you can do:
```cpp
cout << "data:" << data << endl;

cout << "encodedData: ";
size_t encoded_length = encodedLength(pattern, patternLength, dataLength);
// ^ encodedLength is a function in BitWise to calculate length of an array returned by bitEncode.
for (size_t i = 0; i < encoded_length; i++) {
    cout << encodedData[i] << " ";
};
cout << endl;
```
- to compute the length of the encodedData array just do:
```cpp
size_t encoded_length = encodedLength(pattern, patternLength, dataLength);
```
#### Decoding
Much like encoding data, to decode an array of encoded bytes to a particular bitdepth
or arbitrary pattern of bitdepths, feed bitwise::bitDecoder the array of data to be decoded
(passed as an array of bytes/unsigned chars), and another "pattern" array of single
byte long integers (unsigned chars) signifying the number of bits to read for each element of the
final decoded array.

For example:
```cpp
#include "headers/bitwise.h"
#include <iostream>

using namespace std;
using namespace bitwise;

// assuming encodedData and encoded_length are already present (say from the encoding code above)

unsigned char pattern[] = {3, 2, 9};  // pattern of bitdepths to decode by
size_t patternLength = sizeof(pattern); // unsigned chars are one byte long
unsigned short int * decodedData = bitDecode_usint(pattern, patternLength, encodedData, encoded_length);
```
- then for a quick sanity check you can do:
```cpp
cout << "encodedData: ";
for (size_t i = 0; i < encoded_length; i++) {
    cout << encodedData[i] << " ";
};
cout << endl;

cout << "decodedData: ";
size_t decLength = decodedLength(pattern, patternLength, encodedLength);
// ^ decodedLength is a function in BitWise to calculate length of an array returned by bitDecode.
for (size_t i = 0; i < decLength; i++) {
    cout << decodedData[i] << " ";
};
cout << endl;
```

See the Encoding section above for a deeper explanation of the pattern array.

### File IO
#### Reading
- To read a bit-encoded file open an "inbound bit stream" with bitwise::ibitStream (being sure to
initialize with a path to the infile of your choosing) then use ibitStream.read() to buffer a portion
of the file (a certain number of bytes) and decode that buffer according to a pattern of bitdepths.
```cpp
#include "headers/bitwise.h"
#include <iostream>

using namespace std;
using namespace bitwise;

string infile = "infileExample.bin";
unsigned char pattern[] = {3, 2, 9};

ibitStream ibs(infile);
size_t numBytes = ibs.getFileSize();
unsigned short int * decodedData = ibs.read(ibs.getFileSize(), pattern, sizeof(pattern));
```
- then for a quick sanity check you can do:
```cpp
cout << "filesize: " << ibs.getFileSize() << endl;
size_t decLength = ibs.outLength;  // get the length the last array returned from ibs.read()
cout << "decLength: " << decLength << endl;
cout << "decoded contents: ";
for (int i = 0; i < decLength; i++) {
    cout << decodedData[i] << " ";
};
cout << endl;
```
- In this case we decode the entire file at once which may not be practical for large files.
To buffer only a specified number of bytes at once, simply set "n" in ibitStream.read() to the
desired number of bytes. NOTE: This will return decoded arrays of varying length, depending
on your bitdepth pattern. This should rarely if ever be a problem, but if it does become
annoying for your use case at any point, the BitWise encodedLength and decodedLength
functions are there to help. DOUBLE NOTE: the ibitStream.read() functions move the stream cursor forward "n" bytes just like they do in fstream.
- and make sure to close the file steam when you are done with it:
```cpp
ibs.close();
```
#### Writing
- To simultaneously encode and write an array to a file, open an "outbound bit stream" with
bitwise::obitStream (being sure to initialize with a path to the outfile of your choosing) then use
obitStream.write() to write your array to the file (in the process encoding it according to your
pattern of bitdepths).
```cpp
#include "headers/bitwise.h"
#include <iostream>

using namespace std;
using namespace bitwise;

string outfile = "outfileExample.bin";
unsigned short int data[] = {7, 3, 483, 7, 2, 510};
size_t dataLength = sizeof(data) / sizeof(unsigned short int)
unsigned char pattern[] = {3, 2, 9};
size_t patternLength = sizeof(pattern);  // unsigned chars are one byte long

obitStream obs(outfile);
obs.write(data, dataLength, pattern, patternLength);
```
- Then for a quick sanity check you can compare the number of bytes proportedly written to the file
 and the final filesize with:
```cpp
cout << "outLength: " << obs.outLength  << " bytes" << endl;
cout << "filesize: " << obs.getFileSize() << "bytes" << endl;
```
- You can also read the outfile back in with the reading code above or check with your eyes and xxd
if you want to be extra sure you did it right.
- Of course, you should also be sure to close the stream when you are done with it:
```cpp
obs.close();
```
#### Seeking
- The (i/o)bitStream.seek() functions work exactly like the fstream.seekg() function
(in binary mode).
Meaning that they move the cursor a certain number of "bytes in the file" and not
"encoded datites in the file". The latter would involve bit-level seeking which is impossible.
But say you want to seek to 420 bytes from the end of the file and then read back your final
position, you would do:
```cpp
ibs.seek(-420, ios::end);
size_t loc = ibs.tell();
```
## Contributing
For contributors to the project; do this before making your first commit:

- Install pre-commit
```bash
cd /path/to/this/repository/
sudo apt install pre-commit
pre-commit install
```
(we do all of our development on linux)

- To test updates to the readme and other GitHub flavored markdown, simply install Grip
and feed it your desired file.
```bash
pip3 install grip
grip README.md
```
- Then follow the link provided by the Grip sever for a live preview of your work.

- When satisfied with your changes you can compile to an html file with:
```bash
grip README.md --export README.html
```
## Authors

* **Gabe M. LaFond** - *Initial work* - [ExamDay](https://github.com/ExamDay)

See also the list of [contributors](https://github.com/ExamDay/bitwise/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
