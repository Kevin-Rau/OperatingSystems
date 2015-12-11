//Kevin Rau
//Worked with Ian Moore


#include "multi-lookup.h"
#include "queue.h"

#define MINARGS 3 
#define USAGE "<inputFilePath> <outputFilePath>" 
#define MAX_NAME_LENGTH 1025 
#define MIN_RESOLVER_THREADS 2
#define MAX_INPUT_FILES 10
#define SBUFSIZE 1025 
#define INPUTFS "%1024s"
#define MAX_RESOLVER_THREADS 10 
#define QUEUE_SIZE 50 
#define MAX_IP_LENGTH INET6_ADDRSTRLEN


typedef struct requestParams{
    FILE* file_name; 
    pthread_mutex_t* queueL; 
    queue* q; 
} input;


typedef struct resolverParams{
    FILE* file_name;
    pthread_mutex_t* queueL;
    pthread_mutex_t* outL; 
    queue* q; 
    int* files;
} output;

void* requesterThread(void* p) 
{
	char hostname[MAX_NAME_LENGTH]; //grab our hostname 
	//pull out all parameters from our struct
	input* params = p;
	FILE* fName= params->file_name;
	pthread_mutex_t* queuelock = params-> queueL; 
	queue* shared= params->q;
	char* payload; 
	int complete = 0;
	int errorc = 0;
	
	while(fscanf(fName, INPUTFS, hostname) > 0){
		while(!complete)
		{ 		
			errorc = pthread_mutex_lock(queuelock);//Catch the pthread lock error
			if(errorc){//error handeling
				fprintf(stderr, "Queue Mutex lock error %d\n",errorc); 
			}
			if(queue_is_full(shared)){//if the queue is full and we can't write
				errorc=pthread_mutex_unlock(queuelock); //unlock the mutex
				if(errorc){//error handling
					fprintf(stderr,"Queue Mutex unlock error %d\n", errorc);
				}				

				usleep((rand()%100)*1000);//sleep for a random amount of time. 
			}
			else {
				payload=malloc(MAX_NAME_LENGTH); //Malloc some space on the heap
				if(payload==NULL) { //error handling
					fprintf(stderr, "Malloc returned an error\n");
					fprintf(stderr, "Warning results non-deterministic\n");
				}
				//Loads the line into the payload
				payload=strncpy(payload, hostname, MAX_NAME_LENGTH); 
				if(queue_push(shared,payload)==QUEUE_FAILURE){
					fprintf(stderr, "Queue Push error\n");
				}
				//Error handling for unlocking the mutex
				errorc=pthread_mutex_unlock(queuelock);
				if(errorc){
					fprintf(stderr,"Queue Mutex unlock error %d\n", errorc);
				} 
				// We have successfully written to the queue 
				complete=1;
			}
		}
		//Reset write success for the next line
		complete = 0;
	}
	//Close the input file- check for errors
	if(fclose(fName)){
		fprintf(stderr, "Error closing input file \n");
	}
	return NULL;
}
void* resolverThread(void* p) {
	output* params = p;
	//Pulls data out of the parameters 
	FILE* outfile= params->file_name;
	pthread_mutex_t* queuelock = params-> queueL; 
	pthread_mutex_t* filelock = params->outL;
	queue* shared= params->q;
	int* files= params->files;
	//Sets up the reader information from the queue 
	char* payload;
	char ipaddress[INET6_ADDRSTRLEN];
	//Dummy variable to catch error codes
	int errorc=0;
	//As long as there are still writer threads and the queue is not empty 
	while(*files || !queue_is_empty(shared)){
		//Try locking the mutex
		errorc=pthread_mutex_lock(queuelock);
		if(errorc){
			fprintf(stderr, "Queue Mutex lock error %d\n",errorc); 
		}
		//Get something off the queue 
		payload=queue_pop(shared);
		//if the queue is empty or something wrong was pushed on it
		if(payload==NULL)
		{
			//Unlock the mutex, and handle any errors 
			errorc=pthread_mutex_unlock(queuelock);
			if(errorc){
				fprintf(stderr, "Queue Mutex unlock error %d\n", errorc);
			}		
			usleep((rand()%100)*1000);
		}
		//We have data to work with 
		else {
			//Let go of the queue
			errorc = pthread_mutex_unlock(queuelock);
			if(errorc){
				fprintf(stderr, "Queue Mutex unlock error %d\n", errorc);
			}		
			//Look up the ipaddress from the string given by the queue
			if(dnslookup(payload, ipaddress, sizeof(ipaddress))
			 	 == UTIL_FAILURE){ //Handle errors 
                		fprintf(stderr, "dnslookup error: %s\n", payload);
                		strncpy(ipaddress, "", sizeof(ipaddress));
			}		
			//Lock the file mutex and prepare to write
			errorc=pthread_mutex_lock(filelock);
			if(errorc){
				fprintf(stderr, "File Mutex lock error %d\n", errorc);
			}
			//If we fail to output something catch it otherwise write to file 
			errorc=fprintf(outfile,"%s,%s\n", payload, ipaddress);
			if(errorc<0){
				fprintf(stderr, "Output file write error\n");
			}			
			//unlock the file mutex and handle errors
			errorc=pthread_mutex_unlock(filelock);
			if(errorc){
				fprintf(stderr, "File Mutex unlock error %d\n", errorc);
			}
			//Free the space allocated by the payload 
			free(payload);
			//For safety set payload to null
			payload=NULL;
		}

	}
	return NULL;
}
int main(int argc, char* argv[]){ 

	/*variables*/
	int input_files = argc-2; 
	int i;
	int t;
	int rc; 
	int files=1;

	
	//Makes an array of input files so that there is no race condition for the reader threads
	FILE* inputFiles[input_files]; 
	FILE* outputfile=NULL; 
	//Makes an array of thread trackers to keep track of the spawned input and output threads
	
	pthread_t ithreads[input_files];
	pthread_t othreads[MAX_RESOLVER_THREADS];

	pthread_mutex_t queuelock; //make mutexs here 
	pthread_mutex_t filelock;
	queue q;

	//create structs
	input inparameters[input_files];
	output outparameters[MAX_RESOLVER_THREADS];
	
	
	/*check arguments*/
	if(argc < MINARGS)
	{
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}
	// Error Handling for the queue creation
	if(queue_init(&q, QUEUE_SIZE)==QUEUE_FAILURE) 
	{
		fprintf(stderr,"Error creating the Queue\n");
		return EXIT_FAILURE; 
	}
	
	//Error handeling for the queue mutex creation
	rc = pthread_mutex_init(&queuelock,NULL);
	if(rc){ 
		fprintf(stderr, "Error creating the Queue Mutex\n");
		fprintf(stderr, "Error No: %d\n",rc);  
		return EXIT_FAILURE; 
	}
	//Error handeling for the file mutex creation
	rc=pthread_mutex_init(&filelock,NULL);
	if(rc){ 
		fprintf(stderr, "Error creating the output file Mutex\n");
		fprintf(stderr, "Error No: %d\n",rc);  
		return EXIT_FAILURE; 
	}
	
	//Opens the output file and handles any errors
	outputfile = fopen(argv[(argc-1)], "w");
	    if(!outputfile)
	    {
			fprintf(stderr,"Error Opening Output File\n");
			return EXIT_FAILURE;
	    }

	// opens the input files into an array 
	for (i = 1; i < argc-1; i++)
	{
	    inputFiles[i-1] = fopen(argv[i], "r");
	    if(!inputFiles[i-1]) { //error handling for input file opening 
	       fprintf(stderr, "Error Opening Input File: %s\n", argv[i]);
	       return EXIT_FAILURE;
	    }
	}

     //for number of files, create threads
    for(t = 0;t < input_files ; t++)
    {
		FILE* texFile = inputFiles[t];
		inparameters[t].queueL = &queuelock; //Fills the paramater array 
		inparameters[t].file_name = texFile; 
		inparameters[t].q = &q;

		rc = pthread_create(&(ithreads[t]), NULL,requesterThread , &inparameters[t]); 
		if (rc)
		{
            fprintf(stderr,"ERROR; return code from pthread_create() is %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

	//creates the output writing threads
	for(t = 0; t < MAX_RESOLVER_THREADS ; t++)
	{
		//Creates the output file parameters struct 
		outparameters[t].queueL = &queuelock; 
		outparameters[t].file_name = outputfile; 
		outparameters[t].outL = &filelock; 
		outparameters[t].q = &q;
		outparameters[t].files =& files;
		rc = pthread_create(&(othreads[t]), NULL, resolverThread, &(outparameters[t]));
		if (rc)
		{
			fprintf(stderr,"ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
    }


	//interate through all the files, wait for them to finish
	for(t = 0;t < input_files; t++)
	{
        pthread_join(ithreads[t],NULL);
    }
	
	files=0; //no more files, finish up
	//Now we wait for the output threads to finish running
	for(t=0;t<MAX_RESOLVER_THREADS;t++)
	{
        pthread_join(othreads[t],NULL);
    }

	//error handling for closing the output file
	if(fclose(outputfile))
	{
		fprintf(stderr, "Error Closing output file \n");
	}
	
	//clean everything up
	queue_cleanup(&q);
	pthread_mutex_destroy(&queuelock);
	pthread_mutex_destroy(&filelock);

	return EXIT_SUCCESS;
}