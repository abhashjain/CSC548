/*
 * Author- (ajain28) Abhash Jain
 * CSC548 - Assignment 1 question 2  
 * program that determines the point-to-point message latency for 4 pairs of nodes
 *
 *
 */
#include<math.h>
#include"my_mpi.h"

//function to sum the array
double sum(double *time,int count){
	double result =0;
	for(int i=0;i<count;i++){
		result+=time[i];
	}
	return result;
}
//Function to calculate the standard deviation
double SD(double *time, int count){
	double s =0.0,mean, sd=0.0;
	int i;
	s = sum(time,count);
	mean = s/count;
	for(i=0;i<count;i++){
		sd += pow(time[i]-mean,2); 
	}
	return sqrt(sd/count);
}

int main(int argc,char **argv){
	int my_rank; //rank of the process
    int p; //number of process
    int source;  //source process
    int dest;    //dest process
    int tag=50;  // tag number
	struct timeval start,end,final;
	MPI_Status status;
    MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&p);
	//If there are less than 8 process than retun the error
	if(p!=8){
		printf("Error: Less number of process %d!\n",p);
		return 0;
	}
    int msg_size[] = {32,64,128,256,512,1024,1024*2,1024*4,1024*8,1024*16,1024*32,1024*64,1024*128,1024*256,1024*512,1024*1024,1024*1024*2};
	//For loop to iterate over all the msg_size
	for(int k=0;k<10;k++){
	int msg_sz = msg_size[k];
	char *message = (char *) calloc(msg_sz,sizeof(char)) ;
	memset(message,'a',sizeof(char)*msg_sz);
	if(my_rank<4){
		MPI_Send(message,msg_sz+1,MPI_CHAR,my_rank+4,tag,MPI_COMM_WORLD);
		//printf("Rachana Jain: mesage is %s rank is %d\n",message,my_rank);
		//fflush(stdout);
		MPI_Recv(message,msg_sz+1,MPI_CHAR,my_rank+4,tag,MPI_COMM_WORLD,&status);
		//printf("Akasj Jain:  MPI_Send sucess is recieved for rank %d\n",my_rank);
		//fflush(stdout);
	} else {
		MPI_Recv(message,msg_sz+1,MPI_CHAR,my_rank-4,tag,MPI_COMM_WORLD,&status);
		//printf("Before:!!!!!!!!message is recv on %d\n",my_rank);
		//fflush(stdout);
		MPI_Send(message,msg_sz+1,MPI_CHAR,my_rank-4,tag,MPI_COMM_WORLD);
		//printf("After!***** Confirmation send for recv on %d\n",my_rank);
		//fflush(stdout);
	}
	/*
 * 	On Node 0 we send the data to  Node 4
 * 		Node1   -> Node 5
 * 		Node2 ---> Node 6
 * 		Node 3 --Send--> Node 7*/
	if(my_rank==0){
		char buf[50];
		double time[10];
		char time_p[4][50];
		//For loop to run for 10 iteration
		for(int i=0;i<=10;i++){
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD);
            	MPI_Recv(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD);
			MPI_Recv(message,msg_sz+1,MPI_CHAR,4,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1] = final.tv_sec +(double)final.tv_usec/1000000.0 ;
		}
			//calculate the median and standard deviation for 10 iteration
			double t = sum(time,10);
			t/=10;
			double sd = SD(time,10);
			sprintf(buf,"%e %e",t,sd);
			strcpy(time_p[0],buf);
			//Collect the Data from other 4 nodes to print the result.
			for(int i=1;i<4;i++){
				MPI_Recv(buf,50,MPI_CHAR,i,tag,MPI_COMM_WORLD,&status);
				strcpy(time_p[i],buf);
			}
			printf("%d %s %s %s %s\n",msg_sz,time_p[0],time_p[1],time_p[2],time_p[3]);
			fflush(stdout);
	} else if(my_rank==1){
		double time[10]={0.0};
		char buf[50];
		for(int i=0;i<=10;i++) {
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD);
	            MPI_Recv(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD);
        	MPI_Recv(message,msg_sz+1,MPI_CHAR,5,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1] = final.tv_sec +(double)final.tv_usec/1000000.0 ;
		}
		double t = sum(time,10);
		t/=10;
		double sd =SD(time,10);
		sprintf(buf," %e %e",t,sd);
		//send mean and std dev to p0
		MPI_Send(buf,50,MPI_CHAR,0,tag,MPI_COMM_WORLD);
	} else if(my_rank==2){
		double time[10]={0.0};
		char buf[50];
		for(int i=0;i<=10;i++){
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,6,tag,MPI_COMM_WORLD);
       			MPI_Recv(message,msg_sz+1,MPI_CHAR,6,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,strlen(message)+1,MPI_CHAR,6,tag,MPI_COMM_WORLD);
       		MPI_Recv(message,strlen(message)+1,MPI_CHAR,6,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1]=final.tv_sec +(double)final.tv_usec/1000000.0 ;
		}
		double t = sum(time,10);
		t/=10;
		double sd = SD(time,10);
		sprintf(buf," %e %e",t,sd);
		//send mean and std deviaation to Node 0
		MPI_Send(buf,50,MPI_CHAR,0,tag,MPI_COMM_WORLD);
	} else if(my_rank == 3 ){
		double time[10]={0.0};
		char buf[50];
		for(int i=0;i<=10;i++){
			if(i==0){
				MPI_Send(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD);
   				MPI_Recv(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD,&status);
				continue;
			}
			gettimeofday(&start,NULL);
			MPI_Send(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD);
   			MPI_Recv(message,msg_sz+1,MPI_CHAR,7,tag,MPI_COMM_WORLD,&status);
			gettimeofday(&end,NULL);
			timersub(&end,&start,&final);
			time[i-1]=final.tv_sec +(double)final.tv_usec/1000000.0 ;
		}
		double t = sum(time,10);
		t=t/10;
		double sd  = SD(time,10);
		sprintf(buf," %e %e",t,sd);
		//send the information to node 0
		MPI_Send(buf,50,MPI_CHAR,0,tag,MPI_COMM_WORLD);
	} else if(my_rank==4){
		//recv the data from node 0 and send the same data to Node 0
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,0,tag,MPI_COMM_WORLD,&status);
	//		printf("Abhash Jain: Data received is %s for rank %d\n",message,my_rank);
	//		fflush(stdout);
			MPI_Send(message,msg_sz+1,MPI_CHAR,0,tag,MPI_COMM_WORLD);
		}
	} else if(my_rank==5){
		//recv the data from node 1 and send the same data to node 1
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,1,tag,MPI_COMM_WORLD,&status);
			MPI_Send(message,msg_sz+1,MPI_CHAR,1,tag,MPI_COMM_WORLD);
		}
	} else if(my_rank==6){
		// recv the data from node 2 and send the same data to node 2
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,2,tag,MPI_COMM_WORLD,&status);
			MPI_Send(message,msg_sz+1,MPI_CHAR,2,tag,MPI_COMM_WORLD);
		}
	} else if(my_rank==7){
		for(int i=0;i<=10;i++){
			MPI_Recv(message,msg_sz+1,MPI_CHAR,3,tag,MPI_COMM_WORLD,&status);
			MPI_Send(message,msg_sz+1,MPI_CHAR,3,tag,MPI_COMM_WORLD);
		}
	}
	free(message);
	}//End of msg size for loop
	MPI_Finalize();
	return 0;
}
