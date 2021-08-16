#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef struct{
    int shmid;        //shared memory id
    int shmnum;       //shared memory number
    sem_t sem;        //shared memory semaphore
} SharedMem;

//creating children processes
void creat_fork (int n, int* id, int* n_process) {
    for (int i = 0; i < n; i++) {
        id[i] = fork();
        if (id[i] == 0) {
            *n_process = i + 1;
            break;
        } else if (id[i] < 0) {
            printf("Semaphore could not be created.\n");
            exit(1);
        }
    }

    return;
}

//zeroing array
void zeroing (int n, int* id) {
    for (int i = 0; i < n; i++) {
        id[i] = 0;
    }

    return;
}

//function "sleep"
void sleeping (int time_sleep_max, int time_sleep_min) {
    srand(getpid() * time(NULL));
    usleep((rand() % (time_sleep_max - time_sleep_min + 1) + time_sleep_min) * 1000);
    return;
}

//creat share memory for processes
SharedMem* creat_share_memory (key_t key) {
    int shmid = shmget(key, sizeof(SharedMem), IPC_CREAT | 0666);
    if(shmid < 0){
        printf("Shared memory could not be created.\n");
        exit(1);
    }
    SharedMem *shm = shmat(shmid, NULL, 0);
    if(shm == (SharedMem *) -1){
        printf("Shared memory could not be created.\n");
        exit(1);
    }
    shm->shmid = shmid;

    return shm;
}

//delete share memory for processes
void delete_share_memory (SharedMem *shm) {
    sem_destroy(&shm->sem);
    int shmid = shm->shmid;
    if(shmdt(shm)){
        return;
    }
    shmctl(shmid, IPC_RMID, NULL);
}

//print message in to file
void print_message (SharedMem* shm_message, char* who, int process, char* message, FILE* file) {
    sem_wait(&shm_message->sem);
    if (strcmp(who, "Santa") != 0) {
        fprintf(file,"%d: %s %d: %s\n", ++shm_message->shmnum, who, process, message);
    } else {
        fprintf(file,"%d: %s: %s\n", ++shm_message->shmnum, who, message);
    }
    fflush(file);
    sem_post(&shm_message->sem);
}

//checking input parameters
void check_const (int NE, int NR, int TE, int TR) {
    if (NE <= 0 || NE >= 1000) {
        printf("Error: NE is not the right number.\n");
        exit(1);
    }

    if (NR <= 0 || NR >= 20) {
        printf("Error: NR is not the right number.\n");
      exit(1);
    }

    if (TE < 0 || TE > 1000) {
        printf("Error: TE is not the right number.\n");
        exit(1);
    }

    if (TR < 0 || TR > 1000) {
        printf("Error: TR is not the right number.\n");
        exit(1);
    }
}

// process Santa
void process_santa (SharedMem* shm_all, SharedMem* shm_main, SharedMem* shm_santa, SharedMem* shm_elfs,
                    SharedMem* shm_sobs, SharedMem* shm_message, FILE* file, int NR, int NE) {

    sem_post(&shm_all->sem);
    sem_wait(&shm_main->sem);
    print_message(shm_message, "Santa", 0, "going to sleep", file);
    while (1) {
        if (shm_sobs->shmnum == NR){
            print_message(shm_message, "Santa", 0, "closing workshop", file);
            shm_santa->shmnum = -1;
            for (int i = 0; i < NR + NE; i++) {
                sem_post(&shm_elfs->sem);
                sem_post(&shm_sobs->sem);
            }
            for (int i = 0; i < NR; i++) {
                sem_wait(&shm_santa->sem);
            }
            break;
        }

        if (shm_elfs->shmnum >= 3 && shm_sobs->shmnum != NR){
            print_message(shm_message, "Santa", 0, "helping elves", file);
            shm_santa->shmnum = 3;
            for (int i = 0; i < 3; i++) {
                sem_post(&shm_elfs->sem);
            }
            for (int i = 0; i < 3; i++) {
                sem_wait(&shm_santa->sem);
            }
            shm_elfs->shmnum = shm_elfs->shmnum - 3;
            print_message(shm_message, "Santa", 0, "going to sleep", file);
        }
    }
    print_message(shm_message, "Santa", 0, "Christmas started", file);
}

// process elfs
void process_elfs(SharedMem* shm_all, SharedMem* shm_main, SharedMem* shm_santa, SharedMem* shm_elfs,
                 SharedMem* shm_sobs, SharedMem* shm_message, FILE* file, int NR, int TE, int process_elf) {

    sem_post(&shm_all->sem);
    sem_wait(&shm_main->sem);

    print_message(shm_message, "Elf", process_elf, "started", file);
    while (1) {
        sleeping(TE, 0);
        print_message(shm_message, "Elf", process_elf, "need help", file);
        shm_elfs->shmnum++;
        sem_wait(&shm_elfs->sem);
        if (shm_sobs->shmnum == NR && shm_santa->shmnum <= 0){
            print_message(shm_message, "Elf", process_elf, "taking holidays", file);
            break;
        } else {
            shm_santa->shmnum = shm_santa->shmnum - 1;
            print_message(shm_message, "Elf", process_elf, "get help", file);
            sem_post(&shm_santa->sem);
        }
    }
}

// process sobs
void process_sobs(SharedMem* shm_all, SharedMem* shm_main, SharedMem* shm_santa, SharedMem* shm_sobs,
                  SharedMem* shm_message, FILE* file, int TR, int process_sob) {

    sem_post(&shm_all->sem);
    sem_wait(&shm_main->sem);

    print_message(shm_message, "RD", process_sob, "rstarted", file);
    sleeping(TR, TR/2);
    print_message(shm_message, "RD", process_sob, "return home", file);
    shm_sobs->shmnum = shm_sobs->shmnum + 1;
    sem_wait(&shm_sobs->sem);
    print_message(shm_message, "RD", process_sob, "get hitched", file);
    sem_post(&shm_santa->sem);
}

int main(int argc, char** argv) {

    //check count arguments
    if (argc != 5) {
        printf("Not enough arguments.\n");
        return 2;
    }

    //creating and initializing variables
    int NE = atoi(argv[1]);
    int NR = atoi(argv[2]);
    int TE = atoi(argv[3]);
    int TR = atoi(argv[4]);
    check_const(NE, NR, TE, TR);

    //open file
    FILE* file;
    if ((file = fopen("proj2.out", "w")) == NULL) {
        printf("Could not open file\n");
        exit(1);
    }

    /* creating and initializing semaphores */
    SharedMem* shm_sobs = creat_share_memory(1234);
    sem_init(&shm_sobs->sem, 1, 0);
    shm_sobs->shmnum = 0;

    SharedMem* shm_elfs = creat_share_memory(4444);
    sem_init(&shm_elfs->sem, 1, 0);
    shm_elfs->shmnum = 0;

    SharedMem* shm_santa = creat_share_memory(4100);
    sem_init(&shm_santa->sem, 1, 0);
    shm_santa->shmnum = 0;

    SharedMem* shm_all = creat_share_memory(4242);
    sem_init(&shm_all->sem, 1, 0);

    SharedMem* shm_main = creat_share_memory(4343);
    sem_init(&shm_main->sem, 1, 0);

    SharedMem* shm_message = creat_share_memory(5674);
    sem_init(&shm_message->sem, 1, 1);
    shm_message->shmnum = 0;


    /* creating processes: Santa */
    int id = fork();
    if (id < 0)
        exit(1);

    /* process Santa */
    if (id == 0) {
        process_santa(shm_all, shm_main, shm_santa, shm_elfs, shm_sobs, shm_message, file, NR, NE);
        return 0;
    }

    /* creating processes: Elfs */
    int id_elf[NE];
    int process_elf = 0;
    zeroing(NE, id_elf);
    creat_fork(NE, id_elf, &process_elf);

    /* process Elfs */
    if (id_elf[NE-1] == 0) {
        process_elfs(shm_all, shm_main, shm_santa, shm_elfs, shm_sobs, shm_message, file, NR, TE, process_elf);
        return 0;
    }

    /* creating processes: Sobs */
    int id_sobs[NR];
    int process_sob = 0;
    zeroing(NR, id_sobs);
    creat_fork(NR, id_sobs, &process_sob);

    /* process Sobs */
    if (id_sobs[NR-1] == 0) {
        process_sobs(shm_all, shm_main, shm_santa, shm_sobs, shm_message, file, TR, process_sob);
        return 0;
    }

    //semaphore for process synchronization
    for (int i = 0; i < (NR + NE + 1); i++) {
        sem_wait(&shm_all->sem);
    }

    //semaphore for open final step
    for (int i = 0; i < (NR + NE + 2); i++) {
        sem_post(&shm_main->sem);
    }

    //semaphore for waiting for all processes to terminate
    while (wait(NULL) != -1);

    //delete all shared memory
    delete_share_memory(shm_sobs);
    delete_share_memory(shm_elfs);
    delete_share_memory(shm_santa);
    delete_share_memory(shm_all);
    delete_share_memory(shm_main);
    delete_share_memory(shm_message);

    //close file
    fclose(file);

    return 0;
}
