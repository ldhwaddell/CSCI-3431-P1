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

void childCode(int ptc[][2], int ctp[][2], int processNumber, float intervalWidth, int maxIterations)
{
    // Read value from parent
    float x_i, s_i = 0;
    float x_i_plus_1;

    // Close write end of parent to child pipe
    close(ptc[processNumber][WRITE]);
    read(ptc[processNumber][READ], &x_i, sizeof(float));

    // Iterate over the max possible amount of rectangles times
    for (int i = 0; i < maxIterations; i++)
    //while(1)
    {
        // calculate x_i+1
        x_i_plus_1 = x_i + intervalWidth;
        printf("num: %d, xi %f, xi1 %f\n", processNumber, x_i, x_i_plus_1);

        // calculate s_i
        s_i = intervalWidth * (f(x_i) + f(x_i_plus_1)) / 2;

        // send back to parent
        close(ctp[processNumber][READ]);
        write(ctp[processNumber][WRITE], &s_i, sizeof(float));
        printf("writing back si val: %f", s_i);
        // Read in a new x_i value
        read(ptc[processNumber][READ], &x_i, sizeof(float));

        
    }
}

float parentCode(int ptc[][2], int ctp[][2], int numProcesses, int numRectangles, float intervalWidth, float startInterval)
{
    float s_i, x_i = 0;
    float sum = 0;

    // Assign the first N trapezoids to the N child processes
    for (int i = 0; i < numProcesses; i++)
    {
        // Close read end of ptc
        close(ptc[i][READ]);
        // set x_i value
        x_i = startInterval + (i * intervalWidth);

        //  Write to child the x_i value
        printf("writing to child %d value %f\n", i, x_i);
        write(ptc[i][WRITE], &x_i, sizeof(float));
    }

    if (numProcesses == numRectangles)
    {
        for (int i = 0; i < numProcesses; i++)
        {
            close(ctp[i][WRITE]);
            read(ctp[i][READ], &s_i, sizeof(float));

            sum += s_i;
        }
    }
    else
    {
        // For the remaining trapezoids
        for (int j = numProcesses; j < numRectangles; j++)
        {
            close(ctp[(j % numProcesses)][WRITE]);
            read(ctp[(j % numProcesses)][READ], &s_i, sizeof(float));
            // printf("s_i: %f\n", s_i);
            //  update sum
            sum += s_i;
            x_i = startInterval + (j * intervalWidth);
            write(ptc[(j % numProcesses)][WRITE], &x_i, sizeof(float));
        }
    }

    return sum;
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

    // Create an array of numProcesses length to hold the pid of each process
    pid_t pids[numProcesses];

    // Get current process ID so ID of parent is known
    pid_t parent_pid = getpid();

    // Create variables for interval start, end, and width of rectangles
    float startInterval = 0.0;
    float endInterval = 2.0;
    float intervalWidth = (endInterval - startInterval) / numRectangles;

    // Call function to spawn child processes
    printf("\n-------------------------------------------------------------------------\n");
    printf("Approximating function 'x^2 + 1' with %d processes and %d rectangles.\n", numProcesses, numRectangles);
    printf("Rectangle width = %f.\n", intervalWidth);
    printf("-------------------------------------------------------------------------\n\n");
    printf("Creating %d sets of pipes: \n", numProcesses);
    // Create all the pipes
    for (int i = 0; i < numProcesses; i++)
    {

        if (pipe(fd_ctp[i]) < 0)
        {
            printf("[Error]: Unsuccessful pipe creation for child-to-parent pipe %d. Program terminating.\n", i);
            exit(1);
        }
        else
        {
            printf("Child-to-parent pipe %d successfully created\n", i);
        }
        if (pipe(fd_ptc[i]) < 0)
        {
            printf("[Error]: Unsuccessful pipe creation for parent-to-child pipe %d. Program terminating.\n", i);
            exit(1);
        }
        else
        {
            printf("Parent-to-child pipe %d successfully created\n", i);
        }
    }

    printf("\nParent Process ID = %d\n", pids[0]);

    printf("\nCreating %d child processes: \n", numProcesses);

    int maxIterations = (numRectangles + numProcesses - 1) / numProcesses;

    // Create all the children

    for (int i = 0; i < numProcesses; i++)
    {

        // Spawn new child process
        pids[i] = fork();

        if (pids[i] < 0)
        {
            printf("[Error]: Unsuccessful fork by child %d with error: %d. Program terminating.\n", i, pids[i]);
            exit(1);
        }
        else if (pids[i] == 0)
        {

            // printf("Child number %d created. Calling child function\n", i);
            childCode(fd_ptc, fd_ctp, i, intervalWidth, maxIterations);
            break;
        }
    }

    if (getpid() == parent_pid)
    {
        // printf("parent \n");
        // int length = sizeof(pids) / sizeof(pids[0]);

        // for (int i = 0; i < length; i++)
        // {
        //     printf("%d ", pids[i]);
        // }
        // printf("\n");

        printf("Sum is: %f\n", parentCode(fd_ptc, fd_ctp, numProcesses, numRectangles, intervalWidth, startInterval));
        exit(0);
    }

    return 0;
}