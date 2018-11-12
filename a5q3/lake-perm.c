/*
* CSC548 - HW5 - Problem 3
* Author: 1. ajain28 Abhash Jain
*         2. asrivas3 Abhishek Kumar Srivastava
*
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "jemalloc/jemalloc.h"

//Backup and MMAP file name- stored in current folder
#define BACK_FILE "app.back"
#define MMAP_FILE "app.mmap"
#define MMAP_SIZE ((size_t)1 << 30)

#define _USE_MATH_DEFINES

#define XMIN 0.0
#define XMAX 1.0
#define YMIN 0.0
#define YMAX 1.0

#define MAX_PSZ 10
#define TSCALE 1.0
#define VSQR 0.1

//Permanent Storage variable taken the backup
PERM double *u_i0,*u_i1,*u_cpu,*pebs,*un,*uc,*uo;
PERM double t;

void init(double *u, double *pebbles, int n);
void evolve(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t);
void evolve9pt(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t);
int tpdt(double *t, double dt, double end_time);
void print_heatmap(const char *filename, double *u, int n, double h);
void init_pebbles(double *p, int pn, int n);

void run_cpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time,int do_restore);

//extern void run_gpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time, int nthreads);

int main(int argc, char *argv[])
{

/*   if(argc != 5)
  {
    printf("Usage: %s npoints npebs time_finish nthreads \n",argv[0]);
    return 0;
  } */
  //take the argument from the command line to restore the old execution
  int do_restore = argc > 1 && strcmp("-r", argv[1]) == 0;
  const char *mode = (do_restore) ? "r+" : "w+";

    // Persistent memory initialization
  perm(PERM_START, PERM_SIZE);
  mopen(MMAP_FILE, mode, MMAP_SIZE);
  bopen(BACK_FILE, mode);
  
  //Attribute to run the lake program
  int     npoints   = 128;
  int     npebs     = 5;
  double  end_time  = 1.0;
  int     nthreads  = 1;
  int 	  narea	    = npoints * npoints;

  double h;

  double elapsed_cpu;
  struct timeval cpu_start, cpu_end;
  //if restore option is given then restore from the backup file
  if(do_restore){
    printf("Restarting...\n");
    restore();
  }else {
    //Initialize the memory for the task
    u_i0 = (double*)malloc(sizeof(double) * narea);
    u_i1 = (double*)malloc(sizeof(double) * narea);
    pebs = (double*)malloc(sizeof(double) * narea);
    u_cpu = (double*)malloc(sizeof(double) * narea);
    mflush();
    backup();
  }

  printf("Running %s with (%d x %d) grid, until %f, with %d threads\n", argv[0], npoints, npoints, end_time, nthreads);

  h = (XMAX - XMIN)/npoints;
  //if it is not restore then Initialize the value for pebbles and u0 and u1
  if(!do_restore){
    init_pebbles(pebs, npebs, npoints);
    init(u_i0, pebs, npoints);
    init(u_i1, pebs, npoints);
  }
  	
  print_heatmap("lake_1.out", u_i0, npoints, h);

  gettimeofday(&cpu_start, NULL);

  //call the function by passing the do_restore option
  run_cpu(u_cpu, u_i0, u_i1, pebs, npoints, h, end_time,do_restore);
  
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

  print_heatmap("lake_2.out", u_cpu, npoints, h);
  //print_heatmap("lakegpu_f.dat", u_gpu, npoints, h);
  
  //cleanup
  mclose();
  bclose();
  remove(BACK_FILE);
  remove(MMAP_FILE);

  free(u_i0);
  free(u_i1);
  free(pebs);
  free(u_cpu);
//  free(u_gpu);

  return 1;
}

void run_cpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time,int do_restore)
{
  //double *un, *uc, *uo;
  //double t, dt;
  double dt;
  //if it is not restore session then allocate the memory for variables
  if(!do_restore){
    un = (double*)malloc(sizeof(double) * n * n);
    uc = (double*)malloc(sizeof(double) * n * n);
    uo = (double*)malloc(sizeof(double) * n * n);

    memcpy(uo, u0, sizeof(double) * n * n);
    memcpy(uc, u1, sizeof(double) * n * n);

    t = 0.;
  }//end of if do_restore
  dt = h / 2.;
  while(1)
  {
    //evolve(un, uc, uo, pebbles, n, h, dt, t);
    evolve9pt(un, uc, uo, pebbles, n, h, dt, t);
    memcpy(uo, uc, sizeof(double) * n * n);
    memcpy(uc, un, sizeof(double) * n * n);
    //printf(" Iteration for t = %f\n",t);
    if(!tpdt(&t,dt,end_time)) break;
    //take a backup after each iteration
    backup();
  }
  
  memcpy(u, un, sizeof(double) * n * n);
}

void init_pebbles(double *p, int pn, int n)
{
  int i, j, k, idx;
  int sz;

  //srand( time(NULL) );
  srand(1000);
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

  for(i = 0; i < n ; i++)
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
                    uc[idx + n] + uc[idx - n] +0.25*(uc[idx-n+1]+uc[idx+n-1]+uc[idx-n-1]+uc[idx+n+1])  - 5 * uc[idx])/(h * h) + f(pebbles[idx],t));
      }
    }
  }
}

void print_heatmap(const char *filename, double *u, int n, double h)
{
  int i, j, idx;

  FILE *fp = fopen(filename, "w");  

  for( i = 0; i < n; i++ )
  {
    for( j = 0; j < n; j++ )
    {
      idx = j + i * n;
      fprintf(fp, "%f %f %f\n", i*h, j*h, u[idx]);
    }
  }
  
  fclose(fp);
} 
