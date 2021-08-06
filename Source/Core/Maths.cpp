#include "Maths.h"

// ***********************************************************************

double generateGaussian(double mean, double stdDev)
{
    // Implementation of Marsaglia polar method. Generates two normally distributed random variables in the range of 0-1
    // Thread local storage is used to return the alternate of the two random variables every other call
    
    thread_local double spare;
    thread_local bool hasSpare = false;

    if (hasSpare) 
    {
        hasSpare = false;
        return spare * stdDev + mean;
    }
    else {
        double u, v, s;
        do 
        {
            u = (rand() / ((double)RAND_MAX)) * 2.0 - 1.0;
            v = (rand() / ((double)RAND_MAX)) * 2.0 - 1.0;
            s = u * u + v * v;
        } 
        while (s >= 1.0 || s == 0.0);

        s = sqrt(-2.0 * log(s) / s);
        spare = v * s;
        hasSpare = true;

        return mean + stdDev * u * s;
    }
}