#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#ifdef _MSC_VER
#include <windows.h>
#else
#endif


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("error: must give CUDA device ID as command line argument (use nvidia-smi to determine id)\n");
        return 1;
    }

    char cmd[1024];
    sprintf(cmd, "nvidia-smi -a -i %d | grep \"Power Draw\" | cut -d ':' -f 2 | cut -d 'W' -f 1 | awk '{$1=$1};1'",
            atoi(argv[1]));

    FILE *data = fopen("power_results.txt", "w");
    if(data == NULL){
        puts("Could not create result file");
        return 1;
    }
    fclose(data);
    data = fopen("power_results.txt", "a");
    if(data == NULL){
        puts("Could not open result file");
        return 1;
    }

    double start = omp_get_wtime();
    while (1) {

        FILE *fp;
        char path[1035];

        /* Open the command for reading. */
#ifdef _MSC_VER
		fp = _popen(cmd, "r");
#else
		fp = popen(cmd, "r");
#endif
        if (fp == NULL) {
            printf("Failed to run command\n");
            fclose(data);
            return 1;
        }

        /* Read the output a line at a time - output it. */
        while (fgets(path, sizeof(path) - 1, fp) != NULL) {
            //printf("%s", path);
            fprintf(data, "%lf %.2lf\n", omp_get_wtime() - start, atof(path));
        }

        fflush(data);

        /* close */
#ifdef _MSC_VER
		_pclose(fp);
		Sleep(100);
#else
        pclose(fp);
        const struct timespec spec = {0, 1.0e8};
        nanosleep(&spec, NULL);
#endif
    }

    fclose(data);
    return 0;
}
