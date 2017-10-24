#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define SOURCE_NODE                     0
#define N                               4
#define MAX_ARRAY_ELEMENT               8

int LU_Decomposition();
int randInt();
void send(int, int *, int, int);
void receive(int, int *, int, int);
void printMatrix(int *, int);
void printArray(int *, int);
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

    FIND_DET();

    return 0;
}

int FIND_DET() {

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);			// get process rank number
    
    int * row;
    int * temprow;
    row = malloc(N * sizeof(int));
    temprow = malloc(N * sizeof(int));

    if(id == SOURCE_NODE) {
        // source node generates the NxN matrix
        int * ptr;
        ptr = malloc(pow(N,2) * sizeof(int));

        // seed the random number generator so that it generates
	    // different sequence of random numbers after each execution
	    int seed = time(NULL);
	    srand(seed);

        // fill the matrix randomly
        for(int i=0; i<pow(N,2); i++) {
            *(ptr+i) = randInt();
        }

        printMatrix(ptr, N);

        // brodcast each row of the array to the corresponding processor
        for(int k=0; k<N; k++) {
            if(k != SOURCE_NODE) {
                send(SOURCE_NODE, ptr + (k*N), N, k);
            }
        }

        // store the one row used by the source processor
        memcpy(row, ptr + (N*SOURCE_NODE), N * sizeof(int));

        // free the matrix since it will no longer be used
        // only the rows will be used to compute the LU Decomposition
        free(ptr);
    } else {
        // receive the row corresponding to this processor,
        // from the source node
        receive(id, row, N, SOURCE_NODE);
    }

    printArray(row, N);

    // kth iteration of Gaussian Elimination
    for(int k=0; k<N; k++) {
        if(id == k) {
            for(int i=k+1; i<N; i++) {
                send(id, row, N, i);
            }
        } else if(id > k){
            receive(id, temprow, N, k);
            double pivot = (1.0 * (*(row + k))) / (*(temprow + k));

            for(int j=k; j<N; j++) {
                *(row + j) -= (int) ((*(temprow + j)) * pivot);
            }

            printf("process %d for k = %d\n", id, k);
            printArray(row, N);
        }
    }

    // Gaussian elimination Done, now we need to find the
    // determinant of the array. Each process p_k has to 
    // calculate A[k][k] * A[k][k], and the source node will 
    // perform an all to one accumulation
    int determinant = pow(*(row + id), 2);
    int * buff_determinant = malloc(sizeof(int));
    if(id == SOURCE_NODE) {
        for(int i=0; i<N; i++) {
            if(i != id) {
                receive(id, buff_determinant, 1, i);
                determinant += *(buff_determinant);
            }
        }
    } else {
        buff_determinant = &(determinant);
        send(id, buff_determinant, 1, SOURCE_NODE);
    }

    // print the determinant
    if(id == SOURCE_NODE) {
        printf("****************************************\n");
        printf("****************************************\n");
        printf("determinant : %d\n", determinant);
        printf("****************************************\n");
        printf("****************************************\n");
    }
    // cleanup
    free(row);
    free(temprow);
    MPI_Finalize();
}

/*
 * Generates a random integer in the range [-max, +max]
 */
int randInt() {
    return (rand() % (MAX_ARRAY_ELEMENT+1));
}

/*
MPI_Send(
    void* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
	MPI_Comm communicator)
*/
void send(int id, int *ptr, int size, int destination_process_number) {
	MPI_Send(ptr, size, MPI_INT, destination_process_number, 1, MPI_COMM_WORLD);
	char s[64];
	snprintf(s, sizeof(s), "process %d sent %d integers to process: %d\n", id, size, destination_process_number);
	log_output(s);
}

/*
MPI_Recv(
    void* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
 */
void receive(int id, int *ptr, int size, int source_process_number) {
	MPI_Recv(ptr, size, MPI_INT, source_process_number, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char s[64];
	snprintf(s, sizeof(s), "process %d received %d integers from process: %d\n", id, size, source_process_number);
	log_output(s);
}

void printArray(int *ptr, int size) {
	printf("[");
	for(int i=0; i<size; i++) {
		printf("%d, ", *(ptr + i));
	}
	printf("]\n");
}

/*
 * utility function that prints a matrix
 */ 
void printMatrix(int *ptr, int n) {
    char s[64];
    snprintf(s, sizeof(s), "printing a matrix of size %d x %d\n", n, n);
    log_output(s);
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
	FILE *f;
	f = fopen("./output.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
	if (f == NULL) { /* Something is wrong   */}
	fprintf(f, string);
	fclose(f);
	// printf(string);
}
