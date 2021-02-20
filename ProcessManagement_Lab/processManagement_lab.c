#include "processManagement_lab.h"

/**
 * The task function to simulate "work" for each worker process
 * TODO#3: Modify the function to be multiprocess-safe 
 * */
void task(long duration)
{
    // simulate computation for x number of seconds
    usleep(duration*TIME_MULTIPLIER);

    // TODO: protect the access of shared variable below
    //lock semaphore
    sem_wait(sem_global_data);

    // update global variables to simulate statistics
    ShmPTR_global_data->sum_work += duration;
    ShmPTR_global_data->total_tasks ++;
    if (duration % 2 == 1) {
        ShmPTR_global_data->odd++;
    }
    if (duration < ShmPTR_global_data->min) {
        ShmPTR_global_data->min = duration;
    }
    if (duration > ShmPTR_global_data->max) {
        ShmPTR_global_data->max = duration;
    }

    //unlock semaphore
    sem_post(sem_global_data);
}


/**
 * The function that is executed by each worker process to execute any available job given by the main process
 * */
void job_dispatch(int i){

    // TODO#3:  a. Always check the corresponding shmPTR_jobs_buffer[i] for new  jobs from the main process
    //          b. Use semaphore so that you don't busy wait
    //          c. If there's new job, execute the job accordingly: either by calling task(), usleep, exit(3) or kill(getpid(), SIGKILL)
    //          d. Loop back to check for new job 

    while(true){
        sem_wait(sem_jobs_buffer[i]);

        char type = shmPTR_jobs_buffer[i].task_type;
        long time = shmPTR_jobs_buffer[i].task_duration;

        if(type == 't'){
            task(time);
        }

        if(type == 'w'){
            usleep(time*TIME_MULTIPLIER);
        }

        if(type == 'z'){
            exit(3);
        }

        if(type == 'i'){
            kill(getpid(), SIGKILL);
        }

        shmPTR_jobs_buffer[i].task_status == 0;

    }


    //printf("Hello from child %d with pid %d and parent id %d\n", i, getpid(), getppid());
    //exit(0); 

}

/** 
 * Setup function to create shared mems and semaphores
 * **/
void setup(){

    // TODO#1:  a. Create shared memory for global_data struct (see processManagement_lab.h)
    //          b. When shared memory is successfully created, set the initial values of "max" and "min" of the global_data struct in the shared memory accordingly
    // To bring you up to speed, (a) and (b) are given to you already. Please study how it works. 

    //          c. Create semaphore of value 1 which purpose is to protect this global_data struct in shared memory 
    //          d. Create shared memory for number_of_processes job struct (see processManagement_lab.h)
    //          e. When shared memory is successfully created, setup the content of the structs (see handout)
    //          f. Create number_of_processes semaphores of value 0 each to protect each job struct in the shared memory. Store the returned pointer by sem_open in sem_jobs_buffer[i]
    //          g. Return to main

    printf("Started task 1");
    
    ShmID_global_data = shmget(IPC_PRIVATE, sizeof(global_data), IPC_CREAT | 0666);
    if (ShmID_global_data == -1){
        printf("Global data shared memory creation failed\n");
        exit(EXIT_FAILURE);
    }
    ShmPTR_global_data = (global_data *) shmat(ShmID_global_data, NULL, 0);
    if ( (int) ShmPTR_global_data == -1){
        printf("Attachment of global data shared memory failed \n");
        exit(EXIT_FAILURE);
    }

    //set global data min and max
    ShmPTR_global_data->max = -1;
    ShmPTR_global_data->min = INT_MAX;

    //Create semaphore called semglobaldata of value 1 to protect the global data struct, variable already declared
    sem_global_data = sem_open("semglobaldata", O_CREAT | O_EXCL, 0644, 1);
    
    //Loop to check if failed
    while(true){
        if(sem_global_data == SEM_FAILED){
            sem_unlink("semglobaldata");
            sem_global_data = sem_open("semglobaldata", O_CREAT | O_EXCL, 0644, 1);
        }else{
            break;
        }
    }

    //Setting up shared memory for  each job struct, followed the example given by global data shared memory above

    for(int i=0; i<number_of_processes; i++){
        ShmID_jobs = shmget(IPC_PRIVATE, sizeof(job) * number_of_processes, IPC_CREAT | 0666);
        if (ShmID_jobs == -1){
            printf("Jobs shared memory creation failed\n");
            exit(EXIT_FAILURE);
        }

        shmPTR_jobs_buffer = (job *) shmat(ShmID_jobs, NULL, 0);
        if ((int) shmPTR_jobs_buffer == -1){
            printf("Attachment of jobs shared memory failed\n");
            exit(EXIT_FAILURE);
        }
    }

    //Set Up content of job structs
    shmPTR_jobs_buffer->task_duration = 1;
    shmPTR_jobs_buffer->task_type = 't';
    shmPTR_jobs_buffer->task_status = 0;

    for(int i=0; i<number_of_processes; i++){
        char* sem_name = malloc(sizeof(char)*10);
        sprintf(sem_name,"semjobs%i",i);
        sem_jobs_buffer[i] = sem_open(sem_name ,O_CREAT | O_EXCL, 0644, 0);

        //check if semaphore sucessfully opened
        while(true){
            if(sem_jobs_buffer[i] == SEM_FAILED){
                sem_unlink(sem_name);
                sem_jobs_buffer[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 0);
            }else{
                break;
            }
        }
    };

    
    printf("Ended task 1");

    return;

}

/**
 * Function to spawn all required children processes
 **/

void createchildren()
{
    // TODO#2:  a. Create number_of_processes children processes
    //          b. Store the pid_t of children i at children_processes[i]
    //          c. For child process, invoke the method job_dispatch(i)
    //          d. For the parent process, continue creating the next children
    //          e. After number_of_processes children are created, return to main

    
    printf("Started task 2");
    for (int i = 0; i < number_of_processes; i++)
    {
        
        children_processes[i] = fork();

        if (children_processes[i] < 0)
        {
            printf("Child Fork has failed");
            exit(EXIT_FAILURE); //exit error
        }
        if (children_processes[i] == 0)
        {
            job_dispatch(i);
            exit(1);
        }
        else
        {
           //let it continue making children
        }
    }

    
    printf("ended task 2");
    return;
}

/**
 * The function where the main process loops and busy wait to dispatch job in available slots
 * */
void main_loop(char* fileName){

    // load jobs and add them to the shared memory
    FILE* opened_file = fopen(fileName, "r");
    char action; //stores whether its a 'p' or 'w'
    long num; //stores the argument of the job 

    while (fscanf(opened_file, "%c %ld\n", &action, &num) == 2) { //while the file still has input

        //TODO#4: create job, busy wait
        //      a. Busy wait and examine each shmPTR_jobs_buffer[i] for jobs that are done by checking that shmPTR_jobs_buffer[i].task_status == 0. You also need to ensure that the process i IS alive using waitpid(children_processes[i], NULL, WNOHANG). This WNOHANG option will not cause main process to block when the child is still alive. waitpid will return 0 if the child is still alive. 
        //      b. If both conditions in (a) is satisfied update the contents of shmPTR_jobs_buffer[i], and increase the semaphore using sem_post(sem_jobs_buffer[i])
        //      c. Break of busy wait loop, advance to the next task on file 
        //      d. Otherwise if process i is prematurely terminated, revive it. You are free to design any mechanism you want. The easiest way is to always spawn a new process using fork(), direct the children to job_dispatch(i) function. Then, update the shmPTR_jobs_buffer[i] for this process. Afterwards, don't forget to do sem_post as well 
        //      e. The outermost while loop will keep doing this until there's no more content in the input file. 

        //keep track of whether new job has been run
        bool track_slot = false;

        while(true){
            for(int i=0; i<number_of_processes; i++){

                int alive = waitpid(children_processes[i], NULL, WNOHANG);
                int job_status = shmPTR_jobs_buffer[i].task_status;

                if(alive == 0 && job_status == 0){
                    //check process is still alive and previous task is done
                    shmPTR_jobs_buffer[i].task_duration = num;
                    shmPTR_jobs_buffer[i].task_status = 1;
                    shmPTR_jobs_buffer[i].task_type = action;
                    sem_post(sem_jobs_buffer[i]);
                    track_slot = true;
                    break;

                }else if(alive != 0 && job_status == 0 ){
                    //process is not alive but previous task is done
                    children_processes[i] = fork();

                    if(children_processes[i] == 0){
                        job_dispatch(i);
                    }

                    if(children_processes[i] > 0){
                        //parent, child done with process 
                        shmPTR_jobs_buffer[i].task_duration = num;
                        shmPTR_jobs_buffer[i].task_status = 1;
                        shmPTR_jobs_buffer[i].task_type = action;
                        sem_post(sem_jobs_buffer[i]);
                        track_slot = true;
                        break;
                    }
                    
                }else if(job_status == 1 && alive != 0){
                    children_processes[i] = fork();
                    if(children_processes[i] == 0){
                        job_dispatch(i);
                    }else if(children_processes[i] > 0){
                        shmPTR_jobs_buffer[i].task_duration = num;
                        shmPTR_jobs_buffer[i].task_status = 1;
                        shmPTR_jobs_buffer[i].task_type = action;
                        sem_post(sem_jobs_buffer[i]);
                        track_slot = true;
                        break;
                    }
                }
            }

            //after running through the loop, if job has been assigned to a slot, break while loop
            if(track_slot){
                break;
            }

        }


    }
    fclose(opened_file);

    printf("Main process is going to send termination signals\n");

    // TODO#4: Design a way to send termination jobs to ALL worker that are currently alive 
    //find out number of workers alive and number of jobs killed so far

    int workers_alive = 0;
    int jobs_killed = 0;
    for(int i=0; i<number_of_processes; i++){
        if(waitpid(children_processes[i], NULL, WNOHANG) == 0){
            workers_alive++;
        }
    }       

    while(true){
        for(int i = 0; i < number_of_processes; i++){
            if(waitpid(children_processes[i], NULL, WNOHANG) == 0){
                //killing process
                shmPTR_jobs_buffer[i].task_duration = 0;
                shmPTR_jobs_buffer[i].task_status = 1;
                shmPTR_jobs_buffer[i].task_type = 'z';
                jobs_killed++;
                sem_post(sem_jobs_buffer[i]);
            }
        }

        if(workers_alive == jobs_killed){
            //break while loop after ensuring all the processes have been killed
            break;
        }

    }

    //wait for all children processes to properly execute the 'z' termination jobs
    int process_waited_final = 0;
    pid_t wpid;
    while ((wpid = wait(NULL)) > 0){
        process_waited_final ++;
    }
    
    // print final results
    printf("Final results: sum -- %ld, odd -- %ld, min -- %ld, max -- %ld, total task -- %ld\n", ShmPTR_global_data->sum_work, ShmPTR_global_data->odd, ShmPTR_global_data->min, ShmPTR_global_data->max, ShmPTR_global_data->total_tasks);
}

void cleanup(){
    //TODO#4: 
    // 1. Detach both shared memory (global_data and jobs)
    // 2. Delete both shared memory (global_data and jobs)
    // 3. Unlink all semaphores in sem_jobs_buffer

    int detach_shared_mem; 
    int remove_shared_mem;
    int unlink_sem;

    detach_shared_mem = shmdt((void * ) ShmID_global_data);
    remove_shared_mem = shmctl(ShmID_global_data, IPC_RMID, NULL);
    detach_shared_mem = shmdt((void * ) ShmID_jobs);
    remove_shared_mem = shmctl(ShmID_jobs, IPC_RMID, NULL);

    unlink_sem = sem_unlink("semglobaldata");
    if(unlink_sem != 0){
        printf("Semaphore has failed to close");
    }

    for(int i = 0; i<number_of_processes; i++ ){
        char *sem_name = malloc(sizeof(char)*16);
        sprintf(sem_name, "semjobs%d", i);
        unlink_sem = sem_unlink(sem_name);

        if(unlink_sem != 0){
            printf("Semaphore jobs %d fails to close", i);
        }

        free(sem_name);
    }

}


// Real main
int main(int argc, char* argv[]){
    printf("Urgh");
    printf("Lab 1 Starts... and I know test");
    
    printf("Finished this line");
    struct timeval start, end;

    long secs_used,micros_used;

    //start timer
    gettimeofday(&start, NULL);

    //Check and parse command line options to be in the right format
    if (argc < 2) {
        printf("Usage: sum <infile> <numprocs>\n");
        exit(EXIT_FAILURE);
    }


    //Limit number_of_processes into 10. 
    //If there's no third argument, set the default number_of_processes into 1.  
    if (argc < 3){
        number_of_processes = 1;
    }
    else{
        if (atoi(argv[2]) < MAX_PROCESS) number_of_processes = atoi(argv[2]);
        else number_of_processes = MAX_PROCESS;
    }

    printf("Running tasks");
    setup();
    createchildren();
    main_loop(argv[1]);

    //parent cleanup
    cleanup();

    //stop timer
    gettimeofday(&end, NULL);

    double start_usec = (double) start.tv_sec * 1000000 + (double) start.tv_usec;
    double end_usec =  (double) end.tv_sec * 1000000 + (double) end.tv_usec;

    printf("Your computation has used: %lf secs \n", (end_usec - start_usec)/(double)1000000);


    return (EXIT_SUCCESS);
}
