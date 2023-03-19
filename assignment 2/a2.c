#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>

typedef struct {
    int process_id;
    int thread_id;
}process_and_thread_id;

sem_t *T4_2Started;
sem_t *T4_1Terminated;
sem_t *six_at_most;
sem_t *T2_3Terminated;
sem_t *T4_3Terminated;
int running_threads = 0;
bool T12Arrived = false;
bool T12Ended = false;
int threads_that_finished = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t six_running_threads = PTHREAD_COND_INITIALIZER;
pthread_cond_t T12Finished  = PTHREAD_COND_INITIALIZER;

void *thread_function(void* arg)
{
    process_and_thread_id *id = (process_and_thread_id*) arg;
    int process_id = id->process_id;
    int thread_id = id->thread_id;
    if(thread_id == 1) {
        sem_wait(T4_2Started);
    }
    
    if(thread_id == 3) {
        sem_wait(T2_3Terminated);
    }

    info(BEGIN, process_id, thread_id);

    if(thread_id == 2) {
        sem_post(T4_2Started);
    }
    
    if(thread_id == 2) {
        sem_wait(T4_1Terminated);
    }

    info(END, process_id, thread_id);

    if(thread_id == 1) {
        sem_post(T4_1Terminated);
    }
    
    
    if(thread_id == 3) {
        sem_post(T4_3Terminated);
    }
    
    
    return NULL;
}

void *thread_function2(void* arg) {
    process_and_thread_id *id = (process_and_thread_id*) arg;
    int process_id = id->process_id;
    int thread_id = id->thread_id;
    sem_wait(six_at_most);
    info(BEGIN, process_id, thread_id); 
    /*
    
    
    if(thread_id == 12) {
        pthread_mutex_lock(&lock);
        info(BEGIN, process_id, thread_id);
        T12Arrived = true;
        running_threads ++;
        if(running_threads != 6) {
            pthread_cond_wait(&six_running_threads, &lock);
        }
        pthread_mutex_unlock(&lock);
    }
    else {
        pthread_mutex_lock(&lock);
        info(BEGIN, process_id, thread_id);
        running_threads ++;
        if(T12Arrived) {
            if(running_threads == 6) {
                pthread_cond_signal(&six_running_threads);
            }
            pthread_cond_wait(&T12Finished, &lock);
        }
        pthread_mutex_unlock(&lock);
    }

    pthread_mutex_lock(&lock);
    if(thread_id != 12 && T12Arrived) {
        pthread_cond_wait(&T12Finished, &lock);
    }
    pthread_mutex_unlock(&lock);

    
    
    pthread_mutex_lock(&lock);
    info(END, process_id, thread_id);
    if(thread_id == 12) {
        pthread_cond_broadcast(&T12Finished);
        T12Arrived = false;
        T12Ended = true;
    }
    else {
        
        if(threads_that_finished > 38 && !T12Ended) {
            pthread_cond_wait(&T12Finished, &lock);
        }
        
    }
    running_threads --;
    threads_that_finished ++;
    pthread_mutex_unlock(&lock);
    */
    info(END, process_id, thread_id); 
    
    sem_post(six_at_most);

    
    return NULL;
}

void *thread_function3(void* arg) {
    process_and_thread_id *id = (process_and_thread_id*) arg;
    int process_id = id->process_id;
    int thread_id = id->thread_id;
    if(thread_id == 1) {
        sem_wait(T4_3Terminated);
    }
    info(BEGIN, process_id, thread_id);

    info(END, process_id, thread_id);
    if(thread_id == 3) {
        sem_post(T2_3Terminated);
    }
    return NULL;
}

int main(){
    init();

    info(BEGIN, 1, 0);
    int childPid1 = fork();
    switch(childPid1) {
        case -1: 
            printf("ERROR\n");
			exit(1);
		case 0: // child, in P2
			info(BEGIN, 2, 0);
            // create semaphores
            T4_3Terminated = sem_open("T4_3Terminated", O_CREAT, 0600, 0); 
            T2_3Terminated = sem_open("T2_3Terminated", O_CREAT, 0600, 0);
            pthread_t *th = malloc(4* sizeof(pthread_t));
            process_and_thread_id *identifier = (process_and_thread_id*)malloc(4* sizeof(process_and_thread_id));
            for(int i = 0; i < 4; i++ ) {
                identifier[i].thread_id = i+1;
                identifier[i].process_id = 2;
                pthread_create(&th[i], NULL, thread_function3, &identifier[i]);
            }
            
            // destroy semaphores
            sem_unlink("T2_3Terminated");
            sem_unlink("T4_3Terminated");
            int childPid2 = fork();
            switch(childPid2) {
                case -1:
                    printf("ERROR\n");
                    exit(1);
                case 0: // child of P2, in P3
                    info(BEGIN, 3, 0);
                    info(END, 3, 0);
                    exit(0);
                default: // still in P2, create P4
                    waitpid(childPid2, NULL, 0); // wait for P3 to finish
                    int childPid3 = fork();
                    switch(childPid3) {
                        case -1:    
                            printf("ERROR\n");
                            exit(1);
                        case 0: // in P4, child of P2
                            info(BEGIN, 4, 0);
                            // create P6
                            int childPid5 = fork();
                            switch(childPid5) {
                                case -1:
                                    printf("ERROR\n");
                                    exit(1);
                                case 0: // in P6
                                    info(BEGIN, 6, 0);
                                    // create P7
                                    int childPid6 = fork();
                                    switch(childPid6) {
                                        case -1:
                                            printf("ERROR\n");
                                            exit(1);
                                        case 0: // in P7
                                            info(BEGIN, 7, 0);
                                            info(END, 7, 0);
                                            exit(0);
                                        default: // still in P6
                                            waitpid(childPid6, NULL, 0); // wait for P7
                                            // create P8
                                            int childPid7 = fork();
                                            switch(childPid7) {
                                                case -1: 
                                                    printf("ERROR\n");
                                                    exit(1);
                                                case 0: // in P8
                                                    info(BEGIN, 8, 0);
                                                    info(END, 8, 0);
                                                    exit(0);
                                                default: // still in P6
                                                    waitpid(childPid7, NULL, 0); // wait for P8
                                                    break;
                                            }
                                    }
                                    // create semaphores
                                    six_at_most = sem_open("six_at_most", O_CREAT, 0600, 6); 
                                    //six_threads_running = sem_open("six_threads_running", O_CREAT, 0600, 0);
                                    // create threads in P6
                                    pthread_t *th = malloc(44* sizeof(pthread_t));
                                    process_and_thread_id *identifier = (process_and_thread_id*)malloc(44* sizeof(process_and_thread_id));
                                    for(int i = 0; i < 44; i++ ){
                                        identifier[i].thread_id = i+1;
                                        identifier[i].process_id = 6;
                                        pthread_create(&th[i], NULL, thread_function2, &identifier[i]);
                                    }
                                    for(int i = 0; i < 44; i++) {
                                        pthread_join(th[i], NULL);
                                    }
                                    info(END, 6, 0);
                                    // destroy semaphores
                                    sem_unlink("six_at_most");
                                    //sem_unlink("six_threads_running");
                                    exit(0);
                                default: // still in P4
                                    waitpid(childPid5, NULL, 0); // wait for P6 to finish
                                    // create P9
                                    int childPid8 = fork();
                                    switch(childPid8) {
                                        case -1: 
                                            printf("ERROR\n");
                                            exit(1);
                                        case 0: // in P9
                                            info(BEGIN, 9, 0);
                                            info(END, 9, 0);
                                            exit(0);
                                        default: // still in P4
                                            waitpid(childPid8, NULL, 0); // wait for P9
                                            break;
                                    }
                            }
                            // create semaphores
                            T4_2Started = sem_open("T4_2Started", O_CREAT, 0600, 0); 
                            T4_1Terminated = sem_open("T4_1Terminated", O_CREAT, 0600, 0);
                            // create threads in P4
                            pthread_t *th = malloc(4* sizeof(pthread_t));
                            process_and_thread_id *identifier = (process_and_thread_id*)malloc(4* sizeof(process_and_thread_id));
                            for(int i = 0; i < 4; i++ ){
                                identifier[i].thread_id = i+1;
                                identifier[i].process_id = 4;
                                pthread_create(&th[i], NULL, thread_function, &identifier[i]);
                            }
                            for(int i = 0; i < 4; i++) {
                                pthread_join(th[i], NULL);
                            }
                            info(END, 4, 0);
                            // destroy semaphores
                            sem_unlink("T4_2Started");
                            sem_unlink("T4_1Terminated");
                            exit(0);
                        default: // still in P2
                            waitpid(childPid3, NULL, 0); // wait for P4 to finish
                            break;
                    }
            }
            for(int i = 0; i < 4; i++) {
                pthread_join(th[i], NULL);
            }
            info(END, 2, 0);
			exit(0);
			break;
		default: // parent, still in P1
            waitpid(childPid1, NULL, 0);
            // create P5
            int childPid4 = fork();
            switch(childPid4) {
                case -1:
                    printf("ERROR\n");
                    exit(1);
                case 0: // in P5
                    info(BEGIN, 5, 0);
                    info(END, 5, 0);
                    exit(0);
                default: // still in P1
                    waitpid(childPid4, NULL, 0);
                    break;
            }
    }
    info(END, 1, 0);
    return 0;
}
