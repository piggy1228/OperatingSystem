#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include "hw3.h"

#define MAX_SIZE 20000

/*
gcc -Wall hw3.c -pthread
./a.out hw3-testdir1 5 hw3-output01.txt 
valgrind --leak-check=full -v ./a.out hw3-testdir1 5 hw3-output01.txt 
*/

//read all the txt file name from the directory
/* global mutex variable to synchronize child threads */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* definition of a struct to send to each thread,
    because the thread function can only accept one arg, i.e., void *
 */
 typedef struct
{
  char** buffer;
  int num;
  FILE* fp;
  int count;

  //count variable count the total number of the words have been 


} buffer_class;

typedef struct{
    buffer_class* bc;
    char file_name[MAX_SIZE];
}B_class;


void read_dir(char* dir_name,char** files){
    char* file_name = NULL;

    //open the directory to get the file

    DIR * dir = opendir(dir_name); 
    struct dirent *pDirent;
    //if the command line put the file name not the directory
    if(dir == NULL){
        fprintf(stderr,"ERROR: Directory does not exist.\n");
        return;
    }
    
    //change the directory into the txt file
    int c = chdir(dir_name);
    if (c != 0){
        fprintf(stderr,"ERROR: Could not change directory.\n");
        return;
    }
    int i = 0;
    while ((pDirent = readdir(dir)) != NULL) {
        //only store txt file in the directory
        if ( pDirent->d_type != DT_REG ){
            continue;
        }
        //get the name of all the files
        file_name =  pDirent->d_name;
        
        files[i] = strdup(file_name);
       
       // memcpy(files[i],file_name,bytes)
        printf("MAIN: Created child thread for \"%s\"\n",files[i]);
        fflush( stdout );
        i++;
    }
    
    closedir(dir);
    return;
}

void write_file(buffer_class* bc){
    if(bc -> num == 0){
        return;
    }
    bc -> num = 0;

    char* out_str = calloc(MAX_SIZE,sizeof(char));
    int i;

    if(strlen(bc->buffer[0]) == 0){
 
        return;
    }

    strcpy(out_str,bc->buffer[0]);
    free(bc -> buffer[0]);
    bc -> buffer[0] = "";
    out_str = strcat(out_str,"\n");
    bc -> count ++;
   
    for(i = 1; i<maxwords;i++){
        if(strlen(bc->buffer[i]) == 0){
            break;
        }
        bc -> count++;
        out_str = strcat(out_str,bc->buffer[i]);
        free(bc -> buffer[i]);
        bc -> buffer[i] = "";
        out_str = strcat(out_str,"\n");
    }

	fprintf(bc -> fp, out_str, MAX_SIZE); /* write a formatted string to file */
	free(out_str);
}



void critical_section(buffer_class* bc, char* word,unsigned int mytid ){
    bc -> buffer[bc -> num] = word;

    printf("TID %u: Stored \"%s\" in shared buffer at index [%d]\n",(unsigned int)mytid, bc -> buffer[bc->num],bc->num);
    fflush( stdout );
    bc -> num++;
    if(bc -> num ==maxwords){
        write_file(bc);
        printf("MAIN: Buffer is full; writing %d words to output file\n",maxwords);
        fflush( stdout );
    }

    
}



void* write_buffer(void* arg){
    pthread_t mytid = pthread_self();    /* get my thread ID */
   // buffer_class* bc = (buffer_class *)arg;
   B_class* B = (B_class *)arg;
   
   buffer_class * bc = B -> bc;

    printf("TID %u: Opened \"%s\"\n",(unsigned int) mytid,B->file_name);
    fflush( stdout );
    //open the file and read file
    FILE* fd = fopen(B->file_name, "r");
     //read the words from the file 
    char word[80];
    int x;
    int counter = 0;

    while((x = fgetc(fd)) != EOF){
         //if the there is a space or punctuation
        int alphanumeric = isalpha(x)||isdigit(x);
        if(alphanumeric == 0){

            //remove all the 1 length word
            if(counter <= 1){
                counter = 0;
                continue;
            }

            //initialize the word array
            word[counter] = '\0';
            counter = 0;
            char* w = strdup(word);
            pthread_mutex_lock( &mutex );
            critical_section(bc,w,(unsigned int)mytid);
            pthread_mutex_unlock( &mutex );
            
            //empty the word array
            word[0] = 0 ;
            
        }
        
        //if the character read is alpha
        else{
            word[counter] = x;
            counter++;
        }
    }

    //for the last word in the char array to write to the output file
    if (counter > 1){
        word[counter] = '\0';
        char* w = strdup(word);
    
        pthread_mutex_lock( &mutex );
        critical_section(bc,w,(unsigned int)mytid);
        pthread_mutex_unlock( &mutex );
    }

    
    fclose(fd);
    printf("TID %u: Closed \"%s\"; and exiting\n",(unsigned int) mytid,B->file_name);
    fflush( stdout );
    free(B);
    return bc;
}



int main(int argc, char *argv[]){
        if(argc != 4){
        fprintf(stderr,"ERROR: Invalid arguments\nUSAGE: ./a.out <input-directory> <buffer-size> <output-file>\n");
        return EXIT_FAILURE;
    }

    //To get the directory name and file name
    char* dir_name;
    char* out_file = argv[3];
    maxwords = atoi(argv[2]);
    dir_name = argv[1];
    //dyanmically allocate memory 
    words = calloc(maxwords,sizeof(char*));
    char** files = calloc(100,sizeof(char*));
    
    printf("MAIN: Dynamically allocated memory to store %d words\n",maxwords);
    fflush( stdout );
    printf("MAIN: Opened \"%s\" directory\n",dir_name);
    fflush( stdout );
    
    
    read_dir(dir_name,files);

    //open a new file
   
   
    //total child thread number
    int num = 0;
    while(files[num] != NULL){
        num++;
    }
  // printf("number is %d\n",num);
   
   /* keep track of the thread IDs */
   pthread_t tid[num];
   
   int i;
   int rc;
   
   printf("MAIN: Closed \"%s\" directory\n",dir_name);
   fflush( stdout );
   printf("MAIN: Created \"%s\" output file\n",out_file);
   fflush( stdout );
   chdir("..");
   FILE* fp = fopen(out_file, "w");
   chdir(dir_name);
   buffer_class* bc = malloc(sizeof(buffer_class));
   bc -> buffer = words;
   bc ->num = 0;
   bc -> count = 0;
   bc -> fp = fp;

   for(i = 0; i< num;i++){
        B_class* b_class = malloc(sizeof(B_class));
        strcpy(b_class ->file_name,files[i]);
        b_class -> bc = bc;

        rc = pthread_create( &tid[i], NULL, write_buffer, b_class);
        
        //printf("MAIN: Created child thread for \"%s\" \n",files[i]);
        if ( rc != 0 ){
            fprintf( stderr, "MAIN: Could not create thread (%d)\n", rc );
            return EXIT_FAILURE;
        }
   }
  
   /* wait for child threads to terminate */
   for(i = 0;i<num;i++){
        rc = pthread_join( tid[i], NULL);   
        printf( "MAIN: Joined child thread: %u\n",(unsigned int) tid[i]);
        fflush( stdout );
   }
   
   // to clean up the final words in the buffer

   write_file(bc);
   printf("MAIN: All threads are done; writing %d words to output file\n",bc->count%maxwords);
   fflush( stdout );
   
   //close the output file
   fclose(fp);

  //free all the dynamically error
    free(bc);
    free(words);
    for(i = 0;i<num;i++){
        free(files[i]);
    }
    free(files);
    
    return EXIT_SUCCESS;
}
