#include <stdlib.h>
#include <stdio.h>
#include "nvapiOC.h"

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s device_id core_oc vram_oc\n", argv[0]);
        return 1;
    }
    //
    nvapiInit();
    nvapiOC(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    nvapiUnload();
    return 0;
}
