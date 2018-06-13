#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

static const int BUFFER_SIZE = 1024;

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <device_id> <interval_sleep_ms>\n", argv[0]);
        return 1;
    }
    int device_id = atoi(argv[1]);
    int sleep_time_ms = atoi(argv[2]);

    char cmd[BUFFER_SIZE];
    snprintf(cmd, BUFFER_SIZE,
             "nvidia-smi -a -i %d | grep \"Power Draw\" | cut -d ':' -f 2 | cut -d 'W' -f 1 | awk '{$1=$1};1'",
             device_id);

    char data_filename[BUFFER_SIZE];
    snprintf(data_filename, BUFFER_SIZE, "power_results_%i.txt", device_id);
    FILE *data = fopen(data_filename, "w");
    if (data == NULL) {
        puts("Could not create result file");
        return 1;
    }
    fclose(data);
    data = fopen(data_filename, "a");
    if (data == NULL) {
        puts("Could not open result file");
        return 1;
    }

    while (true) {

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
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
#ifdef _WIN32
            fprintf(data, "%I64d %.2lf\n",
                    dur.count(), atof(path));
#else
            fprintf(data, "%lld %.2lf\n",
                    dur.count(), atof(path));
#endif
        }

        fflush(data);

        /* close */
#ifdef _MSC_VER
        _pclose(fp);
#else
        pclose(fp);
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
    }

    fclose(data);
    return 0;
}
