#include<S1Core/S1Core.h>
#include <stdbool.h>

extern LLKernel *llKernel;

void gset_initS1Machine(bool emulated, int chipRows, int chipCols, int apeRows, int apeCols, int traceFlags){
    // Initialize Singular arithmetic on CPU
    initSingularArithmetic ();
    scInitializeMachine ((emulated ? scEmulated : scRealMachine),
                        chipRows, chipCols, apeRows, apeCols,
                        traceFlags, 0 /* DDR */, 0 /* randomize */, 0 /* torus */);

    // Exit if S1 is still running.
    // (scInitializeMachine is supposed to completely reset the machine,
    // so this should not be able to happen, but current CU has a bug.)
    if (scReadCURunning() != 0) {
        printf("S1 is RUNNING AFTER RESET.  Terminating execution.\n");
        exit(1);
    }

}

void gset_startKernelCreate(){
    // Initialize the kernel creating code
    scNovaInit();
    scEmitLLKernelCreate();
}

void gset_compileAndRunKernel(){
    scKernelTranslate();
    scLLKernelLoad(llKernel, 0); // Load the low level kernel at location 0
    scLLKernelFree(llKernel); // Free host memory
    scLLKernelExecute(0); // Start execution at PC=0
}

void gset_stopS1Machine(){
    scLLKernelWaitSignal(); // Wait for the kernel to halt
    scTerminateMachine(); // Terminate the machine
}