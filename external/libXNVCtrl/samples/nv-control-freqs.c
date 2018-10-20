/*
 * Copyright (c) 2006-2008 NVIDIA, Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/*
 * nv-control-targets.c - NV-CONTROL client that demonstrates how to
 * talk to various target types on an X Server (X Screens, GPU, FrameLock)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

#include "nv-control-screen.h"



int main(int argc, char *argv[])
{
    Display *dpy;
    Bool ret;
    int major, minor;
    int num_gpus;
    char *gpu_name;
    NVCTRLAttributeValidValuesRec valid_values;
	if(argc < 4){
		fprintf(stderr, "Usage: %s gpu_id mem_oc graph_oc\n", argv[0]);
		return 1;
	}
	int gpu = atoi(argv[1]), mem_clock_offset = atoi(argv[2]), graph_clock_offset = atoi(argv[3]);
    
    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }
    
    /* Print some information */

    printf("\n");
    printf("Using NV-CONTROL extension %d.%d on %s\n",
           major, minor, XDisplayName(NULL));


    /* Start printing server system information */

    printf("\n");
    printf("Server System Information:\n");
    printf("\n");


    /* Get the number of gpus in the system */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU, &num_gpus);
    if (!ret) {
        fprintf(stderr, "Failed to query number of gpus\n");
        return 1;
    }
    printf("  number of GPUs: %d\n", num_gpus);

    

        printf("\n\n");
        printf("GPU %d information:\n", gpu);


        /* GPU name */

        ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                                NV_CTRL_TARGET_TYPE_GPU,
                                                gpu, // target_id
                                                0, // display_mask
                                                NV_CTRL_STRING_PRODUCT_NAME,
                                                &gpu_name);
        if (!ret) {
            fprintf(stderr, "Failed to query gpu product name\n");
            return 1;
        }
        printf("   Product Name                    : %s\n", gpu_name);

		/* GPU bus id */
		int busid;
        ret = XNVCTRLQueryTargetAttribute(dpy,
                                                NV_CTRL_TARGET_TYPE_GPU,
                                                gpu, // target_id
                                                0, // display_mask
                                                NV_CTRL_PCI_BUS,
                                                &busid);
        printf("   GPU bus id                      : %i\n",
               (ret ? busid : -1));

		/*
         * Query the valid values for NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET
         */

        ret = XNVCTRLQueryValidTargetAttributeValues(dpy,
                                                     NV_CTRL_TARGET_TYPE_GPU,
                                                     gpu,
                                                     3,//performance level
                                                     NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,
                                                     &valid_values);
        if (!ret) {
            fprintf(stderr, "Unable to query the valid values for "
                    "NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET on GPU %i (%s)\n",
                    gpu, gpu_name);
            return 1;
        }

        printf("   Valid values for NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET: %li - %li.\n",
               valid_values.u.range.min, valid_values.u.range.max);


		if(mem_clock_offset < valid_values.u.range.min || mem_clock_offset > valid_values.u.range.max){
			fprintf(stderr, "Specified value %i for "
                    "NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET on GPU %i (%s) out of range\n",
                    mem_clock_offset, gpu, gpu_name);
			return 1;
		}


        /*
         * Query the valid values for NV_CTRL_GPU_NVCLOCK_OFFSET
         */

        ret = XNVCTRLQueryValidTargetAttributeValues(dpy,
                                                     NV_CTRL_TARGET_TYPE_GPU,
                                                     gpu,
                                                     3,//performance level
                                                     NV_CTRL_GPU_NVCLOCK_OFFSET,
                                                     &valid_values);
        if (!ret) {
            fprintf(stderr, "Unable to query the valid values for "
                    "NV_CTRL_GPU_NVCLOCK_OFFSET on GPU %i (%s)\n",
                    gpu, gpu_name);
            return 1;
        }

        printf("   Valid values for NV_CTRL_GPU_NVCLOCK_OFFSET: %li - %li.\n",
               valid_values.u.range.min, valid_values.u.range.max);

		
		if(graph_clock_offset < valid_values.u.range.min || graph_clock_offset > valid_values.u.range.max){
			fprintf(stderr, "Specified value %i for "
                    "NV_CTRL_GPU_NVCLOCK_OFFSET on GPU %i (%s) out of range\n",
                    graph_clock_offset, gpu, gpu_name);
			return 1;
		}


		/*Set mem clock offset*/

		ret = XNVCTRLSetTargetAttributeAndGetStatus(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      gpu,
                                      3,//performance level
                                      NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,
                                      mem_clock_offset);

		if (!ret) {
            fprintf(stderr, "Unable to set value %i for "
                    "NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET on GPU %i (%s)\n",
                    mem_clock_offset, gpu, gpu_name);
            return 1;
        }

		printf("   Set attribute NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET: %i.\n",
               mem_clock_offset);

		/*Set gpu clock offset*/

		ret = XNVCTRLSetTargetAttributeAndGetStatus(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      gpu,
                                      3,//performance level
                                      NV_CTRL_GPU_NVCLOCK_OFFSET,
                                      graph_clock_offset);

		if (!ret) {
            fprintf(stderr, "Unable to set value %i for "
                    "NV_CTRL_GPU_NVCLOCK_OFFSET on GPU %i (%s)\n",
                    graph_clock_offset, gpu, gpu_name);
            return 1;
        }

		printf("   Set attribute NV_CTRL_GPU_NVCLOCK_OFFSET: %i.\n",
               graph_clock_offset);

        /*
         * Query for NV_CTRL_GPU_CURRENT_CLOCK_FREQS
         */

		int current_clock_freqs;
        ret = XNVCTRLQueryTargetAttribute(dpy,
                                                     NV_CTRL_TARGET_TYPE_GPU,
                                                     gpu,
                                                     0,
                                                     NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
                                                     &current_clock_freqs);
        if (!ret) {
            fprintf(stderr, "Unable to query the value for "
                    "NV_CTRL_GPU_CURRENT_CLOCK_FREQS on GPU %i (%s)\n",
                    gpu, gpu_name);
            return 1;
        }

        printf("   Values for NV_CTRL_GPU_CURRENT_CLOCK_FREQS: graph=%i mem=%i.\n",
               (current_clock_freqs >> 16), (current_clock_freqs & 0xffff));

		XFree(gpu_name);
        gpu_name = NULL;

    return 0;
}
