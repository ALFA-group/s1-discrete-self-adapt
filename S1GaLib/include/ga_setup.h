#ifndef GA_SETUP_H
#define GA_SETUP_H

/**
 * @brief Initialize the S1 machine or emulator with the specified geometry.
 *
 * @param emulated If true, run in emulator mode; if false, use hardware.
 * @param chipRows Number of chip rows in the board configuration.
 * @param chipCols Number of chip columns in the board configuration.
 * @param apeRows Number of APE rows per chip.
 * @param apeCols Number of APE columns per chip.
 * @param traceFlags Bitmask controlling trace/logging behavior.
 */
void gset_initS1Machine(bool emulated, int chipRows, int chipCols, int apeRows, int apeCols, int traceFlags);

/**
 * @brief Begin creating the kernel for the current S1 configuration.
 */
void gset_startKernelCreate();

/**
 * @brief Compile and run the currently defined kernel.
 */
void gset_compileAndRunKernel();

/**
 * @brief Shut down the S1 machine/emulator and release resources.
 */
void gset_stopS1Machine();


#endif //GA_SETUP_H