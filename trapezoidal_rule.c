#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PROCESSES 8
#define MAX_RECTANGLES 64
#define READ 0
#define WRITE 1

// Function x^2 +1 to approximate
float f(float x)
{
    return (x * x) + 1;
}

// Function to get the number of rectangles to approximate the function with
int GetNumRectangles()
{

    int num;
    printf("Enter the number of rectangles to approximate the function x^2+1 with: ");
    // Ensure input is integer
    if ((scanf("%d", &num) == 1) && num > 0 && num <= MAX_RECTANGLES)
    {
        return num;
    }
    else
    {
        return -1;
    }
}

// Function to get the number of processes to use in the approximation
int GetNumProcesses()
{
    int num;
    printf("Enter the number of processes to approximate the function x^2+1 with: ");
    // Ensure input is integer
    if ((scanf("%d", &num) == 1) && num > 0 && num <= MAX_PROCESSES)
    {

        return num;
    }
    else
    {
        return -1;
    }
}

// Function to spawn in N processes as user inputs
void spawnChildren(int N, int ctp[][2], int ptc[][2])
{
    pid_t pid;

    for (int i = 1; i < N + 1; i++)
    {

        // Spawn new child process
        pid = fork();

        if (pid < 0)
        {
            printf("[Error]: Unsuccessful fork by child %d with pid: %d. Program terminating.\n", i, getpid());
            exit(1);
        }
        else if (pid == 0)
        {

            printf("Child number %d created with pid %d\n", i, getpid());
            break;
        }
        else
        {
            // Parent
            close(ptc[i][READ]);
            write(ptc[i][WRITE], &i, sizeof(int));
        }
    }
}

int main(void)
{
    int numRectangles, numProcesses;

    // Validate number of rectangles user enters
    numRectangles = GetNumRectangles();
    if (numRectangles == -1)
    {
        printf("[Error]: Must enter an integer in range [1,64] for number of rectangles to approximate with!\n");
        exit(1);
    }

    // Validate number of processes user enters
    numProcesses = GetNumProcesses();
    if (numProcesses == -1)
    {
        printf("[Error]: Must enter an integer in range [1, 8] for number processes to approximate the function with!\n");
        exit(1);
    }

    // if user wants more processes than rectangles, just set them equal to eachother to stop useless process creation
    if (numProcesses > numRectangles)
    {
        printf("[Warning]: Requested number of processes greater than number of rectangles. Creating %d rectangles instead\n", numProcesses);
        numProcesses = numRectangles;
    }

    // Create numProcesses amount of pipes for parent to child and child to parent communication
    int fd_ctp[numProcesses][2];
    int fd_ptc[numProcesses][2];

    // Create variables for interval start, end, and width of rectangles
    float startInterval = 0.0, endInterval = 2.0;

    float intervalWidth = (endInterval - startInterval) / numRectangles;

    // Call function to spawn child processes
    printf("\n-------------------------------------------------------------------------\n");
    printf("Approximating function 'x^2 + 1' with %d processes and %d rectangles.\n", numProcesses, numRectangles);
    printf("Rectangle width = %f.\n", intervalWidth);
    printf("-------------------------------------------------------------------------\n\n");
    printf("Creating %d child processes: \n", numProcesses);


    spawnChildren(numProcesses, fd_ctp, fd_ptc);


    return 0;
}