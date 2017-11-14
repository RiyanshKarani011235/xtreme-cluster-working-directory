#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define SOURCE_NODE                     0
#define N                               8
#define p                               4

void matrixMultiply();
int randInt();
void send(int, int *, int, int);
void receive(int, int *, int, int);
void printMatrix(double *, int);
void printArray(double *, int);
void logOutput(char *);

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

    matrixMultiplyKTimes();

    MPI_Finalize();

    return 0;
}

void fillMatrixINputMethod1(int *ptr, int length, int width) {

}

void fillMatrixInputMethod2(double * ptr, int n) {
    // input method 1
    double input[4] = {-1.0, 0.0, 1.0, 0.0};
    double * row = malloc(sizeof(double) * n);

    // copy the input array n/4 times in the first row
    for(int i=0; i<n; i+=4) {
        memcpy(row+i, input, sizeof(input));
    }

    // copy the first row to every other ith row
    // after circular right shifting it by i positions
    for(int i=0; i<n; i++) {
        memcpy(ptr + (i*n) + i, row, sizeof(double)*n - i);
        memcpy(ptr + (i*n), row + n - i, sizeof(double)*i);
    }
}

void matrixMultiplyKTimes() {

    // int dim[2] = {(int) (N / sqrt(p)), (int) (N / sqrt(p))};
    // int period = {0, 0};
    // MPI_Comm Cart;
    // MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, 1, &Cart);

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);			// get process rank number

    // double * A = malloc(sizeof(double) * pow(, 2));
    // double * B = malloc(sizeof(double) * pow(N / sqrt(p), 2));
    double * X;

    if(id == SOURCE_NODE) {
        // generate the source matrix
        X = malloc(sizeof(double) * N * N);
        fillMatrixInputMethod2(X, N);
        printMatrix(X, N);

        
    } else {
    }

}

void multiplyMatrices(double * X, double * Y, int n) {
    
}


/*
 * Generates a random integer in the range [-max, +max]
 */
// int randInt() {
//     return (rand() % (MAX_ARRAY_ELEMENT+1));
// }

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
	logOutput(s);
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
	logOutput(s);
}

void printArray(double *ptr, int size) {
	printf("[");
	for(int i=0; i<size; i++) {
		printf("%lf, ", *(ptr + i));
	}
	printf("]\n");
}

void printMatrix(double *ptr, int n) {
    char s[64];
    snprintf(s, sizeof(s), "printing a matrix of size %d x %d\n", n, n);
    logOutput(s);
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            char s[128];
            snprintf(s, sizeof(s), "%lf ", *(ptr + (n*i) + j));
            logOutput(s);
        }
        logOutput("\n");
    }
}
void logOutput(char * string) {
	FILE *f;
	f = fopen("./output.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
	if (f == NULL) { /* Something is wrong   */}
	fprintf(f, string);
	fclose(f);
	// printf(string);
}
