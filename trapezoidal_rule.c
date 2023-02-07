#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PROCESSES 8

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
    if (scanf("%d", &num) == 1)
    {
        return num;
    }
    else
    {
        return -1;
    }
}

int GetNumProcesses()
{
    int num;
    printf("Enter the number of processes to approximate the function x^2+1 with: ");
    // Ensure input is integer
    if (scanf("%d", &num) == 1)
    {
        return num;
    }
    else
    {
        return -1;
    }
}

int main(void)
{
    int numRectangles, numProcesses;
    int fds0[2];
    int pipes[8] = {fds0[2], fds0[2]};

    numRectangles = GetNumRectangles();
    if (numRectangles == -1)
    {
        printf("[Error]: Must enter an integer for number of rectangles to approximate with\n");
        exit(1);
    }
    numProcesses = GetNumProcesses();

    if (numProcesses == -1)
    {
        printf("[Error]: Must enter an integer less than 9 for processes to approximate the function with\n");
        exit(1);
    }

    // Call function to spawn child processes
    //  Serves as entry to program
    printf("Approximating with number of rectangles = %d\n", numRectangles);

    float test;
    test = f(-4);
    printf("%f", test);

    return 0;
}

// Child Example
// pid_t pid=fork();

// if(pid < 0)
// {
//     perror("couldn't fork");
//     exit(1);
// }

// if(pid == 0) {
//         // Child code
//         do_stuff();
//         exit(0);
// }

// Parent code