/*
    COMP3520 Assignment 2 - Job List Generator

    usage:

        ./random <OUTPUT_FILE>
        where <OUTPUT_FILE> is the filename for the job list file
*/

/************************************************************************************************************************

    ** Revision history **

    Current version: 1.0
    Date: 7 September 2019

    1.0: Original version

    Contributors:
    1. COMP3520 teaching staff
       Centre for Distributed and High Performance Computing
       School of Computer Science
       The University of Sydney
       NSW 2006
       Australia

    Copyright of this code and associated material is vested in the University of Sydney.

    This code is NOT in the Public Domain. Unauthorized posting of this code or derivatives thereof is not permitted.

    ** DO NOT REMOVE THIS NOTICE. **

 ***********************************************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main (int argc, char *argv[])
{
    FILE * output_stream = NULL;
    int no_of_jobs = 0;
    float lambda_arrival = 0;
    float lambda_service = 0;
    double L = 0, p = 0, u = 0;
    int i, k = 0, x = 0, y = 0;

    if (argc <= 0)
    {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    CHECKPOINT:
    /* Ask the user to enter some parameters */

    printf("Please enter the number of jobs that are to be dispatched: ");
    scanf("%d", &no_of_jobs);

    if (no_of_jobs < 1)
    {
        fprintf(stderr, "Sorry, the number of jobs to be dispatched must be at least one.\n");
        goto CHECKPOINT;
    }

    printf("Please enter the mean of the random Poisson distribution for intervals between job arrivals: ");
    scanf("%f", &lambda_arrival);

    printf("Please enter the inverse of the mean of the random exponential distribution for job execution duration: ");
    scanf("%f", &lambda_service);

    srand(time(0)); // Seed the random number generator

    /* Try to open a new file for writing. If the file already exists, it will be overwritten. */
    if ((output_stream = fopen(argv[1], "w")) == NULL)
    {
        fprintf(stderr, "FATAL: Unable to open file for writing.\n");
        exit(EXIT_FAILURE);
    }

    /* Randomly generate jobs */
    for (i = 0; i < no_of_jobs; i++)
    {
        /* Generate intervals between job arrivals in accordance with the Poisson distribution */
        L = exp(-1*lambda_arrival);
        k = 0;
        p = 1;
        do
        {
            k += 1;
            u = rand()/(double)RAND_MAX;
            p *= u;
        } while (p > L);
        x += k - 1;

        /* Generate job service times in accordance with the exponential distribution */
        u = rand()/(double)((long long)RAND_MAX+1);
        y = 1 + floor(log(1 - u)/(-1 * lambda_service));

        /* Save the generated values to the output file */
        fprintf(output_stream, "%d, %d\n", x, y);
    }
    fclose(output_stream);
    exit(EXIT_SUCCESS);    
}
