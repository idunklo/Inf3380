#include <mpi.h>   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
typedef struct
{
    int num_rows;
    int num_cols;
    double *matrix[0];
}
M; 
*/

void print_flush(const char* str, int rank)
{
    printf("%d :: %s\n", rank, str);
    fflush(NULL);
}     

void allocate_matrix(double*** matrix, int num_rows, int num_cols)
{
    int i;
    *matrix = (double**)malloc((num_rows)*sizeof(double*));
    (*matrix)[0] = (double*)calloc((num_rows)*(num_cols),sizeof(double));  
    for (i=1; i<(num_rows); i++)
        (*matrix)[i] = (*matrix)[i-1]+(num_cols);  
    /*mat -> num_cols = num_cols;
    mat -> num_rows = num_rows;*/

}

void deallocate(double ***matrix){
    free((*matrix)[0]);
    free(*matrix);
}


void find_size( int num_rows, int num_cols, int *my_num_rows, int *my_num_cols, int my_rank, int sqrt_p)
   
{
    int x = my_rank % sqrt_p;;
    int y = (int)floor((double)my_rank / sqrt_p);

    *my_num_cols = floor((x+1) * (double)num_cols / sqrt_p) - floor(x*(double)num_cols/ sqrt_p);
    *my_num_rows = floor((y+1) * (double)num_rows / sqrt_p) - floor(y*(double)num_rows/ sqrt_p); 

    //using pointer to change the value outside the function. 
    //dividing number of columns and row. See example solution exam 2012.  
}    

void MatrixMultiply(int n, double *a, double *b, double *c)
{
    int i, j, k;

    for (i=0; i<n; i++)
        for (j=0; j<n; j++)
            for (k=0; k<n; k++)
                c[i*n+j] += a[i*n+k]*b[k*n+j];
}

void MatrixMatrixMultiply(double *a, double *b, double *c, int num_cols, int num_rows, MPI_Comm comm)
{
    /*from the teaching book */
    int i, j;
    int n;
    int nlocal;
    int num_procs, dims[2], periods[2];
    int myrank, my2drank, mycoords[2];
    int uprank, downrank, leftrank, rightrank, coords[2];
    int shiftsource, shiftdest;
    MPI_Status status; 
    MPI_Comm comm_2d;

    MPI_Comm_size(comm, &num_procs);
    MPI_Comm_rank(comm_2d, &my2drank);
    MPI_Cart_coords(comm_2d, my2drank, 2, mycoords);

    MPI_Cart_shift(comm_2d, 1, -1, &rightrank, &leftrank);
    MPI_Cart_shift(comm_2d, 0, -1, &downrank, &uprank);

    nlocal = n/dims[0];

    MPI_Cart_shift(comm_2d, 0, -mycoords[0], &shiftsource, &shiftdest);
    MPI_Sendrecv_replace(a, nlocal*nlocal, MPI_DOUBLE, shiftdest, 1, shiftsource, 1, comm_2d, &status); 

 MPI_Cart_shift(comm_2d, 1, -mycoords[0], &shiftsource, &shiftdest);
    MPI_Sendrecv_replace(b, nlocal*nlocal, MPI_DOUBLE, shiftdest, 1, shiftsource, 1, comm_2d, &status);  

for (i=0; i<dims[0]; i++){
    MatrixMultiply(nlocal, a, b, c); /* c=c + a*b */

    MPI_Sendrecv_replace(a, nlocal*nlocal, MPI_DOUBLE, leftrank, 1, rightrank, 1, comm_2d, &status);

    MPI_Sendrecv_replace(b, nlocal*nlocal, MPI_DOUBLE, uprank, 1, downrank, 1, comm_2d, &status);   

    MPI_Cart_shift(comm_2d, 0, +mycoords[0], &shiftsource, &shiftdest);
    MPI_Sendrecv_replace(a, nlocal*nlocal, MPI_DOUBLE, shiftdest, 1, shiftsource, 1, comm_2d, &status);   

 MPI_Cart_shift(comm_2d, 0, +mycoords[0], &shiftsource, &shiftdest);
    MPI_Sendrecv_replace(b, nlocal*nlocal, MPI_DOUBLE, shiftdest, 1, shiftsource, 1, comm_2d, &status);  

    MPI_Comm_free(&comm_2d);    
}
}
    

void read_matrix_binaryformat (char* filename, double*** matrix,
        int* num_rows, int* num_cols)
{
    int i;
    FILE* fp = fopen (filename,"rb");
    fread (num_rows, sizeof(int), 1, fp);                   
    fread (num_cols, sizeof(int), 1, fp);
    /* storage allocation of the matrix */
    *matrix = (double**)malloc((*num_rows)*sizeof(double*));
    (*matrix)[0] = (double*)malloc((*num_rows)*(*num_cols)*sizeof(double));
    for (i=1; i<(*num_rows); i++)
        (*matrix)[i] = (*matrix)[i-1]+(*num_cols);
    /* read in the entire matrix */
    fread ((*matrix)[0], sizeof(double), (*num_rows)*(*num_cols), fp);
    fclose (fp);
}

void write_matrix_binaryformat (char* filename, double** matrix,
        int num_rows, int num_cols)
{
    FILE *fp = fopen (filename,"wb");
    fwrite (&num_rows, sizeof(int), 1, fp);
    fwrite (&num_cols, sizeof(int), 1, fp);
    fwrite (matrix[0], sizeof(double), num_rows*num_cols, fp);
    fclose (fp);
}

void find_max(int *values, int *max, int val_size) {
    // find max value of a general array

    *max = values[0];
    for(int i=1; i<val_size; ++i) {
        if(values[i] > *max) {
            *max = values[i];
        }
    }
}


int main(int argc, char *argv[])
{
    //Declaration of variables
    int my_rank, num_procs;  
    int num_rows, num_cols;
    double **A, **B, **C;
    int nrA, ncA, nrB, ncB;
    double **a, **b, **c;  
    int nra, nca, nrb, ncb;
    int sqrt_p;
    int *rsA, *csA, *rsB, *csB;
    int mrA, mcA, mrB, mcB;

    //Declare MPI-suff
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    char *Mat_A, *Mat_B, *Mat_C;

    Mat_A = argv[1];
    Mat_B = argv[2];
    Mat_C = argv[3]; 

    sqrt_p = sqrt(num_procs);

    print_flush("taken arguments", my_rank);   

    if(my_rank == 0){
        read_matrix_binaryformat(Mat_A, &A, &nrA, &ncA );
        read_matrix_binaryformat(Mat_B, &B, &nrB, &ncB );
    }
    print_flush("files read", my_rank);  

    MPI_Bcast(&nrA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&ncA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nrB, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&ncB, 1, MPI_INT, 0, MPI_COMM_WORLD);  

    find_size(nrA, ncA, &nra, &nca, my_rank, sqrt_p); 
    find_size(nrB, ncB, &nrb, &ncb, my_rank, sqrt_p);

    rsA = (int*)calloc(num_procs,sizeof(int)); 
    csA = (int*)calloc(num_procs,sizeof(int)); 
    rsB = (int*)calloc(num_procs,sizeof(int)); 
    csB = (int*)calloc(num_procs,sizeof(int)); 

    for(int i=0; i<num_procs; ++i) {
        if(i == my_rank) {
            rsA[i] = nra;
        }
        MPI_Sendrecv(&nra, 1, MPI_INT, i, 1, &(rsA[i]), 1, MPI_INT, i, 1,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&nca, 1, MPI_INT, i, 1, &(csA[i]), 1, MPI_INT, i, 1,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&nrb, 1, MPI_INT, i, 1, &(rsB[i]), 1, MPI_INT, i, 1,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&nrb, 1, MPI_INT, i, 1, &(csB[i]), 1, MPI_INT, i, 1,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("nra[%i] = %i, my_rank:%i\n", i, rsA[i], my_rank);
    }

    if(my_rank == 0) {
        find_max(rsA, &mrA, num_procs);
        find_max(csA, &mcA, num_procs);
        find_max(rsB, &mrB, num_procs);
        find_max(csB, &mcB, num_procs);
    }

    allocate_matrix(&a, mrA, mcA);
    allocate_matrix(&b, mrB, mcB);
    allocate_matrix(&c, mrA, mcB);

    if(my_rank ==0){
        deallocate(&A);
        deallocate(&B);
    }


    print_flush("deallocated", my_rank);  

    MPI_Finalize();
    return 0;  

}

