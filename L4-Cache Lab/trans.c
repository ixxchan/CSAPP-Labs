/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void trans_block(int Bsize, int M, int N, int A[N][M], int B[M][N])
{
    int i, j, ii, jj;

    for (j = 0; j < M; j += Bsize)
        for (i = 0; i < N; i += Bsize)
            for (ii = i; ii < i + Bsize && ii < N; ++ii)
                for (jj = j; jj < j + Bsize && jj < M; ++jj)
                    B[jj][ii] = A[ii][jj];
}

void trans_a(int M, int N, int A[N][M], int B[M][N]);
/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, ii, jj, a0, a1, a2, a3, a4, a5, a6, a7;

    if (M == 61)
        for (j = 0; j < M; j += 16)
            for (i = 0; i < N; i += 16)
                for (ii = i; ii < i + 16 && ii < N; ++ii)
                    for (jj = j; jj < j + 16 && jj < M; ++jj)
                        B[jj][ii] = A[ii][jj];
    else if (M == 32)
        // this idea comes from
        // https://github.com/Seterplus/CSAPP/blob/master/cachelab/trans.c
        for (j = 0; j < M; j += 8)
            for (i = 0; i < N; i += 8)
                if (i != j)
                    for (ii = i; ii < i + 8; ++ii)
                        for (jj = j; jj < j + 8; ++jj)
                            B[jj][ii] = A[ii][jj];
                // the same places at A and B shares same set index
                // which needs to be handled specially
                // otherwise will cause eviction
                else
                {
                    // first copy
                    for (ii = i; ii < i + 8; ++ii)
                    {
                        // we should load them together, otherwise
                        // write B will cause eviction
                        a0 = A[ii][j];
                        a1 = A[ii][j + 1];
                        a2 = A[ii][j + 2];
                        a3 = A[ii][j + 3];
                        a4 = A[ii][j + 4];
                        a5 = A[ii][j + 5];
                        a6 = A[ii][j + 6];
                        a7 = A[ii][j + 7];
                        B[ii][j] = a0;
                        B[ii][j + 1] = a1;
                        B[ii][j + 2] = a2;
                        B[ii][j + 3] = a3;
                        B[ii][j + 4] = a4;
                        B[ii][j + 5] = a5;
                        B[ii][j + 6] = a6;
                        B[ii][j + 7] = a7;
                    }
                    // then transpose in place
                    for (ii = i; ii < i + 8; ++ii)
                        for (jj = ii; jj < j + 8; ++jj)
                        {
                            a0 = B[jj][ii];
                            B[jj][ii] = B[ii][jj];
                            B[ii][jj] = a0;
                        }
                }
    else
        for (j = 0; j < M; j += 4)
            for (i = 0; i < N; i += 4)
                if (i != j)
                    for (ii = i; ii < i + 4; ++ii)
                        for (jj = j; jj < j + 4; ++jj)
                            B[jj][ii] = A[ii][jj];
                // the same places at A and B shares same set index
                // which needs to be handled specially
                // otherwise will cause eviction
                else
                {
                    // first copy
                    for (ii = i; ii < i + 4; ++ii)
                    {
                        // we should load them together, otherwise
                        // write B will cause eviction
                        a0 = A[ii][j];
                        a1 = A[ii][j + 1];
                        a2 = A[ii][j + 2];
                        a3 = A[ii][j + 3];
                        B[ii][j] = a0;
                        B[ii][j + 1] = a1;
                        B[ii][j + 2] = a2;
                        B[ii][j + 3] = a3;
                    }
                    // then transpose in place
                    for (ii = i; ii < i + 4; ++ii)
                        for (jj = ii; jj < j + 4; ++jj)
                        {
                            a0 = B[jj][ii];
                            B[jj][ii] = B[ii][jj];
                            B[ii][jj] = a0;
                        }
                }
    // trans_a(M,N,A,B);
    //assert(is_transpose(M, N, A, B));
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

char trans_a_desc[] = "trial";
void trans_a(int M, int N, int A[N][M], int B[M][N])
{
    int ii, j, k, Bsize = 8;

    for (j = 0; j < M; j += Bsize)
    {
        for (ii = 0; ii < N; ii++)
        {
            for (k = 0; k < Bsize && j + k < M; ++k)
                B[j + k][ii] = A[ii][j + k];
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    // registerTransFunction(trans_a, trans_a_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
