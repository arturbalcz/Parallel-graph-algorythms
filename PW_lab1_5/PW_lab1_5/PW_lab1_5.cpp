// PW_lab1_5.cpp : Defines the entry point for the console application.
//

#include"stdafx.h"
#include<iostream>
#include<omp.h>
#include<vector>
#include<ctime>
#include<cstdlib>

#define NUM_THREDS 4
#define N 1000
#define P 0.5
#define DYNAMIC_CHUNK 1

using namespace std; 
typedef bool TaB[N][N];

int num_t1, num_t2; 

unsigned int seeds[NUM_THREDS]; 

TaB A;

void gen_seeds()
{
	srand(time(NULL) + 654);
	seeds[0] = time(NULL) + 894;

	for (int i = 1; i < NUM_THREDS; i++)
	{
		seeds[i] = seeds[0] + 451 * i;
	}
}

int rand_r(unsigned int *seed)
{
	unsigned int next = *seed;
	int result;
	next *= 1103515245;
	next += 12345;
	result = (unsigned int)(next / 65536) % 2048;
	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int)(next / 65536) % 1024;
	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int)(next / 65536) % 1024;
	*seed = next;
	return result;
}

void initA(int n, TaB &A)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			A[i][j] = 0; 
		}
	}
}

void printA(int n, TaB A)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			cout << A[i][j] << " ";
		}

		cout << endl;
	}
}

void gnp(int n, float p, TaB &A)
{
	int i, j;  

	for (i = 0; i < n - 1; i++)
	{
		for (j = i+1; j < n; j++)
		{
			if (((rand() % 100) +1) <= p*100)
			{
				A[i][j] = true;
				A[j][i] = true;
			}
		}
	}
}

void gnp_parallel(int n, float p, TaB &A)
{
	int i, j; 

	{
		#pragma omp parallel for private(i,j) schedule(dynamic, DYNAMIC_CHUNK)
		for (i = 0; i < n - 1; i++)
		{
			num_t1 = omp_get_num_threads(); 

			for (j = i + 1; j < n; j++)
			{
				num_t2 = omp_get_num_threads();

				if (((rand_r(&seeds[omp_get_thread_num()]) % 100) + 1) <= p * 100)
				{
					A[i][j] = true;
					A[j][i] = true;
				}
			}
		}
	}
}

int count_edges(int n, TaB &A)
{
	int i, j, k = 0;
	for (i = 0; i < n - 1; i++)
	{
		for (j = i + 1; j < n; j++)
		{
			if (A[i][j]) k++; 
		}
	}

	return k;
}

int count_edges_parallel(int n, TaB &A)
{
	int i, j, k = 0;

	#pragma omp parallel for shared(k) private(i,j) schedule(guided)
	for (i = 0; i < n - 1; i++)
	{
		int local_k = 0;

		for (j = i + 1; j < n; j++)
		{
			if (A[i][j])
			{
				local_k++;
			}
		}
#pragma omp atomic 
		k += local_k; 
	}

	return k;
}

int count_edges_reduction(int n, TaB &A)
{
	int i, j, k = 0;

#pragma omp parallel for private(i,j) reduction(+:k) schedule(guided)
	for (i = 0; i < n - 1; i++)
	{
		for (j = i + 1; j < n; j++)
		{
			if (A[i][j])
			{
				k++;
			}
		}
	}

	return k;
}

int main()
{
	gen_seeds(); 

	omp_set_num_threads(NUM_THREDS); 

	int edges = 0; 

	double start, end, sum = 0; 
	double start_g, end_g, sum_g = 0;
	double start_p_g, end_p_g, sum_p_g = 0;
	double start_p, end_p, sum_p = 0;
	double start_r, end_r = 0, sum_r = 0; 

	initA(N, A); 

	for (int i = 0; i < 100; i++)
	{
		start_p_g = omp_get_wtime();
		gnp_parallel(N, P, A);
		end_p_g = omp_get_wtime() - start_p_g;
		//cout << endl << "time parallel = " << end_p_g << endl;

		sum_p_g += end_p_g; 

		start_p = omp_get_wtime();
		edges = count_edges_parallel(N, A); 
		end_p = omp_get_wtime() - start_p;
		//cout << endl << "time parallel = " << end_p << endl;
		//cout << edges << endl; 

		sum_p += end_p;

		start_r = omp_get_wtime();
		edges = count_edges_reduction(N, A);
		end_r = omp_get_wtime() - start_r;
		//cout << endl << "time parallel = " << end_r << endl;
		//cout << edges << endl;

		sum_r += end_r;

		initA(N, A);

		start_g = omp_get_wtime();
		gnp(N, P, A);
		end_g = omp_get_wtime() - start_g;
		//cout << endl << "time = " << end_g << endl;

		sum_g += end_g; 

		start = omp_get_wtime();
		edges=count_edges(N, A);
		end = omp_get_wtime() - start;
		//cout<<endl<<"time = " << end << endl;
		//cout << edges << endl;
		
		sum += end;
	}

	cout << "Gnp time = " << sum_g / 100 << endl; 
	cout << "Parallel Gnp time = " << sum_p_g / 100 << endl; 
	cout << "Gnp acceleration = " << (sum_g / 100) / (sum_p_g / 100) << endl << endl;
	cout << "Counting edges time = " << sum / 100 << endl;
	cout << "Parallel counting edges time = " << sum_p / 100 << endl;
	cout << "Counting acceleration = " << (sum / 100) / (sum_p / 100) << endl << endl;
	cout << "Parallel counting edges with sum reduction time = " << sum_r / 100 << endl;
	cout << "Counting acceleration (with reduction) = " << (sum / 100) / (sum_r / 100) << endl << endl;

	//system("pause"); 

    return 0;
}

