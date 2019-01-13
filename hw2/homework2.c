#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>


char** read_expr(char* file_name){
    FILE* fd = fopen(file_name, "r");

    //if there is no such file exists, print error message
    if(fd ==NULL){
        perror("ERROR: File does not exist.\n");
        return NULL;
    }
    
    char** expressions;
    expressions = calloc(128,sizeof(char*));
    char word[128];
    int x;
    int i = 0;
    int j = 0;
    while((x = fgetc(fd)) != EOF){
        if(x == '#'){
           continue; 
        }
        if(isblank(x)){
            if(j == 0){
                continue;
            }
            word[j] = '\0';
            expressions[i] = strdup(word);
           // printf("word is %s\n",word);
            //empty word
            word[0] = 0;
            j = 0;
            i++;
        }
        else{
            if(x == ')'){
                if(j !=0){
                    word[j] = '\0';
                    expressions[i] = strdup(word);
                    //empty word
                    word[0] = 0;
                    j = 0;
                    i++;
                }
                expressions[i] = ")";
                i++;
            }
            else{
                word[j] = x;
                j++;
            }
        }
    }

    //printf("size of array is %s\n",expressions[1]);
    fclose(fd);
    return expressions;
}

char* convert_str(char** expressions){
    char* str = malloc(128*sizeof(char));
    int i = 1;
    str = strcat(strdup(expressions[0])," ");
    while(expressions[i+1] != NULL){
       str = strcat(str,strdup(expressions[i]));
       if(expressions[i+1][0] !=')'){
            str = strcat(str," ");
       }
       i++;
    }
    str = strcat(str,")");
    return str;
}



int cal(int* numbers,int size,char o){
    int result = numbers[0];
   
    int i;
    if(o == '+'){
        for(i = 1; i<size;i++){
            result += numbers[i];
            // printf("CUL NUMBER IS %d \n",numbers[i]);
        }
    }
    else if(o == '*'){
        for(i = 1; i<size;i++){
            result *= numbers[i];
        }
    }
    else if(o == '-'){
         for(i = 1; i<size;i++){
            result -= numbers[i];
        }
    }
    else if(o == '/'){
         for(i = 1; i<size;i++){
             
             if(numbers[i] == 0){
               // printf("PID %d: ERROR: division by zero is not allowed; exiting\n",getpid());
                return result;
             }
            result =result/numbers[i];
        }
    }
    return result;
}


//count the number of elements in the expression
int count_elements(char** expr){
    int count = 0;
    int i = 1;
    int result = 0;
    while(expr[i] != NULL){
        if(expr[i][0] == '('){
            count++;
            while(expr[i][0] != ')'||count !=0){
                if(expr[i][0] == ')'){
                    count++;
                }
                if(expr[i][0] == '('){
                    count--;
                }

                i++;
            }
        }
        result++;
        i++;
    }
    return result;
}


int check_error(char** expr){
    int i = 0;
    if(expr[i][1] != '+' && expr[i][1] != '-' &&expr[i][1] != '*'&& expr[i][1] != '/'){
        return 1;
    }

    while(expr[i] != NULL){
        if(expr[0][1] == '/' ){
            if(expr[i][0] == '0'){
                return 3;
            }
        }
        i++;
    }
    if(i == 3){
        return 2;
    }
    
    return 0;
}




char** subexpr(char** expr,int i){
    char** expressions = malloc(128*sizeof(char*));
    int j = 0;
    int count = 0;
    int flag = 0;
    while(count != 0 || flag == 0){
        if(expr[i][0] == '('){
            count++;
        }
        if(expr[i][0] == ')'){
            count--;
            flag = 1;
        }
        expressions[j] = strdup(expr[i]);

        i++;
        j++;
    }

    return expressions;
}




int process_expr(char** expr, int i){
    //printf("\nLevel i:%d\n",i);
    int PROCS = count_elements(expr);
    //pid_t pid;
    int flag = check_error(expr);  
    char* e = convert_str(expr);
    printf("PID %d: My exression is \"%s\"\n",getpid(),e);
    fflush( stdout );
    int answer = 0;
    int status = 0;
    pid_t pids[PROCS-1];
    //create Pipe to store the number
    int p[PROCS][2];
    int j;
    int rc;
    for (j = 0;j<PROCS;j++ ){
        
        rc = pipe(p[j]);
        if(rc == -1){
            perror("ERROR: pipe() failed\n");
            return EXIT_FAILURE;
        }
    }

    //get the operand in the exoression
    char operand =  expr[i][1];
    char* invalid  = NULL;
    if(flag != 1){
        printf("PID %d: Starting \"%c\" operation\n",getpid(),operand);
        fflush( stdout );
    }
    else{
        char from[2];
        from[0] = expr[0][1];
        from[1] = '\0';
        invalid = strstr(expr[0],from);
        printf("PID %d: ERROR: unknown \"%s\" operator; exiting\n",getpid(),invalid);
        fflush( stdout );
        exit(0);

    }

    i++;
    int bytes_written;
    int bytes_read;
    char* bytes;
    char tmp[128];
    
    for(j = 0; j< PROCS-1;j++){
        pids[j] = fork();
        if(pids[j]==0){
            
            if(expr[i][0] == '('){
                char** sube = subexpr(expr,i);
                
                    int flag_sub = check_error(sube);
                    int a = process_expr(sube,0);
                    sprintf(tmp, "%d", a);
                    if(flag_sub == 0){
                        printf(" sending \"%s\" on pipe to parent\n",tmp);
                        fflush( stdout );
                    }

   
                close(p[j][0]);
                p[j][0] = -1;
                //printf("BYTES WRITTEN IS :%s\n",tmp);
                 bytes_written = write( p[j][1], tmp, 128 );
                 if ( bytes_written == 0 ){
                    printf( "CHILD: the child process terminated without reading data\n" );
                }

            }
            else{

                if(!(flag == 3 && expr[i][0] == '0') ){
                    bytes = expr[i];
                    printf("PID %d: My expression is \"%s\"\n",getpid(),bytes);
                    printf("PID %d: Sending \"%s\" on pipe to parent\n",getpid(),bytes);
                }
                close(p[j][0]);
                p[j][0] = -1;
               // printf("BYTES WRITTEN IS :%s\n",bytes);
                bytes_written = write( p[j][1], bytes, 128 );
                if ( bytes_written == 0 ){
                    printf( "CHILD: the child process terminated without reading data\n" );
                }
            }
            exit(0);
            }
            
            
            if(expr[i][0] == '('){
                int count = 1;
                while (count != 0){
                    i++;
                    if(expr[i][0] == '('){
                        count++;
                    }
                    if(expr[i][0] ==')'){
                        count--;
                    }
                }
            }
            i++;
        }

    
    char buffer[128];
    int num = 0;
    int numbers[PROCS-1];
  /*  
    for(j = 0; j < PROCS-1; j++) {
        wait(&status);
    }*/
        for(j = 0; j < PROCS-1; j++) {
            //wait(&status);
            if(pids[j]>0){
                wait(&status);
                close(p[j][1]);
                p[j][1] = -1;
                bytes_read = read(p[j][0], buffer, 128);
                num = atoi(buffer);
                numbers[j] = num;
                
                //printf("PARENT:::THE OPERATION IS %c\n",operand);
                if ( bytes_read == 0 ){
                    numbers[j] = 0;
                   // printf( "PARENT: the child process terminated without writing data\n" );
                }
                 answer = cal(numbers,PROCS-1,operand);
             
           // printf("BUFFER IS: %s\n",buffer);
            }
            //printf("NUMBER IS: %d\n",numbers[j]);
            
            //printf("BYTES read: %d\n",bytes_read);
        }
        
        
      
    if(flag == 0){
        printf("PID %d: Processed \"%s\";",getpid(),e);
        fflush( stdout );
    }
    else if(flag == 1){
        printf("PID %d: ERROR: unknown \"%s\" operator; exiting\n",getpid(),invalid);
        fflush( stdout );
    }
    else if(flag == 2){
        printf("PID %d: ERROR: not enough operands; exiting\n",getpid());
        fflush( stdout );
    }
    
    else if(flag == 3){
        printf("PID %d: ERROR: division by zero is not allowed; exiting\n",getpid());
        fflush( stdout );
    }
    
    return answer;
}


int main(int argc, char* argv[]){
    if(argc != 2){
        perror("ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>\n");
        return EXIT_FAILURE;
    }
    //read from the file to get the example
    char* file_name = argv[1];
    char** expressions = read_expr(file_name);
    int answer = process_expr(expressions, 0);
    printf(" final answer is \"%d\"\n",answer);
    fflush( stdout );

    return EXIT_SUCCESS;
}


