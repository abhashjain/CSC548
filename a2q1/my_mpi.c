#include "my_mpi.h"

char * print_time(){
    int size = 0;
    time_t t;
    char *buf;
     
    t=time(NULL); /* get current calendar time */    
    char *timestr = asctime( localtime(&t) );
    timestr[strlen(timestr) - 1] = 0;  //Getting rid of \n
    size = strlen(timestr)+ 1 + 2; //Additional +2 for square braces
    buf = (char*)malloc(sizeof(char)*size);
    memset(buf, 0x0, size);
    snprintf(buf,size,"[%s]", timestr);
    return buf;
}

void debug_logger(char *str) {
	FILE *fp;
	fp = fopen("debug_log.txt","a+");
	char *pt = print_time();
	fprintf(fp,"[%s]:%s : %s\n","DEBUG",pt,str);
	free(pt);
	fclose(fp);
}

int hostname_to_ip(char * hostname , char* ip) 
{
	hostname[strcspn(hostname,"\n")] = '\0';
	//sprintf(log_buf,"Abhash:%s my_rank %d and hostname is %s",__FUNCTION__, m_data->my_rank,hostname);
	//printf("Abhash:%s my_rank %d and hostname is %s",__FUNCTION__, m_data->my_rank,hostname);
	//fflush(stdout);
	//sprintf(log_buf,"abhash host is %s!",hostname);
	//DEBUG_LOG(log_buf);
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
  
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {   
        /* get the host info*/
        herror("abhash:gethostbyname");
        return 1;
    } 
    addr_list = (struct in_addr **) he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) 
    {   
        /*Return the first one;*/
        strcpy(ip , inet_ntoa(*addr_list[i]) );
		//sprintf(log_buf,"aj ip is %s!",ip);
		//DEBUG_LOG(log_buf);
        return 0;
    }       
    return 1;
}


int MPI_Init( int *argc, char ***argv ){
	FILE *fp;
	m_data = (struct mpi_data*) malloc(sizeof(struct mpi_data));
	m_data->portNumber = atoi(argv[0][4]);
	m_data->total_process = atoi(argv[0][3]);
	m_data->my_rank= atoi(argv[0][2]);
	char buf[HOST_LENGTH];
	char *line = NULL;
	size_t len =0;
	ssize_t read;
	hostname_to_ip(argv[0][1],buf);
	strcpy(m_data->hostName,buf);
	m_data->all_host_table = (char **)malloc(sizeof(char *)*m_data->total_process);
	//store the rank to IP address mapping
	for(int i=0;i<m_data->total_process;i++){
		m_data->all_host_table[i] = (char *)malloc(sizeof(char) * HOST_LENGTH);
	}
	sprintf(log_buf,"port number %d my_rank %d total_process %d my_ip %s",m_data->portNumber,m_data->my_rank,m_data->total_process,m_data->hostName);
	DEBUG_LOG(log_buf);
	fp = fopen(NODEFILE,"r");
	if(fp==NULL){
		sprintf(log_buf,"%s : File %s is not open failed!",__FUNCTION__,NODEFILE);
		DEBUG_LOG("File is not opened\n");
		return 0;
	}
	int i=0;
	while((read = getline(&line, &len, fp)) != -1){
		line[strcspn(line,"\n")] = '\0';
		hostname_to_ip(line,buf);
		sprintf(log_buf,"%s : read line is %s and hostname is %s!",__FUNCTION__,line,buf);
		DEBUG_LOG(line);
		strcpy(m_data->all_host_table[i++],buf);
	}
	fclose(fp);
	fp = fopen(PORTFILE,"r");
	if(fp==NULL){
		debug_logger("Port file doesn't exist\n");
		return 0;
	}
	i=0;
	//same thing to do for port entry
	m_data->port_table = (int*)malloc(sizeof(int) * m_data->total_process);
	while((read = getline(&line, &len, fp)) != -1){
		line[strcspn(line,"\n")] = '\0';
		debug_logger(line);
		m_data->port_table[i++] = atoi(line);
	}
	fclose(fp);

	//create a new server socket and set up till listen mode
	int opt =1;
	if((server_fd = socket(AF_INET,SOCK_STREAM, 0))==0){
		sprintf(log_buf,"ERROR: socket creation failed for %d\n",m_data->my_rank);
		DEBUG_LOG(log_buf);
		return -1;
	}
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                &opt, sizeof(opt))) 
    {   
        sprintf(log_buf,"ERROR: OPtion set failed for %d\n",m_data->my_rank);
        DEBUG_LOG(log_buf);
        return -1; 
    }
	server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons( m_data->port_table[m_data->my_rank] );
    if (bind(server_fd, (struct sockaddr *)&server_addr,sizeof(server_addr))<0) 
    {   
        sprintf(log_buf,"ERROR: bind failed for %d\n",m_data->my_rank);
        DEBUG_LOG(log_buf);
        return -1; 
    }   
    if (listen(server_fd, 100) < 0)  
    {   
        sprintf(log_buf,"ERROR: listen failed for %d\n",m_data->my_rank);
        DEBUG_LOG(log_buf);
        return -1;
    }
	sprintf(log_buf,"%s: Done for rank %d",__FUNCTION__,m_data->my_rank);
	DEBUG_LOG(log_buf);
	return 1;
}

int MPI_Comm_rank( MPI_Comm comm, int *rank ){
	*rank = m_data->my_rank;
	return 0;
}

int MPI_Comm_size( MPI_Comm comm, int *size ){
	*size = m_data->total_process;
	return 0;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm){
	debug_logger("Entered in MPI_SEND");
	
	int sock = 0; 
    struct sockaddr_in client_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)  
    {
		sprintf(log_buf,"ERROR: Send: socket creation failed for rank %d\n",m_data->my_rank);   
        DEBUG_LOG(log_buf);
		return -1; 
    }   
    memset(&client_addr, '0', sizeof(client_addr)); 
    client_addr.sin_family = AF_INET; 
    client_addr.sin_port = htons(m_data->port_table[dest]); 
    
    if(inet_pton(AF_INET,m_data->all_host_table[dest] , &client_addr.sin_addr)<=0) 
    {   
		sprintf(log_buf,"ERROR: %s: Invalid address %d\n",__FUNCTION__,m_data->my_rank);   
        DEBUG_LOG(log_buf);
        return -1; 
    }   

    if (connect(sock,(struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)  
    {   
		sprintf(log_buf,"ERROR: %s: connection failed %d\n",__FUNCTION__,m_data->my_rank);   
        DEBUG_LOG(log_buf);
        return -1; 
    }
	send(sock,buf,count,0);
	char ack;
	read(sock,&ack,1);
	if(ack!='y'){
		sprintf(log_buf, "Error: Send: ACK failed for %d!",m_data->my_rank);
		DEBUG_LOG(log_buf);
		return -1;
	}
	sprintf(log_buf,"Send is successful for rank %d\n",m_data->my_rank);
	DEBUG_LOG(log_buf);
	return 0;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status){
	DEBUG_LOG("Inside MPI_Recv");
	int client_socket;
	int addrlen = sizeof(server_addr);
	if ((client_socket = accept(server_fd, (struct sockaddr *)&server_addr,(socklen_t*)&addrlen))<0)
    {   
        sprintf(log_buf,"ERROR:accept failed for %d\n",m_data->my_rank);
        DEBUG_LOG(log_buf);
        return -1; 
    }   
    DEBUG_LOG("ready to send data!");
    //TODO
    read(client_socket,buf,count);
	DEBUG_LOG("MPI_Recv: msg read!");
	char ack = 'y';
	send(client_socket,&ack,1,0);
	DEBUG_LOG("MPI_Recv: Ack Send!");
	close(client_socket);
	return 0;
}

int MPI_Finalize( void ){
	close(server_fd);
	for(int i=0;i<m_data->total_process;i++){
		free(m_data->all_host_table[i]);
	}
	free(m_data->port_table);
	free(m_data);
	return 0;
}

