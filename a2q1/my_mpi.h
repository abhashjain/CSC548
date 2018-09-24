#ifndef MY_MPI
#define MY_MPI
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include<time.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<errno.h>

//#define MPI_Status "int"
typedef struct _MPI_Status {
  int count;
  int cancelled;
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
} MPI_Status, *PMPI_Status;

typedef int MPI_Datatype;
#define MPI_CHAR           ((MPI_Datatype)1)
#define MPI_Comm int
#define NODEFILE "nodefile.txt"
#define PORTFILE "port.txt"
#define BUFFER_SIZE 1024
#define HOST_LENGTH 256

//#define MPI_Datatype char*
//#define MPI_CHAR char
//#define MPI_DOUBLE sizeof(double)

#define DEBUG_LOG(str) debug_logger(str)

void debug_logger(char *str);

int MPI_COMM_WORLD;

struct mpi_data {
	char hostName[HOST_LENGTH];
	int portNumber;
	int total_process;
	int my_rank;
	char buf[BUFFER_SIZE];
	char **all_host_table;
	int *port_table;
};

struct mpi_data *m_data;
char log_buf[HOST_LENGTH];
int server_fd;
struct sockaddr_in server_addr;

int MPI_Init( int *argc, char ***argv );
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status);
int MPI_Finalize( void );
int MPI_Comm_rank( MPI_Comm comm, int *rank );
int MPI_Comm_size( MPI_Comm comm, int *size );


#endif
