A simple RLE compressor.
Requires a 64-bit processor.

This toy compressor loads the entire input file into RAM, so it's not meant to handle very large files.

Usage:

./build.sh
./build/rle compress resources/wiki_example.txt resources/wiki_example.rle
./build/rle decompress resources/wiki_example.rle resources/wiki_example_decompressed.txt
