// Vikrant Suresh Tripathi
// Roll No. 2103141

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 20
#define WINNING_CONDITION 10

static int terminationFlag = 0;

// Custom signal handler to handle the termination of the game
void handleTermination(int sig) {
    if (sig == SIGUSR1) {
        terminationFlag = 1;
    } else if (sig == SIGUSR2) {
        terminationFlag = 2;
    }
}

int main() {
    pid_t child1_pid, child2_pid;

    // Create the pipe descriptors
    int pipe1[2], pipe2[2];
    pipe(pipe1);
    pipe(pipe2);

    // Assign the custom signal handler to SIGUSR1 and SIGUSR2 signals
    signal(SIGUSR1, handleTermination);
    signal(SIGUSR2, handleTermination);

    if ((child1_pid = fork()) == 0) {
        // Child 1
        srand((unsigned int)time(NULL) ^ getpid());
        close(pipe1[0]); // Close the read end as the child will not read

        // Generate a random number and place it on the pipe
        while (!terminationFlag) {
            int randomNumber = rand() % 100;
            char buffer[BUFSIZE];
            sprintf(buffer, "%d", randomNumber);
            write(pipe1[1], buffer, BUFSIZE);
        }

        // Handle the exit of the child process by displaying a message
        if (terminationFlag == 1) {
            printf("Child 1: I am the winner\n");
        } else if (terminationFlag == 2) {
            printf("Child 1: Child 2 is the winner\n");
        }
        printf("Exiting from Child 1\n\n");
    } else {
        if ((child2_pid = fork()) == 0) {
            // Child 2
            srand((unsigned int)time(NULL) ^ getpid());
            close(pipe2[0]); // Close the read end as the child will not read

            // Generate a random number and place it on the pipe
            while (!terminationFlag) {
                int randomNumber = rand() % 100;
                char buffer[BUFSIZE];
                sprintf(buffer, "%d", randomNumber);
                write(pipe2[1], buffer, BUFSIZE);
            }

            // Handle the exit of the child process by displaying a message
            if (terminationFlag == 1) {
                printf("Child 2: Child 1 is the winner\n");
            } else if (terminationFlag == 2) {
                printf("Child 2: I am the winner\n");
            }
            printf("Exiting from Child 2\n\n");
        } else {
            // Parent process
            close(pipe1[1]);
            close(pipe2[1]); // Close the write ends as the parent will not write

            srand((unsigned int)time(NULL) ^ getpid());
            int round = 1;
            int scoreChild1 = 0, scoreChild2 = 0;
            char buffer[BUFSIZE];
            char winner;
            int numberChild1, numberChild2, choice;

            while (1) {
                printf("Round number: %d\n", round++);
                choice = rand() % 2; // Parent chooses a flag: 0 for MAX and 1 for MIN

                // Read and print the first integer from Child 1
                read(pipe1[0], buffer, BUFSIZE);
                sscanf(buffer, "%d", &numberChild1);
                printf("Integer received from Child 1: %d\n", numberChild1);

                // Read and print the second integer from Child 2
                read(pipe2[0], buffer, BUFSIZE);
                sscanf(buffer, "%d", &numberChild2);
                printf("Integer received from Child 2: %d\n", numberChild2);

                // Determine the winner for the round and update scores
                if (choice) {
                    printf("Parent's choice of flag: MIN\n");
                } else {
                    printf("Parent's choice of flag: MAX\n");
                }

                if (numberChild1 == numberChild2) {
                    printf("This round is a tie!\n");
                } else if ((numberChild1 < numberChild2 && choice) || (numberChild1 > numberChild2 && !choice)) {
                    printf("Child 1 gets a point\n");
                    scoreChild1++;
                } else {
                    printf("Child 2 gets a point\n");
                    scoreChild2++;
                }

                printf("Updated scores: Child 1 = %d, Child 2 = %d\n\n", scoreChild1, scoreChild2);

                // Break when the score of any child reaches the winning condition
                if (scoreChild1 == WINNING_CONDITION || scoreChild2 == WINNING_CONDITION) {
                    break;
                }
            }

            // Send appropriate signal to the children to let them know who the winner is
            if (scoreChild1 > scoreChild2) {
                kill(child1_pid, SIGUSR1);
                kill(child2_pid, SIGUSR1);
            } else {
                kill(child1_pid, SIGUSR2);
                kill(child2_pid, SIGUSR2);
            }

            // Wait for the children to terminate and then terminate the parent
            int statusChild1, statusChild2;
            waitpid(child1_pid, &statusChild1, 0);
            waitpid(child2_pid, &statusChild2, 0);
            printf("Exiting from Parent\n");
        }
    }

    return 0;
}
