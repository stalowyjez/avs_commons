#include <avsystem/commons/rbtree.h>
#include "rbtree.h"

#ifndef NDEBUG
static int rb_is_cleanup_in_progress(AVS_RBTREE_CONST(void) tree) {
    return *tree && (_AVS_RB_PARENT_CONST(*tree) != NULL);
}

static int rb_is_node_detached(AVS_RBTREE_ELEM(void) elem) {
    return _AVS_RB_NODE(elem)->color == DETACHED
        && _AVS_RB_PARENT(elem) == NULL
        && _AVS_RB_LEFT(elem) == NULL
        && _AVS_RB_RIGHT(elem) == NULL;
}
#else
# define rb_is_cleanup_in_progress(_) 0

static int rb_is_node_detached(AVS_RBTREE_ELEM(void) elem) {
    return _AVS_RB_NODE(elem)->color == DETACHED;
}
#endif

static AVS_RBTREE_CONST(void) rb_tree_const(AVS_RBTREE(void) tree) {
    return (AVS_RBTREE_CONST(void))(intptr_t)tree;
}

enum rb_color _avs_rb_node_color(AVS_RBTREE_ELEM(void) elem) {
    if (!elem) {
        return BLACK;
    } else {
        /* checking the color of a detached node is pointless, so
         * this function should never be called on one */
        assert(_AVS_RB_NODE(elem)->color == RED
                || _AVS_RB_NODE(elem)->color == BLACK);
        return _AVS_RB_NODE(elem)->color;
    }
}

AVS_RBTREE(void) avs_rbtree_new__(avs_rbtree_element_comparator_t *cmp) {
    struct rb_tree *tree = (struct rb_tree*)_AVS_RB_ALLOC(sizeof(struct rb_tree));
    if (!tree) {
        return NULL;
    }

    tree->cmp = cmp;
    tree->root = NULL;

    return &tree->root;
}

void avs_rbtree_elem_delete__(AVS_RBTREE_ELEM(void) *node_ptr) {
    if (node_ptr && *node_ptr) {
        assert(rb_is_node_detached(*node_ptr));
        _AVS_RB_DEALLOC(_AVS_RB_NODE(*node_ptr));
        *node_ptr = NULL;
    }
}

void avs_rbtree_delete__(AVS_RBTREE(void) *tree_ptr) {
    struct rb_tree *tree;

    if (!tree_ptr || !*tree_ptr) {
        return;
    }

    assert(!**tree_ptr); /* should only be called on empty trees */
    tree = _AVS_RB_TREE(*tree_ptr);
    _AVS_RB_DEALLOC(tree);
    *tree_ptr = NULL;
}

static size_t rb_subtree_size(const AVS_RBTREE_ELEM(void) root) {
    if (!root) {
        return 0;
    }

    return (1 + rb_subtree_size(_AVS_RB_LEFT_CONST(root))
            + rb_subtree_size(_AVS_RB_RIGHT_CONST(root)));
}

size_t avs_rbtree_size__(AVS_RBTREE_CONST(void) tree) {
    assert(!rb_is_cleanup_in_progress(tree)
           && "avs_rbtree_size__ called while tree deletion in progress");

    return rb_subtree_size(_AVS_RB_TREE_CONST(tree)->root);
}

AVS_RBTREE_ELEM(void) avs_rbtree_elem_new_buffer__(size_t elem_size) {
    struct rb_node *node =
            (struct rb_node*)_AVS_RB_ALLOC(_AVS_NODE_SPACE__ + elem_size);
    if (!node) {
        return NULL;
    }

    node->color = DETACHED;

    return (char*)node + _AVS_NODE_SPACE__;
}

static AVS_RBTREE_ELEM(void) *
rb_find_ptr(struct rb_tree *tree,
            const void *val,
            AVS_RBTREE_ELEM(void) *out_parent_of_found) {
    AVS_RBTREE_ELEM(void) parent = NULL;
    AVS_RBTREE_ELEM(void) *curr = NULL;

    assert(tree);
    assert(val);

    curr = &tree->root;

    while (*curr) {
        int cmp = tree->cmp(val, *curr);

        if (cmp == 0) {
            break;
        } else {
            parent = *curr;
            curr = (cmp < 0) ? _AVS_RB_LEFT_PTR(*curr)
                             : _AVS_RB_RIGHT_PTR(*curr);
        }
    }

    if (out_parent_of_found) {
        *out_parent_of_found = parent;
    }
    return curr;
}

AVS_RBTREE_ELEM(void) avs_rbtree_find__(AVS_RBTREE_CONST(void) tree,
                                        const void *val) {
    AVS_RBTREE_ELEM(void) *elem_ptr;

    assert(!rb_is_cleanup_in_progress(tree)
           && "avs_rbtree_find__ called while tree deletion in progress");

    elem_ptr = rb_find_ptr(_AVS_RB_TREE((AVS_RBTREE(void))(intptr_t)tree),
                           val, NULL);
    return elem_ptr ? *elem_ptr : NULL;
}

static AVS_RBTREE_ELEM(void) rb_sibling(AVS_RBTREE_ELEM(void) elem,
                                        AVS_RBTREE_ELEM(void) parent) {
    AVS_RBTREE_ELEM(void) p_left = NULL;
    AVS_RBTREE_ELEM(void) p_right = NULL;

    assert(parent);

    p_left = _AVS_RB_LEFT(parent);
    p_right = _AVS_RB_RIGHT(parent);

    if (elem == p_left) {
        return p_right;
    } else {
        assert(elem == p_right);
        return p_left;
    }
}

/**
 * Returns a reference to parent's pointer to @p node.
 * If @p node is the root, returns a root reference from the @p tree.
 */
static AVS_RBTREE_ELEM(void) *rb_own_parent_ptr(struct rb_tree *tree,
                                                AVS_RBTREE_ELEM(void) node) {
    AVS_RBTREE_ELEM(void) parent = _AVS_RB_PARENT(node);
    if (!parent) {
        return &tree->root;
    }

    if (_AVS_RB_LEFT(parent) == node) {
        return _AVS_RB_LEFT_PTR(parent);
    } else {
        assert(_AVS_RB_RIGHT(parent) == node);
        return _AVS_RB_RIGHT_PTR(parent);
    }
}

/**
 *      (parent)                 (parent)
 *         |                        |
 *         |                        |
 *       (root)                  (pivot)
 *        /  \         -->        /   \
 *       /    \                  /     \
 *     (A)  (pivot)           (root)   (B)
 *           /   \             /  \
 *          /     \           /    \
 *  (grandchild)  (B)       (A)  (grandchild)
 */
static void rb_rotate_left(struct rb_tree *tree,
                           AVS_RBTREE_ELEM(void) root) {
    AVS_RBTREE_ELEM(void) parent = _AVS_RB_PARENT(root);
    AVS_RBTREE_ELEM(void) *own_parent_ptr = rb_own_parent_ptr(tree, root);
    AVS_RBTREE_ELEM(void) pivot = NULL;
    AVS_RBTREE_ELEM(void) grandchild = NULL;

    assert(own_parent_ptr);

    pivot = _AVS_RB_RIGHT(root);
    assert(pivot);

    *own_parent_ptr = pivot;
    _AVS_RB_PARENT(pivot) = parent;

    grandchild = _AVS_RB_LEFT(pivot);
    _AVS_RB_LEFT(pivot) = root;
    _AVS_RB_PARENT(root) = pivot;

    _AVS_RB_RIGHT(root) = grandchild;
    if (grandchild) {
        _AVS_RB_PARENT(grandchild) = root;
    }
}

/**
 *       (parent)                 (parent)
 *          |                        |
 *          |                        |
 *        (root)                  (pivot)
 *         /  \         -->        /   \
 *        /    \                  /     \
 *    (pivot)  (A)              (B)    (root)
 *     /   \                            /  \
 *    /     \                          /    \
 *  (B)  (grandchild)         (grandchild)  (A)
 */
static void rb_rotate_right(struct rb_tree *tree,
                            AVS_RBTREE_ELEM(void) root) {
    AVS_RBTREE_ELEM(void) parent = _AVS_RB_PARENT(root);
    AVS_RBTREE_ELEM(void) *own_parent_ptr = rb_own_parent_ptr(tree, root);
    AVS_RBTREE_ELEM(void) pivot = NULL;
    AVS_RBTREE_ELEM(void) grandchild = NULL;

    assert(own_parent_ptr);

    pivot = _AVS_RB_LEFT(root);
    assert(pivot);

    *own_parent_ptr = pivot;
    _AVS_RB_PARENT(pivot) = parent;

    grandchild = _AVS_RB_RIGHT(pivot);
    _AVS_RB_RIGHT(pivot) = root;
    _AVS_RB_PARENT(root) = pivot;

    _AVS_RB_LEFT(root) = grandchild;
    if (grandchild) {
        _AVS_RB_PARENT(grandchild) = root;
    }
}

static void rb_insert_fix(struct rb_tree *tree,
                          AVS_RBTREE_ELEM(void) elem) {
    AVS_RBTREE_ELEM(void) parent = NULL;
    AVS_RBTREE_ELEM(void) grandparent = NULL;
    AVS_RBTREE_ELEM(void) uncle = NULL;

    /* case 1 */
    if (elem == tree->root) {
        _AVS_RB_NODE(elem)->color = BLACK;
        return;
    }

    _AVS_RB_NODE(elem)->color = RED;

    /* case 2 */
    parent = _AVS_RB_PARENT(elem);
    assert(parent);
    if (_avs_rb_node_color(parent) == BLACK) {
        return;
    }

    /* case 3 */
    grandparent = _AVS_RB_PARENT(parent);
    uncle = rb_sibling(parent, grandparent);

    if (_avs_rb_node_color(uncle) == RED) {
        _AVS_RB_NODE(parent)->color = BLACK;
        _AVS_RB_NODE(uncle)->color = BLACK;
        _AVS_RB_NODE(grandparent)->color = RED;
        rb_insert_fix(tree, grandparent);
        return;
    }

    /* case 4 */
    if (elem == _AVS_RB_RIGHT(parent)
            && parent == _AVS_RB_LEFT(grandparent)) {
        rb_rotate_left(tree, parent);
        elem = _AVS_RB_LEFT(elem);
    } else if (elem == _AVS_RB_LEFT(parent)
                   && parent == _AVS_RB_RIGHT(grandparent)) {
        rb_rotate_right(tree, parent);
        elem = _AVS_RB_RIGHT(elem);
    }

    /* case 5 */
    parent = _AVS_RB_PARENT(elem);
    assert(grandparent == _AVS_RB_PARENT(parent));

    _AVS_RB_NODE(parent)->color = BLACK;
    _AVS_RB_NODE(grandparent)->color = RED;
    if (elem == _AVS_RB_LEFT(parent)) {
        rb_rotate_right(tree, grandparent);
    } else {
        rb_rotate_left(tree, grandparent);
    }
}

AVS_RBTREE_ELEM(void) avs_rbtree_attach__(AVS_RBTREE(void) tree_,
                                          AVS_RBTREE_ELEM(void) elem) {
    struct rb_tree *tree = _AVS_RB_TREE(tree_);
    AVS_RBTREE_ELEM(void) *dst = NULL;
    AVS_RBTREE_ELEM(void) parent = NULL;

    assert(!rb_is_cleanup_in_progress(rb_tree_const(tree_))
           && "avs_rbtree_attach__ called while tree deletion in progress");
    assert(tree_);
    assert(elem);
    assert(rb_is_node_detached(elem));

    dst = rb_find_ptr(tree, elem, &parent);
    assert(dst);

    if (*dst) {
        /* already present */
        return *dst;
    } else {
        *dst = elem;
        _AVS_RB_PARENT(elem) = parent;
    }

    rb_insert_fix(tree, elem);
    return elem;
}

static AVS_RBTREE_ELEM(void) rb_min(AVS_RBTREE_ELEM(void) root) {
    AVS_RBTREE_ELEM(void) min = root;
    AVS_RBTREE_ELEM(void) left = root;

    if (!root) {
        return NULL;
    }

    do {
        min = left;
        left = _AVS_RB_LEFT(min);
    } while (left);

    return min;
}

AVS_RBTREE_ELEM(void) avs_rbtree_first__(AVS_RBTREE(void) tree) {
    /* operation allowed while delete in progress to allow delete resumption */
    return rb_min(_AVS_RB_TREE(tree)->root);
}

static AVS_RBTREE_ELEM(void) rb_max(AVS_RBTREE_ELEM(void) root) {
    AVS_RBTREE_ELEM(void) max = root;
    AVS_RBTREE_ELEM(void) right = root;

    if (!root) {
        return NULL;
    }

    do {
        max = right;
        right = _AVS_RB_RIGHT(max);
    } while (right);

    return max;
}

AVS_RBTREE_ELEM(void) avs_rbtree_last__(AVS_RBTREE(void) tree) {
    assert(!rb_is_cleanup_in_progress(rb_tree_const(tree))
           && "avs_rbtree_last__ called while tree deletion in progress");
    return rb_max(_AVS_RB_TREE(tree)->root);
}

AVS_RBTREE_ELEM(void) avs_rbtree_elem_next__(AVS_RBTREE_ELEM(void) elem) {
    AVS_RBTREE_ELEM(void) right = _AVS_RB_RIGHT(elem);
    AVS_RBTREE_ELEM(void) parent = NULL;
    AVS_RBTREE_ELEM(void) curr = NULL;

    if (right) {
        return rb_min(right);
    }

    parent = _AVS_RB_PARENT(elem);
    curr = elem;
    while (parent && _AVS_RB_RIGHT(parent) == curr) {
        curr = parent;
        parent = _AVS_RB_PARENT(parent);
    }

    return parent;
}

AVS_RBTREE_ELEM(void) avs_rbtree_elem_prev__(AVS_RBTREE_ELEM(void) elem) {
    AVS_RBTREE_ELEM(void) left = _AVS_RB_LEFT(elem);
    AVS_RBTREE_ELEM(void) parent = NULL;
    AVS_RBTREE_ELEM(void) curr = NULL;

    if (left) {
        return rb_max(left);
    }

    parent = _AVS_RB_PARENT(elem);
    curr = elem;
    while (parent && _AVS_RB_LEFT(parent) == curr) {
        curr = parent;
        parent = _AVS_RB_PARENT(parent);
    }

    return parent;
}

static void swap(void **a,
                 void **b) {
    void *tmp;

    assert(a);
    assert(b);

    tmp = *a;
    *a = *b;
    *b = tmp;
}

/**
 * Swaps parent/left/right pointers and color. Retains value.
 */
static void rb_swap_nodes(struct rb_tree *tree,
                          AVS_RBTREE_ELEM(void) a,
                          AVS_RBTREE_ELEM(void) b) {
    AVS_RBTREE_ELEM(void) *a_parent_ptr = NULL;
    AVS_RBTREE_ELEM(void) *b_parent_ptr = NULL;
    enum rb_color col;

    assert(a);
    assert(b);

    if (a == b) {
        return;
    }

    a_parent_ptr = rb_own_parent_ptr(tree, a);
    b_parent_ptr = rb_own_parent_ptr(tree, b);

    /* simply swapping pointers in case where one node is a parent of
     * another would set parent pointer of the former parent to itself */
    if (_AVS_RB_PARENT(a) == b) {
        _AVS_RB_PARENT(a) = a;
    } else if (_AVS_RB_PARENT(b) == a) {
        _AVS_RB_PARENT(b) = b;
    }

    swap(a_parent_ptr, b_parent_ptr);
    swap(_AVS_RB_PARENT_PTR(a), _AVS_RB_PARENT_PTR(b));

    swap(_AVS_RB_LEFT_PTR(a), _AVS_RB_LEFT_PTR(b));
    if (_AVS_RB_LEFT(a)) {
        AVS_RBTREE_ELEM(void) left = _AVS_RB_LEFT(a);
        _AVS_RB_PARENT(left) = a;
    }
    if (_AVS_RB_LEFT(b)) {
        AVS_RBTREE_ELEM(void) left = _AVS_RB_LEFT(b);
        _AVS_RB_PARENT(left) = b;
    }

    swap(_AVS_RB_RIGHT_PTR(a), _AVS_RB_RIGHT_PTR(b));
    if (_AVS_RB_RIGHT(a)) {
        AVS_RBTREE_ELEM(void) right = _AVS_RB_RIGHT(a);
        _AVS_RB_PARENT(right) = a;
    }
    if (_AVS_RB_RIGHT(b)) {
        AVS_RBTREE_ELEM(void) right = _AVS_RB_RIGHT(b);
        _AVS_RB_PARENT(right) = b;
    }

    col = _avs_rb_node_color(a);
    _AVS_RB_NODE(a)->color = _avs_rb_node_color(b);
    _AVS_RB_NODE(b)->color = col;
}

static void rb_detach_fix(struct rb_tree *tree,
                          AVS_RBTREE_ELEM(void) elem,
                          AVS_RBTREE_ELEM(void) parent) {
    AVS_RBTREE_ELEM(void) sibling = NULL;

    assert(_avs_rb_node_color(elem) == BLACK);

    /* case 1 */
    if (!parent) {
        return;
    }

    /* case 2 */
    sibling = rb_sibling(elem, parent);
    if (_avs_rb_node_color(sibling) == RED) {
        _AVS_RB_NODE(parent)->color = RED;
        _AVS_RB_NODE(sibling)->color = BLACK;

        if (elem == _AVS_RB_LEFT(parent)) {
            rb_rotate_left(tree, parent);
        } else {
            rb_rotate_right(tree, parent);
        }
    }

    /* case 3 */
    sibling = rb_sibling(elem, parent);
    if (_avs_rb_node_color(parent) == BLACK
            && _avs_rb_node_color(sibling) == BLACK
            && _avs_rb_node_color(_AVS_RB_LEFT(sibling)) == BLACK
            && _avs_rb_node_color(_AVS_RB_RIGHT(sibling)) == BLACK) {
        _AVS_RB_NODE(sibling)->color = RED;
        rb_detach_fix(tree, parent, _AVS_RB_PARENT(parent));
        return;
    }

    /* case 4 */
    if (_avs_rb_node_color(parent) == RED
            && _avs_rb_node_color(sibling) == BLACK
            && _avs_rb_node_color(_AVS_RB_LEFT(sibling)) == BLACK
            && _avs_rb_node_color(_AVS_RB_RIGHT(sibling)) == BLACK) {
        _AVS_RB_NODE(sibling)->color = RED;
        _AVS_RB_NODE(parent)->color = BLACK;
        return;
    }

    /* case 5 */
    assert(sibling);
    assert(_avs_rb_node_color(sibling) == BLACK);
    if (elem == _AVS_RB_LEFT(parent)
            && _avs_rb_node_color(_AVS_RB_RIGHT(sibling)) == BLACK) {
        assert(_avs_rb_node_color(_AVS_RB_LEFT(sibling)) == RED);

        _AVS_RB_NODE(sibling)->color = RED;
        _AVS_RB_NODE(_AVS_RB_LEFT(sibling))->color = BLACK;
        rb_rotate_right(tree, sibling);
    } else if (elem == _AVS_RB_RIGHT(parent)
               && _avs_rb_node_color(_AVS_RB_LEFT(sibling)) == BLACK) {
        assert(_avs_rb_node_color(_AVS_RB_RIGHT(sibling)) == RED);

        _AVS_RB_NODE(sibling)->color = RED;
        _AVS_RB_NODE(_AVS_RB_RIGHT(sibling))->color = BLACK;
        rb_rotate_left(tree, sibling);
    }

    /* case 6 */
    sibling = rb_sibling(elem, parent);

    _AVS_RB_NODE(sibling)->color = _avs_rb_node_color(parent);
    _AVS_RB_NODE(parent)->color = BLACK;

    if (elem == _AVS_RB_LEFT(parent)) {
        _AVS_RB_NODE(_AVS_RB_RIGHT(sibling))->color = BLACK;
        rb_rotate_left(tree, parent);
    } else {
        _AVS_RB_NODE(_AVS_RB_LEFT(sibling))->color = BLACK;
        rb_rotate_right(tree, parent);
    }
}

AVS_RBTREE_ELEM(void) avs_rbtree_detach__(AVS_RBTREE(void) tree_,
                                          AVS_RBTREE_ELEM(void) elem) {
    struct rb_tree *tree = _AVS_RB_TREE(tree_);
    AVS_RBTREE_ELEM(void) left = NULL;
    AVS_RBTREE_ELEM(void) right = NULL;
    AVS_RBTREE_ELEM(void) child = NULL;
    AVS_RBTREE_ELEM(void) parent = NULL;

    if (!elem) {
        return NULL;
    }

    assert(!rb_is_node_detached(elem)
           && "cannot detach an node that's already detached");
    assert(!rb_is_cleanup_in_progress(rb_tree_const(tree_))
           && "avs_rbtree_detach__ called while tree deletion in progress");
    assert(tree_);
    assert(elem);

    left = _AVS_RB_LEFT(elem);
    right = _AVS_RB_RIGHT(elem);

    if (left && right) {
        AVS_RBTREE_ELEM(void) replacement = avs_rbtree_elem_next__(elem);
        rb_swap_nodes(tree, elem, replacement);
        return avs_rbtree_detach__(tree_, elem);
    }

    child = left ? left : right;
    parent = _AVS_RB_PARENT(elem);

    if (child) {
        assert(_AVS_RB_PARENT(child) == elem);
        _AVS_RB_PARENT(child) = parent;
    }

    *rb_own_parent_ptr(tree, elem) = child;
    _AVS_RB_PARENT(elem) = NULL;
    _AVS_RB_LEFT(elem) = NULL;
    _AVS_RB_RIGHT(elem) = NULL;

    assert(_avs_rb_node_color(elem) == BLACK
            || _avs_rb_node_color(child) == BLACK);
    if (_avs_rb_node_color(elem) == RED
            || _avs_rb_node_color(child) == RED) {
        if (child) {
            /* if elem is red, child is already black
             * if child is red, we need to repaint it */
            _AVS_RB_NODE(child)->color = BLACK;
        }

        _AVS_RB_NODE(elem)->color = DETACHED;
        return elem;
    }

    /* both node and child are black */
    rb_detach_fix(tree, child, parent);

    _AVS_RB_NODE(elem)->color = DETACHED;
    assert(_avs_rb_node_color(tree->root) == BLACK);
    return elem;
}

static AVS_RBTREE_ELEM(void) rb_postorder_first(AVS_RBTREE_ELEM(void) node) {
    while (node) {
        if (_AVS_RB_LEFT(node)) {
            node = _AVS_RB_LEFT(node);
        } else if (_AVS_RB_RIGHT(node)) {
            node = _AVS_RB_RIGHT(node);
        } else {
            return node;
        }
    }

    return NULL;
}

AVS_RBTREE_ELEM(void) avs_rbtree_cleanup_first__(AVS_RBTREE(void) tree) {
    return rb_postorder_first(*tree);
}

static AVS_RBTREE_ELEM(void) rb_postorder_next(AVS_RBTREE_ELEM(void) curr) {
    AVS_RBTREE_ELEM(void) parent = _AVS_RB_PARENT(curr);
    if (!parent) {
        return NULL;
    }

    if (_AVS_RB_RIGHT(parent) && _AVS_RB_RIGHT(parent) != curr) {
        return rb_postorder_first(_AVS_RB_RIGHT(parent));
    }

    return parent;
}

AVS_RBTREE_ELEM(void) avs_rbtree_cleanup_next__(AVS_RBTREE(void) tree) {
    AVS_RBTREE_ELEM(void) next;
    AVS_RBTREE_ELEM(void) *curr_ptr;

    next = rb_postorder_next(*tree);
    curr_ptr = rb_own_parent_ptr(_AVS_RB_TREE(tree), *tree);

    _AVS_RB_NODE(*tree)->color = DETACHED;
    _AVS_RB_PARENT(*tree) = NULL;
    /* at this point, child nodes should be cleaned up */
    assert(_AVS_RB_LEFT(*tree) == NULL);
    assert(_AVS_RB_RIGHT(*tree) == NULL);

    avs_rbtree_elem_delete__(curr_ptr);

    return *tree = next;
}

#ifdef AVS_UNIT_TESTING
#include "test/test_rbtree.c"
#endif
