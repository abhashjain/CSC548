//Author : 
//1. ajain28 Abhash Jain
//2. asrivas3 Abhishek Kumar Srivastava
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
	MPI_Request request,req1,req2,req3,req4;
	double start,end,final;
	
	//Measure the time
	start = MPI_Wtime(); //start the timer
	
	
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
	int rem = floor((double)NGRID/number_of_nodes);
	int index = rem;	//number of data given to each nodes
	if(my_rank == number_of_nodes - 1)
		rem = NGRID - ((number_of_nodes-1)*(floor((double)NGRID/number_of_nodes)));
	double *xc = (double *) malloc(sizeof(double)*(rem+2));
	double dx;
	int imin,imax;
	double *yc,*dyc;
	imin = 1;
	imax = number_of_nodes;
	double *xarr=NULL,*yarr=NULL,*dyarr=NULL;
	if(my_rank ==number_of_nodes-1){
		xarr = (double *) malloc(sizeof(double)*(NGRID+2));
		yarr = (double *) malloc(sizeof(double)*(NGRID+2));
		dyarr = (double *) malloc(sizeof(double)*(NGRID+2));
	}
	for(i=1;i<=rem;i++){
		xc[i] = XI + (XF-XI) *(double)(my_rank*index+i-1)/(double)(NGRID-1);
	}
	//To Handle the case when every node get 1 grid value then calculating the
	//dx value become a corner case, so we handle it by calculating the dx on 
	//last rank node and distribute the calculate value to every node from 0 to n-2
	if(my_rank==number_of_nodes-1){
		double a1 =  XI + (XF-XI) *(double)(my_rank*index+1-1)/(double)(NGRID-1);;
		double a2 =  XI + (XF-XI) *(double)(my_rank*index+2-1)/(double)(NGRID-1);;
		dx = a2 - a1;
	}
	//Broadcast dx value to all nodes from last node
	MPI_Bcast(&dx,1,MPI_DOUBLE,number_of_nodes-1,MPI_COMM_WORLD);

	//to calculate the boundry x values on end nodes
	xc[0]=xc[1]-dx;
	xc[rem+1] = xc[rem]+dx; 
	
	/*for(i=0;i<=rem+1;i++){
		printf("rank %d number count is %d: %lf\n",my_rank,i,xc[i]);
	}
	printf("\n");*/
	
	//allocate function array to store yc and dericvative value
	yc = (double *) malloc(sizeof(double)*(rem+2));
	dyc = (double *)malloc(sizeof(double)*(rem+2));
	//define the function
	for(i=1;i<=rem;i++){
		yc[i] = fn(xc[i]);
	}
	//Get boundry values from neigbouring nodes
	if(number_of_nodes > 1 ){
	if(my_rank==0){
		//On Rank 0 node we calculate the 1st value and transfer the last value to next node
		//and recv next node 1st value in last index
		yc[0] = fn(xc[0]);
		if (p2p == 0) //if blocking option then use blocking send and recv fn for corner value
		{
			MPI_Send(&(yc[rem]), 1, MPI_DOUBLE, my_rank + 1, my_rank, MPI_COMM_WORLD);
			MPI_Recv(&(yc[rem + 1]), 1, MPI_DOUBLE, my_rank + 1, my_rank + 1, MPI_COMM_WORLD, &status);
		}
		else
		{
			MPI_Isend(&(yc[rem]), 1, MPI_DOUBLE, my_rank + 1, my_rank, MPI_COMM_WORLD,&req1);
			MPI_Irecv(&(yc[rem + 1]), 1, MPI_DOUBLE, my_rank + 1, my_rank + 1, MPI_COMM_WORLD, &req2);
			MPI_Wait(&req1, &status);
			MPI_Wait(&req2, &status);
		}
	} else if(my_rank==number_of_nodes-1){ //transport boundry value on last node
		yc[rem+1] = fn(xc[rem+1]);
		if (p2p == 0)
		{
			MPI_Send(&(yc[1]), 1, MPI_DOUBLE, my_rank - 1, my_rank, MPI_COMM_WORLD);
			MPI_Recv(&(yc[0]), 1, MPI_DOUBLE, my_rank - 1, my_rank - 1, MPI_COMM_WORLD, &status);
		}
		else
		{
			MPI_Isend(&(yc[1]), 1, MPI_DOUBLE, my_rank - 1, my_rank, MPI_COMM_WORLD,&req1);
			MPI_Irecv(&(yc[0]), 1, MPI_DOUBLE, my_rank - 1, my_rank - 1, MPI_COMM_WORLD, &req2);
			MPI_Wait(&req1, &status);
			MPI_Wait(&req2, &status);
		}
	} else{ //transport boundry value on mid nodes according to blocking and non-blocking option
		if (p2p == 0)	//Blocking send and recv
		{
			MPI_Send(&(yc[1]), 1, MPI_DOUBLE, my_rank - 1, my_rank, MPI_COMM_WORLD);
			MPI_Recv(&(yc[0]), 1, MPI_DOUBLE, my_rank - 1, my_rank - 1, MPI_COMM_WORLD, &status);
			MPI_Send(&(yc[rem]), 1, MPI_DOUBLE, my_rank + 1, my_rank, MPI_COMM_WORLD);
			MPI_Recv(&(yc[rem + 1]), 1, MPI_DOUBLE, my_rank + 1, my_rank + 1, MPI_COMM_WORLD, &status);
		}
		else		//non-blocking send and recv
		{
			MPI_Isend(&(yc[1]), 1, MPI_DOUBLE, my_rank - 1, my_rank, MPI_COMM_WORLD,&req1);
			MPI_Irecv(&(yc[0]), 1, MPI_DOUBLE, my_rank - 1, my_rank - 1, MPI_COMM_WORLD, &req2);
			MPI_Isend(&(yc[rem]), 1, MPI_DOUBLE, my_rank + 1, my_rank, MPI_COMM_WORLD,&req3);
			MPI_Irecv(&(yc[rem + 1]), 1, MPI_DOUBLE, my_rank + 1, my_rank + 1, MPI_COMM_WORLD, &req4);
			MPI_Wait(&req1, &status);
			MPI_Wait(&req2, &status);
			MPI_Wait(&req3, &status);
			MPI_Wait(&req4, &status);
		}
	}
	}//End of if for number of nodes
	else {	//there is only one node than calculate boundry values
		yc[0] = fn(xc[0]);
		yc[rem+1] = fn(xc[rem+1]);
	}
	/*for(i=0;i<=rem+1;i++){
		printf("rank %d number count is %d: yc: %lf\n",my_rank,i,yc[i]);
	}
	printf("\n");
	*/
	//Calculate the derivative value from the fn. value
	for(i=1;i<=rem;i++){
		dyc[i] = ((yc[i+1]-yc[i-1])/(2.0*dx));
	}
	/*for(i=0;i<=rem+1;i++){
		printf("rank %d number count is %d: xc: %lf yc: %lf, dyc: %lf \n",my_rank,i,xc[i],yc[i],dyc[i]);
	}
	printf("\n");
	*/
	if(gt==0){ //gather function
		MPI_Gather(&(xc[1]),index,MPI_DOUBLE,xarr,index,MPI_DOUBLE,number_of_nodes-1,MPI_COMM_WORLD);
		MPI_Gather(&(yc[1]),index,MPI_DOUBLE,yarr,index,MPI_DOUBLE,number_of_nodes-1,MPI_COMM_WORLD);
		MPI_Gather(&(dyc[1]),index,MPI_DOUBLE,dyarr,index,MPI_DOUBLE,number_of_nodes-1,MPI_COMM_WORLD);

		//Collect the remaining values from last node in case of uneven distrbution
		if(my_rank== number_of_nodes-1 && NGRID - index*number_of_nodes >0){
			memcpy(&(xarr[index*number_of_nodes]),&(xc[index+1]),(NGRID - index*number_of_nodes)*sizeof(double));
			memcpy(&(yarr[index*number_of_nodes]),&(yc[index+1]),(NGRID - index*number_of_nodes)*sizeof(double));
			memcpy(&(dyarr[index*number_of_nodes]),&(dyc[index+1]),(NGRID - index*number_of_nodes)*sizeof(double));
		}
	} else if(gt==1) { //Manual Gather
		if(p2p==0){  //p2p blocking
			if(my_rank==number_of_nodes-1){	//recv the data on last node
				int i;
				for(i=0;i<number_of_nodes-1;i++){
					MPI_Recv(&(xarr[i*index]),index,MPI_DOUBLE,i,0,MPI_COMM_WORLD,&status);
					MPI_Recv(&(yarr[i*index]),index,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&status);
					MPI_Recv(&(dyarr[i*index]),index,MPI_DOUBLE,i,2,MPI_COMM_WORLD,&status);
				}
				//copy the values from last node to final output array
				memcpy(&(xarr[i*index]),&(xc[1]),sizeof(double)*(NGRID-(index*(number_of_nodes-1))));
				memcpy(&(yarr[i*index]),&(yc[1]),sizeof(double)*(NGRID-(index*(number_of_nodes-1))));
				memcpy(&(dyarr[i*index]),&(dyc[1]),sizeof(double)*(NGRID-(index*(number_of_nodes-1))));
				
			} else { //use send to share the value to last node
				MPI_Send(&(xc[1]),index,MPI_DOUBLE,number_of_nodes-1,0,MPI_COMM_WORLD);
				MPI_Send(&(yc[1]),index,MPI_DOUBLE,number_of_nodes-1,1,MPI_COMM_WORLD);
				MPI_Send(&(dyc[1]),index,MPI_DOUBLE,number_of_nodes-1,2,MPI_COMM_WORLD);
			}
		} else if(p2p==1) { //p2p non-blocking
			if(my_rank==number_of_nodes-1){ //recv the data on last node
			int i;	
			for(i=0;i<number_of_nodes-1;i++){
					MPI_Irecv(&(xarr[i*index]),index,MPI_DOUBLE,i,0,MPI_COMM_WORLD,&request);
					MPI_Irecv(&(yarr[i*index]),index,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&req1);
					MPI_Irecv(&(dyarr[i*index]),index,MPI_DOUBLE,i,2,MPI_COMM_WORLD,&req2);
					//As MPI_Irecv are non-blocking call,so we have to wait to recv the data
					MPI_Wait(&request,&status);
					MPI_Wait(&req1,&status);
					MPI_Wait(&req2,&status);
				}
				//copy the remaing values on last node to final output array
				memcpy(&(xarr[i*index]),&(xc[1]),sizeof(double)*(NGRID-(index*(number_of_nodes-1))));
				memcpy(&(yarr[i*index]),&(yc[1]),sizeof(double)*(NGRID-(index*(number_of_nodes-1))));
				memcpy(&(dyarr[i*index]),&(dyc[1]),sizeof(double)*(NGRID-(index*(number_of_nodes-1))));
			} else { //send the data to last node
				MPI_Isend(&(xc[1]),index,MPI_DOUBLE,number_of_nodes-1,0,MPI_COMM_WORLD,&request);
				MPI_Isend(&(yc[1]),index,MPI_DOUBLE,number_of_nodes-1,1,MPI_COMM_WORLD,&req1);
				MPI_Isend(&(dyc[1]),index,MPI_DOUBLE,number_of_nodes-1,2,MPI_COMM_WORLD,&req2);
				MPI_Wait(&request,&status);
				MPI_Wait(&req1,&status);
				MPI_Wait(&req2,&status);
		
			}
		}
	}
	free(xc); //free memory use by xc array
	if(my_rank==number_of_nodes-1){
		//print the data to file fn-<count>.dat file
		print_data(NGRID,xarr,yarr,dyarr);
		//free the memory used to store the final output
		free(xarr);
		free(yarr);
		free(dyarr);
	}
	//Stop the Timer
	end = MPI_Wtime();
	final = end-start;
	
	if(my_rank==number_of_nodes-1){
		printf("Time Taken %lf Seconds\n",final);
	}
	
	MPI_Finalize();
	return 0;
}//End of Main
//End of Program
