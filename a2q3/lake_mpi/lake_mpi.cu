#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"

#define _USE_MATH_DEFINES

#define XMIN 0.0
#define XMAX 1.0
#define YMIN 0.0
#define YMAX 1.0

#define MAX_PSZ 10
#define TSCALE 1.0
#define VSQR 0.1

void init(double *u, double *pebbles, int n);
void evolve(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t);
void evolve9pt(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t);
int tpdt(double *t, double dt, double end_time);
void print_heatmap(const char *filename, double *u, int n, double h);
void init_pebbles(double *p, int pn, int n);

void run_cpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time);

extern void run_gpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time, int nthreads);

int my_rank,m_size;
int tag=50;
MPI_Status status;


int main(int argc, char *argv[])
{

  if(argc != 5)
  {
    printf("Usage: %s npoints npebs time_finish nthreads \n",argv[0]);
    return 0;
  }

  int     npoints   = atoi(argv[1]);
  int     npebs     = atoi(argv[2]);
  double  end_time  = (double)atof(argv[3]);
  int     nthreads  = atoi(argv[4]);
  int 	  narea	    = npoints * npoints;
	
  if(npoints%4!=0){
   printf("Enter number of points which are divisible by 4!");
   return 0;
  }  
  int number_of_nodes;

  MPI_Init(&argc,&argv);

  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&number_of_nodes);
  printf("Number of nodes %d\n",number_of_nodes);
  /*if(number_of_nodes!=4){
  	printf("Error: Use 4 nodes!\n");
	MPI_Finalize();
	return 0;
  }
*/
/* m_size is memory required on every node, on the 
* boundry nodes you just have n extra values and other 
* node you will have 2n extra values received from the peer node.
*/  
  m_size = (narea/4);

  if(my_rank==0 || my_rank==3){
	m_size = m_size + npoints;
  } else {
  	m_size = m_size + (2*npoints);
  }
  
  double *u_i0, *u_i1;
  double *u_cpu, *u_gpu, *pebs;
  double h;

  double elapsed_cpu, elapsed_gpu;
  struct timeval cpu_start, cpu_end, gpu_start, gpu_end;
  
  u_i0 = (double*)malloc(sizeof(double) * m_size);
  u_i1 = (double*)malloc(sizeof(double) * m_size);
  if(my_rank==0){
  	pebs = (double*)malloc(sizeof(double) * npoints*npoints);
  } else {
	pebs = (double*)malloc(sizeof(double)*m_size);
  }
  u_cpu = (double*)malloc(sizeof(double) * m_size);
  u_gpu = (double*)malloc(sizeof(double) * m_size);

  printf("Running %s with (%d x %d) grid, until %f, with %d threads\n", argv[0], npoints, npoints, end_time, nthreads);

  h = (XMAX - XMIN)/npoints;
  //Send the pebbles value calculate from the first node
  if(my_rank==0){
  	init_pebbles(pebs, npebs, npoints);
  	MPI_Send(pebs+npoints,((npoints*npoints)/4)+(2*npoints),MPI_DOUBLE,1,tag,MPI_COMM_WORLD);
  	MPI_Send(pebs+(3*npoints),((npoints*npoints)/4)+(2*npoints),MPI_DOUBLE,2,tag,MPI_COMM_WORLD);
  	MPI_Send(pebs+(5*npoints),((npoints*npoints)/4)+(npoints),MPI_DOUBLE,3,tag,MPI_COMM_WORLD);
	
  }else{
	//recv the pebbels 
	MPI_Recv(pebs,m_size,MPI_DOUBLE,0,tag,MPI_COMM_WORLD,&status);
  }

  init(u_i0, pebs, npoints);
  init(u_i1, pebs, npoints);
  
   char heat[20];
  sprintf(heat,"lake_i_%d.dat",my_rank);
  print_heatmap(heat, u_i0, npoints, h);

  gettimeofday(&cpu_start, NULL);
  run_cpu(u_cpu, u_i0, u_i1, pebs, npoints, h, end_time);
  gettimeofday(&cpu_end, NULL);

  elapsed_cpu = ((cpu_end.tv_sec + cpu_end.tv_usec * 1e-6)-(
                  cpu_start.tv_sec + cpu_start.tv_usec * 1e-6));
  printf("CPU took %f seconds\n", elapsed_cpu);

  /*gettimeofday(&gpu_start, NULL);
  run_gpu(u_gpu, u_i0, u_i1, pebs, npoints, h, end_time, nthreads);
  gettimeofday(&gpu_end, NULL);
  elapsed_gpu = ((gpu_end.tv_sec + gpu_end.tv_usec * 1e-6)-(
                  gpu_start.tv_sec + gpu_start.tv_usec * 1e-6));
  printf("GPU took %f seconds\n", elapsed_gpu);
*/
  char filename[20];
  sprintf(filename,"lake_f_%d.dat",my_rank); 
  print_heatmap(filename, u_cpu, npoints, h);
  //sprintf(filename,"lakegpu_f_%d.dat",my_rank); 
  //print_heatmap("lakegpu_f.dat", u_gpu, npoints, h);
  

  free(u_i0);
  free(u_i1);
  free(pebs);
  free(u_cpu);
  free(u_gpu);
  MPI_Finalize();
  return 1;
}

void run_cpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time)
{
  double *un, *uc, *uo;
  double t, dt;

  un = (double*)malloc(sizeof(double) * m_size);
  uc = (double*)malloc(sizeof(double) * m_size);
  uo = (double*)malloc(sizeof(double) * m_size);

  memcpy(uo, u0, sizeof(double) * m_size);
  memcpy(uc, u1, sizeof(double) * m_size);

  t = 0.;
  dt = h / 2.;

  while(1)
  {
	if(my_rank==0){
		MPI_Send(uo+(m_size-(2*n)),n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD);
		MPI_Send(uc+(m_size-(2*n)),n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD);
		MPI_Recv(uo+(m_size-n),n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD,&status);
		MPI_Recv(uc+(m_size-n),n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD,&status);
	} else if(my_rank==1){
		MPI_Recv(uo,n,MPI_DOUBLE,0,tag,MPI_COMM_WORLD,&status);
		MPI_Recv(uc,n,MPI_DOUBLE,0,tag,MPI_COMM_WORLD,&status);
		MPI_Send(uo+n,n,MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
		MPI_Send(uc+n,n,MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
		MPI_Send(uo+(m_size-(2*n)),n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD);
		MPI_Send(uc+(m_size-(2*n)),n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD);
		MPI_Recv(uo+(m_size-n),n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD,&status);
		MPI_Recv(uc+(m_size-n),n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD,&status);
	} else if(my_rank==2){
		MPI_Recv(uo,n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD,&status);
		MPI_Recv(uc,n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD,&status);
		MPI_Send(uo+n,n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD);
		MPI_Send(uc+n,n,MPI_DOUBLE,1,tag,MPI_COMM_WORLD);
		MPI_Send(uo+(m_size-(2*n)),n,MPI_DOUBLE,3,tag,MPI_COMM_WORLD);
		MPI_Send(uc+(m_size-(2*n)),n,MPI_DOUBLE,3,tag,MPI_COMM_WORLD);
		MPI_Recv(uo+(m_size-n),n,MPI_DOUBLE,3,tag,MPI_COMM_WORLD,&status);
		MPI_Recv(uc+(m_size-n),n,MPI_DOUBLE,3,tag,MPI_COMM_WORLD,&status);
	}else{
		MPI_Recv(uo,n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD,&status);
		MPI_Recv(uc,n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD,&status);
		MPI_Send(uo+n,n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD);
		MPI_Send(uc+n,n,MPI_DOUBLE,2,tag,MPI_COMM_WORLD);
		
	}
    //evolve(un, uc, uo, pebbles, n, h, dt, t);
    evolve9pt(un, uc, uo, pebbles, n, h, dt, t);
	
    memcpy(uo, uc, sizeof(double) * m_size);
    memcpy(uc, un, sizeof(double) * m_size);

    if(!tpdt(&t,dt,end_time)) break;
  }
  
  memcpy(u, un, sizeof(double) * m_size);
}

void init_pebbles(double *p, int pn, int n)
{
  int i, j, k, idx;
  int sz;

  srand( time(NULL) );
  memset(p, 0, sizeof(double) * n * n);

  for( k = 0; k < pn ; k++ )
  {
    i = rand() % (n - 4) + 2;
    j = rand() % (n - 4) + 2;
    sz = rand() % MAX_PSZ;
    idx = j + i * n;
    p[idx] = (double) sz;
  }
}

double f(double p, double t)
{
  return -expf(-TSCALE * t) * p;
}

int tpdt(double *t, double dt, double tf)
{
  if((*t) + dt > tf) return 0;
  (*t) = (*t) + dt;
  return 1;
}

void init(double *u, double *pebbles, int n)
{
  int i, j, idx;

  for(i = 0; i < n/4; i++)
  {
    for(j = 0; j < n ; j++)
    {
      idx = j + i * n;
      u[idx] = f(pebbles[idx], 0.0);
    }
  }
}

void evolve(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t)
{
  int i, j, idx;

  for( i = 0; i < n; i++)
  {
    for( j = 0; j < n; j++)
    {
      idx = j + i * n;

      if( i == 0 || i == n - 1 || j == 0 || j == n - 1)
      {
        un[idx] = 0.;
      }
      else
      {
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n] - 4 * uc[idx])/(h * h) + f(pebbles[idx],t));
      }
    }
  }
}
void evolve9pt(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t)
{
  int i, j, idx,k;
  if(my_rank==0 ||my_rank==3){
  	k=(n/4)+1;
  }else{
  	k=(n/4)+2;
  }

  for( i = 0; i < k; i++)
  {
    for( j = 0; j < n; j++)
    {
      idx = j + i * n;

      if( i == 0 || i == n - 1 || j == 0 || j == n - 1)
      {
        un[idx] = 0.;
      }
      else
      {
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n] +0.25*(uc[idx-n+1]+uc[idx+n-1]+uc[idx-n-1]+uc[idx+n+1])  - 5 * uc[idx])/(h * h) + f(pebbles[idx],t));
      }
    }
  }
}

void print_heatmap(const char *filename, double *u, int n, double h)
{
  int i, j, idx,k,start=0;

  FILE *fp = fopen(filename, "w");  
  k=n/4;
  if(my_rank!=0){
	start = 1;
  }
  
  for( i = start; i < k; i++ )
  {
    for( j = 0; j < n; j++ )
    {
      idx = j + i * n;
      fprintf(fp, "%f %f %f\n", i*h, j*h, u[idx]);
    }
  }
  
  fclose(fp);
} 
