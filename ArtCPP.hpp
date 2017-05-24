//
// Created by Players Inc on 13/03/2017.
//


#ifndef MVCCART_ARTCPP_HPP
#define MVCCART_ARTCPP_HPP

#include <iostream>
#include <inttypes.h>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/tuple/tuple.hpp>
#include <stdint.h>
#include <stdbool.h>
#include "core/Tuple.hpp"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include "mvcc.hpp"
#ifdef __i386__
#include <emmintrin.h>
#else
#ifdef __amd64__
#include <emmintrin.h>
#endif
#endif

using namespace boost;

/**
 * Macros to manipulate pointer tags
 */
#define IS_LEAF(x) (((uintptr_t)x & 1))
#define SET_LEAF(x) ((void*)((uintptr_t)x | 1))
#define LEAF_RAW(x) ((art_leaf*)((void*)((uintptr_t)x & ~1)))




#define NODE4   1
#define NODE16  2
#define NODE48  3
#define NODE256 4
#define MAX_PREFIX_LEN 10
#define MAX_VERSION_DEPTH 100

typedef char DefaultKeyType[20];
typedef boost::shared_mutex Mutex;
typedef boost::recursive_mutex::scoped_lock RecursiveScopedLock;
typedef boost::shared_lock<Mutex> SharedLock;
typedef boost::upgrade_lock<Mutex> UpgradeLock;
typedef boost::upgrade_to_unique_lock<Mutex> WriteLock;
Mutex _access;

///Callbacks for Predicates, UpdateFun and etc.
typedef int(*art_callback)(void *data, const unsigned char *key, uint32_t key_len, void* value);

/**
 * This struct is included as part
 * of all the various node sizes
 */
typedef struct {
    uint8_t type;
    uint8_t num_children;
    uint32_t partial_len;
    unsigned char partial[MAX_PREFIX_LEN];

    /*  ReadLock _Readlock;
      WriteLock _Writelock;
      UpgradeLock _UpgradeLock;


      void lock_shared() {
          //lock.lock();
          _Readlock.lock();
      }

      void unlock_shared() {
          _Readlock.unlock();
      }

     void lock_exclusive() {
        //  lock.lock();
      }

      void unlock_execlusive() {
         // lock.unlock();
      }

      // upgrade to exclusive lock
      void upgrade_lock() {
         // lock.lock();
      }

      // downgrade to shared lock
      void downgrade_lock() {
          //lock.unlock();
      }*/

} art_node;

/**
 * Small node with only 4 children
 */
typedef struct {
    art_node n;
    unsigned char keys[4];
    art_node *children[4];
} art_node4;

/**
 * Node with 16 children
 */
typedef struct {
    art_node n;
    unsigned char keys[16];
    art_node *children[16];
} art_node16;

/**
 * Node with 48 children, but
 * a full 256 byte field.
 */
typedef struct {
    art_node n;
    unsigned char keys[256];
    art_node *children[48];
} art_node48;

/**
 * Full node with 256 children
 */
typedef struct {
    art_node n;
    art_node *children[256];
} art_node256;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct {
    void *value;//point it to Snapshot->Current();
    uint32_t key_len;
    unsigned char key[];
}art_leaf;

/**
 * Main struct, points to root.
 */
typedef struct {
    art_node *root;
    uint64_t size;
} art_tree;



typedef std::function<bool( void *)> Pred;
typedef std::function<bool( void *)> UpdelFunc;


/**
    * Allocates a node of the given type,
    * initializes to zero and sets the type.
    */
static art_node* alloc_node(uint8_t type) {
    art_node* n;
    switch (type) {
        case NODE4:
            n = (art_node*)calloc(1, sizeof(art_node4));
            break;
        case NODE16:
            n = (art_node*)calloc(1, sizeof(art_node16));
            break;
        case NODE48:
            n = (art_node*)calloc(1, sizeof(art_node48));
            break;
        case NODE256:
            n = (art_node*)calloc(1, sizeof(art_node256));
            break;
        default:
            abort();
    }
    n->type = type;
    return n;
}

/**
 * Initializes an ART tree
 * @return 0 on success.
 */
int art_tree_init(std::shared_ptr<art_tree> t) {
    t->root = NULL;
    t->size = 0;
    return 0;
}

// Recursively destroys the tree
static void destroy_node(art_node *n) {
    // Break if null
    if (!n) return;

    // Special case leafs
    if (IS_LEAF(n)) {
        free(LEAF_RAW(n));
        return;
    }

    // Handle each node type
    int i;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
        case NODE4:
            p.p1 = (art_node4*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p1->children[i]);
            }
            break;

        case NODE16:
            p.p2 = (art_node16*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p2->children[i]);
            }
            break;

        case NODE48:
            p.p3 = (art_node48*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p3->children[i]);
            }
            break;

        case NODE256:
            p.p4 = (art_node256*)n;
            for (i=0;i<256;i++) {
                if (p.p4->children[i])
                    destroy_node(p.p4->children[i]);
            }
            break;

        default:
            abort();
    }

    // Free ourself on the way up
    free(n);
}

/**
 * Destroys an ART tree
 * @return 0 on success.
 */
int art_tree_destroy(std::shared_ptr<art_tree> t) {
    destroy_node(t->root);
    return 0;
}



static art_node** find_child(art_node *n, unsigned char c) {
    int i, mask, bitfield;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
        case NODE4:
            p.p1 = (art_node4*)n;
            for (i=0 ; i < n->num_children; i++) {
                /* this cast works around a bug in gcc 5.1 when unrolling loops
                 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59124
                 */
                if (((unsigned char*)p.p1->keys)[i] == c)
                    return &p.p1->children[i];
            }
            break;

            {
                case NODE16:
                    p.p2 = (art_node16*)n;

                // support non-86 architectures
#ifdef __i386__
                // Compare the key to all 16 stored keys
                __m128i cmp;
                cmp = _mm_cmpeq_epi8(_mm_set1_epi8(c),
                        _mm_loadu_si128((__m128i*)p.p2->keys));

                // Use a mask to ignore children that don't exist
                mask = (1 << n->num_children) - 1;
                bitfield = _mm_movemask_epi8(cmp) & mask;
#else
#ifdef __amd64__
                // Compare the key to all 16 stored keys
                __m128i cmp;
                cmp = _mm_cmpeq_epi8(_mm_set1_epi8(c),
                                     _mm_loadu_si128((__m128i*)p.p2->keys));

                // Use a mask to ignore children that don't exist
                mask = (1 << n->num_children) - 1;
                bitfield = _mm_movemask_epi8(cmp) & mask;
#else
                // Compare the key to all 16 stored keys
                unsigned bitfield = 0;
                for (short i = 0; i < 16; ++i) {
                    if (p.p2->keys[i] == c)
                        bitfield |= (1 << i);
                }

                // Use a mask to ignore children that don't exist
                bitfield &= mask;
#endif
#endif

                /*
                 * If we have a match (any bit set) then we can
                 * return the pointer match using ctz to get
                 * the index.
                 */
                if (bitfield)
                    return &p.p2->children[__builtin_ctz(bitfield)];
                break;
            }

        case NODE48:
            p.p3 = (art_node48*)n;
            i = p.p3->keys[c];
            if (i)
                return &p.p3->children[i-1];
            break;

        case NODE256:
            p.p4 = (art_node256*)n;
            if (p.p4->children[c])
                return &p.p4->children[c];
            break;

        default:
            abort();
    }
    return NULL;
}

// Simple inlined if
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

/**
 * Returns the number of prefix characters shared between
 * the key and node.
 */
static int check_prefix(const art_node *n, const unsigned char *key, int key_len, int depth) {
    int max_cmp = min(min(n->partial_len, MAX_PREFIX_LEN), key_len - depth);
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth+idx])
            return idx;
    }
    return idx;
}

/**
 * Checks if a leaf matches
 * @return 0 on success.
 */
static int leaf_matches(const art_leaf *n, const unsigned char *key, int key_len, int depth) {
    (void)depth;
    // Fail if the key lengths are different
    if (n->key_len != (uint32_t)key_len) return 1;

    // Compare the keys starting at the depth
    return memcmp(n->key, key, key_len);
}

/**
 * Searches for a value in the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_search(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len);

// Find the minimum leaf under a node
static art_leaf* minimum(const art_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx;
    switch (n->type) {
        case NODE4:
            return minimum(((const art_node4*)n)->children[0]);
        case NODE16:
            return minimum(((const art_node16*)n)->children[0]);
        case NODE48:
            idx=0;
            while (!((const art_node48*)n)->keys[idx]) idx++;
            idx = ((const art_node48*)n)->keys[idx] - 1;
            return minimum(((const art_node48*)n)->children[idx]);
        case NODE256:
            idx=0;
            while (!((const art_node256*)n)->children[idx]) idx++;
            return minimum(((const art_node256*)n)->children[idx]);
        default:
            abort();
    }
}

// Find the maximum leaf under a node
static art_leaf* maximum(const art_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx;
    switch (n->type) {
        case NODE4:
            return maximum(((const art_node4*)n)->children[n->num_children-1]);
        case NODE16:
            return maximum(((const art_node16*)n)->children[n->num_children-1]);
        case NODE48:
            idx=255;
            while (!((const art_node48*)n)->keys[idx]) idx--;
            idx = ((const art_node48*)n)->keys[idx] - 1;
            return maximum(((const art_node48*)n)->children[idx]);
        case NODE256:
            idx=255;
            while (!((const art_node256*)n)->children[idx]) idx--;
            return maximum(((const art_node256*)n)->children[idx]);
        default:
            abort();
    }
}

/**
 * Returns the minimum valued leaf
 */
art_leaf* art_minimum(art_tree *t)
{
    return minimum((art_node*)t->root);
}

/**
 * Returns the maximum valued leaf
 */
art_leaf* art_maximum(art_tree *t)
{
    return maximum((art_node*)t->root);
}


static art_leaf* make_leaf(const unsigned char *key, int key_len, void *value) {
    art_leaf *l = (art_leaf*)malloc(sizeof(art_leaf)+key_len);
    //memcpy(l->value, value, sizeof(value));
    l->value = value;
    l->key_len = key_len;
    memcpy(l->key, key, key_len);
    return l;
}

static int longest_common_prefix(art_leaf *l1, art_leaf *l2, int depth) {
    int max_cmp = min(l1->key_len, l2->key_len) - depth;
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (l1->key[depth+idx] != l2->key[depth+idx])
            return idx;
    }
    return idx;
}

static void copy_header(art_node *dest, art_node *src) {
    dest->num_children = src->num_children;
    dest->partial_len = src->partial_len;
    memcpy(dest->partial, src->partial, min(MAX_PREFIX_LEN, src->partial_len));
}

static void add_child256(art_node256 *n, art_node **ref, unsigned char c, void *child) {
    (void)ref;
    n->n.num_children++;
    n->children[c] = (art_node*)child;
}

static void add_child48(art_node48 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 48) {
        int pos = 0;
        while (n->children[pos]) pos++;
        n->children[pos] = (art_node*)child;
        n->keys[c] = pos + 1;
        n->n.num_children++;
    } else {
        art_node256 *new_node = (art_node256*)alloc_node(NODE256);
        for (int i=0;i<256;i++) {
            if (n->keys[i]) {
                new_node->children[i] = n->children[n->keys[i] - 1];
            }
        }
        copy_header((art_node*)new_node, (art_node*)n);
        *ref = (art_node*)new_node;
        free(n);
        add_child256(new_node, ref, c, child);
    }
}

static void add_child16(art_node16 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 16) {
        unsigned mask = (1 << n->n.num_children) - 1;

        // support non-x86 architectures
#ifdef __i386__
        __m128i cmp;

            // Compare the key to all 16 stored keys
            cmp = _mm_cmplt_epi8(_mm_set1_epi8(c),
                    _mm_loadu_si128((__m128i*)n->keys));

            // Use a mask to ignore children that don't exist
            unsigned bitfield = _mm_movemask_epi8(cmp) & mask;
#else
#ifdef __amd64__
        __m128i cmp;

        // Compare the key to all 16 stored keys
        cmp = _mm_cmplt_epi8(_mm_set1_epi8(c),
                             _mm_loadu_si128((__m128i*)n->keys));

        // Use a mask to ignore children that don't exist
        unsigned bitfield = _mm_movemask_epi8(cmp) & mask;
#else
        // Compare the key to all 16 stored keys
            unsigned bitfield = 0;
            for (short i = 0; i < 16; ++i) {
                if (c < n->keys[i])
                    bitfield |= (1 << i);
            }

            // Use a mask to ignore children that don't exist
            bitfield &= mask;
#endif
#endif

        // Check if less than any
        unsigned idx;
        if (bitfield) {
            idx = __builtin_ctz(bitfield);
            memmove(n->keys+idx+1,n->keys+idx,n->n.num_children-idx);
            memmove(n->children+idx+1,n->children+idx,
                    (n->n.num_children-idx)*sizeof(void*));
        } else
            idx = n->n.num_children;

        // Set the child
        n->keys[idx] = c;
        n->children[idx] = (art_node*)child;
        n->n.num_children++;

    } else {
        art_node48 *new_node = (art_node48*)alloc_node(NODE48);

        // Copy the child pointers and populate the key map
        memcpy(new_node->children, n->children,
               sizeof(void*)*n->n.num_children);
        for (int i=0;i<n->n.num_children;i++) {
            new_node->keys[n->keys[i]] = i + 1;
        }
        copy_header((art_node*)new_node, (art_node*)n);
        *ref = (art_node*)new_node;
        free(n);
        add_child48(new_node, ref, c, child);
    }
}

static void add_child4(art_node4 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 4) {
        int idx;
        for (idx=0; idx < n->n.num_children; idx++) {
            if (c < n->keys[idx]) break;
        }

        // Shift to make room
        memmove(n->keys+idx+1, n->keys+idx, n->n.num_children - idx);
        memmove(n->children+idx+1, n->children+idx,
                (n->n.num_children - idx)*sizeof(void*));

        // Insert element
        n->keys[idx] = c;
        n->children[idx] = (art_node*)child;
        n->n.num_children++;

    } else {
        art_node16 *new_node = (art_node16*)alloc_node(NODE16);

        // Copy the child pointers and the key map
        memcpy(new_node->children, n->children,
               sizeof(void*)*n->n.num_children);
        memcpy(new_node->keys, n->keys,
               sizeof(unsigned char)*n->n.num_children);
        copy_header((art_node*)new_node, (art_node*)n);
        *ref = (art_node*)new_node;
        free(n);
        add_child16(new_node, ref, c, child);
    }
}

static void add_child(art_node *n, art_node **ref, unsigned char c, void *child) {
    switch (n->type) {
        case NODE4:
            return add_child4((art_node4*)n, ref, c, child);
        case NODE16:
            return add_child16((art_node16*)n, ref, c, child);
        case NODE48:
            return add_child48((art_node48*)n, ref, c, child);
        case NODE256:
            return add_child256((art_node256*)n, ref, c, child);
        default:
            abort();
    }
}

/**
 * Calculates the index at which the prefixes mismatch
 */
static int prefix_mismatch(const art_node *n, const unsigned char *key, int key_len, int depth) {
    int max_cmp = min(min(MAX_PREFIX_LEN, n->partial_len), key_len - depth);
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth+idx])
            return idx;
    }

    // If the prefix is short we can avoid finding a leaf
    if (n->partial_len > MAX_PREFIX_LEN) {
        // Prefix is longer than what we've checked, find a leaf
        art_leaf *l = minimum(n);
        max_cmp = min(l->key_len, key_len)- depth;
        for (; idx < max_cmp; idx++) {
            if (l->key[idx+depth] != key[depth+idx])
                return idx;
        }
    }
    return idx;
}

static void remove_child256(art_node256 *n, art_node **ref, unsigned char c) {
    n->children[c] = NULL;
    n->n.num_children--;

    // Resize to a node48 on underflow, not immediately to prevent
    // trashing if we sit on the 48/49 boundary
    if (n->n.num_children == 37) {
        art_node48 *new_node = (art_node48*)alloc_node(NODE48);
        *ref = (art_node*)new_node;
        copy_header((art_node*)new_node, (art_node*)n);

        int pos = 0;
        for (int i=0;i<256;i++) {
            if (n->children[i]) {
                new_node->children[pos] = n->children[i];
                new_node->keys[i] = pos + 1;
                pos++;
            }
        }
        free(n);
    }
}

static void remove_child48(art_node48 *n, art_node **ref, unsigned char c) {
    int pos = n->keys[c];
    n->keys[c] = 0;
    n->children[pos-1] = NULL;
    n->n.num_children--;

    if (n->n.num_children == 12) {
        art_node16 *new_node = (art_node16*)alloc_node(NODE16);
        *ref = (art_node*)new_node;
        copy_header((art_node*)new_node, (art_node*)n);

        int child = 0;
        for (int i=0;i<256;i++) {
            pos = n->keys[i];
            if (pos) {
                new_node->keys[child] = i;
                new_node->children[child] = n->children[pos - 1];
                child++;
            }
        }
        free(n);
    }
}

static void remove_child16(art_node16 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->n.num_children--;

    if (n->n.num_children == 3) {
        art_node4 *new_node = (art_node4*)alloc_node(NODE4);
        *ref = (art_node*)new_node;
        copy_header((art_node*)new_node, (art_node*)n);
        memcpy(new_node->keys, n->keys, 4);
        memcpy(new_node->children, n->children, 4*sizeof(void*));
        free(n);
    }
}

static void remove_child4(art_node4 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->n.num_children--;

    // Remove nodes with only a single child
    if (n->n.num_children == 1) {
        art_node *child = n->children[0];
        if (!IS_LEAF(child)) {
            // Concatenate the prefixes
            int prefix = n->n.partial_len;
            if (prefix < MAX_PREFIX_LEN) {
                n->n.partial[prefix] = n->keys[0];
                prefix++;
            }
            if (prefix < MAX_PREFIX_LEN) {
                int sub_prefix = min(child->partial_len, MAX_PREFIX_LEN - prefix);
                memcpy(n->n.partial+prefix, child->partial, sub_prefix);
                prefix += sub_prefix;
            }

            // Store the prefix in the child
            memcpy(child->partial, n->n.partial, min(prefix, MAX_PREFIX_LEN));
            child->partial_len += n->n.partial_len + 1;
        }
        *ref = child;
        free(n);
    }
}

static void remove_child(art_node *n, art_node **ref, unsigned char c, art_node **l) {
    switch (n->type) {
        case NODE4:
            return remove_child4((art_node4*)n, ref, l);
        case NODE16:
            return remove_child16((art_node16*)n, ref, l);
        case NODE48:
            return remove_child48((art_node48*)n, ref, c);
        case NODE256:
            return remove_child256((art_node256*)n, ref, c);
        default:
            abort();
    }
}

static art_leaf* recursive_delete(std::shared_ptr<art_tree> t,art_node *n, art_node **ref, const unsigned char *key, int key_len, int depth) {
    // Search terminated
    if (!n) return NULL;

    // Handle hitting a leaf node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        if (!leaf_matches(l, key, key_len, depth)) {
            *ref = NULL;
            return l;
        }
        return NULL;
    }

    // Bail if the prefix does not match
    if (n->partial_len) {
        int prefix_len = check_prefix(n, key, key_len, depth);
        if (prefix_len != min(MAX_PREFIX_LEN, n->partial_len)) {
            return NULL;
        }
        depth = depth + n->partial_len;
    }

    // Find child node
    art_node **child = find_child(n, key[depth]);
    if (!child) return NULL;

    // If the child is leaf, delete from this node
    if (IS_LEAF(*child)) {
        art_leaf *l = LEAF_RAW(*child);
        if (!leaf_matches(l, key, key_len, depth)) {
            remove_child(n, ref, key[depth], child);
            return l;
        }
        return NULL;

        // Recurse
    } else {
        return recursive_delete(t,*child, child, key, key_len, depth+1);
    }
}

/**
 * Deletes a value from the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_delete(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len)
{
    art_leaf *l = recursive_delete(t,t->root, &t->root, key, key_len, 0);
    if (l)
    {
        t->size--;
        void *old = l->value;
        free(l);
        return old;
    }
    return NULL;
}

/**
 * Deletes a value from the ART tree by Predicate
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
/// Recursively iterates over the tree
int recursive_iter(art_node *n, art_callback cb, void *data)
{

    RecursiveScopedLock _recursiveLock;
    //UpgradeLock _sharedLock(_access);
    // Handle base cases
    if (!n) return 0;
    if (IS_LEAF(n))
    {
        art_leaf *l = LEAF_RAW(n);
        return cb(data, (const unsigned char*)l->key, l->key_len, l->value);
    }

    int idx, res;
    switch (n->type)
    {
        case NODE4:
            for (int i=0; i < n->num_children; i++) {
                //_sharedLock.unlock();
                res = recursive_iter(((art_node4*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE16:
            for (int i=0; i < n->num_children; i++) {
               // _sharedLock.unlock();
                res = recursive_iter(((art_node16*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE48:
            for (int i=0; i < 256; i++) {
                idx = ((art_node48*)n)->keys[i];
                if (!idx) continue;
               // _sharedLock.unlock();
                res = recursive_iter(((art_node48*)n)->children[idx-1], cb, data);
                if (res) return res;
            }
            break;

        case NODE256:
            for (int i=0; i < 256; i++) {
                if (!((art_node256*)n)->children[i]) continue;
                //_sharedLock.unlock();
                res = recursive_iter(((art_node256*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;
        default:
           // _sharedLock.unlock();
            abort();
    }
    return 0;
}

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each. The call back gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter(std::shared_ptr<art_tree> t,art_callback cb, void *data);




/**
 * Checks if a leaf prefix matches
 * @return 0 on success.
 */
static int leaf_prefix_matches(const art_leaf *n, const unsigned char *prefix, int prefix_len) {
    // Fail if the key length is too short
    if (n->key_len < (uint32_t)prefix_len) return 1;

    // Compare the keys
    return memcmp(n->key, prefix, prefix_len);
}

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each that matches a given prefix.
 * The call back gets a key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg prefix The prefix of keys to read
 * @arg prefix_len The length of the prefix
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter_prefix(art_tree *t, const unsigned char *key, int key_len, art_callback cb, void *data) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len, depth = 0;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (art_node*)LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_prefix_matches((art_leaf*)n, key, key_len)) {
                art_leaf *l = (art_leaf*)n;
                return cb(data, (const unsigned char*)l->key, l->key_len, l->value);
            }
            return 0;
        }

        // If the depth matches the prefix, we need to handle this node
        if (depth == key_len) {
            art_leaf *l = minimum(n);
            if (!leaf_prefix_matches(l, key, key_len))
                return recursive_iter(n, cb, data);
            return 0;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {
            prefix_len = prefix_mismatch(n, key, key_len, depth);

            // Guard if the mis-match is longer than the MAX_PREFIX_LEN
            if ((uint32_t)prefix_len > n->partial_len) {
                prefix_len = n->partial_len;
            }

            // If there is no match, search is terminated
            if (!prefix_len) {
                return 0;

                // If we've matched the prefix, iterate on this node
            } else if (depth + prefix_len == key_len) {
                return recursive_iter(n, cb, data);
            }

            // if there is a full match, go deeper
            depth = depth + n->partial_len;
        }

        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return 0;
}

void* recursive_insert(art_node *n, art_node **ref, const unsigned char *key, int key_len, void *value, int depth, int *old)
{
    //RecursiveScopedLock(_access);
    //SharedLock _sharedLock(_access)
    UpgradeLock _upgradeableReadLock(_access);
    //unique_lock<shared_mutex> lock(_access);


    // If we are at a NULL node, inject a leaf
    if (!n) {
        ///write lock to create a new leaf at the root
       WriteLock _writeLock(_upgradeableReadLock);
       *ref = (art_node *) SET_LEAF(make_leaf(key, key_len, value));
       return NULL;
    }

    // If we are at a leaf, we need to replace it with a node
    if (IS_LEAF(n)) {
        ///write lock here
        WriteLock _writeLock(_upgradeableReadLock);

        art_leaf *l = LEAF_RAW(n);

        // Check if we are updating an existing value
        if (!leaf_matches(l, key, key_len, depth)) {
            *old = 1;

            ///MVCC update current version
            void *old_val = l->value;
            l->value = value;
            return old_val;
        }

        // New value, we must split the leaf into a node4
        art_node4 *new_node = (art_node4*)alloc_node(NODE4);

        // Create a new leaf
        /// MVCC make new mvcc object pointing to current-version;
        art_leaf *l2 = make_leaf(key, key_len, value);

        // Determine longest prefix
        int longest_prefix = longest_common_prefix(l, l2, depth);
        new_node->n.partial_len = longest_prefix;
        memcpy(new_node->n.partial, key+depth, min(MAX_PREFIX_LEN, longest_prefix));
        // Add the leafs to the new node4
        *ref = (art_node*)new_node;
        add_child4(new_node, ref, l->key[depth+longest_prefix], SET_LEAF(l));
        add_child4(new_node, ref, l2->key[depth+longest_prefix], SET_LEAF(l2));
        return NULL;
    }

    // Check if given node has a prefix
    if (n->partial_len) {
        // Determine if the prefixes differ, since we need to split
        int prefix_diff = prefix_mismatch(n, key, key_len, depth);
        if ((uint32_t)prefix_diff >= n->partial_len) {
            depth += n->partial_len;
            goto RECURSE_SEARCH;
        }

        // Create a new node
        ///Write lock here again to create new node
        WriteLock _writeLock(_upgradeableReadLock);

        art_node4 *new_node = (art_node4*)alloc_node(NODE4);
        *ref = (art_node*)new_node;
        new_node->n.partial_len = prefix_diff;
        memcpy(new_node->n.partial, n->partial, min(MAX_PREFIX_LEN, prefix_diff));

        // Adjust the prefix of the old node
        if (n->partial_len <= MAX_PREFIX_LEN) {
            add_child4(new_node, ref, n->partial[prefix_diff], n);
            n->partial_len -= (prefix_diff+1);
            memmove(n->partial, n->partial+prefix_diff+1,
                    min(MAX_PREFIX_LEN, n->partial_len));
        } else {
            n->partial_len -= (prefix_diff+1);
            art_leaf *l = minimum(n);
            add_child4(new_node, ref, l->key[depth+prefix_diff], n);
            memcpy(n->partial, l->key+depth+prefix_diff+1,
                   min(MAX_PREFIX_LEN, n->partial_len));
        }

        // Insert the new leaf

        ///mvcc make new leaf
        art_leaf *l = make_leaf(key, key_len, value);
        add_child4(new_node, ref, key[depth+prefix_diff], SET_LEAF(l));
        return NULL;
    }

    RECURSE_SEARCH:;
    // Find a child to recurse to
    art_node **child = find_child(n, key[depth]);
    if (child) {
        //lock.unlock();
        _upgradeableReadLock.unlock();
        return recursive_insert(*child, child, key, key_len, value, depth+1, old);
    }

    // No child, node goes within us
    ///mvcc make new leaf
    art_leaf *l = make_leaf(key, key_len, value);
    add_child(n, ref, key[depth], SET_LEAF(l));
    return NULL;
}

/**
 * Inserts a new value into the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void* art_insert(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len, void *value);


template <typename RecordType, typename KeyType = DefaultKeyType>
class ArtCPP {

    public: std::shared_ptr<art_tree> t;
    public: char buf[512];
    public: int res;
    public: uint64_t ARTSize;

    /**
     * Represents a leaf. These are
    * of arbitrary size, as they include the key with multiversioned object to get lock free Read access.
    */
    typedef struct mv_art_leaf {
        void *value;
        uint32_t key_len;
        unsigned char key[];
    };

    __inline bool IS_MV_LEAF(art_node* p)
    {
        return ((uintptr_t)p & 1) != 0;
    }

    __inline mv_art_leaf* SET_MV_LEAF(mv_art_leaf* p)
    {
        return (mv_art_leaf*) ((uintptr_t)p | 1);
    }

    __inline mv_art_leaf* MV_LEAF_RAW(void* p)
    {
        return (mv_art_leaf*)((uintptr_t)p & ~1);
    }

    /**
    * Checks if a leaf matches
    * @return 0 on success.
    */
    static int mv_leaf_matches(const mv_art_leaf *n, const unsigned char *key, int key_len, int depth) {
        (void)depth;
        // Fail if the key lengths are different
        if (n->key_len != (uint32_t)key_len) return 1;

        // Compare the keys starting at the depth
        return memcmp(n->key, key, key_len);
    }

    static int mv_longest_common_prefix(mv_art_leaf *l1, mv_art_leaf *l2, int depth) {
        int max_cmp = min(l1->key_len, l2->key_len) - depth;
        int idx;
        for (idx=0; idx < max_cmp; idx++) {
            if (l1->key[depth+idx] != l2->key[depth+idx])
                return idx;
        }
        return idx;
    }


    public: ArtCPP()
    {
        t = std::make_shared<art_tree>();
        art_tree_init(t);
    }

    /**
     * Destructor for table.
     */
    ~ArtCPP()
    {
        art_tree_destroy(t);
    }


    /**
    * Returns the size of the ART tree.
    */
    #ifdef BROKEN_GCC_C99_INLINE
    # define art_size(t) ((t)->size)
    #else
    inline uint64_t art_size()
    {
        return t->size;
    }
    #endif


    auto mv_art_delete(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len,size_t txn_id,std::string& status)
    {
        int old_val = 0;
        auto old = mv_recursive_delete(t->root, &t->root, key, key_len,0, &old_val,txn_id,status);
        if (!old_val) t->size++;
        return old;
    }

    void* mv_recursive_delete(art_node *n, art_node **ref, const unsigned char *key, int key_len, int depth, int *old,size_t txn_id,std::string& status)
    {
        UpgradeLock _upgradeableReadLock(_access);

        // If we are at a NULL node, inject a leaf
        if (!n) {
            ///write lock to create a new leaf at the root

            return NULL;
        }

        // If we are at a leaf, we need to replace it with a node
        if (IS_MV_LEAF(n)) {
            ///write lock here
            WriteLock _writeLock(_upgradeableReadLock);

            mv_art_leaf *l = MV_LEAF_RAW(n);

            // Check if we are updating an existing value
            if (!mv_leaf_matches(l, key, key_len, depth))
            {
                *old = 1;
                ///MVCC update current version
                //std::cout<<"overwritting="<<txn_id<<key<<std::endl;
                mvcc11::mvcc<RecordType>* _mvcc = reinterpret_cast<mvcc11::mvcc<RecordType>*>(l->value);
                _mvcc->deleteMV(txn_id,status);
                l->value = reinterpret_cast<void*>(_mvcc);
                return l->value;
            }

            /*
            // New value, we must split the leaf into a node4
            art_node4 *new_node = (art_node4*)alloc_node(NODE4);

            // Create a new leaf
            /// MVCC make new mvcc object pointing to current-version;
            mv_art_leaf *l2 = make_mvv_leaf(key, key_len, value,txn_id,status);

            // Determine longest prefix
            int longest_prefix = mv_longest_common_prefix(l, l2, depth);
            new_node->n.partial_len = longest_prefix;
            memcpy(new_node->n.partial, key+depth, min(MAX_PREFIX_LEN, longest_prefix));
            // Add the leafs to the new node4
            *ref = (art_node*)new_node;
            add_child4(new_node, ref, l->key[depth+longest_prefix], SET_MV_LEAF(l));
            add_child4(new_node, ref, l2->key[depth+longest_prefix], SET_MV_LEAF(l2));*/
            return NULL;
        }

        // Check if given node has a prefix
        if (n->partial_len) {
            // Determine if the prefixes differ, since we need to split
            int prefix_diff = prefix_mismatch(n, key, key_len, depth);
            if ((uint32_t)prefix_diff >= n->partial_len) {
                depth += n->partial_len;
                goto RECURSE_SEARCH;
            }

            // Create a new node
            ///Write lock here again to create new node
            WriteLock _writeLock(_upgradeableReadLock);

            art_node4 *new_node = (art_node4*)alloc_node(NODE4);
            *ref = (art_node*)new_node;
            new_node->n.partial_len = prefix_diff;
            memcpy(new_node->n.partial, n->partial, min(MAX_PREFIX_LEN, prefix_diff));

            // Adjust the prefix of the old node
            if (n->partial_len <= MAX_PREFIX_LEN) {
                add_child4(new_node, ref, n->partial[prefix_diff], n);
                n->partial_len -= (prefix_diff+1);
                memmove(n->partial, n->partial+prefix_diff+1, min(MAX_PREFIX_LEN, n->partial_len));
            } else {
                n->partial_len -= (prefix_diff+1);
                art_leaf *l = minimum(n);
                add_child4(new_node, ref, l->key[depth+prefix_diff], n);
                memcpy(n->partial, l->key+depth+prefix_diff+1, min(MAX_PREFIX_LEN, n->partial_len));
            }

            // Insert the new leaf

            ///mvcc make new leaf
            //mv_art_leaf *l = make_mvv_leaf(key, key_len, value,txn_id,status);
            //add_child4(new_node, ref, key[depth+prefix_diff], SET_MV_LEAF(l));
            return NULL;
        }

        RECURSE_SEARCH:;
        // Find a child to recurse to
        art_node **child = find_child(n, key[depth]);
        if (child)
        {
            //lock.unlock();
            _upgradeableReadLock.unlock();
            return mv_recursive_delete(*child, child, key, key_len,depth+1, old,txn_id,status);
        }

        // No child, node goes within us
        ///mvcc make new leaf
        //mv_art_leaf *l = make_mvv_leaf(key, key_len, value,txn_id,status);
        //add_child(n, ref, key[depth], SET_MV_LEAF(l));
        return NULL;
    }

    auto mv_art_insert(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len, RecordType&  value,size_t txn_id,std::string& status)
    {
        int old_val = 0;
        //art_node *root =static_cast<art_node*>(t->root);
        auto old = mv_recursive_insert(t->root, &t->root, key, key_len, value, 0, &old_val,txn_id,status);
        if (!old_val) t->size++;
        return old;
    }

    void * mv_recursive_insert(art_node *n, art_node **ref, const unsigned char *key, int key_len, RecordType& value, int depth, int *old,size_t txn_id,std::string& status)
    {
        UpgradeLock _upgradeableReadLock(_access);

        // If we are at a NULL node, inject a leaf
        if (!n) {
            ///write lock to create a new leaf at the root
            WriteLock _writeLock(_upgradeableReadLock);
            auto snapshot = SET_MV_LEAF(make_mvv_leaf(key, key_len, value,txn_id,status));
            *ref = (art_node*) snapshot;
            return NULL;
        }

        // If we are at a leaf, we need to replace it with a node
        if (IS_MV_LEAF(n)) {
            ///write lock here
            WriteLock _writeLock(_upgradeableReadLock);

            mv_art_leaf *l = MV_LEAF_RAW(n);

            // Check if we are updating an existing value
            if (!mv_leaf_matches(l, key, key_len, depth))
            {
                *old = 1;

                ///MVCC update current version
                //std::cout<<"overwritting="<<txn_id<<key<<std::endl;
                mvcc11::mvcc<RecordType>* _mvcc = reinterpret_cast<mvcc11::mvcc<RecordType>*>(l->value);
                _mvcc->overwriteMV(txn_id,value,status);
                l->value = static_cast<void*>(_mvcc);
                return l->value;
            }

            // New value, we must split the leaf into a node4
            art_node4 *new_node = (art_node4*)alloc_node(NODE4);

            // Create a new leaf
            /// MVCC make new mvcc object pointing to current-version;
            mv_art_leaf *l2 = make_mvv_leaf(key, key_len, value,txn_id,status);

            // Determine longest prefix
            int longest_prefix = mv_longest_common_prefix(l, l2, depth);
            new_node->n.partial_len = longest_prefix;
            memcpy(new_node->n.partial, key+depth, min(MAX_PREFIX_LEN, longest_prefix));
            // Add the leafs to the new node4
            *ref = (art_node*)new_node;
            add_child4(new_node, ref, l->key[depth+longest_prefix], SET_MV_LEAF(l));
            add_child4(new_node, ref, l2->key[depth+longest_prefix], SET_MV_LEAF(l2));
            return NULL;
        }

        // Check if given node has a prefix
        if (n->partial_len) {
            // Determine if the prefixes differ, since we need to split
            int prefix_diff = prefix_mismatch(n, key, key_len, depth);
            if ((uint32_t)prefix_diff >= n->partial_len) {
                depth += n->partial_len;
                goto RECURSE_SEARCH;
            }

            // Create a new node
            ///Write lock here again to create new node
            WriteLock _writeLock(_upgradeableReadLock);

            art_node4 *new_node = (art_node4*)alloc_node(NODE4);
            *ref = (art_node*)new_node;
            new_node->n.partial_len = prefix_diff;
            memcpy(new_node->n.partial, n->partial, min(MAX_PREFIX_LEN, prefix_diff));

            // Adjust the prefix of the old node
            if (n->partial_len <= MAX_PREFIX_LEN) {
                add_child4(new_node, ref, n->partial[prefix_diff], n);
                n->partial_len -= (prefix_diff+1);
                memmove(n->partial, n->partial+prefix_diff+1,
                        min(MAX_PREFIX_LEN, n->partial_len));
            } else {
                n->partial_len -= (prefix_diff+1);
                art_leaf *l = minimum(n);
                add_child4(new_node, ref, l->key[depth+prefix_diff], n);
                memcpy(n->partial, l->key+depth+prefix_diff+1,
                       min(MAX_PREFIX_LEN, n->partial_len));
            }

            // Insert the new leaf

            ///mvcc make new leaf
            mv_art_leaf *l = make_mvv_leaf(key, key_len, value,txn_id,status);
            add_child4(new_node, ref, key[depth+prefix_diff], SET_MV_LEAF(l));
            return NULL;
        }

        RECURSE_SEARCH:;
        // Find a child to recurse to
        art_node **child = find_child(n, key[depth]);
        if (child)
        {
            //lock.unlock();
            _upgradeableReadLock.unlock();
            return mv_recursive_insert(*child, child, key, key_len, value, depth+1, old,txn_id,status);
        }

        // No child, node goes within us
        ///mvcc make new leaf
        mv_art_leaf *l = make_mvv_leaf(key, key_len, value,txn_id,status);
        add_child(n, ref, key[depth], SET_MV_LEAF(l));
        return NULL;
    }

    static mv_art_leaf* make_mvv_leaf(const unsigned char *key, int key_len,RecordType& value,const size_t txn_id,std::string& status)
    {
        mv_art_leaf *l = (mv_art_leaf*)malloc(sizeof(mv_art_leaf)+key_len);
        l->key_len = key_len;
        memcpy(l->key, key, key_len);
        mvcc11::mvcc<RecordType>* _mvcc = new mvcc11::mvcc<RecordType>(txn_id,value,status);
        l->value = static_cast<void*>(_mvcc);
        //std::cout<<"Inserting by::"<<txn_id<<"-"<<key<<std::endl;
        return l;
    }

    static int mv_art_iter(std::shared_ptr<art_tree> t,art_callback cb, void *data)
    {
        std::cout<<"howdy";
        return mv_recursive_iter(t->root, cb, data);
    }

    static int mv_recursive_iter(art_node *n, art_callback cb, void *data)
    {

        RecursiveScopedLock _recursiveLock;
        //UpgradeLock _sharedLock(_access);
        // Handle base cases
        if (!n) return 0;
        if (IS_LEAF(n))
        {
            art_leaf *l = LEAF_RAW(n);
            return cb(data, (const unsigned char*)l->key, l->key_len, l->value);
        }

        int idx, res;
        switch (n->type)
        {
            case NODE4:
                for (int i=0; i < n->num_children; i++) {
                    //_sharedLock.unlock();
                    res = recursive_iter(((art_node4*)n)->children[i], cb, data);
                    if (res) return res;
                }
                break;

            case NODE16:
                for (int i=0; i < n->num_children; i++) {
                    // _sharedLock.unlock();
                    res = recursive_iter(((art_node16*)n)->children[i], cb, data);
                    if (res) return res;
                }
                break;

            case NODE48:
                for (int i=0; i < 256; i++) {
                    idx = ((art_node48*)n)->keys[i];
                    if (!idx) continue;
                    // _sharedLock.unlock();
                    res = recursive_iter(((art_node48*)n)->children[idx-1], cb, data);
                    if (res) return res;
                }
                break;

            case NODE256:
                for (int i=0; i < 256; i++) {
                    if (!((art_node256*)n)->children[i]) continue;
                    //_sharedLock.unlock();
                    res = recursive_iter(((art_node256*)n)->children[i], cb, data);
                    if (res) return res;
                }
                break;
            default:
                // _sharedLock.unlock();
                abort();
        }
        return 0;
    }

    int art_iter_prefix(art_tree *t, const unsigned char *key, int key_len, art_callback cb, void *data)
    {
        art_node **child;
        art_node *n = t->root;
        int prefix_len, depth = 0;
        while (n) {
            // Might be a leaf
            if (IS_LEAF(n)) {
                n = (art_node*)LEAF_RAW(n);
                // Check if the expanded path matches
                if (!leaf_prefix_matches((art_leaf*)n, key, key_len)) {
                    art_leaf *l = (art_leaf*)n;
                    return cb(data, (const unsigned char*)l->key, l->key_len, l->value);
                }
                return 0;
            }

            // If the depth matches the prefix, we need to handle this node
            if (depth == key_len) {
                art_leaf *l = minimum(n);
                if (!leaf_prefix_matches(l, key, key_len))
                    return recursive_iter(n, cb, data);
                return 0;
            }

            // Bail if the prefix does not match
            if (n->partial_len) {
                prefix_len = prefix_mismatch(n, key, key_len, depth);

                // Guard if the mis-match is longer than the MAX_PREFIX_LEN
                if ((uint32_t)prefix_len > n->partial_len) {
                    prefix_len = n->partial_len;
                }

                // If there is no match, search is terminated
                if (!prefix_len) {
                    return 0;

                    // If we've matched the prefix, iterate on this node
                } else if (depth + prefix_len == key_len) {
                    return recursive_iter(n, cb, data);
                }

                // if there is a full match, go deeper
                depth = depth + n->partial_len;
            }

            // Recursively search
            child = find_child(n, key[depth]);
            n = (child) ? *child : NULL;
            depth++;
        }
        return 0;
    }

    void* searchKey(const unsigned char* key, int key_len)
    {
        art_search(this->t, (unsigned char *)key, key_len);
    }

    static int recursive_iterByPredicate(art_node *n, art_callback cb, void *data,Pred predicate)
    {
        // Handle base cases
        if (!n) return 0;
        if (IS_LEAF(n))
        {
            art_leaf *l = LEAF_RAW(n);
            //printf("l-val %s",(const unsigned char *)l->value);
            if (l->value !=NULL) {
                if (predicate(l->value)) {
                    return cb(data, (const unsigned char *) l->key, l->key_len, l->value);
                }
                return NULL;
            }
        }

        int idx, res;
        switch (n->type)
        {
            case NODE4:
                for (int i=0; i < n->num_children; i++) {
                    res = recursive_iterByPredicate(((art_node4*)n)->children[i], cb, data,predicate);
                    if (res) return res;
                }
                break;

            case NODE16:
                for (int i=0; i < n->num_children; i++) {
                    res = recursive_iterByPredicate(((art_node16*)n)->children[i], cb, data,predicate);
                    if (res) return res;
                }
                break;

            case NODE48:
                for (int i=0; i < 256; i++) {
                    idx = ((art_node48*)n)->keys[i];
                    if (!idx) continue;
                    res = recursive_iterByPredicate(((art_node48*)n)->children[idx-1], cb, data,predicate);
                    if (res) return res;
                }
                break;

            case NODE256:
                for (int i=0; i < 256; i++) {
                    if (!((art_node256*)n)->children[i]) continue;
                    res = recursive_iterByPredicate(((art_node256*)n)->children[i], cb, data,predicate);
                    if (res) return res;
                }
                break;
            default:
                abort();
        }
        return 0;
    }

    int mv_art_iterByPredicate(std::shared_ptr<art_tree> t,art_callback cb, void *data, Pred filter)
    {
        return recursive_iterByPredicate(t->root, cb, data,filter);
    }

    int mv_art_update_by_predicate(std::shared_ptr<art_tree> t,art_callback cb,RecordType& value,size_t txn_id,std::string& status,void *data, Pred filter)
    {
        return recursive_update_by_predicate(t->root,cb,value,txn_id,status, data,filter);
    }

    static int recursive_update_by_predicate(art_node *n,art_callback cb,RecordType& value,size_t txn_id,std::string& status,void *data,Pred predicate)
    {
        // Handle base cases
        if (!n) return 0;
        if (IS_LEAF(n))
        {
            art_leaf *l = LEAF_RAW(n);
            if (l->value !=NULL)
            {
                if (predicate(l->value))
                {
                    mvcc11::mvcc<RecordType>* _mvcc = reinterpret_cast<mvcc11::mvcc<RecordType>*>(l->value);
                    _mvcc->overwriteMV(txn_id,value,status);
                    l->value = static_cast<void*>(_mvcc);
                    return cb(data, (const unsigned char *) l->key, l->key_len, l->value);
                }
            }
        }

        int idx, res;
        switch (n->type)
        {
            case NODE4:
                for (int i=0; i < n->num_children; i++) {
                    res = recursive_update_by_predicate(((art_node4*)n)->children[i],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;

            case NODE16:
                for (int i=0; i < n->num_children; i++) {
                    res = recursive_update_by_predicate(((art_node16*)n)->children[i],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;

            case NODE48:
                for (int i=0; i < 256; i++) {
                    idx = ((art_node48*)n)->keys[i];
                    if (!idx) continue;
                    res = recursive_update_by_predicate(((art_node48*)n)->children[idx-1],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;

            case NODE256:
                for (int i=0; i < 256; i++) {
                    if (!((art_node256*)n)->children[i]) continue;
                    res = recursive_update_by_predicate(((art_node256*)n)->children[i],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;
            default:
                abort();
        }
        return 0;
    }

    int mv_art_delete_by_predicate(std::shared_ptr<art_tree> t,art_callback cb,RecordType& value,size_t txn_id,std::string& status,void *data, Pred filter)
    {
        return recursive_delete_by_predicate(t->root,cb,value,txn_id,status, data,filter);
    }

    static int recursive_delete_by_predicate(art_node *n,art_callback cb,RecordType& value,size_t txn_id,std::string& status,void *data,Pred predicate)
    {
        // Handle base cases
        if (!n) return 0;
        if (IS_LEAF(n))
        {
            art_leaf *l = LEAF_RAW(n);
            if (l->value !=NULL) {
                if (predicate(l->value))
                {
                    mvcc11::mvcc<RecordType>* _mvcc = reinterpret_cast<mvcc11::mvcc<RecordType>*>(l->value);
                    _mvcc->deleteMV(txn_id,status);
                    l->value = static_cast<void*>(_mvcc);
                    return cb(data, (const unsigned char *) l->key, l->key_len, l->value);
                }
                return NULL;
            }
        }

        int idx, res;
        switch (n->type)
        {
            case NODE4:
                for (int i=0; i < n->num_children; i++) {
                    res = recursive_update_by_predicate(((art_node4*)n)->children[i],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;

            case NODE16:
                for (int i=0; i < n->num_children; i++) {
                    res = recursive_update_by_predicate(((art_node16*)n)->children[i],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;

            case NODE48:
                for (int i=0; i < 256; i++) {
                    idx = ((art_node48*)n)->keys[i];
                    if (!idx) continue;
                    res = recursive_update_by_predicate(((art_node48*)n)->children[idx-1],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;

            case NODE256:
                for (int i=0; i < 256; i++) {
                    if (!((art_node256*)n)->children[i]) continue;
                    res = recursive_update_by_predicate(((art_node256*)n)->children[i],cb,value,txn_id,status,data,predicate);
                    if (res) return res;
                }
                break;
            default:
                abort();
        }
        return 0;
    }

};

void* art_insert(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len, void *value)
{
    int old_val = 0;
    //art_node *root =static_cast<art_node*>(t->root);
    void *old = recursive_insert(t->root, &t->root, key, key_len, value, 0, &old_val);
    if (!old_val) t->size++;
    return old;
}

int art_iter(std::shared_ptr<art_tree> t,art_callback cb, void *data)
{
    //int old_val = 0;
    //art_node *root =static_cast<art_node*>(t->root);
    std::cout<<"howdy";
    return recursive_iter(t->root, cb, data);
}

void* art_search(std::shared_ptr<art_tree> t,const unsigned char *key, int key_len) {
    SharedLock _sharedLock(_access);
    art_node **child;
    art_node *n = t->root;
    int prefix_len, depth = 0;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (art_node*)LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_matches((art_leaf*)n, key, key_len, depth)) {
                std::cout<<"Searched Value ="<<(char *)(((art_leaf*)n)->value);
                return ((art_leaf*)n)->value;
            }
            return NULL;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {
            prefix_len = check_prefix(n, key, key_len, depth);
            if (prefix_len != min(MAX_PREFIX_LEN, n->partial_len))
                return NULL;
            depth = depth + n->partial_len;
        }

        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return NULL;
}


#endif //MVCCART_ARTCPP_HPP
