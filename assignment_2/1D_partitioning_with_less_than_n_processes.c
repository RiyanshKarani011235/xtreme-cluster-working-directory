#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define SOURCE_NODE                     0
#define N                               8
#define MAX_ARRAY_ELEMENT               8
#define NUM_PROCESSES                   4

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

    LU_Decomposition();

    return 0;
}

int LU_Decomposition() {

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);			// get process rank number
    
    const int NUM_ROWS_PER_PROCESS = N / NUM_PROCESSES; 

    int * row;
    int * temprow;
    row = malloc(N * NUM_ROWS_PER_PROCESS * sizeof(int));
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

        // brodcast NUM_ROWS_PER_PROCESS rows of 
        // the array to the corresponding processor
        for(int k=0; k<N; k+=NUM_ROWS_PER_PROCESS) {
            if(k != SOURCE_NODE) {
                send(SOURCE_NODE, ptr + (k*N), N*NUM_ROWS_PER_PROCESS, k/NUM_ROWS_PER_PROCESS);
            }
        }

        // store the rows used by the source processor
        memcpy(row, ptr + (N*SOURCE_NODE), N * NUM_ROWS_PER_PROCESS * sizeof(int));

        // free the matrix since it will no longer be used
        // only the rows will be used to compute the LU Decomposition
        free(ptr);
    } else {
        // receive the row corresponding to this processor,
        // from the source node
        receive(id, row, N*NUM_ROWS_PER_PROCESS, SOURCE_NODE);
   }

    printArray(row, N * NUM_ROWS_PER_PROCESS);

    // kth iteration of Gaussian Elimination
    for(int k=0; k<N; k++) {

        // calculate the process rank that is supposed
        // to handle the kth row for gaussian elemination
        int active_row_process_id = k / NUM_ROWS_PER_PROCESS;

        char s[64];
        snprintf(s, sizeof(s), "id: %d, k: %d, active process: %d\n", id, k, active_row_process_id);
        log_output(s);

        if(id == active_row_process_id) {
            // the current row in gaussian elimination belongs
            // to this processor
            for(int i=active_row_process_id+1; i<NUM_PROCESSES; i++) {
                send(id, row + ((k % NUM_ROWS_PER_PROCESS) * N), N, i);
            }

            // perform elimination on other rows belonging to this processor
            for(int i=(k%NUM_ROWS_PER_PROCESS)+1; i<NUM_ROWS_PER_PROCESS; i++) {
                printf("process %d for k = %d\n", id, k);

                temprow = row;

                double pivot = (1.0 * (*(row + (i*N) + k))) / (*(temprow + k));

                printf("pivot: %lf\n", pivot);
                
                for(int j=k; j<N; j++) {
                    *(row + (i*N) + j) -= (int) ((*(temprow + j)) * pivot);
                }

                for(int j=0; j<NUM_ROWS_PER_PROCESS; j++) {
                    printArray(row + (N*j), N);
                }
            }

        } else if(id > active_row_process_id){
            // the current row in gaussian elimination does
            // not belong to this processor
            receive(id, temprow, N, active_row_process_id);

            for(int i=0; i<NUM_ROWS_PER_PROCESS; i++) {
                double pivot = (1.0 * (*(row + (i*N) + k))) / (*(temprow + k));

                for(int j=k; j<N; j++) {
                    *(row + (i*N) + j) -= (int) ((*(temprow + j)) * pivot);
                }

                printf("process %d for k = %d\n", id, k);
                printArray(row, N);
            }
        }
    }

    // Gaussian elimination Done, now we need to find the
    // determinant of the array. Each process p_k has to 
    // calculate A[k][k] * A[k][k], and the source node will 
    // perform an all to one accumulation

    int determinant = 0;
    int * buff_determinant = malloc(sizeof(int));

    for(int k=0; k<N; k++) {
        // calculate the process rank that is supposed
        // to handle the kth row for calculating determinant
        int active_row_process_id = k / NUM_ROWS_PER_PROCESS;

        // calculate the square of kth diagonal element
        if(id == active_row_process_id) {
            int row_number = k % NUM_ROWS_PER_PROCESS;
            determinant += pow(*(row + (N * row_number) + k), 2);
        }
    }

    if(id == SOURCE_NODE) {
        for(int i=0; i<NUM_PROCESSES; i++) {
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
    // since temprow was assigned to row previously, freeing
    // both temprow and row would lead to an error because you
    // are freeing the same memory location twice. To get
    // past this problem you make temprow point to row
    temprow = row;
    free(row);
    MPI_Finalize();
}

/*
 * Generates a random integer in the range [1, max+1]
 */
int randInt() {
    return ((rand() % (MAX_ARRAY_ELEMENT+1)) + 1);
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
