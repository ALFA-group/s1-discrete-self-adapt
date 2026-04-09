#ifndef GA_DATASET_H
#define GA_DATASET_H

#include<S1Core/S1Core.h>
#include<S1GaLib/S1GaLib.h>

/**
 * @brief Function signature used to convert a CU memory value to a textual
 * representation and write it to a FILE stream.
 *
 * @param out FILE stream to write to.
 * @param value Value read from CU memory to convert and print.
 */
typedef void (*ValueConverter)(FILE* out, uint16_t value);

/**
 * @brief Options controlling the output formatting functions.
 *
 * - `entries_per_line`: how many entries to print before inserting a newline
 *   when using grid-like formatters.
 * - `type_out_fn`: converter used to render a single value (e.g. int or
 *   approximate float representation).
 */
typedef struct {
    unsigned short entries_per_line;
    unsigned int data_len;
    ValueConverter type_out_fn;
    char *text;
} FmtOptions;

/**
 * @brief Function signature for formatters that render arrays of CU data.
 *
 * @param data Pointer to an array of `uint16_t` values read from CU memory.
 * @param int Length (number of elements) in the `data` array.
 * @param FILE* Output FILE stream to write formatted text to.
 * @param FmtOptions Options that control formatting behavior.
 */
typedef void (*FmtFunction)(uint16_t*, FILE*, FmtOptions);

/**
 * @brief Registry entry describing a pending transfer from CU memory to the
 * host for formatting and output.
 *
 * - `fmt`: formatter callback used to write the data.
 * - `cuMem`: CU memory expression that identifies the source buffer.
 * - `output_to`: FILE pointer to write formatted output to.
 * - `options`: formatting options passed to `fmt`.
 */
typedef struct {
    FmtFunction fmt;
    scExpr cuMem;
    FILE* output_to;
    FmtOptions options;
} TransferEntry;

/**
 * @brief Inclusive row/column rectangle used to select sub-regions of chips
 * and apes when reading or printing variables.
 *
 * The fields represent start/end indices (inclusive) for rows and columns.
 */
typedef struct {
    unsigned int row_start;
    unsigned int row_end;
    unsigned int col_start;
    unsigned int col_end;
} range_t;

/* --- High-level data output helpers --- */

/**
 * @brief Print the contents of an ape variable for a single chip to stdout
 * in a grid-like format.
 *
 * @param var_name The SC expression referring to the ape variable.
 * @param chip_row Chip row index to print from.
 * @param chip_col Chip column index to print from.
 */
void gd_printApeVarOneChip(scExpr var_name, int chip_row, int chip_col);

/**
 * @brief Print the contents of an ape variable for a single chip from a given range of APEs to stdout
 * in a grid-like format.
 *
 * @param var_name The SC expression referring to the ape variable.
 * @param chip_row Chip row index to print from.
 * @param chip_col Chip column index to print from.
 * @param ape_row_start row of ape to start printing from
 * @param ape_row_end row of ape to stop printing at
 * @param ape_col_start col of ape to start printing from
 * @param ape_col_end col of ape to stop printing at
 */
void gd_printApeVarOneChipRange(scExpr var_name, int chip_row, int chip_col, int ape_row_start, int ape_row_end, int ape_col_start, int ape_col_end);

/**
 * @brief Write the contents of an ape variable for a single chip to the
 * provided FILE stream in CSV format.
 *
 * @param var_name The SC expression referring to the ape variable.
 * @param f FILE stream to write CSV to.
 * @param chip_row Chip row index to write from.
 * @param chip_col Chip column index to write from.
 */
void gd_writeApeVarOneChipToCSV(scExpr var_name, FILE* f, int chip_row, int chip_col);

/**
 * @brief Print the contents of an ape variable for the full board to stdout.
 *
 * This prints values from all chips and all apes in a grid-like layout.
 *
 * @param var_name The SC expression referring to the ape variable.
 */
void gd_printApeVarFullBoard(scExpr var_name);

/**
 * @brief Write the contents of an ape variable for the full board to the
 * provided FILE stream in CSV format.
 *
 * @param var_name The SC expression referring to the ape variable.
 * @param f FILE stream to write CSV to.
 */
void gd_writeApeVarFullBoardToCSV(scExpr var_name, FILE* f);

/**
 * @brief Write the contents of an ape variable for a single chip to `f` in
 * raw binary format. File must be opened in binary mode ("wb").
 *
 * @param var_name SC expression referring to the ape variable to write.
 * @param f FILE stream to write raw binary output to. Must be opened with "wb".
 * @param chip_row Chip row index to write from.
 * @param chip_col Chip column index to write from.
 */
void gd_writeApeVarOneChipToRaw(scExpr var_name, FILE* f, int chip_row, int chip_col);

/**
 * @brief Write the contents of an ape variable for the full board to `f` in
 * raw binary format. File must be opened in binary mode ("wb").
 *
 * @param var_name SC expression referring to the ape variable to write.
 * @param f FILE stream to write raw binary output to. Must be opened with "wb".
 */
void gd_writeApeVarFullBoardToRaw(scExpr var_name, FILE* f);

/**
 * @brief Write an ape memory vector from a single chip to `f` in raw binary
 * format, word-major. Int types written as uint16_t, Approx converted to
 * IEEE 754 float. File must be opened with "wb".
 *
 * @param memVec SC expression referring to the ape memory vector.
 * @param f Output FILE stream. Must be opened with "wb".
 * @param chip_row Chip row index to write from.
 * @param chip_col Chip column index to write from.
 */
void gd_writeApeMemVecOneChipToRaw(scExpr memVec, FILE* f, int chip_row, int chip_col);

/**
 * @brief Write an ape memory vector from all chips to `f` in raw binary
 * format, word-major. Int types written as uint16_t, Approx converted to
 * IEEE 754 float. File must be opened with "wb".
 *
 * @param memVec SC expression referring to the ape memory vector.
 * @param f Output FILE stream. Must be opened with "wb".
 */
void gd_writeApeMemVecFullBoardToRaw(scExpr memVec, FILE* f);

/**
 * @brief Print a custom message string to stdout.
 *
 * @param message The null-terminated string to print.
 */
void gd_printMessage(char *message);

/**
 * @brief Print a CU variable to stdout with an optional descriptive message.
 *
 * @param cuVar SC expression referring to the CU variable to print.
 * @param message Optional descriptive message to print alongside the variable
 *                (can be NULL).
 */
void gd_printCUVar(scExpr cuVar, char *message);

/* --- Type-specific output converters and formatters --- */

/**
 * @brief Write an integer CU value to `out` as a decimal integer.
 *
 * @param out Output FILE stream.
 * @param value Value from CU memory to render as an integer.
 */
void gd_outputInt(FILE* out, uint16_t value);

/**
 * @brief Write an approximate (fixed-point) CU value to `out` as a float.
 *
 * @param out Output FILE stream.
 * @param value Value from CU memory in approximate (fixed-point) format.
 */
void gd_outputApprox(FILE* out, uint16_t value);

/**
 * @brief Return the appropriate ValueConverter for a given SC expression
 * representing a variable (int or approx). Returns NULL for unsupported
 * types.
 *
 * @param var_name SC expression identifying the variable whose type is
 *                 inspected.
 * @return ValueConverter function pointer, or NULL if type unsupported.
 */
ValueConverter gd_getConverter(scExpr var_name);

/**
 * @brief Return the appropriate raw binary formatter for a given SC expression.
 * Returns `raw_fmt` for Int type variables and `raw_fmt_approx` for Approx
 * type variables. Returns NULL for unsupported types.
 *
 * @param var_name SC expression identifying the variable whose type is
 *                 inspected.
 * @return FmtFunction pointer to the appropriate raw formatter, or NULL if
 *         type is unsupported.
 */
FmtFunction gd_getRawFormatter(scExpr var_name);

/* --- Output formatters --- */

/**
 * @brief Formatter that writes `data` as raw binary uint16_t words directly
 * to `out` with no conversion or formatting. File must be opened in binary
 * mode ("wb"). Use for Int type variables.
 *
 * @param data Pointer to the `uint16_t` data array to write.
 * @param out FILE stream to write raw binary output to. Must be opened with "wb".
 * @param opts Formatting options. Only `data_len` is used; all other fields
 *             are ignored.
 */
void raw_fmt(uint16_t* data, FILE* out, FmtOptions opts);

/**
 * @brief Formatter that converts each value from the hardware Approx
 * log-domain format to a standard IEEE 754 float and writes the result as
 * raw binary to `out`. File must be opened in binary mode ("wb"). Use for
 * Approx type variables.
 *
 * @param data Pointer to the `uint16_t` data array in Approx format.
 * @param out FILE stream to write raw binary floats to. Must be opened with "wb".
 * @param opts Formatting options. Only `data_len` is used; all other fields
 *             are ignored.
 */
void raw_fmt_approx(uint16_t* data, FILE* out, FmtOptions opts);

/**
 * @brief Formatter that prints `data` in a grid-like representation using
 * `options.entries_per_line` to decide when to break lines.
 *
 * @param data Pointer to the `uint16_t` data array to format.
 * @param data_len Number of elements in `data`.
 * @param output_to FILE stream to write formatted output to.
 * @param options Formatting options controlling layout and conversion.
 */
void grid_fmt(uint16_t* data, FILE* output_to, FmtOptions options);

/**
 * @brief Formatter that writes `data` as a comma-separated line (CSV).
 *
 * @param data Pointer to the `uint16_t` data array to format.
 * @param data_len Number of elements in `data`.
 * @param output_to FILE stream to write CSV output to.
 * @param options Formatting options
 */
void csv_fmt(uint16_t* data, FILE* output_to, FmtOptions options);

/**
 * @brief Formatter that writes `data` as plain text with a custom text
 * string from `options.text`.
 *
 * @param data Pointer to the `uint16_t` data array to format.
 * @param output_to FILE stream to write output to.
 * @param options Formatting options including the text string to output.
 */
void text_fmt(uint16_t* data, FILE* output_to, FmtOptions options);

/**
 * @brief Formatter that writes CU memory data with custom formatting
 * controlled by options.
 *
 * @param data Pointer to the `uint16_t` data array to format.
 * @param out FILE stream to write output to.
 * @param opts Formatting options controlling the output behavior.
 */
void cu_fmt(uint16_t* data, FILE* out, FmtOptions opts);

/* --- Transfer system API (host/kernel synchronization) --- */

/**
 * @brief Register and execute a transfer of `cuMem` data, formatting it with
 * `fmt` and `options`, and writing to `output_to`. This will pause the
 * kernel, signal the host to perform the transfer, and wait for completion.
 *
 * @param cuMem CU memory expression to transfer.
 * @param output_to FILE stream to write formatted output to.
 * @param fmt Formatter to render the CU data on the host.
 * @param options Formatting options passed to `fmt`.
 */
void gd_transferExecute(scExpr cuMem, FILE* output_to, FmtFunction fmt, FmtOptions options);

/**
 * @brief Blocking loop run on the host that waits for transfer requests from
 * the kernel, performs the registered output callbacks, and signals the
 * kernel to continue. Returns when the kernel signals completion.
 */
void gd_waitForTransfers();

/**
 * @brief Notify the host that the kernel has finished transfers and wait for
 * the host acknowledgement (used to finalize outstanding transfers).
 */
void gd_transferFinish();

/* --- CU memory helpers --- */

/**
 * @brief Read the contents of a CU memory expression into a newly allocated
 * buffer on the host. The returned pointer must be freed by the caller.
 *
 * @param cuMemData SC expression describing the CU memory to read.
 * @return uint16_t* Pointer to newly allocated array containing the data.
 *         Caller is responsible for freeing the returned buffer.
 */
uint16_t* gd_hostReadCuMem(scExpr cuMemData);

/**
 * @brief Populate `ret_cuMemArr` with the ape variable `var_name` for a
 * single chip (all apes of that chip).
 *
 * @param var_name SC expression referring to the ape variable to read.
 * @param chip_row Chip row index to read from.
 * @param chip_col Chip column index to read from.
 * @param ret_cuMemArr CU memory expression representing the destination
 *                     buffer on the CU where values are written.
 */
void gd_cuGetApeVarOneChip(scExpr var_name, int chip_row, int chip_col, scExpr ret_cuMemArr);

/**
 * @brief Populate `ret_cuMemArr` with `var_name` values for the rectangular
 * chip range `chipRange` and ape range `apeRange`.
 *
 * @param var_name SC expression referring to the ape variable to read.
 * @param chipRange Inclusive chip row/col range to iterate over.
 * @param apeRange Inclusive ape row/col range to iterate over.
 * @param ret_cuMemArr CU memory expression representing the destination
 *                     buffer on the CU where values are written.
 */
void gd_cuGetApeVarChipRangeApeRange(scExpr var_name, range_t chipRange, range_t apeRange, scExpr ret_cuMemArr);

/**
 * @brief Read a vector from one ape per chip (ape selected per-chip by the
 * coordinates arrays) and store the results into `outArrCU` on the CU.
 *
 * @param rowCoordsArrCU CU array holding per-chip ape row coordinates.
 * @param colCoordsArrCU CU array holding per-chip ape column coordinates.
 * @param vecToReadApe SC expression for the ape vector to read from each ape.
 * @param outArrCU CU memory expression representing the destination buffer.
 */
void gd_readBackToCUVectorOnePerChip(scExpr rowCoordsArrCU, scExpr colCoordsArrCU, scExpr vecToReadApe, scExpr outArrCU);

/**
 * @brief Read the var from top-left ape of each chip (ape 0,0 of each chip) and write
 * the collected values to `cuMemVecTransfer` on the CU.
 *
 * @param cuMemVecTransfer CU memory expression for the destination vector.
 * @param apeVarToGet SC expression of the ape variable to read from each top-left ape.
 */
void gd_readBackToCUTopLeftApeEachChip(scExpr cuMemVecTransfer, scExpr apeVarToGet);

/**
 * @brief Load a delimited dataset from `fileName` into CU data memory at
 * `cuAddr`.
 *
 * @param fileName Path to the input file.
 * @param nbLines Number of lines (rows) to read from the file.
 * @param nbCols Number of columns per line to read.
 * @param cuAddr CU memory address to write the data to.
 * @param delimiter Character used to split columns (e.g. ',').
 */
void gd_loadDataset(char* fileName, int nbLines, int nbCols, uint32_t cuAddr, char delimiter);

#endif //GA_DATASET_H