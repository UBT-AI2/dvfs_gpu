#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <time.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("error: must give CUDA device ID as command line argument (use nvidia-smi to determine id)\n");
        return 1;
    }

    char cmd[1024];
    sprintf(cmd, "nvidia-smi -a -i %d | grep \"Power Draw\" | cut -d ':' -f 2 | cut -d 'W' -f 1 | awk '{$1=$1};1'",
            atoi(argv[1]));

    FILE *fp = fopen("power_results.txt", "w");
    fclose(fp);

    double start, before, after;

    start = omp_get_wtime();
    while (1) {

        FILE *fp;
        char path[1035];


        FILE *data = fopen("power_results.txt", "a");

        /* Open the command for reading. */
        fp = popen(cmd, "r");
        if (fp == NULL) {
            printf("Failed to run command\n");
            exit(1);
        }

        /* Read the output a line at a time - output it. */
        while (fgets(path, sizeof(path) - 1, fp) != NULL) {
            //printf("%s", path);
            fprintf(data, "%lf %.2lf\n", omp_get_wtime() - start, atof(path));
        }

        /* close */
        pclose(fp);
        fclose(data);

        const struct timespec spec = {0, 1.0e8};
        nanosleep(&spec, NULL);
    }

    return 0;
}
