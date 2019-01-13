#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>

/**
 * Output the file results
 */
void output(int result_num, int* counts, char**words, int total_word, int real_size){
    printf("All done (successfully read %d words; %d unique words).\n",total_word,real_size);
    fflush( stdout ); 

    int i;
    
    if(result_num >real_size){
        result_num = -1;
    }
    //print all the words
    if(result_num == -1){
        printf("All words (and corresponding counts) are:\n");
        fflush( stdout ); 
        for(i=0;i<real_size;i++){
            printf("%s -- %d\n",words[i],counts[i]);
            fflush( stdout ); 
        }
    }
    else{
        printf("First %d words (and corresponding counts) are:\n",result_num);
        fflush( stdout ); 
         for(i=0;i<result_num;i++){
            printf("%s -- %d\n",words[i],counts[i]);
            fflush( stdout ); 
        }
    }
    return;
}


/**
 To compare two string,return 1 when 2 string are the same
*/
int same(char* word1, char* word2){
    int i = 0;
    char c = word1[0];
    
    while(c != '\0'){
        if(word1[i] != word2[i]){
            return 0;
        }
        i++;
        c = word1[i];
    }
    if(word2[i] != '\0'){
        return 0;
    }
    
    return 1;
}



/**
 *To check whether the word occured before
 */
int same_word(char* w, char** words,int* counts,int real_size){
    int i;
    //loop all the word in the words arrayxw
    int ans;
    for(i = 0;i<real_size;i++){

        ans =same(w,words[i]);

        if(ans == 1){
            counts[i]++;
            return 1;
        }

        
    }
    
    
    return 0;
}

/**
 * read one file in the directory and store the word in the array
 */
int* read_file(char* file_name,char*** words, int** counts,int alloc_size,int real_size,int total_word){
    #ifdef DEBUG_MODE
        printf ("[%s]\n", file_name);
        fflush( stdout ); 
    #endif
    static int array_size[3];
    
    FILE* fd = fopen(file_name, "r");
    //if there is no such file exists, print error message
    if(fd ==NULL){
        perror("ERROR: File does not exist.\n");
        return array_size;
    }
    //read the words from the file 
    char word[80];
    int x;
    int counter = 0;
    
    //read the whole file char by char
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
            
            //total_word number increment
            total_word++;
            
            //first check if the word is already in the array
            //if the word exist before
            int exist = same_word(word,*words,*counts,real_size);

            
            //if the word does not exists before
            if(exist == 0){
                (*words)[real_size] = strdup(word);
                (*counts)[real_size] = 1;

                real_size ++;
                }

            //second check does the array need to be reallocate
            if(alloc_size == real_size){
                alloc_size += 16;
                *words = realloc(*words,alloc_size*sizeof(char*));
                *counts = realloc(*counts,alloc_size*sizeof(int));
                printf("Re-allocated parallel arrays to be size %d.\n",alloc_size);
                fflush( stdout ); 
            }
            
            //empty the word array
            word[0] = 0 ;
    
           
        }
        
        //if the character read is alpha
        else{
            word[counter] = x;
            counter++;
        }
    }

    fclose(fd);
    
    // TO store the size for the total words count, the unque number of words
    array_size[0] = real_size;
    array_size[1] = alloc_size;
    array_size[2] = total_word;
    

    return array_size;
}




void read_dir( char* dir_name,char** words, int* counts,int result_num){
    //initialize the array size is 16
    int alloc_size = 16;
    int real_size = 0;
    int total_word = 0;
    char* file_name = NULL;
    

    //open the directory to get the file
    DIR * dir = opendir( dir_name ); 
    
    struct dirent *pDirent;
    
    //if the command line put the file name not the directory
    if(dir == NULL){
        perror("ERROR: Directory does not exist.\n");
        return;
    }
    
    //change the directory into the txt file
    int c = chdir(dir_name);

    if (c != 0){
        perror("ERROR: Could not change directory.\n");
    }
    

    //Initialize to allocate arrays of size 16
    words = calloc(16,sizeof(char*));
    counts = calloc(16,sizeof(int));
    printf("Allocated initial parallel arrays of size 16.\n");
    fflush( stdout ); 
    
    //if the argument is directory, read all the files in the directory
    
    while ((pDirent = readdir(dir)) != NULL) {
        //only store txt file in the directory
        if ( pDirent->d_type != DT_REG ){
            continue;
        }
        //get the name of all the files
        file_name =  pDirent->d_name;
        

        //read all the files in the directory
        
        
        int* array_size = read_file(file_name,&words,&counts,alloc_size,real_size,total_word);
        
        //to reserve the counts for the next files
        alloc_size = array_size[1];
        real_size = array_size[0];
        total_word = array_size[2];
        
        
    }
    
    //output the final result
    output(result_num, counts, words, total_word, real_size);
    
 
    //free the alloc memory
    int i ;
    for(i = 0;i< alloc_size;i++){
        free(words[i]);
    }
    
    free(words);
    free(counts);
    
     //close the current directory
    closedir(dir);
    return;
}


int main(int argc, char *argv[]){
    
    char* dir_name;
    int  result_num= -1;
    
    //Initialize two parallel arrays
    char** words = NULL;
    int* counts = NULL;
    
    //check the command line
    if(argc != 2 && argc != 3){
        perror("ERROR: INVALID MEMORY REFERENCE\n");
        return EXIT_FAILURE;
        
    }
    
    
    //To get the directory name and file name
    dir_name = argv[1];

    //if the output result number is required
    if(argc == 3){
       result_num = atoi(argv[2]);
    }
    //to enter the directory to read all the files
    read_dir(dir_name,words,counts,result_num);

    return EXIT_SUCCESS;
}