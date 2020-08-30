#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <endian.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include "server.h"
#include "thread_pool.h"
#include "helper_fun.h"



/*
    Convert message header from unit8_t to bits
    Store the first 4 bits to digit_type
    Store the fifth bit to recv_compression
    Store the sixth bit to reqr_compresssion
*/
void read_message_header(uint8_t header, struct client_data *data) {
    uint8_t i;
    int j = 0;
    char *tmp = calloc(TYPEDIGIT*2, sizeof(char));

    for(i=0x80;i!=0;i>>=1) {
        if (header&i) {
            tmp[j] = '1';
        }
        else {
            tmp[j] = '0';
        }
        j++;
    }
    //memcpy(data->digit_type, tmp, 4);
    int digit = binary2decimal(tmp, TYPEDIGIT);
    data->digit_type = digit + '0'; // conver int to a char
    data->recv_compression = tmp[5];
    data->reqr_compression = tmp[6];
    free(tmp);
}

// Binary To Hexadecimal (check the first 4 bit only)
//https://www.tutorialspoint.com/learn_c_by_examples/binary_to_hexadecimal_program_in_c.htm
/*
    Check the given digit_type is valid or not (4 bits array)
    Valid type digit:
        Echo: 0x0 (0000): return 0
        Directory listing: 0x2 (0010): return 2
        File size query: 0x4 (0100): return 4
        Retrueve file: 0x6 (0110): return 6
        Shutdown command: 0x8 (1000): return 8
*/
int binary2decimal(char *n, int size) {
    int i,decimal,mul=0;

    for(decimal=0,i=size-1;i>=0;--i,++mul)
      decimal+=(n[i]-48)*(1<<mul);

    return decimal;
}


/*
    convert input uint8_t array to one uint64_t
 */
uint64_t parse(uint8_t *v, uint8_t size ) {
    uint64_t val = v[7];
    int i = 6;
    for(int j = 1; j <size; j++){
        val = val | v[j] << (8*i); // left-shift
        i--;
    }
    return val;
}

/*
    Read 8 byte payload length i
 */
void read_payload_length(struct client_data *data, uint8_t *buffer) {
    for (int i = 0; i<8; i++) {
        data->payload_length[i] = buffer[1+i];
    }
}


/*
    Helper function of file size query (size_query()), and retrieve file (retrieve_file())
 
    Check if file exist and calculate the size of the given 'target_file'
    Send an error message if the 'target_file' not exits, return NULL
    If the target file exist, also calculate file size, saved in 'file_size',
    return the relative path of the target file
 */
char * check_regular_file(struct client_data * data, char * target_file, uint64_t *file_size) {
    //printf("target file is %s \n", target_file);
    char* files_in_dir[200]; // 这个有问题,我感觉不可以这么设置
    DIR *dir;
    int idx = 0;
    int found = 0; // euqal to 1 if the given file exist
    struct dirent* ent;
    char* path = NULL;
    if ((dir = opendir(data->server->target_dir)) == NULL) {
        // fail to open directory
        perror("could not open directory");
        return NULL;
    }
    while ((ent = readdir(dir)) != NULL) {
        if(ent->d_type == DT_REG) // regular file ??? 改成8如果是的话
        {
            files_in_dir[idx] = ent->d_name;
            idx+=1;
        }
    }
    // check the requried file exists in the target directory
    for (int i=0; i < idx; i++) {
        if(strcmp(files_in_dir[i], target_file) == 0) {
            found = 1; // target file exits in directory
            break;
        }
    }
    // if the required file name doesnot exist, return an error response
    if(found == 0) {
        error_response(data->new_socket);
        return NULL;
    }
    
        // the response contains the length of the file with the target file name
        path = realloc(path, strlen(data->server->target_dir) + 3 + strlen(target_file));
        strcpy (path, data->server->target_dir);
        strcat(path,"/");
        strcat(path, target_file);
        // get file size
        struct stat stats;
        if(stat(path, &stats)!=0)
        {
            perror("stats: ");
        }
        *file_size = (uint64_t)stats.st_size; // For regular files, the file size in bytes.
    return path;
}






