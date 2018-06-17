#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 6) {
        puts("Too few arguments");
        return 1;
    }

    int max_idx, min_idx;
    double max = -1e37, min = 1e37;
    for (int i = 1; i < argc; i++) {
        double cur = atof(argv[i]);
        if (cur < min) {
            min = cur;
            min_idx = i;
        }
        if (cur > max) {
            max = cur;
            max_idx = i;
        }
    }
    double res = 0;
    int c = 0;
    for (int i = 1; i < argc; i++)
        if (i != min_idx && i != max_idx) {
            res += atof(argv[i]);
            c++;
        }
    printf("%f\n", res / c);
    return 0;
}
