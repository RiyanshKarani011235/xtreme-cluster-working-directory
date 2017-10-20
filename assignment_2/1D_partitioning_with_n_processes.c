#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define SOURCE_NODE 0
#define N 4
#define MAX_ARRAY_ELEMENT 4

int LU_Decomposition();
int randInt();
void printMatrix(int *, int);
void log_output(char *);

int main(int argc, char **argv) {
    
    int rank;
    int world_size;
    char hostname[256];
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    
    MPI_Init(NULL, NULL); 									// Initialize the MPI environment
    MPI_Comm_size(MPI_COMM_WORLD, &world_size); 			// get total number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);						// get process rank number
    MPI_Get_processor_name(processor_name, &name_len); 		// get the processor name
    
    gethostname(hostname, 255);								// non-MPI function to get the host name
    printf("Hello world! I am process number: %d from processor %s on host %s out of %d processors\n", rank, processor_name, hostname, world_size);

    LU_Decomposition();

    return 0;
}

int LU_Decomposition() {

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);			// get process rank number
    
    int * ptr;
    ptr = malloc(pow(N,2) * sizeof(int));

    if(id == SOURCE_NODE) {
        // source node generates the array
        for(int i=0; i<pow(N,2); i++) {
            *(ptr+i) = randInt();
        }
        printMatrix(ptr, N);
    }

    // brodcast each row of the array to the corresponding processor
    for(int k=0; k<N; k++) {

    }

    // cleanup
    free(ptr);
    MPI_Finalize();
}

/*
 * Generates a random integer in the range [-max, +max]
 */
int randInt() {
	return (rand() % (MAX_ARRAY_ELEMENT+1));
}

/*
 * utility function that prints a matrix
 */ 
void printMatrix(int *ptr, int n) {
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            char s[64];
            snprintf(s, sizeof(s), "%d ", *(ptr + n*i + j));
            log_output(s);
        }
        log_output("\n");
    }
}

void log_output(char * string) {
	// FILE *f;
	// f = fopen("./output.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
	// if (f == NULL) { /* Something is wrong   */}
	// fprintf(f, string);
	// fclose(f);
	printf(string);
}
