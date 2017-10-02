#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define MAX_ARRAY_ELEMENT 5
#define NUM_PROCESSORS 1
#define NUM_PROCESSES_PER_PROCESSOR 2
#define ARRAY_LENGTH 11

int randImt();
int randInitArray(int *, int);
int FIND_SUM(int, int, int);
void printArray(int *, int);
void send(int, int *, int, int);
void receive(int, int *, int, int);
void log_output(char *);

int main(int argc, char **argv) {
	FIND_SUM(NUM_PROCESSORS, NUM_PROCESSES_PER_PROCESSOR, ARRAY_LENGTH);
	return 0;
}

/*
 * Generates a random integer in the range [-max, +max]
 */
int randInt() {
	return (rand() % (MAX_ARRAY_ELEMENT+1)) * pow(-1, (rand() % 2));
}

/* 
 * Given an array as input, this method fills the array
 * With random integers in the range [-max, +max]
 */
int randInitArray(int *ptr, int size) {
	// seed the random number generator so that it generates
	// different sequence of random numbers after each execution
	int seed = time(NULL);
	srand(seed);
	for(int i=0; i<size; i++) {
		*(ptr + i) = randInt();
	}
}

int FIND_SUM(int p, int k, int n) { 

	int world_size;
	int rank;
	char hostname[256];
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	
	MPI_Init(NULL, NULL); 									// Initialize the MPI environment
	MPI_Comm_size(MPI_COMM_WORLD, &world_size); 			// get total number of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);					// get process rank number
	MPI_Get_processor_name(processor_name, &name_len); 		// get the processor name
	
	gethostname(hostname, 255);								// non-MPI function to get the host name
	// printf("Hello world! I am process number: %d from processor %s on host %s out of %d processors\n", rank, processor_name, hostname, world_size);
	
	int *ptr;
	ptr = malloc(n * sizeof(int));	
	int size;
	int can_send = 0;

	if(rank == 0) {
		// generate a randomly initialized array
		can_send = 1;
		if(ptr == NULL) {
			printf("could not allocate memory for the array\n");
			exit(1);
		}
		randInitArray(ptr, n);

	// } else {
	// 	int *ptr;
	// 	ptr = malloc((n/2) * sizeof(int));
	// 	if(ptr == NULL) {
	// 		printf("could not allocate memory for the array\n");
	// 		exit(1);
	// 	}
	// 	receive(ptr, n/2, 0);
	// 	printArray(ptr, n/2);	
	// 	free(ptr);

	}

	int mask = pow(2, log2(world_size))-1;
	for(int i=log2(world_size)-1; i>=0; i--) {
		int check = pow(2, i);
		if(can_send) {
			send(rank, ptr, n, rank + check);
		} else if ((rank & mask) == check) {
			receive(rank, ptr, n, rank - check);
			can_send = 1;
		}
		mask /= 2;
	}

	// int next_split_start = n/2 + n%2;
	// send(ptr + next_split_start, n/2, 1);
	// printArray(ptr, n);	
	free(ptr);

	/*
	 * recursive doubling scatter
	 */

	MPI_Finalize();	
	
	return 0;
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
void send(int rank, int *ptr, int size, int destination_process_number) {
	MPI_Send(ptr, size, MPI_INT, destination_process_number, 1, MPI_COMM_WORLD);
	char s[64];
	snprintf(s, sizeof(s), "process %d sent %d integers to process: %d\n", rank, size, destination_process_number);
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
void receive(int rank, int *ptr, int size, int source_process_number) {
	MPI_Recv(ptr, size, MPI_INT, source_process_number, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char s[64];
	snprintf(s, sizeof(s), "process %d received %d integers from process: %d\n", rank, size, source_process_number);
	log_output(s);
}

void printArray(int *ptr, int size) {
	printf("[");
	for(int i=0; i<size; i++) {
		printf("%d, ", *(ptr + i));
	}
	printf("]\n");
}

void log_output(char * string) {
	// FILE *f;
	// f = fopen("./output.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
	// if (f == NULL) { /* Something is wrong   */}
	// fprintf(f, string);
	printf(string);
}