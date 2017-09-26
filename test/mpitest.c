#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv) {
	int world_size;
	int rank;
	char hostname[256];
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;

	MPI_Init(&argc, &argv); 				// Initialize the MPI environment
	MPI_Comm_size(MPI_COMM_WORLD, &world_size); 		// get total number of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);			// get processor rank number
	MPI_Get_processor_name(processor_name, &name_len); 	// get the processor name
	
	gethostname(hostname, 255);				// non-MPI function to get the host name
	printf("Hello world! I am process number: %d from processor %s on host %s out of %d processors\n", rank, processor_name, hostname, world_size);

	MPI_Finalize();
	return 0;
}
