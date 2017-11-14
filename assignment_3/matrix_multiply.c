#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define SOURCE_NODE                     0
#define N                               8
#define p                               4

void fillMatrixINputMethod1(int *, int, int);
void fillMatrixInputMethod2(double * , int);
void matrixMultiplyKTimes();
void multiplyMatrices(MPI_Comm, int[], int, double *, double *, int) ;
void matrixMultiply();
int randInt();
void send(MPI_Comm, int, double *, int, int);
void receive(MPI_Comm, int, double *, int, int);
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
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);					// get process rank number
    MPI_Get_processor_name(processor_name, &name_len); 		// get the processor name
    
    gethostname(hostname, 255);								// non-MPI function to get the host name
    printf("Hello world! I am process number: %d from processor %s on host %s out of %d processors\n", rank, processor_name, hostname, world_size);

    matrixMultiplyKTimes();

    MPI_Finalize();

    return 0;
}

void matrixMultiplyKTimes() {


    int dim[2] = {sqrt(p), sqrt(p)};
    int period[2] = {1, 1};
    int reorder = 1;
    int coordinates[2];
    int rank;
    MPI_Comm Cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &Cart);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Cart_coords(Cart, rank, 2, coordinates);

    char s[128];
    snprintf(s, sizeof(s), "process : %d has coordinates (%d, %d)\n", rank, coordinates[0], coordinates[1]);
    logOutput(s);

    double * X = malloc(sizeof(double) * N * N);
    if(rank == SOURCE_NODE) {
        // generate the source matrix X
        fillMatrixInputMethod2(X, N);
    }

    multiplyMatrices(Cart, coordinates, rank, X, X, N);
    free(X);
}

void multiplyMatrices(MPI_Comm Cart, int coordinates[], int rank, double * X, double * Y, int n) {
    int sqrtp = sqrt(p);
    int blockSize = n / sqrtp;
    double * A = malloc(sizeof(double) * pow(blockSize, 2));
    double * B = malloc(sizeof(double) * pow(blockSize, 2));
    double * C = malloc(sizeof(double) * pow(blockSize, 2));

    // SEND BLOCK DATA TO CORRESPONDING MESSAGES
    // for(int i=0; i<sqrtp; i++) {
    //     for(int j=0; j<sqrtp; j++) {
    //         // send part of X to processor (i, j)
    //         if(rank == SOURCE_NODE) {
    //             int destinationId;
    //             MPI_Cart_rank(Cart, (int[2]){i, j}, &destinationId);

    //             printf("destination id : %d\n", destinationId);

    //             if(destinationId != SOURCE_NODE) {
    //                 printf("i, j = %d, %d\n", i, j);
    //                 // send data to this destination id
    //                 printf("before the loop starts, blocksize = %d", blockSize);
    //                 for(int k=0; k<blockSize; k++) {
    //                     printf("i, j, k = %d, %d, %d\n", i, j, k);
    //                     // send(rank, X + (n * (i+k)) + j, blockSize, destinationId);
    //                     // send(Cart, rank, X, blockSize, destinationId);
    //                 }
    //                 for(int k=0; k<blockSize; k++) {
    //                     // send(Cart, rank, Y + (n * (i+k)) + j, blockSize, destinationId);
    //                 }
    //             } else {
    //                 // copy data to A and B
    //                 for(int k=0; k<blockSize; k++) {
    //                     memcpy(A + (k*blockSize), X + (n * (i+k)) + j, sizeof(double) * blockSize);
    //                 }
    //                 for(int k=0; k<blockSize; k++) {
    //                     memcpy(B + (k*blockSize), Y + (n * (i+k)) + j, sizeof(double) * blockSize);
    //                 }
    //             }
    //         } else if(i == coordinates[0]  && j == coordinates[1]) {
    //             // receive data for A and B from source
    //             for(int k=0; k<blockSize; k++) {
    //                 printf("&&&&&&&&&&&7777 i, j, k = %d, %d, %d\n", i, j, k); 
    //                 // receive(rank, A+(k*blockSize), blockSize, SOURCE_NODE);
    //                 // receive(Cart, rank, A, blockSize, SOURCE_NODE);
    //             }
    //             for(int k=0; k<blockSize; k++) {
    //                 // receive(Cart, rank, B+(k*blockSize), blockSize, SOURCE_NODE);
    //             }
    //             memcpy(B, A, sizeof(A));
    //         }
    //     }
    // }

    printf("process %d has the following data \n", rank);
    printMatrix(A, blockSize);
    printMatrix(B, blockSize);

    free(A);
    free(B);
    free(C);
}

void simpleMultiplyMatrices(double * A, double * B, double * C, int n) {
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            *(C + (n*i) + j) = 0;
            for(int k=0; k<n; k++) {
                *(C + (n*i) + j) = *(A + (n*i) + k) * *(B + (n*k) + j);
            }
        }
    }
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
        memcpy(ptr + (i*n) + i, row, sizeof(double)*(n - i));
        memcpy(ptr + (i*n), row + n - i, sizeof(double)*i);
    }
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
void send(MPI_Comm comm, int id, double *ptr, int size, int destination_process_number) {
	MPI_Send(ptr, size, MPI_DOUBLE, destination_process_number, 1, comm);
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
void receive(MPI_Comm comm, int id, double *ptr, int size, int source_process_number) {
	MPI_Recv(ptr, size, MPI_DOUBLE, source_process_number, 1, comm, MPI_STATUS_IGNORE);
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
        // for(int j=0; j<n; j++) {
        //     char s[512];
        //     snprintf(s, sizeof(s), "%lf ", *(ptr + (n*i) + j));
        //     logOutput(s);
        // 
        printArray(ptr + (i*n), n);
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
