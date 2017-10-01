#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_ARRAY_ELEMENT 5
#define NUM_PROCESSORS 1
#define NUM_PROCESSES_PER_PROCESSOR 2
#define ARRAY_LENGTH 1000

int randImt();
int randInitArray(int *, int);
int FIND_SUM(int, int, int);

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
	
	int array_size = 10000;

	MPI_Init(NULL, NULL); 									// Initialize the MPI environment
	MPI_Comm_size(MPI_COMM_WORLD, &world_size); 			// get total number of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);					// get process rank number
	MPI_Get_processor_name(processor_name, &name_len); 		// get the processor name
	
	gethostname(hostname, 255);								// non-MPI function to get the host name
	printf("Hello world! I am process number: %d from processor %s on host %s out of %d processors\n", rank, processor_name, hostname, world_size);

	int *ptr;
	ptr = malloc(n * sizeof(int));
	if(rank == 0) {
		printf("sending\n");
	} else {
		printf("receiving\n");
	}

	MPI_Finalize();	
	return 0;
}