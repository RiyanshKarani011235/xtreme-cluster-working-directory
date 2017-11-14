#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SOURCE_NODE                     0
#define N                               8

void matrixMultiply();
int randInt();
void printMatrix(double *, int);
void printArray(double *, int);
void logOutput(char *);

int main(int argc, char **argv) {
    
    matrixMultiply();

    return 0;
}

void fillMatrixINputMethod1(int *ptr, int length, int width) {

}

void fillMatrixInputMethod2(double * ptr, int n) {
    // input method 1
    double input[4] = {-1.0, 0.0, 1.0, 0.0};
    printArray(input, 4);

    // copy the input array n/4 times in the first row
    for(int i=0; i<n; i+=4) {
        memcpy(ptr+i, input, sizeof(input));
    }

    // copy the first row to every other ith row
    // after circular right shifting it by i positions
    for(int i=1; i<n; i++) {
        memcpy(ptr + (i*n) + i, ptr, sizeof(double)*n - i);
        memcpy(ptr + (i*n), ptr + n - i, sizeof(double)*i);
    }
}

void matrixMultiply() {
    double * X = malloc(sizeof(double) * N * N);
    fillMatrixInputMethod2(X, N);

    printMatrix(X, N);

    free(X);
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
        printArray(ptr + (i*n), n);
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
