void set_bit(uint8_t* array, int index){
    int i = index/8; // index within bit array
    int pos = index%8;

    uint8_t flag = 1;

    flag = flag << (8-pos-1); // (BYTE_SIZE - 1 - x % BYTE_SIZE)
    array[i] = array[i] | flag;
}


void clear_bit(uint8_t *array, int index) {
    int i = k/32;
    int pos = k%32;

    unsigned int flag = 1;  // flag = 0000.....00001

    flag = flag << pos;     // flag = 0000...010...000   (shifted k positions)
    flag = ~flag;           // flag = 1111...101..111

    array[i] = array[i] & flag;
}

//https://www.sanfoundry.com/c-program-implement-bit-array/
unit8_t get_bit(uint8_t *array, int index) {
    
    unsigned int mask = 1 & (array[index / 8] >> (index % 8));
    
    uint8_t result = mask & 1;
    
    return result;
}


/*
    Read and store binary file to byte array
    https://stackoverflow.com/questions/28269995/c-read-file-byte-by-byte-using-fread
*/
uint8_t * read_compression_dir(long * filelen) {

    FILE *fileptr;
    uint8_t *buffer;
    int i;
    // read binary file
    fileptr = fopen(COMPRESSION_DIR, "rb");
    fseek(fileptr, 0, SEEK_END);
    *filelen = ftell(fileptr);
    rewind(fileptr); // sets the file position to the beginning of the file
    buffer = malloc((*filelen)*sizeof(uint8_t));
    for(i = 0; i < *filelen; i++) {
       fread(buffer+i, 1, 1, fileptr);  // read one byte each time
    }
    fclose(fileptr);
    return buffer;
}
