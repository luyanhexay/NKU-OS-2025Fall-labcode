#ifndef __LIBS_binary_tree_H__
#define __LIBS_binary_tree_H__

#ifndef __ASSEMBLER__

#include <defs.h>

/* *
 * Simple doubly linked binary_tree implementation.
 *
 * Some of the internal functions ("__xxx") are useful when manipulating
 * whole binary_trees rather than single entries, as sometimes we already know
 * the next/prev entries and we can generate better code by using them
 * directly rather than using the generic single-entry routines.
 * */

struct binary_tree_entry {
    struct binary_tree_entry *parent, *left, *right;
};

typedef struct binary_tree_entry binary_tree_entry_t;

static inline void binary_tree_init(binary_tree_entry_t *elm) __attribute__((always_inline));
// static inline void binary_tree_add(binary_tree_entry_t *binary_treeelm, binary_tree_entry_t *elm) __attribute__((always_inline));
static inline void binary_tree_add_left(binary_tree_entry_t *binary_treeelm, binary_tree_entry_t *elm) __attribute__((always_inline));
static inline void binary_tree_add_right(binary_tree_entry_t *binary_treeelm, binary_tree_entry_t *elm) __attribute__((always_inline));
static inline void binary_tree_del(binary_tree_entry_t *binary_treeelm) __attribute__((always_inline));
static inline void binary_tree_del_init(binary_tree_entry_t *binary_treeelm) __attribute__((always_inline));
static inline bool binary_tree_empty(binary_tree_entry_t *binary_tree) __attribute__((always_inline));
static inline binary_tree_entry_t *binary_tree_parent(binary_tree_entry_t *binary_treeelm) __attribute__((always_inline));
static inline binary_tree_entry_t *binary_tree_left(binary_tree_entry_t *binary_treeelm) __attribute__((always_inline));
static inline binary_tree_entry_t *binary_tree_right(binary_tree_entry_t *binary_treeelm) __attribute__((always_inline));

// static inline void __binary_tree_add(binary_tree_entry_t *elm, binary_tree_entry_t *prev, binary_tree_entry_t *next) __attribute__((always_inline));
// static inline void __binary_tree_del(binary_tree_entry_t *prev, binary_tree_entry_t *next) __attribute__((always_inline));

/* *
 * binary_tree_init - initialize a new entry
 * @elm:        new entry to be initialized
 * */
static inline void
binary_tree_init(binary_tree_entry_t *elm) {
    elm->parent = NULL;
    elm->left = NULL;
    elm->right = NULL;
}

/* *
 * binary_tree_add - add a new entry
 * @binary_treeelm:    binary_tree head to add after
 * @elm:        new entry to be added
 *
 * Insert the new element @elm *after* the element @binary_treeelm which
 * is already in the binary_tree.
 * */
// static inline void
// binary_tree_add(binary_tree_entry_t *binary_treeelm, binary_tree_entry_t *elm) {
//     binary_tree_add_after(binary_treeelm, elm);
// }

/* *
 * binary_tree_add_left - add a new entry
 * @binary_treeelm:    binary_tree head to add left
 * @elm:        new entry to be added
 *
 * Insert the new element @elm *left* the element @binary_treeelm which
 * is already in the binary_tree.
 * */
static inline void
binary_tree_add_left(binary_tree_entry_t *binary_treeelm, binary_tree_entry_t *elm) {
    binary_treeelm->left = elm;
    elm->parent = binary_treeelm;
    // __binary_tree_add(elm, binary_treeelm->prev, binary_treeelm);
}

/* *
 * binary_tree_add_right - add a new entry
 * @binary_treeelm:    binary_tree head to add right
 * @elm:        new entry to be added
 *
 * Insert the new element @elm *right* the element @binary_treeelm which
 * is already in the binary_tree.
 * */
static inline void
binary_tree_add_right(binary_tree_entry_t *binary_treeelm, binary_tree_entry_t *elm) {
    binary_treeelm->right = elm;
    elm->parent = binary_treeelm;
    // __binary_tree_add(elm, binary_treeelm, binary_treeelm->next);
}

/* *
 * binary_tree_del - deletes entry from binary_tree
 * @binary_treeelm:    the element to delete from the binary_tree
 *
 * Note: binary_tree_empty() on @binary_treeelm does not return true after this, the entry is
 * in an undefined state.
 * */
static inline void
binary_tree_del(binary_tree_entry_t *binary_treeelm) {
    if (binary_treeelm->left) 
        binary_tree_del(binary_treeelm->left);
    if (binary_treeelm->right)
        binary_tree_del(binary_treeelm->right);
    if (binary_treeelm->parent) {
        if (binary_treeelm->parent->left == binary_treeelm)
            binary_treeelm->parent->left = NULL;
        else if (binary_treeelm->parent->right == binary_treeelm)
            binary_treeelm->parent->right = NULL;
    }
    binary_treeelm->parent = NULL;
    // __binary_tree_del(binary_treeelm->prev, binary_treeelm->next);
}

/* *
 * binary_tree_del_init - deletes entry from binary_tree and reinitialize it.
 * @binary_treeelm:    the element to delete from the binary_tree.
 *
 * Note: binary_tree_empty() on @binary_treeelm returns true after this.
 * */
static inline void
binary_tree_del_init(binary_tree_entry_t *binary_treeelm) {
    binary_tree_del(binary_treeelm);
    binary_tree_init(binary_treeelm);
}

/* *
 * binary_tree_empty - tests whether a binary_tree is empty
 * @binary_tree:       the binary_tree to test.
 * */
static inline bool
binary_tree_empty(binary_tree_entry_t *binary_tree) {
    return binary_tree->left == NULL && binary_tree->right == NULL;
}

/* *
 * binary_tree_parent - get the parent entry
 * @binary_treeelm:    the binary_tree head
 **/
static inline binary_tree_entry_t *
binary_tree_parent(binary_tree_entry_t *binary_treeelm) {
    return binary_treeelm->parent;
}

/* *
 * binary_tree_left - get the left entry
 * @binary_treeelm:    the binary_tree head
 **/
static inline binary_tree_entry_t *
binary_tree_left(binary_tree_entry_t *binary_treeelm) {
    return binary_treeelm->left;
}

/* *
 * binary_tree_right - get the right entry
 * @binary_treeelm:    the binary_tree head
 **/
static inline binary_tree_entry_t *
binary_tree_right(binary_tree_entry_t *binary_treeelm) {
    return binary_treeelm->right;
}

// /* *
//  * Insert a new entry between two known consecutive entries.
//  *
//  * This is only for internal binary_tree manipulation where we know
//  * the prev/next entries already!
//  * */
// static inline void
// __binary_tree_add(binary_tree_entry_t *elm, binary_tree_entry_t *prev, binary_tree_entry_t *next) {
//     prev->next = next->prev = elm;
//     elm->next = next;
//     elm->prev = prev;
// }

// /* *
//  * Delete a binary_tree entry by making the prev/next entries point to each other.
//  *
//  * This is only for internal binary_tree manipulation where we know
//  * the prev/next entries already!
//  * */
// static inline void
// __binary_tree_del(binary_tree_entry_t *prev, binary_tree_entry_t *next) {
//     prev->next = next;
//     next->prev = prev;
// }

#endif /* !__ASSEMBLER__ */

#endif /* !__LIBS_binary_tree_H__ */

