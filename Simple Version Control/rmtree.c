/* USYD CODE CITATION ACKNOWLEDGEMENT
 * I declare that the whole of the following code has been copied from 
 * stackoverflow webpage in the answer of the question 
 * "How to create a temporary directory in C?" 
 * 
 * Stackoverflow webpage
 * https://stackoverflow.com/questions/18792489/how-to-create-a-temporary-directory-in-c
 */ 


#define  _POSIX_C_SOURCE 200809L
#define  _XOPEN_SOURCE 500L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ftw.h>

/* Call-back to the 'remove()' function called by nftw() */
static int remove_callback(const char *pathname,
                __attribute__((unused)) const struct stat *sbuf,
                __attribute__((unused)) int type,
                __attribute__((unused)) struct FTW *ftwb) {
  return remove (pathname);
}

void rmtree(const char *pathname) {
	if (nftw (pathname, remove_callback, FOPEN_MAX,
            FTW_DEPTH | FTW_MOUNT | FTW_PHYS) == -1) {
            perror("tempdir: error: ");
            exit(EXIT_FAILURE);
    }
}

