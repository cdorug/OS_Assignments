#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

int main() {
    if(mkfifo("RESP_PIPE_47164", 0600) < 0) {
        perror("ERROR\ncannot create the response pipe");
        exit(1);
    }
    else {
        // open pipe for reading
        int req_pipe = open("REQ_PIPE_47164", O_RDONLY);
        if(req_pipe < 0) {
            perror("ERROR\ncannot open the request pipe");
            exit(1);
        }
        int resp_pipe = open("RESP_PIPE_47164", O_WRONLY);
        unsigned char connect[] = {0x07, 0x43, 0x4f, 0x4e, 0x4e, 0x45, 0x43, 0x54}; // "CONNECT" message
        write(resp_pipe, connect, 8);
        //printf("NEW RUN -------------------\n");
        printf("SUCCESS\n");
        unsigned char request_length; 
        char request[40];
        unsigned char *shared_obj = NULL;
        unsigned char *mapped_file = NULL;
        unsigned int file_size = -1;
        int file_opened = 0;
        unsigned int size;
        while(1) {
            memset(request, 0, 40);
            read(req_pipe, &request_length, 1);
            int read_success = read(req_pipe, request, request_length);
            if(read_success == -1 || request_length != read_success) {
                printf("READING REQUEST ERROR\n");
            }
            request[read_success] = 0;
            //printf("***REQUEST-> SIZE: %d, COMMAND: %s\n", request_length, request);
            if(strcmp(request, "PING") == 0) {
                // this is a ping request
                unsigned int number = 47164;
                unsigned char ping_pong[] = {0x04, 0x50, 0x49, 0x4e, 0x47, 0x04, 0x50, 0x4f, 0x4e, 0x47};
                write(resp_pipe, ping_pong, 10);
                write(resp_pipe, &number, sizeof(unsigned int));
                //printf("WROTE RESPONSE PING PONG %d\n", number);
                        
            }
            else if(strcmp(request, "CREATE_SHM") == 0) {
                // this is a create shm request
                //printf("request = %s\n", request);
                read(req_pipe, &size, sizeof(unsigned int));
                char *shared_obj_name = "/9RS60L2";
                int fd = shm_open(shared_obj_name, O_CREAT | O_RDWR, 0664);
                if (fd < 0) { 
                    perror("Cannot create sh mem");
                    exit(10);
                }
                if (ftruncate(fd, size) < 0) {
                    perror("cannot set size of shared mem with ftruncate");
                    exit(20);                
                }
                shared_obj = (unsigned char*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if(shared_obj == MAP_FAILED) {
                    char response[] = {0x0a, 0x43, 0x52, 0x45, 0x41, 0x54, 0x45, 0x5f, 0x53, 0x48, 0x4d, 0x05, 0x45, 0x52, 0x52, 0x4f, 0x52}; // "CREATE_SHM" "ERROR"
                    write(resp_pipe, response, 17);
                    //printf("WROTE RESPONSE CREATE_SHM ERROR\n");
                }
                else {
                    char response[] = {0x0a, 0x43, 0x52, 0x45, 0x41, 0x54, 0x45, 0x5f, 0x53, 0x48, 0x4d, 0x07, 0x53, 0x55, 0x43, 0x43, 0x45, 0x53, 0x53}; // "CREATE_SHM" "SUCCESS"
                    write(resp_pipe, response, 19);
                    //printf("WROTE RESPONSE CREATE_SHM SUCCESS\n");
                }
                continue; 
            }
            else if(strcmp(request, "WRITE_TO_SHM") == 0) {
                //printf("READ REQUEST WRITE TO SHM\n");
                unsigned int offset;
                unsigned int value;
                read(req_pipe, &offset, sizeof(unsigned int));
                read(req_pipe, &value, sizeof(unsigned int));
                //printf("WRITE_TO_SHM --- offset = %u, value = %u\n", offset, value);
                if(offset >= 0 && offset < (size - sizeof(value))) {
                    *((int*)(shared_obj + offset)) = value;
                    char response[] = {12, 'W', 'R', 'I', 'T', 'E', '_', 'T', 'O', '_', 'S', 'H', 'M', 7, 'S', 'U', 'C', 'C', 'E', 'S', 'S'};
                    write(resp_pipe, response, 21);
                    //printf("WROTE RESPONSE WRITE TO SHM SUCCESS\n");
                }
                else {
                    // error
                    char response[] = {12, 'W', 'R', 'I', 'T', 'E', '_', 'T', 'O', '_', 'S', 'H', 'M', 5, 'E', 'R', 'R', 'O', 'R'};
                    write(resp_pipe, response, 19);
                    //printf("WROTE RESPONSE WRITE TO SHM ERROR\n");
                }
                continue; 
            }
            else if(strcmp(request, "MAP_FILE") == 0) {
                char filename_size; 
                char filename[100];
                read(req_pipe, &filename_size, 1);
                read(req_pipe, filename, filename_size);
                printf("%s\n", filename);
                struct stat file_stat;
                stat(filename, &file_stat);
                file_size = file_stat.st_size;
                //printf("OPENED FILE'S SIZE IS: %d\n", file_size);
                int fd = open(filename, O_RDONLY);
                mapped_file = (unsigned char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
                if(mapped_file == MAP_FAILED) {
                    // error
                    //printf("ERROR CODE IS: %d\n", errno);
                    char response[] = {8, 'M', 'A', 'P', '_', 'F', 'I', 'L', 'E', 5, 'E', 'R', 'R', 'O', 'R'};
                    write(resp_pipe, response, 15);
                    //printf("WROTE RESPONSE MAP_FILE ERROR\n");
                    file_opened = 0;
                }
                else {
                    char response[] = {8, 'M', 'A', 'P', '_', 'F', 'I', 'L', 'E', 7, 'S', 'U', 'C', 'C', 'E', 'S', 'S'};
                    file_opened = 1;
                    write(resp_pipe, response, 17);      
                    //printf("WROTE RESPONSE MAP_FILE SUCCESS\n");
                }
                continue;
            }
            else if(strcmp(request, "READ_FROM_FILE_OFFSET") == 0) {
                //printf("READ REQUEST READ FROM FILE OFFSET\n");
                unsigned int offset;
                unsigned int no_of_bytes;
                read(req_pipe, &offset, sizeof(unsigned int));
                read(req_pipe, &no_of_bytes, sizeof(unsigned int));
                //printf("SIZE OF FILE BEFORE READING FROM OFFSET: %d\n", file_size);
                if(shared_obj != NULL && file_opened == 1) {
                    //printf("OFFSET: %d, NUMBER OF BYTES: %d\n", offset, no_of_bytes);
                }
                if(shared_obj == NULL || file_opened != 1 || (offset + no_of_bytes) > file_size) {
                    // error
                    char response[] = {21, 'R', 'E', 'A', 'D', '_', 'F', 'R', 'O', 'M', '_', 'F', 'I', 'L', 'E', '_', 'O', 'F', 'F', 'S', 'E', 'T', 5, 'E', 'R', 'R', 'O', 'R'};
                    write(resp_pipe, response, 28);
                    //printf("WROTE RESPONSE READ_FROM_FILE_OFFSET ERROR\n");
                }
                else {
                    for(int i = 0; i < no_of_bytes; i ++) {
                        shared_obj[i] = mapped_file[offset + i];
                    }
                    char response[] = {21, 'R', 'E', 'A', 'D', '_', 'F', 'R', 'O', 'M', '_', 'F', 'I', 'L', 'E', '_', 'O', 'F', 'F', 'S', 'E', 'T', 7, 'S', 'U', 'C', 'C', 'E', 'S', 'S'};
                    write(resp_pipe, response, 30);
                    //printf("WROTE RESPONSE READ_FROM_FILE_OFFSET SUCCESS\n");
                }
                continue;
            }
            else if(strcmp(request, "READ_FROM_FILE_SECTION") == 0) {
                //printf("READ REQUEST READ FROM FILE SECTION\n");
                unsigned int section_no, offset, no_of_bytes;
                read(req_pipe, &section_no, sizeof(unsigned int));
                read(req_pipe, &offset, sizeof(unsigned int));
                read(req_pipe, &no_of_bytes, sizeof(unsigned int));
                //printf("SECTION NO: %d\n", section_no);
                unsigned int no_of_sections_SF = (unsigned int)mapped_file[8];
                //printf("NO OF SECTIONS THIS FILE HAS: %d\n", no_of_sections_SF);
                unsigned int searched_section = 9 + (section_no - 1)* 16;
                //printf("SEARCHED SECTION HEADER CONTENT OFFSET: %c%c%c%c\n", mapped_file[searched_section + 8], mapped_file[searched_section + 9], mapped_file[searched_section + 10], mapped_file[searched_section + 11]);
                int content_offset = (int)(mapped_file[searched_section + 11] << 24) | (int)(mapped_file[searched_section + 10] << 16) | (int)(mapped_file[searched_section + 9] << 8) | (int)(mapped_file[searched_section + 8]);
                //int sect_size = (int)(mapped_file[searched_section + 15] << 24) | (int)(mapped_file[searched_section + 14] << 16) | (int)(mapped_file[searched_section + 13] << 8) | (int)(mapped_file[searched_section + 12]);
                if(section_no > no_of_sections_SF) {
                    // error
                    char response[] = {22, 'R', 'E', 'A', 'D', '_', 'F', 'R', 'O', 'M', '_', 'F', 'I', 'L', 'E', '_', 'S', 'E', 'C', 'T', 'I', 'O', 'N', 5, 'E', 'R', 'R', 'O', 'R'};
                    write(resp_pipe, response, 29);
                    //printf("WROTE RESPONSE READ_FROM_FILE_SECTION ERROR\n");
                }
                else {
                    for(int i = 0; i < no_of_bytes; i ++) {
                        shared_obj[i] = mapped_file[content_offset + offset + i];
                    }
                    char resp_size = 22;
                    char *response = "READ_FROM_FILE_SECTION";
                    char resp_status_size = 5;
                    char *status = "ERROR";
                    write(resp_pipe, &resp_size, 1);
                    write(resp_pipe, response, 22);
                    write(resp_pipe, &resp_status_size, 1);
                    write(resp_pipe, status, 5);
                } 
                continue;
            }
            else if(strcmp(request, "EXIT") == 0) {
                close(req_pipe);
                close(resp_pipe);
                unlink("REQ_PIPE_47164");
                unlink("RESP_PIPE_47164");
                printf("EXITED\n");
                exit(1);
            }
            exit(1);
        }
    }
    return 0;
}