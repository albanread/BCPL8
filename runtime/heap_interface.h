#ifndef HEAP_INTERFACE_H
#define HEAP_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for list structures
typedef struct ListHeader ListHeader;
typedef struct ListAtom ListAtom;

// Function pointer types for mapping and predicate functions
typedef double (*FloatMapFunc)(double);
typedef int64_t (*PredicateFunc)(int64_t);

// List manipulation function declarations
ListHeader* BCPL_LIST_MAP(ListHeader* original_header, void* map_func_ptr);
ListHeader* BCPL_LIST_FILTER(ListHeader* original_header, PredicateFunc predicate);
ListHeader* BCPL_REVERSE_LIST(ListHeader* original_header);
ListAtom*   BCPL_FIND_IN_LIST(ListHeader* header, int64_t value_bits, int64_t type_tag);
ListHeader* BCPL_SHALLOW_COPY_LIST(ListHeader* original_header);
ListHeader* BCPL_DEEP_COPY_LIST(ListHeader* original_header);
ListAtom*   bcpl_list_get_rest(ListHeader* header);
int64_t     list_get_head_as_int(ListHeader* header);

// BCPL_ capitalized C ABI wrappers for runtime-published functions:
int64_t  BCPL_LIST_GET_HEAD_AS_INT(void* header_ptr);
double   BCPL_LIST_GET_HEAD_AS_FLOAT(void* header_ptr);
void*    BCPL_LIST_GET_TAIL(void* header_ptr);
void*    BCPL_LIST_GET_REST(void* header_ptr);
int64_t  BCPL_GET_ATOM_TYPE(void* header_ptr);
void     BCPL_LIST_APPEND_LIST(void* header_ptr, void* list_to_append_ptr);

// Declarations for SPLIT/JOIN helpers
ListHeader* BCPL_LIST_CREATE_EMPTY(void);
/**
 * Appends a BCPL string to a list.
 * BCPL strings are represented as pointers to arrays of 32-bit Unicode code points (uint32_t*).
 */
void BCPL_LIST_APPEND_STRING(ListHeader* header, uint32_t* value);

// Internal (typed) versions for use within the runtime:
double   list_get_head_as_float(ListHeader* header);
int64_t  list_get_atom_type(ListHeader* header);
void     BCPL_LIST_APPEND_LIST_TYPED(ListHeader* header, ListHeader* list_to_append);

// Deep copy from a read-only list literal (for list literals)
struct ListLiteralHeader;
ListAtom* BCPL_DEEP_COPY_LITERAL_LIST(struct ListLiteralHeader* literal_header);

#ifdef __cplusplus
}
#endif

#endif // HEAP_INTERFACE_H