# include <stdio.h>
# include <math.h>

int main ()
{
    int posneg;
    double sum;
    int n;
    
    posneg = -1;
    sum = 1;

    for(n = 2; n < 21; n = n+2){
        double var;
        double twopower;

        twopower = pow(2, n);
        var = posneg/twopower;

        sum = sum + var;

        posneg = posneg * -1;
    }

    printf("the sum equals %f", sum);
         

    return 0;
}
