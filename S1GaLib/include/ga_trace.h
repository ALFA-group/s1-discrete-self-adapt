#ifndef GA_TRACE_H
#define GA_TRACE_H

#include<S1Core/S1Core.h>
#include<S1GaLib/S1GaLib.h>

/** Maximum length for custom trace messages */
#define TRACE_MESSAGE_MAX_LEN 256

// =============================================================================
// Type Definitions
// =============================================================================

/**
 * Output format types for traced values
 */
typedef enum {
    sc_uint,
    sc_approx,
    sc_int
} TraceType;

/**
 * Convert a TraceType enum to its string representation
 * 
 * @param type The trace type to convert
 * @return String name of the type, or "UNKNOWN" if invalid
 */
static inline const char* trace_type_to_string(TraceType type) {
    switch(type) {
        case sc_uint:   return "sc_uint";
        case sc_approx: return "sc_approx";
        case sc_int:    return "sc_int";
        default:        return "UNKNOWN";
    }
}

/**
 * Grid position identifying a specific APE within the chip array
 */
typedef struct {
    unsigned short apeRow;
    unsigned short apeCol;
    unsigned short chipRow;
    unsigned short chipCol;
} grid_pos;

/**
 * Complete trace information for a single trace operation
 */
typedef struct {
    unsigned short apeRow;
    unsigned short apeCol;
    grid_pos pos;
    grid_pos posEnd; // for printing multiple apes in a range
    scExpr expr;
    char expr_name[32];
    TraceType type;
    char text[TRACE_MESSAGE_MAX_LEN];
} TraceInfo;

/**
 * Function pointer type for printing values of a specific type
 * 
 * @param value The value to print (interpreted according to the specific function)
 */
typedef void (*TypePrintFn)(unsigned short);

/**
 * Function pointer type for formatting and printing trace output
 * 
 * @param info Trace metadata
 * @param state Current machine state containing APE memory
 * @param printer Function to use for printing individual values
 */
typedef void (*TraceFmtFn)(TraceInfo, MachineState*, TypePrintFn);

/**
 * Registry entry for a single trace
 * 
 * Combines formatting function, trace metadata, and print function into
 * a single registered trace that can be executed by ID.
 */
typedef struct {
    TraceFmtFn fmt;
    TraceInfo info;
    TypePrintFn print;
    char id[12];
} TraceEntry;

// =============================================================================
// Public API
// =============================================================================

/**
 * Convert linear APE coordinates to hierarchical grid position
 * 
 * Translates flat (row, col) APE indices into a grid_pos structure that
 * identifies both the chip location and the APE location within that chip.
 * 
 * @param row Global APE row index
 * @param col Global APE column index
 * @return Grid position structure
 * 
 * @note This function will terminate the program if the coordinates map
 *       to a chip position outside the configured board dimensions
 */
grid_pos apeCoordinatesToGridPos(unsigned short row, unsigned short col);

/**
 * Trace an expression for a single APE (infer type from expression)
 * @param expr Expression to trace
 * @param apeRow APE row coordinate
 * @param apeCol APE column coordinate
 * @param expr_name Name of the expression for display
 */
void traceExprOneApe(scExpr expr, unsigned short apeRow, unsigned short apeCol, char* expr_name);

/**
 * Trace an expression for a single APE with explicit type
 * @param expr Expression to trace
 * @param apeRow APE row coordinate
 * @param apeCol APE column coordinate
 * @param output_type Explicit trace type
 * @param expr_name Name of the expression for display
 */
void traceExprOneApeTyped(scExpr expr, unsigned short apeRow, unsigned short apeCol, TraceType output_type, char* expr_name);

/**
 * Trace an expression for a range of APEs (infer type from expression)
 * @param expr Expression to trace
 * @param rowStart APE range start row coordinate
 * @param colStart APE range start col coordinate
 * @param rowEnd APE range end row coordinate
 * @param colEnd APE range end col coordinate
 * @param output_type Explicit trace type
 * @param expr_name Name of the expression for display
 */
void traceExprApeRange(scExpr expr, unsigned short rowStart, unsigned short colStart, unsigned short rowEnd, unsigned short colEnd, char* expr_name);

/**
 * Trace an expression for a range of APEs with explicit type
 * @param expr Expression to trace
 * @param rowStart APE range start row coordinate
 * @param colStart APE range start col coordinate
 * @param rowEnd APE range end row coordinate
 * @param colEnd APE range end col coordinate
 * @param output_type Explicit trace type
 * @param expr_name Name of the expression for display
 */
void traceExprApeRangeTyped(scExpr expr, unsigned short rowStart, unsigned short colStart, unsigned short rowEnd, unsigned short colEnd, TraceType output_type, char* expr_name);

/**
 * Trace an expression for a single APE with automatic name stringification
 * 
 * @param expr Expression to trace (will be stringified for display)
 * @param apeNumRow APE row coordinate
 * @param apeNumCol APE column coordinate
 */
#define TraceExprOneApe(expr, apeNumRow, apeNumCol) traceExprOneApe(expr, apeNumRow, apeNumCol, #expr);

/**
 * Trace an expression with explicit output type formatting
 * 
 * Like TraceExprOneApe(), but allows manual type specification.
 * 
 * @param expr Expression to trace (auto-stringified)
 * @param apeNumRow APE row coordinate  
 * @param apeNumCol APE column coordinate
 * @param output_type Display format: sc_uint, sc_int, or sc_approx
 */
#define TraceExprOneApeTyped(expr, apeNumRow, apeNumCol, output_type) traceExprOneApeTyped(expr, apeNumRow, apeNumCol, output_type, #expr);

/**
 * Trace an expression for a single APE with automatic name stringification
 * 
 * @param expr Expression to trace (will be stringified for display)
 * @param rowStart APE range start row coordinate
 * @param colStart APE range start col coordinate
 * @param rowEnd APE range end row coordinate
 * @param colEnd APE range end col coordinate
 */
#define TraceExprApeRange(expr, rowStart, colStart, rowEnd, colEnd) traceExprApeRange(expr, rowStart, colStart, rowEnd, colEnd, #expr);

/**
 * Trace an expression with explicit output type formatting
 * 
 * Like TraceExprApeRange, but allows manual type specification.
 * 
 * @param expr Expression to trace (auto-stringified)
 * @param rowStart APE range start row coordinate
 * @param colStart APE range start col coordinate
 * @param rowEnd APE range end row coordinate
 * @param colEnd APE range end col coordinate
 * @param output_type Display format: sc_uint, sc_int, or sc_approx
 */
#define TraceExprApeRangeTyped(expr, rowStart, colStart, rowEnd, colEnd, output_type) traceExprApeRangeTyped(expr, rowStart, colStart, rowEnd, colEnd, output_type, #expr);

#endif //GA_TRACE_H
