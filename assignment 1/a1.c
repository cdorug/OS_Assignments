#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>

enum command {nothing, list, parse, extract, findall};

void list_contents(char* path, bool recursive, int size_smaller, bool has_perm_write){
    DIR* dir;
    
    struct dirent* dir_entry;
    char name[700];
    dir = opendir(path);
    
    if(dir == 0) {
        return;
    }
    
    while((dir_entry=readdir(dir)) != 0) {
            snprintf(name, 700, "%s/%s", path, dir_entry->d_name);
            struct stat i_node;
            lstat(name, &i_node);
            if(strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
            {
                continue;
            }
            else {
                if(recursive) {
                    if(S_ISDIR(i_node.st_mode)) {
                        list_contents(name, recursive, size_smaller, has_perm_write);
                    }
                }
                if(has_perm_write) {
                    if(S_ISREG(i_node.st_mode) || S_ISDIR(i_node.st_mode)) {
                        if(!(i_node.st_mode & S_IWUSR)) {
                            continue;
                        }
                    }
                }
                if(size_smaller != -1) {
                    if(S_ISDIR(i_node.st_mode)) {
                        continue;
                    }
                    else if(S_ISREG(i_node.st_mode)) {
                        if(i_node.st_size >= size_smaller) {
                            continue;
                        }
                    }
                }
                printf("%s\n", name);
            }
    }
    free(dir);
    free(dir_entry);
}

int main(int argc, char **argv){
    if(argc >= 2){
        enum command cmd;
        cmd = nothing;
        if(strcmp(argv[1], "variant") == 0){
            printf("47164\n");
        }
        else {
            // check if a valid command has been given
            for(int i = 1; i < argc; i ++) {
                if(strcmp(argv[i], "list") == 0)
                {
                    cmd = list;
                }
                else if(strcmp(argv[i], "parse") == 0) {
                    cmd = parse;
                }
                else if(strcmp(argv[i], "extract") == 0) {
                    cmd = extract;
                }
                else if(strcmp(argv[i], "findall") == 0) {
                    cmd = findall;
                }
            }
        }
        // list command
        if(cmd == list) {
            // parse the list of arguments another time to look for options
            bool recursive = false;
            bool has_perm_write = false;
            int size_smaller = -1;
            char check_size_option[10];
            char check_path_option[10];
            char path[700]; 
            for(int i = 0; i < argc; i ++) {
                if(strcmp(argv[i], "recursive") == 0) {
                    recursive = true;
                }
                else if(argv[i][0] == 's') {
                    strncpy(check_size_option, argv[i], 13);
                    check_size_option[13] = '\0';
                    if(strcmp(check_size_option, "size_smaller=") == 0) {
                        if(strlen(argv[i]) > 13) {
                            size_smaller = atoi(argv[i] + 13);
                        }
                    }
                }
                else if(strcmp(argv[i], "has_perm_write") == 0) {
                    has_perm_write = true;
                }
                else if(argv[i][0] == 'p') {
                    strncpy(check_path_option, argv[i], 5);
                    check_path_option[5] = '\0';
                    if(strcmp(check_path_option, "path=") == 0) {
                        strcpy(path, argv[i] + 5);
                    }
                }
            }
            if(strlen(path) == 0) {
                // error with the path
                printf("ERROR\ninvalid directory path");
            }
            else {
                
                DIR* directory = opendir(path);
                if(directory) {
                    closedir(directory);
                }
                else {
                    printf("ERROR\ninvalid directory path");
                }
                
                printf("SUCCESS\n");
                list_contents(path, recursive, size_smaller, has_perm_write);
            }
        }
        else if(cmd == parse) {
            char path[700];
            bool magic_error = false, version_error = false, section_error = false, section_type_error = false;
            char check_path_option[10];
            for(int i = 0; i < argc; i ++) {
                if(argv[i][0] == 'p') {
                    strncpy(check_path_option, argv[i], 5);
                    check_path_option[5] = '\0';
                    if(strcmp(check_path_option, "path=") == 0) {
                        strcpy(path, argv[i] + 5);
                    }
                }
            }
            int fd = open(path, O_RDONLY);
            unsigned char header[9];
            // first read the header, 9 bytes
            read(fd, header, 9);
            if(!((header[0] == 'B') && (header[1] == '8'))) {
                magic_error = true;
                goto err;
            }
            // build the version number, 4 bytes
            int version = (int)(header[7] << 24) | (int)(header[6] << 16) | (int)(header[5] << 8) | (int)(header[4]);
            if(!(version >= 122 && version <= 145)) {
                version_error = true;
                goto err;
            }
            // build the number of sections number, 1 byte
            int no_of_sections = (int)(header[8]);
            if(!(no_of_sections >= 4 && no_of_sections <= 14)){
                section_error = true;
                goto err;
            }
            // check if the sections' types are within the allowed values

            unsigned char type;
            lseek(fd, 7, SEEK_CUR);
            
            
            for(int i = 0; i < no_of_sections; i ++) {
                read(fd, &type, 1);
                if(!(type == 92 || type == 80 || type == 21 || type == 17 || type == 60 )){
                    section_type_error = true;
                    goto err;
                }
                lseek(fd, 15, SEEK_CUR);
            }
            //if no error go back and print the sections
            printf("SUCCESS\n");
            printf("version=%d\n", version);
            printf("nr_sections=%d\n", no_of_sections);
            // now print all the sections
            unsigned char section[16];
            lseek(fd, 9, SEEK_SET);
            int i;
            for(i = 1; i <= no_of_sections; i ++) {
                printf("section%d: ", i);
                read(fd, section, 16);
                for(int j = 0; j < 7; j ++) {
                    //print section name
                    if(section[j] != 0) {
                        printf("%c", section[j]);
                    }
                }
                printf(" %d ", section[7]);
                int section_size = (int)(section[15] << 24) | (int)(section[14] << 16) | (int)(section[13] << 8) | (int)(section[12]);
                printf("%d\n", section_size);
            } 
            
            err:
            if(magic_error) {
                printf("ERROR\nwrong magic");
            }
            else if(version_error) {
                printf("ERROR\nwrong version");
            }
            else if(section_error) {
                printf("ERROR\nwrong sect_nr");
            }
            else if(section_type_error) {
                printf("ERROR\nwrong sect_types");
            }
        }
        else if(cmd == extract) {
            bool file_error = false, section_error = false, line_error = false;
            char path[700];
            int section;
            int line;
            char check_path_option[10];
            char check_section_option[10];
            char check_line_option[10];
            for(int i = 0; i < argc; i ++) {
                if(argv[i][0] == 'p') {
                    strncpy(check_path_option, argv[i], 5);
                    check_path_option[5] = '\0';
                    if(strcmp(check_path_option, "path=") == 0) {
                        strcpy(path, argv[i] + 5);
                    }
                }
                else if(argv[i][0] == 's') {
                    strncpy(check_section_option, argv[i], 8);
                    check_section_option[8] = '\0';
                    if(strcmp(check_section_option, "section=") == 0) {
                        if(strlen(argv[i]) > 8) {
                            section = atoi(argv[i] + 8);
                        }
                    }
                }
                else if(argv[i][0] == 'l') {
                    strncpy(check_line_option, argv[i], 5);
                    check_line_option[5] = '\0';
                    if(strcmp(check_line_option, "line=") == 0) {
                        if(strlen(argv[i]) > 5) {
                            line = atoi(argv[i] + 5);
                        }
                    }
                }
            }
            int fd = open(path, O_RDONLY);
            if(fd == -1) {
                file_error = true;
                goto error;
            }
            int i = 1;
            lseek(fd, 9, SEEK_SET);
            while(i != section) {
                lseek(fd, 16, SEEK_CUR);
                i ++;
            }
            unsigned char sect[16];
            read(fd, sect, 16);
            int offset = (int)(sect[11] << 24) | (int)(sect[10] << 16) | (int)(sect[9] << 8) | (int)(sect[8]);
            int sect_size = (int)(sect[15] << 24) | (int)(sect[14] << 16) | (int)(sect[13] << 8) | (int)(sect[12]);
            lseek(fd, offset, SEEK_SET);
            char section_body[10000];
            read(fd, section_body, sect_size);
            char *token = strtok(section_body, "\n");
            i = 1;
            while( token != NULL && i != line) {
                token = strtok(NULL, "\n");
                i ++;
            }
            printf("SUCCESS\n");
            for(i = strlen(token) - 1; i >= 0; i --) {
                if(token[i] > 0) {
                    printf("%c", token[i]);
                }
            }
            printf("\n");
            error:
            if(file_error) {
                printf("ERROR\ninvalid file");
            }  
        }
        else if(cmd == findall) {
            
        }
    }
    return 0;
}
