#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define SOURCE_NODE                     0
#define N                               8
#define p                               4
#define K                               1

void fillMatrixINputMethod1(int *, int, int);
void fillMatrixInputMethod2(double * , int);
void matrixMultiplyKTimes();
void multiplyMatrices(MPI_Comm, int[], int, double *, double *, double *, int) ;
void simpleMultiplyMatrices(double *, double *, double *, int);
double determinantOfMatrix(double *, int, int);
void getCofactor(double *, double *, int, int, int);
int randInt();
void send(MPI_Comm, int, double *, int, int);
void receive(MPI_Comm, int, double *, int, int);
void printMatrix(double *, int);
void printArray(double *, int);
void logOutput(char *);
void wait_(int);

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
    double * Y = malloc(sizeof(double) * N * N);
    double * Result = malloc(sizeof(double) * N * N);
    if(rank == SOURCE_NODE) {
        // generate the source matrix X
        fillMatrixInputMethod2(X, N);
        memcpy(Y, X, sizeof(double) * N * N);
        printMatrix(X, N);
    }

    // multiply K times
    for(int i=0; i<K; i++) {
        // multiply matrices using Canon's algorithm
        multiplyMatrices(Cart, coordinates, rank, X, Y, Result, N);
        memcpy(Y, Result, sizeof(double) * N * N);
    }

    // if(rank == SOURCE_NODE) {
    //     // compute determinant at the source node
    //     double determinant = determinantOfMatrix(Result, N, N);
    //     printf("determinant = %lf", determinant);
    // }
    
    free(X);
    free(Y);
    free(Result);
}

void multiplyMatrices(MPI_Comm Cart, int coordinates[], int rank, double * X, double * Y, double * Result, int n) {
    int sqrtp = sqrt(p);
    int blockSize = n / sqrtp;
    double * A = malloc(sizeof(double) * pow(blockSize, 2));
    double * B = malloc(sizeof(double) * pow(blockSize, 2));
    double * C = malloc(sizeof(double) * pow(blockSize, 2));
    double * CTemp = malloc(sizeof(double) * pow(blockSize, 2));

    // INITIALIZE C TO ALL ZEROS
    for(int i=0; i<blockSize; i++) {
        *(C + i) = 0;
    }

    // SEND BLOCK DATA TO CORRESPONDING PROCESSES
    for(int i=0; i<sqrtp; i++) {
        for(int j=0; j<sqrtp; j++) {
            // send part of X to processor (i, j)
            if(rank == SOURCE_NODE) {
                int destinationId;
                MPI_Cart_rank(Cart, (int[2]){i, j}, &destinationId);

                if(destinationId != SOURCE_NODE) {
                    // send data to this destination id
                    for(int k=0; k<blockSize; k++) {
                        send(Cart, rank, X + (n * ((i*blockSize)+k)) + (j*blockSize), blockSize, destinationId);
                    }
                    for(int k=0; k<blockSize; k++) {
                        send(Cart, rank, Y + (n * ((i*blockSize)+k)) + (j*blockSize), blockSize, destinationId);
                    }
                } else {
                    // copy data to A and B
                    for(int k=0; k<blockSize; k++) {
                        memcpy(A + (k*blockSize), X + (n * (i+k)) + j, sizeof(double) * blockSize);
                    }
                    for(int k=0; k<blockSize; k++) {
                        memcpy(B + (k*blockSize), Y + (n * (i+k)) + j, sizeof(double) * blockSize);
                    }
                }
            } else if(i == coordinates[0]  && j == coordinates[1]) {
                // receive data for A and B from source
                for(int k=0; k<blockSize; k++) {
                    receive(Cart, rank, A+(k*blockSize), blockSize, SOURCE_NODE);
                }
                for(int k=0; k<blockSize; k++) {
                    receive(Cart, rank, B+(k*blockSize), blockSize, SOURCE_NODE);
                }
                memcpy(B, A, sizeof(A));
            }
        }
    }

    // wait_(rank);
    // printf("process %d has the following data \n", rank);
    // printMatrix(A, blockSize);
    // printMatrix(B, blockSize);

    // MULTIPLICATION ITERATIONS
    double * tempArray = malloc(sizeof(double) * blockSize * blockSize);
    for(int i=0; i<=sqrtp; i++) {

        // SIMPLY MULTIPLY A AND B IN EACH BLOCK AND ADD
        // THE RESULTS TO C
        simpleMultiplyMatrices(A, B, CTemp, blockSize);
        for(int i=0; i<blockSize*blockSize; i++) {
            *(C + i) += *(CTemp + i);
        }

        // decide whether to left shift a row
        if(coordinates[0] <= i) {

            // left shift A by 1 
            // printf("process %d left shifting by 1\n", rank);
            int left, right;
            MPI_Cart_shift(Cart, 1, 1, &left, &right);
            
            if(coordinates[1] == sqrtp - 1) {
                // last process at the right, sends first
                send(Cart, rank, A, blockSize*blockSize, right);
                receive(Cart, rank, A, blockSize*blockSize, left);
            } else {
                // not the last process at the right, receives first
                receive(Cart, rank, tempArray, blockSize*blockSize, left);
                send(Cart, rank, A, blockSize*blockSize, right);
                memcpy(A, tempArray, sizeof(double) * blockSize * blockSize);
            }
        }

        // decide whether to up shift a column
        if(coordinates[1] <= i) {
            // up shift B by 1
            // printf("process %d up shifting by 1\n", rank);
            int up, down;
            MPI_Cart_shift(Cart, 0, 1, &up, &down);
            // printf("for process %d, up is %d, down in %d\n", rank, up, down);

            if(coordinates[0] == sqrtp - 1) {
                // last process at the bottom, sends first
                send(Cart, rank, B, blockSize*blockSize, up);
                receive(Cart, rank, B, blockSize*blockSize, down);
            } else {
                // not the last process at the bottom, receives first
                receive(Cart, rank, tempArray, blockSize*blockSize, down);
                send(Cart, rank, B, blockSize*blockSize, up);
                memcpy(B, tempArray, sizeof(double) * sizeof(A));
            }
        }
    }

    double * tempArray1 = malloc(sizeof(double) * blockSize * blockSize);
    if(rank == SOURCE_NODE) {
        // collect the result from every process and 
        // accumulate it into Y
        for(int i=0; i<sqrtp; i++) {
            for(int j=0; j<sqrtp; j++) {
                int sourceId;
                MPI_Cart_rank(Cart, (int[2]){i, j}, &sourceId);
                if(sourceId == SOURCE_NODE) {
                    memcpy(tempArray1, C, sizeof(double) * blockSize * blockSize);
                } else {
                    receive(Cart, rank, tempArray1, blockSize * blockSize, sourceId);
                }

                for(int k=0; k<blockSize; k++) {
                    memcpy(Result + (n*((i*blockSize) + k) + (j*blockSize)), tempArray1 + (k*blockSize), sizeof(double) * blockSize);
                }
            }
        }
    } else {
        send(Cart, rank, C, blockSize * blockSize, SOURCE_NODE);
    }
    
    free(tempArray);
    free(tempArray1);
    free(A);
    free(B);
    free(C);
    free(CTemp);
}

void simpleMultiplyMatrices(double * A, double * B, double * C, int n) {
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            *(C + (n*i) + j) = 0;
            for(int k=0; k<n; k++) {
                *(C + (n*i) + j) += *(A + (n*i) + k) * *(B + (n*k) + j);
            }
        }
    }
}

// Function to get cofactor of mat[p][q] in temp[][]. n is current
// dimension of mat[][]
void getCofactor(double * mat, double * temp, int r, int q, int n) {
    int i = 0, j = 0;
 
    // Looping for each element of the matrix
    for (int row = 0; row < n; row++)
    {
        for (int col = 0; col < n; col++)
        {
            //  Copying into temporary matrix only those element
            //  which are not in given row and column
            if (row != r && col != q)
            {
                *(temp + (i*n) + j++) = *(mat + (n*row) + col);
 
                // Row is filled, so increase row index and
                // reset col index
                if (j == n - 1)
                {
                    j = 0;
                    i++;
                }
            }
        }
    }
}
 
//Recursive function for finding determinant of matrix.
// n is current dimension of mat
double determinantOfMatrix(double * mat, int size, int n) {
    double D = 0; // Initialize result
 
    //  Base case : if matrix contains single element
    if (n == 1)
        return *(mat);
 
    double * temp = malloc(sizeof(double) * size * size); // To store cofactors
 
    int sign = 1;  // To store sign multiplier
 
     // Iterate for each element of first row
    for (int f = 0; f < n; f++) {
        // Getting Cofactor of mat[0][f]
        getCofactor(mat, temp, 0, f, n);
        D += sign * *(mat + f) * determinantOfMatrix(temp, size, n - 1);
 
        // terms are to be added with alternate sign
        sign = -sign;
    }
 
    return D;
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
    int k = 0;
    for(int i=0; i<n; i++) {
        k += i;
        printf("k = %d\n", k);
        int j = k % n;
        memcpy(ptr + (i*n) + j, row, sizeof(double)*(n - j));
        memcpy(ptr + (i*n), row + n - j, sizeof(double)*j);
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

void wait_(int rank) {
    int m = 0;
    while(m < rank * 300000000) {
        m++;
    }
}
