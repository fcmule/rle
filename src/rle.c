#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint64_t size;
    uint8_t *buffer;
} FileContent;

// These two structs are different things represented in the same way
typedef FileContent SizedBuffer;

static uint64_t get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    uint64_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

static FileContent read_entire_file(char *file_path) {
    FileContent file_content;
    FILE *file = fopen(file_path, "rb");
    if (file != NULL) {
        file_content.size = get_file_size(file);
        file_content.buffer = malloc(file_content.size);
        // The entire content of the file is loaded into RAM
        fread(file_content.buffer, file_content.size, 1, file);
        fclose(file);
    } else {
        fprintf(stderr, "Could not read file at path: %s\n", file_path);
        file_content.size = 0;
        file_content.buffer = NULL;
    }
    return file_content;
}

static uint64_t rle_compress(FileContent in_file_content, SizedBuffer sized_out_buffer) {
    uint64_t in_buffer_size = in_file_content.size;
    uint64_t out_buffer_size = sized_out_buffer.size;
    uint8_t *in_buffer = in_file_content.buffer;
    uint8_t *out_buffer = sized_out_buffer.buffer;
    uint64_t i = 0, j = 0;
    while (i < in_buffer_size) {
        uint8_t count = 1;
        while (i < in_buffer_size-1 && count < 255 &&
               in_buffer[i] == in_buffer[i+1]) {
            count++;
            i++;
        }
        // Check if the output buffer has enough space left for the entry
        if ((int64_t)j < ((int64_t)(out_buffer_size)-1)) {
            out_buffer[j++] = in_buffer[i];
            out_buffer[j++] = count;
            i++;
        } else {
            fprintf(stderr,
                "Output buffer is not big enough to contain the compressed file content\n");
            break;
        }
    }
    return j;
}

static void compress(char *in_file_path, char *out_file_path) {
    // Since the entire content of the file is being loaded into RAM, the size
    // of the files being compressed can't exceed its size. That said, this
    // is just a toy compressor that's not meant to handle very large files
    FileContent in_file_content = read_entire_file(in_file_path);
    if (!in_file_content.size) { return; }
    SizedBuffer sized_out_buffer;
    // The size of the raw file is always present and occupies the first 8 bytes.
    // In the worst case, each byte occupies two bytes (1 for the original and 1 for the count)
    sized_out_buffer.size = 8 + 2 * in_file_content.size;
    sized_out_buffer.buffer = malloc(sized_out_buffer.size);
    uint64_t compressed_size = rle_compress(in_file_content, sized_out_buffer);
    FILE *out_file = fopen(out_file_path, "wb");
    fwrite((void *)&in_file_content.size, 8, 1, out_file); // For decompression
    fwrite(sized_out_buffer.buffer, compressed_size, 1, out_file);
    fclose(out_file);
}

static uint64_t rle_decompress(FileContent in_file_content, SizedBuffer sized_out_buffer) {
    uint64_t in_buffer_size = in_file_content.size;
    uint64_t out_buffer_size = sized_out_buffer.size;
    uint8_t *in_buffer = in_file_content.buffer;
    uint8_t *out_buffer = sized_out_buffer.buffer;
    uint64_t i = 0, j = 0;
    while (i < in_buffer_size-1) {
        uint8_t byte = in_buffer[i];
        uint8_t count = in_buffer[i+1];
        // Check if the output buffer has enough space left for the bytes
        if ((j + count) <= out_buffer_size) {
            while (count--) {
                out_buffer[j++] = byte;
            }
            i += 2;
        } else {
            fprintf(stderr,
                "Output buffer is not big enough to contain the decompressed file content\n");
            break;
        }
    }
    return j;
}

static void decompress(char *in_file_path, char *out_file_path) {
    FileContent in_file_content = read_entire_file(in_file_path);
    if (in_file_content.size < 8) {
        if (in_file_content.size > 0) {
            fprintf(stderr,
                "Incorrect format, the first 8 bytes should represent the size of the decompressed file\n");
        }
        return;
    }
    SizedBuffer sized_out_buffer;
    // The size of the raw file occupies the first 8 bytes
    sized_out_buffer.size = *(uint64_t *)in_file_content.buffer;
    sized_out_buffer.buffer = malloc(sized_out_buffer.size);
    in_file_content.buffer += 8; // Remove the read size from the buffer
    in_file_content.size -= 8; // Adjust the size of the buffer after the removal
    uint64_t actual_decompressed_size = rle_decompress(in_file_content, sized_out_buffer);
    if (sized_out_buffer.size != actual_decompressed_size) {
        fprintf(stderr,
            "The file '%s' specified a decompressed size of %llu bytes, but %llu were produced\n",
            in_file_path, sized_out_buffer.size, actual_decompressed_size);
    }
    FILE *out_file = fopen(out_file_path, "wb");
    fwrite(sized_out_buffer.buffer, actual_decompressed_size, 1, out_file);
    fclose(out_file);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: rle compress   [in_file_path] [out_file_path]\n");
        fprintf(stderr, "       rle decompress [in_file_path] [out_file_path]\n");
        return 1;
    }

    char *command = argv[1];
    char *in_file_path = argv[2];
    char *out_file_path = argv[3];

    if (strcmp(command, "compress") == 0) {
        compress(in_file_path, out_file_path);
    } else if (strcmp(command, "decompress") == 0) {
        decompress(in_file_path, out_file_path);
    } else {
        fprintf(stderr,
            "Wrong command: %s. Choose between 'compress' and 'decompress\n", command);
        return 1;
    }

    return 0;
}
