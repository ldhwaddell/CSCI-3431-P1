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
    //Ensure input is integer
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
    int numRectangles;

    numRectangles = GetNumRectangles();
    if (numRectangles == -1)
    {
        printf("[Error]: Must enter an integer\n");
        exit(1);
    }
    else
    {
        // Call function to spawn child processes
        //  Serves as entry to program
        printf("success\n");
    }

    printf("Approximating with number of rectangles = %d\n", numRectangles);

    float test;
    test = f(-4);
    printf("%f", test);

    return 0;
}
