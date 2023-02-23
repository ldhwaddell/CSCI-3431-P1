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

// Function used to get the upper and lower bound to integrate over, while validating input
float getInterval(char str[])
{
    float num;
    printf("Enter the %s of the interval to integrate over: ", str);
    // Ensure input is integer
    if ((scanf("%f", &num) == 1))
    {
        return num;
    }
    else
    {
        return -1;
    }
}

// Funciton to get the # of rectangles and # of processes the compute the integration with, while vlaidaitng input
int getIntegrationParams(char str[], int maxBound)
{
    int num;
    printf("Enter the number of %s to approximate the function x^2+1 with: ", str);
    // Ensure input is integer
    if ((scanf("%d", &num) == 1) && num > 0 && num <= maxBound)
    {
        return num;
    }
    else
    {
        return -1;
    }
}

// Function to be called by each child process. Function loops numerous times
// reading in values from the parent, writing back the calculated area of trapezoid, and writing back the values.
void childCode(int ptc[][2], int ctp[][2], int processNumber, float intervalWidth, int maxIterations)
{
    // Read value from parent
    float x_i, x_i_plus_1, s_i = 0;

    // Iterate over the max possible amount of rectangles times
    for (int i = 0; i < maxIterations; i++)
    {
        // Close write end of parent to child pipe
        close(ptc[processNumber][WRITE]);
        // Read in a new x_i value
        read(ptc[processNumber][READ], &x_i, sizeof(float));

        // calculate x_i+1
        x_i_plus_1 = x_i + intervalWidth;

        // calculate s_i
        s_i = intervalWidth * (f(x_i) + f(x_i_plus_1)) / 2;

        // Close read end of child to parent pipe
        close(ctp[processNumber][READ]);
        // Send s_i back to parent
        write(ctp[processNumber][WRITE], &s_i, sizeof(float));
    }
    // Child exits once iterating maxIterations number of times
    exit(0);
}

// Function called by the parentCode function with goal of making a final iteraiton over the pipes from children to parent
// and making sure they do not have any information remaining in them
float flushPipes(int ctp[][2], int numProcesses, float currentSum)
{
    float s_i;
    printf("\n-------------------------------------------------------------------------");
    printf("\nComputations complete. Flushing Pipes\n");
    for (int i = 0; i < numProcesses; i++)
    {
        close(ctp[i][WRITE]);
        read(ctp[i][READ], &s_i, sizeof(float));
        printf("Reading back value %f from child %d\n", s_i, i);
        currentSum += s_i;
    }
    printf("-------------------------------------------------------------------------\n");

    return currentSum;
}

// Code to be exectude only by the parent process
// Job is to assign various x_i values to children and read back their responses
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
        printf("Writing value %f to child %d\n", x_i, i);
        write(ptc[i][WRITE], &x_i, sizeof(float));
    }

    // For the remaining trapezoids
    for (int j = numProcesses; j < numRectangles; j++)
    {
        close(ctp[(j % numProcesses)][WRITE]);
        read(ctp[(j % numProcesses)][READ], &s_i, sizeof(float));
        printf("Reading value %f from child %d\n", s_i, j % numProcesses);
        //  update sum
        sum += s_i;
        x_i = startInterval + (j * intervalWidth);
        printf("Writing value %f to child %d\n", x_i, j % numProcesses);

        close(ptc[(j % numProcesses)][READ]);
        write(ptc[(j % numProcesses)][WRITE], &x_i, sizeof(float));
    }

    printf("-------------------------------------------------------------------------\n");

    sum = flushPipes(ctp, numProcesses, sum);

    return sum;
}

int main(void)
{
    int numRectangles, numProcesses, maxIterations;
    float startInterval, endInterval, intervalWidth, sum;
    pid_t parent_pid;
    char start[] = "start";
    char end[] = "end";
    char rectangles[] = "rectangles";
    char processes[] = "processes";

    // Get start of interval to sum over
    startInterval = getInterval(start);
    if (startInterval == -1)
    {
        printf("[Error]: Must enter a float or an integer for start of range to integrate over!\n");
        exit(1);
    }

    // Get end of interval to sum over
    endInterval = getInterval(end);
    if (endInterval == -1)
    {
        printf("[Error]: Must enter a float or an integer for end of range to integrate over!\n");
        exit(1);
    }

    // Error checking on the interval vlaues
    if (startInterval == endInterval)
    {
        printf("[Error]: Interval start value must not be the same as interval end value!\n");
        exit(1);
    }
    else if (startInterval > endInterval)
    {
        printf("\n[Warning]: Requested start interval is less than interval end. Swapping values to use %f as interval start and %f as interval end.\n\n", endInterval, startInterval);
        float temp;
        temp = startInterval;
        startInterval = endInterval;
        endInterval = temp;
    }

    // Validate number of rectangles user enters
    numRectangles = getIntegrationParams(rectangles, MAX_RECTANGLES);
    if (numRectangles == -1)
    {
        printf("[Error]: Must enter an integer in range [1,64] for number of rectangles to approximate with!\n");
        exit(1);
    }

    // Validate number of processes user enters
    numProcesses = getIntegrationParams(processes, MAX_PROCESSES);
    if (numProcesses == -1)
    {
        printf("[Error]: Must enter an integer in range [1, 8] for number processes to approximate the function with!\n");
        exit(1);
    }

    // if user wants more processes than rectangles, just set them equal to eachother to stop useless process creation
    if (numProcesses > numRectangles)
    {
        printf("\n[Warning]: Requested number of processes greater than number of rectangles. Using %d processes instead.\n", numRectangles);
        numProcesses = numRectangles;
    }

    // Create numProcesses amount of pipes for parent to child and child to parent communication
    int fd_ctp[numProcesses][2];
    int fd_ptc[numProcesses][2];

    // Get current process ID so ID of parent is known
    parent_pid = getpid();

    // Assign value to variable for width of rectangles
    intervalWidth = (endInterval - startInterval) / numRectangles;

    // Max iterations represents the theoretical greatest number of times a child would need to calculate a rectangle
    maxIterations = (numRectangles + numProcesses - 1) / numProcesses;

    // Call function to spawn child processes
    printf("\n-------------------------------------------------------------------------\n");
    printf("Approximating function 'x^2 + 1' with %d processes and %d rectangles.\n", numProcesses, numRectangles);
    printf("Interval range [%f, %f]\n", startInterval, endInterval);
    printf("Rectangle width = %f.\n", intervalWidth);
    printf("-------------------------------------------------------------------------\n\n");
    printf("-------------------------------------------------------------------------\n");
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
    printf("-------------------------------------------------------------------------\n\n");
    printf("-------------------------------------------------------------------------\n");
    printf("Parent Process ID = %d\n", parent_pid);
    printf("-------------------------------------------------------------------------\n\n");
    printf("-------------------------------------------------------------------------\n");
    printf("Creating %d child processes: \n", numProcesses);

    // Create all the children
    pid_t pid;

    for (int i = 0; i < numProcesses; i++)
    {

        // Spawn new child process
        pid = fork();

        if (pid < 0)
        {
            printf("[Error]: Unsuccessful fork by child %d with error: %d. Program terminating.\n", i, pid);
            exit(1);
        }
        else if (pid == 0)
        {
            // Child executes child code then breaks to ensure child loop ends
            childCode(fd_ptc, fd_ctp, i, intervalWidth, maxIterations);
            break;
        }
    }

    // Ensure only the parent (child spawning) process runs the parent code
    if (getpid() == parent_pid)
    {
        sum = parentCode(fd_ptc, fd_ctp, numProcesses, numRectangles, intervalWidth, startInterval);
        printf("\nSum with %d rectangles spread over %d processes is: %f\n", numRectangles, numProcesses, sum);
        // Ensure all child processes finish before exiting
        exit(0);
    }

    return 0;
}