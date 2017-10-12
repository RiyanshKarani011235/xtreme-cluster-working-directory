#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define MAX_ARRAY_ELEMENT 5
#define NUM_PROCESSORS 1
#define NUM_PROCESSES_PER_PROCESSOR 2
#define ARRAY_LENGTH 100
#define SOURCE_NODE 0

int randInt();
int randInitArray(int *, int);
int FIND_SUM(int, int, int);
void printArray(int *, int);
void send(int, int *, int, int);
void receive(int, int *, int, int);
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

	FIND_SUM(NUM_PROCESSORS, world_size, ARRAY_LENGTH);
	return 0;
}

int FIND_SUM(int p, int k, int n) { 

	double start_time = MPI_Wtime();
	
	int id;
	MPI_Comm_rank(MPI_COMM_WORLD, &id);			// get process rank number

	int virtual_id = id ^ SOURCE_NODE;			// virtual id with respect to the 
												// source node
	
	int *ptr;			// pointer to the array (for which sum is to be computed)
	ptr = malloc(n * sizeof(int));	
	int can_send = 0; 	// flag that determines whether a node is waiting
						// to receive data, or sending data to other nodes
						// as per the recursive doubling schedule

	if(virtual_id == SOURCE_NODE) {
		// generate a randomly initialized array
		can_send = 1;
		if(ptr == NULL) {
			printf("could not allocate memory for the array\n");
			exit(1);
		}
		randInitArray(ptr, n);
		printArray(ptr, n);
	}

	// recursive doubling scatter
	int num_to_distribute = n - (n%k);
	int *start_ptr;
	int *end_ptr;

	if(virtual_id == SOURCE_NODE) {
		start_ptr = ptr + (n%k);
	} else {
		start_ptr = ptr;
	}
	end_ptr = ptr + n - 1;

	int mask = pow(2, log2(k))-1;
	for(int i=log2(k)-1; i>=0; i--) {
		int check = pow(2, i);
		num_to_distribute = num_to_distribute / 2;
		end_ptr = start_ptr + num_to_distribute;
		if(can_send) {
			send(id, end_ptr, num_to_distribute, id + check);
		} else if ((virtual_id & mask) == check) {
			receive(id, start_ptr, num_to_distribute, id - check);
			can_send = 1;
		}
		mask /= 2;
	}

	// calculating the part of the array for which this process
	// will calculate the sum
	int array_size_to_consider = (n - (n%k))/k;
	
	// boundary condition, when n is not divisible by k
	// the last array will calculate the sum for n%k extra 
	// elements of the array
	if(virtual_id == SOURCE_NODE) {
		array_size_to_consider += n%k;
	}

	char string[100];
	snprintf(string, sizeof(string), "process %d summing over the array : \n", virtual_id);
	printArray(ptr, array_size_to_consider);

	int sum = 0;
	for(int i=0; i<array_size_to_consider; i++) {
		sum += *(ptr + i);
	}

	char s[64];
	snprintf(s, sizeof(s), "process %d sum = %d\n", virtual_id, sum);
	log_output(s);

	// all to one reduction
	can_send = 1;	// flag that determines whether a node has finished
					// sending the data, since every node does this only
					// once in all to one reduction
	mask = 1;
	for(int i=0; i<log2(k); i++) {
		int check = pow(2, i);
		mask = mask | check;
		if((virtual_id & mask) == check) {
			send(id, &sum, 1, id - check);
			can_send = 0;
		} else if(can_send) {
			int sum_add;
			receive(id, &sum_add, 1, id + check);
			sum += sum_add;
		}
	}

	if(virtual_id == SOURCE_NODE) {
		char s[64];
		snprintf(s, sizeof(s), "the total sum is : %d\n", sum);
		log_output(s);
	}
	
	free(ptr);

	double elapsed_time = MPI_Wtime() - start_time;
	snprintf(string, sizeof(string), "");
	snprintf(string, sizeof(string), "process %d elapsed time = %f\n", virtual_id, elapsed_time);
	log_output(string);

	MPI_Finalize();	
	
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

void log_output(char * string) {
	FILE *f;
	f = fopen("./output.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
	if (f == NULL) { /* Something is wrong   */}
	fprintf(f, string);
	fclose(f);
}