#include "runtime.h"
#include "ListDataTypes.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Remove ListHeader alias; use explicit struct from ListDataTypes.h

// Global free list pointer
ListAtom* g_free_list_head = NULL;

// --- NEW: ListHeader freelist globals ---
ListHeader* g_header_free_list_head = NULL;

// === Freelist Metrics and Constants ===
static size_t g_freelist_node_count = 0;
static size_t g_total_nodes_allocated_from_heap = 0;
static const size_t INITIAL_FREELIST_CHUNK_SIZE = 8192; // 8k nodes
static const size_t HEADER_FREELIST_CHUNK_SIZE = INITIAL_FREELIST_CHUNK_SIZE / 4; // 2048 headers

// === Freelist Management ===

// --- NEW: Management functions for the header freelist ---

// Pre-allocates a chunk of ListHeaders and adds them to the header freelist.
static void replenishHeaderFreelist() {
    size_t headers_to_alloc = HEADER_FREELIST_CHUNK_SIZE;
    ListHeader* new_chunk = (ListHeader*)malloc(sizeof(ListHeader) * headers_to_alloc);
    if (!new_chunk) {
        exit(1); // Out of memory
    }
    // Link the new headers together in a chain using the 'head' pointer for the link.
    for (size_t i = 0; i < headers_to_alloc - 1; ++i) {
        new_chunk[i].head = (ListAtom*)&new_chunk[i + 1];
    }
    new_chunk[headers_to_alloc - 1].head = (ListAtom*)g_header_free_list_head;
    g_header_free_list_head = new_chunk;
}

// Gets a ListHeader from the freelist, replenishing if necessary.
static ListHeader* getHeaderFromFreelist() {
    if (g_header_free_list_head == NULL) {
        replenishHeaderFreelist();
    }
    ListHeader* header = g_header_free_list_head;
    g_header_free_list_head = (ListHeader*)g_header_free_list_head->head;
    return header;
}

// Returns a ListHeader to the freelist.
static void returnHeaderToFreelist(ListHeader* header) {
    if (!header) return;
    header->head = (ListAtom*)g_header_free_list_head;
    g_header_free_list_head = header;
}
// --- END NEW ---

// Pre-allocate a new chunk of nodes and add them to the freelist.
static void replenishFreelist() {
    size_t nodes_to_alloc = INITIAL_FREELIST_CHUNK_SIZE;
    ListAtom* new_chunk = (ListAtom*)malloc(sizeof(ListAtom) * nodes_to_alloc);
    if (!new_chunk) {
        exit(1); // Out of memory
    }
    g_total_nodes_allocated_from_heap += nodes_to_alloc;
    // Link all the new nodes together in a chain.
    for (size_t i = 0; i < nodes_to_alloc - 1; ++i) {
        new_chunk[i].next = &new_chunk[i + 1];
    }
    new_chunk[nodes_to_alloc - 1].next = NULL; // Last node points to null.
    // Prepend this new chunk to the (potentially empty) existing freelist.
    new_chunk[nodes_to_alloc - 1].next = g_free_list_head;
    g_free_list_head = new_chunk;
    g_freelist_node_count += nodes_to_alloc;
}

// Get a node from the freelist, replenishing if necessary.
static ListAtom* getNodeFromFreelist() {
    if (g_free_list_head == NULL) {
        replenishFreelist();
    }
    ListAtom* node = g_free_list_head;
    g_free_list_head = g_free_list_head->next;
    g_freelist_node_count--;
    return node;
}

// Return a node to the freelist.
static void returnNodeToFreelist(ListAtom* node) {
    if (!node) return;
    node->next = g_free_list_head;
    g_free_list_head = node;
    g_freelist_node_count++;
}

// Freelist initializer for runtime startup.
extern "C" void initialize_freelist() {
    if (g_total_nodes_allocated_from_heap == 0) {
        replenishFreelist();
        replenishHeaderFreelist(); // Add this line to pre-allocate header pool
    }
}

#ifdef __cplusplus
#endif

// ============================================================================
// JIT MODE IMPLEMENTATION (C++ Bridge)
// This code is compiled only when JIT_MODE is defined.
// ============================================================================
#ifdef JIT_MODE

#include "../HeapManager/HeapManager.h"
#include <iostream>
#include <cstdint>

    // The existing C bridge code that calls the C++ HeapManager
    void* bcpl_alloc_words(int64_t num_words, const char* func, const char* var) {
        if (num_words <= 0) return nullptr;
        return HeapManager::getInstance().allocVec(num_words);
    }

    void* bcpl_alloc_chars(int64_t num_chars) {
        if (num_chars < 0) return nullptr;
        return HeapManager::getInstance().allocString(num_chars);
    }

    void bcpl_free(void* ptr) {
        if (!ptr) return;
        HeapManager::getInstance().free(ptr);
    }
#else

extern "C" {
    // Minimalist C implementation using malloc
    void* bcpl_alloc_words(int64_t num_words, const char* func, const char* var) {
        // Allocate space for the 64-bit length prefix plus the data
        size_t total_size = sizeof(uint64_t) + num_words * sizeof(uint64_t);
        uint64_t* ptr = (uint64_t*)malloc(total_size);
        if (!ptr) {
            return NULL;
        }
        // Store the number of elements in the first 8 bytes
        ptr[0] = num_words;
        // Return a pointer to the payload area, just after the length
        return (void*)(ptr + 1);
    }

    void* bcpl_alloc_chars(int64_t num_chars) {
        // Allocate space for the 64-bit length prefix plus the character data and null terminator
        size_t total_size = sizeof(uint64_t) + (num_chars + 1) * sizeof(uint32_t);
        uint64_t* ptr = (uint64_t*)malloc(total_size);
        if (!ptr) {
            return NULL;
        }
        // Store the number of characters in the first 8 bytes
        ptr[0] = num_chars;
        uint32_t* payload = (uint32_t*)(ptr + 1);
        // Add the null terminator at the end of the string
        payload[num_chars] = 0;
        // Return a pointer to the payload area
        return (void*)payload;
    }

    void bcpl_free(void* payload) {
        if (!payload) {
            return;
        }
        // The provided pointer is to the payload. To free the whole allocation,
        // we must get the original pointer, which is 8 bytes before the payload.
        uint64_t* original_ptr = ((uint64_t*)payload) - 1;
        free(original_ptr);
    }
}
#endif

extern "C" {

// Frees a BCPL list (linked list of ListAtom nodes)
void bcpl_free_list(ListHeader* header) {
    if (!header) return;

    ListAtom* head = header->head;
    while (head) {
        ListAtom* next = head->next;
        // If the atom holds a pointer to heap memory, free it
        if (head->type == ATOM_STRING || head->type == ATOM_LIST_POINTER) {
            if (head->value.ptr_value) {
                bcpl_free(head->value.ptr_value);
            }
        }
        returnNodeToFreelist(head);
        head = next;
    }

    // Return the header to the freelist instead of calling free()
    returnHeaderToFreelist(header);
}

// (Removed deprecated list_create function)

// Returns the head value as int (skipping the header)
int64_t list_get_head_as_int(ListHeader* header) {
    if (!header || header->type != ATOM_SENTINEL || !header->head) {
        return 0;
    }
    return header->head->value.int_value;
}

// C-compatible wrapper for runtime bridge
extern "C" int64_t BCPL_LIST_GET_HEAD_AS_INT(void* header_ptr) {
    return list_get_head_as_int((ListHeader*)header_ptr);
}

// C-compatible wrapper for runtime bridge
extern "C" int64_t list_get_head_as_int_c(void* header_ptr) {
    return list_get_head_as_int((ListHeader*)header_ptr);
}

/**
 * Returns the raw bits, suitable for integers or pointers.
 */


/**
 * Returns a double-precision float value.
 */
double list_get_head_as_float(ListHeader* header) {
    if (!header || header->type != ATOM_SENTINEL || !header->head) return 0.0;
    return header->head->value.float_value;
}

// C-compatible wrapper for runtime bridge
extern "C" double BCPL_LIST_GET_HEAD_AS_FLOAT(void* header_ptr) {
    return list_get_head_as_float((ListHeader*)header_ptr);
}

// Returns the atom type of the head (for filtering in FOREACH)
int64_t list_get_atom_type(ListHeader* header) {
    if (!header || header->type != ATOM_SENTINEL || !header->head) return -1;
    return (int64_t)header->head->type;
}

// C-compatible wrapper for runtime bridge
extern "C" int64_t BCPL_GET_ATOM_TYPE(void* header_ptr) {
    return list_get_atom_type((ListHeader*)header_ptr);
}

// Returns the tail pointer (TL)
extern "C" ListAtom* bcpl_list_get_rest(ListHeader* header) {
    if (!header || header->type != ATOM_SENTINEL) {
        return NULL;
    }
    return header->head ? header->head->next : NULL;
}

// C-compatible wrapper for runtime bridge
extern "C" void* BCPL_LIST_GET_REST(void* header_ptr) {
    ListHeader* header = (ListHeader*)header_ptr;
    if (!header || header->type != ATOM_SENTINEL) {
        return NULL;
    }
    return header->head ? (void*)header->head->next : NULL;
}

// Returns a pointer to the NTH list node (0-based), or NULL if out of bounds
extern "C" void* BCPL_LIST_GET_NTH(void* header_ptr, int64_t n) {
    ListHeader* header = (ListHeader*)header_ptr;
    if (!header || header->type != ATOM_SENTINEL || n < 0) {
        return NULL;
    }
    ListAtom* current = header->head;
    int64_t idx = 0;
    while (current && idx < n) {
        current = current->next;
        ++idx;
    }
    return (idx == n && current) ? (void*)current : NULL;
}

// C-compatible function for runtime bridge: returns the next node after the head
extern "C" void* BCPL_LIST_GET_TAIL(void* header_ptr) {
    ListHeader* header = (ListHeader*)header_ptr;
    if (!header || header->type != ATOM_SENTINEL) {
        return NULL;
    }
    return header->head ? (void*)header->head->next : NULL;
}

// ===================== New List Creation/Append ABI =====================

// Create and return a sentinel (dummy) head node with tail pointer.
// The list pointer will now NEVER be NULL.
ListHeader* BCPL_LIST_CREATE_EMPTY(void) {
    ListHeader* header = getHeaderFromFreelist(); // Use the freelist
    header->type = ATOM_SENTINEL;
    header->pad = 0;
    header->length = 0;
    header->head = NULL;
    header->tail = NULL;
    return header;
}

// C-compatible wrapper for runtime bridge (returns void, matches published ABI)
// Appends a nested list to a list (internal typed version)
void BCPL_LIST_APPEND_LIST_TYPED(ListHeader* header, ListHeader* list_to_append) {
    ListAtom* new_node = getNodeFromFreelist();
    new_node->type = ATOM_LIST_POINTER;
    new_node->pad = 0;
    new_node->value.ptr_value = list_to_append;
    new_node->next = NULL;

    if (header->head == NULL) {
        header->head = new_node;
        header->tail = new_node;
    } else {
        header->tail->next = new_node;
        header->tail = new_node;
    }
    header->length++;
}

// C-compatible wrapper for runtime bridge (returns void, matches published ABI)
extern "C" void BCPL_LIST_APPEND_LIST(void* header_ptr, void* list_to_append_ptr) {
    BCPL_LIST_APPEND_LIST_TYPED((ListHeader*)header_ptr, (ListHeader*)list_to_append_ptr);
}

// O(1) append using the tail pointer in the header
void BCPL_LIST_APPEND_INT(ListHeader* header, int64_t value) {
    if (!header || header->type != ATOM_SENTINEL) return;

    ListAtom* new_node = getNodeFromFreelist();
    new_node->type = ATOM_INT;
    new_node->pad = 0;
    new_node->value.int_value = value;
    new_node->next = NULL;

    if (header->head == NULL) {
        header->head = new_node;
        header->tail = new_node;
    } else {
        header->tail->next = new_node;
        header->tail = new_node;
    }
    header->length++;
}

// O(1) append for float
void BCPL_LIST_APPEND_FLOAT(ListHeader* header, double value) {
    if (!header || header->type != ATOM_SENTINEL) return;

    ListAtom* new_node = getNodeFromFreelist();
    new_node->type = ATOM_FLOAT;
    new_node->pad = 0;
    new_node->value.float_value = value;
    new_node->next = NULL;

    if (header->head == NULL) {
        header->head = new_node;
        header->tail = new_node;
    } else {
        header->tail->next = new_node;
        header->tail = new_node;
    }
    header->length++;
}

// O(1) append for string
void BCPL_LIST_APPEND_STRING(ListHeader* header, uint32_t* value) {
    if (!header || header->type != ATOM_SENTINEL) return;

    ListAtom* new_node = getNodeFromFreelist();
    new_node->type = ATOM_STRING;
    new_node->pad = 0;
    new_node->value.ptr_value = value;
    new_node->next = NULL;

    if (header->head == NULL) {
        header->head = new_node;
        header->tail = new_node;
    } else {
        header->tail->next = new_node;
        header->tail = new_node;
    }
    header->length++;
}

// Appends a nested list to a list


// Add more BCPL_LIST_APPEND_* as needed for other types...

// === DEEP COPY FROM LITERAL LIST HEADER ===

/**
 * @brief Creates a deep copy from a read-only list literal.
 * @param literal_header A pointer to the clean, compiler-generated ListLiteralHeader.
 * @return A pointer to a standard, heap-allocated ListAtom header.
 *
 * The ListLiteralHeader is assumed to have the following layout:
 * struct ListLiteralHeader {
 *     int32_t type;      // ATOM_SENTINEL
 *     int32_t pad;
 *     ListAtom* tail;    // offset 8
 *     ListAtom* head;    // offset 16
 *     int64_t length;    // (optional, not used by runtime)
 * };
 */


#include <stdio.h> // For printf

ListHeader* BCPL_DEEP_COPY_LITERAL_LIST(ListLiteralHeader* literal_header) {
    if (!literal_header) return NULL;

    ListHeader* new_header = (ListHeader*)malloc(sizeof(ListHeader));
    new_header->type = ATOM_SENTINEL;
    new_header->pad = 0;
    new_header->length = literal_header->length;
    new_header->head = NULL;
    new_header->tail = NULL;

    ListAtom* current_original = literal_header->head;

    while (current_original != NULL) {
        ListAtom* new_node = getNodeFromFreelist();
        new_node->type = current_original->type;
        new_node->pad = 0;
        new_node->next = NULL;

        // Deep copy logic for strings/nested lists (as before).
        switch (current_original->type) {
            case ATOM_STRING:
                new_node->value.ptr_value = current_original->value.ptr_value;
                break;
            case ATOM_LIST_POINTER:
                new_node->value.ptr_value = BCPL_DEEP_COPY_LITERAL_LIST(
                    (ListLiteralHeader*)current_original->value.ptr_value
                );
                break;
            default:
                new_node->value = current_original->value;
                break;
        }

        // Append the new node using the head/tail pointers.
        if (new_header->head == NULL) {
            new_header->head = new_node;
            new_header->tail = new_node;
        } else {
            new_header->tail->next = new_node;
            new_header->tail = new_node;
        }

        current_original = current_original->next;
    }

    return new_header;
}

// === MAP (higher-order) ===
typedef int64_t (*IntMapFunc)(int64_t);
typedef double (*FloatMapFunc)(double);



/**
 * @brief Filter a list using a predicate function pointer.
 * The predicate should take an int64_t and return int64_t (nonzero = keep).
 * Only integer values are supported in this simple version.
 */
typedef int64_t (*PredicateFunc)(int64_t);

extern "C" ListHeader* BCPL_LIST_FILTER(ListHeader* original_header, PredicateFunc predicate) {
    if (!original_header || !predicate) return NULL;

    ListHeader* new_header = BCPL_LIST_CREATE_EMPTY();
    ListAtom* current_original = original_header->head;

    while (current_original != NULL) {
        // Call the predicate with the value of the current node.
        // NOTE: This simple version assumes integer values. A more robust
        // implementation would check atom->type and pass floats correctly.
        int64_t result = predicate(current_original->value.int_value);

        // If the predicate returns true (non-zero)...
        if (result != 0) {
            // ...create a new node and shallow copy the original element.
            ListAtom* new_node = getNodeFromFreelist();
            new_node->type = current_original->type;
            new_node->pad = 0;
            new_node->value = current_original->value;
            new_node->next = NULL;

            // Append the new node to our new list (O(1) operation).
            if (new_header->head == NULL) {
                new_header->head = new_node;
                new_header->tail = new_node;
            } else {
                new_header->tail->next = new_node;
                new_header->tail = new_node;
            }
            new_header->length++;
        }
        current_original = current_original->next;
    }

    return new_header;
}

// Returns the rest of the list (non-destructive)
// (Already defined above with correct logic)

    // Fast freelist return for use by code generator (TL)
    void returnNodeToFreelist_runtime(ListAtom* node) {
        returnNodeToFreelist(node);
    }
} // extern "C"

// Free all nodes in the global free list
extern "C" void BCPL_FREE_CELLS(void) {
    ListAtom* current = g_free_list_head;
    while (current) {
        ListAtom* next = current->next;
        free(current);
        current = next;
    }
    g_free_list_head = NULL;
    g_freelist_node_count = 0;
    g_total_nodes_allocated_from_heap = 0;
}

// Expose the address of the global free list pointer
extern "C" ListAtom** get_g_free_list_head_address(void) {
    return &g_free_list_head;
}

/**
 * @brief Create a reversed copy of a list (non-destructive).
 * Allocates new nodes for the structure, prepending each element to the new list.
 */
extern "C" ListHeader* BCPL_REVERSE_LIST(ListHeader* original_header) {
    if (!original_header) return NULL;

    // Create a new empty list that will hold the reversed elements
    ListHeader* new_header = BCPL_LIST_CREATE_EMPTY();
    ListAtom* current_original = original_header->head; // Start at the first real node

    while (current_original != NULL) {
        // Get a new node for our copy
        ListAtom* new_node = getNodeFromFreelist();

        // Shallow copy the data from the original node
        new_node->type = current_original->type;
        new_node->value = current_original->value;

        // --- Prepend to the new list ---
        new_node->next = new_header->head;
        new_header->head = new_node;
        if (new_header->tail == NULL) {
            new_header->tail = new_node;
        }
        new_header->length++;

        current_original = current_original->next;
    }

    return new_header;
}

/**
 * @brief Find the first occurrence of a value in a list.
 * @param header The list header.
 * @param value_bits The value to search for (int or float bits).
 * @param type_tag 1 for INT, 2 for FLOAT.
 * @return Pointer to the first matching node, or NULL if not found.
 */
extern "C" ListAtom* BCPL_FIND_IN_LIST(ListHeader* header, int64_t value_bits, int64_t type_tag) {
    if (!header) return NULL;

    ListAtom* current = header->head;

    while (current != NULL) {
        bool match = false;
        if (type_tag == ATOM_INT && current->type == ATOM_INT) {
            if (current->value.int_value == value_bits) {
                match = true;
            }
        } else if (type_tag == ATOM_FLOAT && current->type == ATOM_FLOAT) {
            // Reinterpret the integer bits as a double for comparison
            double target_float;
            memcpy(&target_float, &value_bits, sizeof(double));
            if (current->value.float_value == target_float) {
                match = true;
            }
        }
        // Add more type comparisons (e.g., for strings) here if needed

        if (match) {
            return current; // Return the pointer to the node where the match was found
        }

        current = current->next;
    }

    return NULL; // Value not found
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a shallow copy of a list.
 * Allocates new nodes for the structure, but value fields are copied as-is (pointers are not duplicated).
 */
ListHeader* BCPL_SHALLOW_COPY_LIST(ListHeader* original_header) {
    if (!original_header) return NULL;

    // Create a new empty list to be our copy
    ListHeader* new_header = BCPL_LIST_CREATE_EMPTY();
    ListAtom* current_original = original_header->head; // Start at the first real node

    while (current_original != NULL) {
        // Get a new node for our copy
        ListAtom* new_node = getNodeFromFreelist();

        // --- Shallow Copy ---
        // Copy the type and the value directly. If it's a pointer,
        // we are copying the pointer's value, not what it points to.
        new_node->type = current_original->type;
        new_node->value = current_original->value;
        new_node->next = NULL;

        // Append the new node to our new list (this is an O(1) operation)
        if (new_header->head == NULL) {
            new_header->head = new_node;
            new_header->tail = new_node;
        } else {
            new_header->tail->next = new_node;
            new_header->tail = new_node;
        }
        new_header->length++;

        current_original = current_original->next;
    }

    return new_header;
}

/**
 * @brief Creates a new list by concatenating two existing lists (non-destructive).
 * @return A new ListHeader* for the concatenated list. Originals are unchanged.
 */
extern "C" ListHeader* BCPL_CONCAT_LISTS(ListHeader* list1_header, ListHeader* list2_header) {
    // 1. Create a shallow copy of the first list. This is efficient and provides
    //    the base for our new concatenated list.
    ListHeader* new_header = BCPL_SHALLOW_COPY_LIST(list1_header);
    if (!new_header) { // Handle case where list1 might be null
        return BCPL_SHALLOW_COPY_LIST(list2_header);
    }

    // 2. Iterate through the second list and append shallow copies of its nodes
    //    to the new list.
    if (list2_header && list2_header->head) {
        ListAtom* current_original = list2_header->head;
        while (current_original != NULL) {
            // Get a new node for our copy
            ListAtom* new_node = getNodeFromFreelist();

            // Shallow Copy the data from the original node
            new_node->type = current_original->type;
            new_node->value = current_original->value;
            new_node->next = NULL;

            // Append the new node to our new list (O(1) operation)
            if (new_header->head == NULL) {
                new_header->head = new_node;
                new_header->tail = new_node;
            } else {
                new_header->tail->next = new_node;
                new_header->tail = new_node;
            }
            new_header->length++;
            
            current_original = current_original->next;
        }
    }
    
    return new_header;
}

/**
 * @brief Create a deep copy of a list.
 * Allocates new nodes for the structure, and duplicates heap-allocated data (strings, nested lists, etc.).
 */
ListHeader* BCPL_DEEP_COPY_LIST(ListHeader* original_header) {
    if (!original_header) return NULL;

    ListHeader* new_header = BCPL_LIST_CREATE_EMPTY();
    ListAtom* current_original = original_header->head;

    while (current_original != NULL) {
        ListAtom* new_node = getNodeFromFreelist();
        new_node->type = current_original->type;
        new_node->next = NULL;

        // --- Deep Copy Logic ---
        switch (current_original->type) {
            case ATOM_STRING: {
                uint32_t* original_str = (uint32_t*)current_original->value.ptr_value;
                if (original_str) {
                    // Find length of original string (assuming null terminated)
                    size_t len = 0;
                    while(original_str[len] != 0) len++;

                    // Allocate new string and copy content
                    uint32_t* new_str = (uint32_t*)bcpl_alloc_chars(len);
                    if (new_str) {
                        memcpy(new_str, original_str, (len + 1) * sizeof(uint32_t));
                    }
                    new_node->value.ptr_value = new_str;
                } else {
                    new_node->value.ptr_value = NULL;
                }
                break;
            }
            case ATOM_LIST_POINTER: {
                // Recursively deep copy nested lists
                new_node->value.ptr_value = BCPL_DEEP_COPY_LIST((ListHeader*)current_original->value.ptr_value);
                break;
            }
            // Add cases for VEC, TABLE etc. here if they can be list elements

            default:
                // For simple types like INT or FLOAT, a direct copy is sufficient
                new_node->value = current_original->value;
                break;
        }

        // Append the new node to our new list (O(1) operation)
        if (new_header->head == NULL) {
            new_header->head = new_node;
            new_header->tail = new_node;
        } else {
            new_header->tail->next = new_node;
            new_header->tail = new_node;
        }
        new_header->length++;

        current_original = current_original->next;
    }

    return new_header;
}

    }
