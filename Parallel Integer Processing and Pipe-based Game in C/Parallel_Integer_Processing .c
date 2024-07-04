// Vikrant Suresh Tripathi
// Roll No. 2103141

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/wait.h>

// Function to create a semaphore
int sem_create(int num_semaphores) {
    int semid;

    /* create new semaphore set of semaphores */
    if ((semid = semget(IPC_PRIVATE, num_semaphores, IPC_CREAT | 0600)) < 0) {
        perror("error in creating semaphore");  /* 0600 = read/alter by user */
        exit(1);
    }
    return semid;
}

// Function to initialize a semaphore
void sem_init(int semid, int index, int value) {
    if (semctl(semid, index, SETVAL, value) < 0) {
        perror("error in initializing semaphore");
        exit(1);
    }
}

// Function to perform a P (wait) operation on a semaphore
void P(int semid, int index) {
    struct sembuf sops[1];      /* only one semaphore operation to be executed */

    sops[0].sem_num = index;      /* define operation on semaphore with given index */
    sops[0].sem_op = -1;            /* subtract 1 to value for P operation */
    sops[0].sem_flg = 0;            /* type "man semop" in shell window for details */

    if (semop(semid, sops, 1) == -1) {
        perror("error in semaphore operation");
        exit(1);
    }
}

// Function to perform a V (signal) operation on a semaphore
void V(int semid, int index) {
    struct sembuf sops[1];      /* define operation on semaphore with given index */

    sops[0].sem_num = index;        /* define operation on semaphore with given index */
    sops[0].sem_op = 1;             /* add 1 to value for V operation */
    sops[0].sem_flg = 0;             /* type "man semop" in shell window for details */

    if (semop(semid, sops, 1) == -1) {
        perror("error in semaphore operation");
        exit(1);
    }
}

int main() {
    // Create semaphores for synchronization
    int* b0;
    int* b1;
    int semid = sem_create(4);  // Three semaphores for P0, P1, and P2

    // Initialize semaphores
    sem_init(semid, 0, 1);  // Semaphore for P0, initialized to 1
    sem_init(semid, 1, 0);  // Semaphore for P1, initialized to 0
    sem_init(semid, 2, 1);  // Semaphore for P2, initialized to 0
    sem_init(semid, 3, 0);

    // Create shared memory for B0 and B1 (you can use mmap)
    caddr_t B0, B1;
    B0 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    B1 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    b0 = (int*)B0;
    b1 = (int*)B1;

    *B0 = 0;
    *B1 = 0;

    pid_t pid;

    // Create P0 process
    if ((pid = fork()) == 0) {
        // P0 process logic
        for (int i = 1; i <= 1000; i++) {
            P(semid, 0);    // Wait for semaphore 0
            *b0 = i;        // Place the current value in B0
            V(semid, 1);    // Signal semaphore 1
        }
        P(semid, 0);        // Wait for semaphore 0
        *b0 = 0;            // Place the sentinel value in B0
        V(semid, 1);        // Signal semaphore 1
        exit(0);
    }

    // Create P1 process
    if ((pid = fork()) == 0) {
        // P1 process logic
        while (1) {
            P(semid, 1);    // Wait for semaphore 1
            int value = *b0;    // Read the value from B0
            V(semid, 0);        // Signal semaphore 0

            if (value == 0) {
                P(semid, 2);    // Wait for semaphore 2
                *b1 = 0;        // Place the sentinel value in B1
                V(semid, 3);    // Signal semaphore 3
                exit(0);
            }
            if (value % 2 != 0) {
                P(semid, 2);    // Wait for semaphore 2
                *b1 = value;    // Place the value in B1
                V(semid, 3);    // Signal semaphore 3
            }
        }
    }

    // Create P2 process
    if ((pid = fork()) == 0) {
        int count = 0;
        // P2 process logic
        while (1) {
            P(semid, 3);    // Wait for semaphore 3
            int value = *b1;    // Read the value from B1
            V(semid, 2);        // Signal semaphore 2

            if (value == 0) {
                printf("Total count of numbers not divisible by 2 and 3: %d\n", count);
                exit(0);
            }
            if (value % 3 != 0) {
                printf("%d\n", value);
                count++;        // Increment the count for each number not divisible by 3
            }
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    // Clean up semaphores
    semctl(semid, 0, IPC_RMID);

    return 0;
}
