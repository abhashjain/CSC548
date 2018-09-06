//Author : Abhash Jain (ajain28), Abhishek Kumar Srivastava (asrivas3)
// CSC548 - Assignment-1 - Question 3 
#include<stdio.h>
#include<string.h>
#include"mpi.h"
#include<math.h>
#include<stdlib.h>
#define XI 1
#define XF 100

double fn(double);

//Print the output to a *.dat file
void print_data(int np,double *xc,double *yc,double *dyc){
	int i;
	char filename[256];
	sprintf(filename,"fn-%d.dat",np);
	FILE *fp = fopen(filename,"w");
	if(fp==NULL){
		printf("Error:File open failed!\n");
	}
	for(i=0;i<np;i++){
		fprintf(fp,"%f %f %f\n",xc[i],yc[i],dyc[i]);
	}
	fclose(fp);
}

//main Function
int main(int argc,char *argv[]){
	//First argument to program will be GRID Size 
	//2nd argument will be option to gather information
	int my_rank;
	int p2p,gt;
	int NGRID;
	int number_of_nodes;
	MPI_Init(&argc,&argv);
	MPI_Status status;
	MPI_Request request,req1,req2;
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&number_of_nodes);
	if(argc>3){
		NGRID = atoi(argv[1]);
		p2p = atoi(argv[2]);
		gt = atoi(argv[3]);
	}else{
		printf("<usage>: <exe> <GRID-SIZE> <p2p mode> <gather mode>\n");
		return 0;
	}
	int i;
	int rem = ceil((double)NGRID/number_of_nodes);
	int index = rem;
	if(my_rank == number_of_nodes - 1)
		rem = NGRID - ((number_of_nodes-1)*(ceil((double)NGRID/number_of_nodes)));
	double *xc = (double *) malloc(sizeof(double)*(rem+2));
	double dx;
	int imin,imax;
	double *yc,*dyc;
	imin = 1;
	imax = number_of_nodes;
	double *xarr=NULL,*yarr=NULL,*dyarr=NULL;
	if(my_rank ==0){
		xarr = (double *) malloc(sizeof(double)*(NGRID+2));
		yarr = (double *) malloc(sizeof(double)*(NGRID+2));
		dyarr = (double *) malloc(sizeof(double)*(NGRID+2));
	}
	for(i=1;i<=rem;i++){
		//my_rank*number_of_nodes+i+my_rank-1
		xc[i] = XI + (XF-XI) *(double)(my_rank*index+i-1)/(double)(NGRID-1);
	}
	dx = xc[2]-xc[1];
	//to calculate the boundry x values on end nodes
	xc[0]=xc[1]-dx;
	xc[rem+1] = xc[rem]+dx; 
	/*for(i=1;i<=rem;i++){
		printf("rank %d number count is %d: %lf\n",my_rank,i,xc[i]);
	}
	printf("\n");
	*/
	//allocate function array
	yc = (double *) malloc(sizeof(double)*(rem+2));
	dyc = (double *)malloc(sizeof(double)*(rem+2));
	//define the function
	for(i=1;i<=rem;i++){
		yc[i] = fn(xc[i]);
	}
	//set boundry values from neigbouring nodes
	if(my_rank==0){
		MPI_Send(&(yc[rem]),1,MPI_DOUBLE,my_rank+1,my_rank,MPI_COMM_WORLD);
		yc[0] = fn(xc[0]);
		MPI_Recv(&(yc[rem+1]),1,MPI_DOUBLE,my_rank+1,my_rank+1,MPI_COMM_WORLD,&status);
	} else if(my_rank==number_of_nodes-1){
		yc[rem+1] = fn(xc[rem+1]);
		MPI_Send(&(yc[1]),1,MPI_DOUBLE,my_rank-1,my_rank,MPI_COMM_WORLD);
		MPI_Recv(&(yc[0]),1,MPI_DOUBLE,my_rank-1,my_rank-1,MPI_COMM_WORLD,&status);
	} else{
		MPI_Send(&(yc[1]),1,MPI_DOUBLE,my_rank-1,my_rank,MPI_COMM_WORLD);
		MPI_Recv(&(yc[0]),1,MPI_DOUBLE,my_rank-1,my_rank-1,MPI_COMM_WORLD,&status);
		MPI_Send(&(yc[rem]),1,MPI_DOUBLE,my_rank+1,my_rank,MPI_COMM_WORLD);
		MPI_Recv(&(yc[rem+1]),1,MPI_DOUBLE,my_rank+1,my_rank+1,MPI_COMM_WORLD,&status);	
	}
	for(i=1;i<=rem;i++){
		dyc[i] = ((yc[i+1]-yc[i-1])/(2.0*dx));
	}
	/*for(i=0;i<=rem+1;i++){
		printf("rank %d number count is %d: yc: %lf, dyc: %lf \n",my_rank,i,yc[i],dyc[i]);
	}
	printf("\n");
	*/
	int count = index > rem ? index: rem; 
	
	if(gt==0){ //gather function
		MPI_Gather(&(xc[1]),count,MPI_DOUBLE,xarr,count,MPI_DOUBLE,0,MPI_COMM_WORLD);
		MPI_Gather(&(yc[1]),count,MPI_DOUBLE,yarr,count,MPI_DOUBLE,0,MPI_COMM_WORLD);
		MPI_Gather(&(dyc[1]),count,MPI_DOUBLE,dyarr,count,MPI_DOUBLE,0,MPI_COMM_WORLD);
	} else if(gt==1) { //Manual Gather
		if(p2p==0){  //p2p blocking
			if(my_rank==0){	//recv the data
				memcpy(xarr,&(xc[1]),sizeof(double)*index);
				memcpy(yarr,&(yc[1]),sizeof(double)*index);
				memcpy(dyarr,&(dyc[1]),sizeof(double)*index);
				for(int i=1;i<number_of_nodes;i++){
					if(i==number_of_nodes-1)
						count = rem;
					else 
						count = index;
					MPI_Recv(&(xarr[i*index]),count,MPI_DOUBLE,i,0,MPI_COMM_WORLD,&status);
					MPI_Recv(&(yarr[i*index]),count,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&status);
					MPI_Recv(&(dyarr[i*index]),count,MPI_DOUBLE,i,2,MPI_COMM_WORLD,&status);
				}
			} else { //use send 
				if(my_rank==number_of_nodes-1)
             		count = rem;
				else
		            count = index;
				MPI_Send(&(xc[1]),count,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
				MPI_Send(&(yc[1]),count,MPI_DOUBLE,0,1,MPI_COMM_WORLD);
				MPI_Send(&(dyc[1]),count,MPI_DOUBLE,0,2,MPI_COMM_WORLD);
			}
		} else if(p2p==1) { //p2p non-blocking
			if(my_rank==0){ //recv the data
				memcpy(xarr,&(xc[1]),sizeof(double)*index);
				memcpy(yarr,&(yc[1]),sizeof(double)*index);
				memcpy(dyarr,&(dyc[1]),sizeof(double)*index);
				for(int i=1;i<number_of_nodes;i++){
					if(i==number_of_nodes-1)
						count = rem;
					else 
						count = index;
					MPI_Irecv(&(xarr[i*index]),count,MPI_DOUBLE,i,0,MPI_COMM_WORLD,&request);
					MPI_Irecv(&(yarr[i*index]),count,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&req1);
					MPI_Irecv(&(dyarr[i*index]),count,MPI_DOUBLE,i,2,MPI_COMM_WORLD,&req2);
					//As MPI_Irecv are non-blocking call,so we have to wait to recv the data
					MPI_Wait(&request,&status);
					MPI_Wait(&req1,&status);
					MPI_Wait(&req2,&status);
			}
			} else { //send the data
				if(my_rank==number_of_nodes-1)
             		count = rem;
				else
		            count = index;
				MPI_Isend(&(xc[1]),count,MPI_DOUBLE,0,0,MPI_COMM_WORLD,&request);
				MPI_Isend(&(yc[1]),count,MPI_DOUBLE,0,1,MPI_COMM_WORLD,&req1);
				MPI_Isend(&(dyc[1]),count,MPI_DOUBLE,0,2,MPI_COMM_WORLD,&req2);
				MPI_Wait(&request,&status);
				MPI_Wait(&req1,&status);
				MPI_Wait(&req2,&status);
		
			}
		}
	}
	free(xc);
	if(my_rank==0){
		/*for(int i=0;i<NGRID;i++){
			printf("%lf\t" ,xarr[i]);	
		}
		printf("\n");
		for(int i=0;i<NGRID;i++){
			printf("%lf\t" ,yarr[i]);	
		}
		printf("\n");
		for(int i=0;i<NGRID;i++){
			printf("%lf\t" ,dyarr[i]);	
		}*/
		print_data(NGRID,xarr,yarr,dyarr);
		free(xarr);
		free(yarr);
		free(dyarr);
	}
	MPI_Finalize();
	return 0;
}
