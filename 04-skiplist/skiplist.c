/// 2-3-4 skiplist is an isometry of a 2-3-4 tree.
///
/// The layer i nodes consist all the nodes in the tree which height is less than or equal to H - i.

#import <stdlib.h>
#import <stdio.h>
#include <assert.h>

struct Skiplist {
    size_t height;
    size_t capacity;
    struct Skiplist **links; // links to the next node
    int value;
};

struct Skiplist *skiplist_new() {
  struct Skiplist *head = malloc(sizeof(struct Skiplist));
  head->height = 1;
  head->capacity = 1;
  head->value = 0;
  head->links = malloc(head->capacity * sizeof(struct Skiplist *));

  return head;
}

void skiplist_increase_height(struct Skiplist *list) {
  if (list->height == list->capacity) {
    list->capacity *= 2;
    list->links = realloc(list->links, list->capacity * sizeof(struct Skiplist *));
  }
  ++list->height;
}

void skiplist_free(struct Skiplist *list) {
  struct Skiplist *next = 0;
  // The layer 1 is assumed to have all the nodes
  while (list != 0) {
    next = list->links[0];
    free(list->links);
    free(list);
    list = next;
  }
}

/// Search the skiplist and return the node in each layer which is the last node which value is larger than val.
struct Skiplist **skiplist_locate(struct Skiplist *list, int val) {
  struct Skiplist **vec = malloc(sizeof(struct Skiplist *) * list->height);

  struct Skiplist *node = list;
  for (size_t i = list->height; i > 0; --i) {
    while (node->links[i - 1] != 0 && node->links[i - 1]->value < val) {
      node = node->links[i - 1];
    }
    vec[i - 1] = node;
  }

  return vec;
}

/// Search the skiplist
struct Skiplist *skiplist_search(struct Skiplist *list, int val) {
  struct Skiplist *node = list;
  for (size_t i = list->height; i > 0; --i) {
    while (node->links[i - 1] != 0 && node->links[i - 1]->value < val) {
      node = node->links[i - 1];
    }
    if (node->links[i - 1] != 0 && node->links[i - 1]->value == val) {
      return node->links[i - 1];
    }
  }

  return 0;
}

// Distance from start to end in the specific layer
size_t skiplist_distance(struct Skiplist *start, struct Skiplist *end, size_t layer) {
  size_t distance = 0;
  while (start != 0 && start != end) {
    start = start->links[layer];
    ++distance;
  }
  return distance;
}

void skiplist_try_upgrade(struct Skiplist *list, struct Skiplist **vec, size_t layer) {
  if (layer + 1 < list->height) {
    struct Skiplist *left = vec[layer + 1];
    struct Skiplist *right = left->links[layer + 1];
    if (skiplist_distance(left, right, layer) > 4) {
      // upgrade the second
      struct Skiplist *upgraded = left->links[layer]->links[layer];
      skiplist_increase_height(upgraded);
      upgraded->links[layer + 1] = right;
      left->links[layer + 1] = upgraded;
      skiplist_try_upgrade(list, vec, layer + 1);
    }
  } else if (skiplist_distance(list, 0, layer) > 4) {
    struct Skiplist *upgraded = list->links[layer]->links[layer];
    skiplist_increase_height(list);
    skiplist_increase_height(upgraded);
    upgraded->links[layer + 1] = 0;
    list->links[layer + 1] = upgraded;
  }
}

void skiplist_insert(struct Skiplist *list, int val) {
  struct Skiplist **vec = skiplist_locate(list, val);
  // Insert into layer 1
  struct Skiplist *node = skiplist_new();
  node->value = val;
  node->links[0] = vec[0]->links[0];
  vec[0]->links[0] = node;

  skiplist_try_upgrade(list, vec, 0);

  free(vec);
}

void skiplist_downgrade(struct Skiplist *list, struct Skiplist **vec, size_t layer) {
  if (layer + 1 < list->height) {
    struct Skiplist *left = vec[layer + 1];
    struct Skiplist *right = vec[layer + 1]->links[layer + 1];

    if (skiplist_distance(left, right, layer) == 1) {
      int downgraded = 0;
      
      if (right != 0 && right->height == layer + 2) {
        left->links[layer + 1] = right->links[layer + 1];
        --right->height;
        right = left->links[layer + 1];
        downgraded = 1;
      } else if (left->height == layer + 2) {
        // Need to find the prev node of left, the update vec element in the upper layer is a good start.
        struct Skiplist *prev = layer + 2 < list->height ? vec[layer + 2] : list;
        while (prev != 0 && prev->links[layer + 1] != left) {
          prev = prev->links[layer + 1];
        }
        if (prev != 0) {
          prev->links[layer + 1] = right;
          --left->height;
          left = prev;
          downgraded = 1;
        }
      }
      
      if (downgraded) {
        // Check reflow
        if (skiplist_distance(left, right, layer) == 4) {
          // upgrade the 2nd
          struct Skiplist *upgraded = left->links[layer]->links[layer];
          skiplist_increase_height(upgraded);
          upgraded->links[layer + 1] = left->links[layer + 1];
          left->links[layer + 1] = upgraded;
        } else {
          skiplist_downgrade(list, vec, layer + 1);
        }
      }
    }
  } else if (skiplist_distance(list, 0, layer) == 1) {
    // No more topmost layer nodes
    --list->height;
  }
}

void skiplist_remove(struct Skiplist *list, int val) {
  struct Skiplist **vec = skiplist_locate(list, val);

  if (vec[0]->links[0] != 0 && vec[0]->links[0]->value == val) {
    struct Skiplist *prev = vec[0];
    struct Skiplist *remove = prev->links[0];

    if (prev->links[0]->height == 1) {
      prev->links[0] = prev->links[0]->links[0];
      free(remove->links);
      free(remove);
    } else {
      struct Skiplist *next = remove->links[0];
      assert(next != 0 && next->height == 1);

      // swap with the next node
      for (size_t i = 0; i < remove->height; ++i) {
        assert(vec[i]->links[i] == remove);
        vec[i]->links[i] = next;
        vec[i] = next;
      }

      remove->links[0] = next->links[0];
      free(next->links);
      next->links = remove->links;
      next->height = remove->height;
      next->capacity = remove->capacity;

      free(remove);
    }

    // now node has been removed, re-balance the tree
    skiplist_downgrade(list, vec, 0);
  }

  free(vec);
}

void skiplist_debug(struct Skiplist *list) {
  printf("  o");
  for (size_t i = 0; i < list->height; ++i) {
    printf(" v");
    if (list->links[i] != 0) {
      assert(list->links[i]->height > i);
    }
    struct Skiplist *next = list->links[0];
    while (next != 0 && next->height <= i) {
      next = next->links[0];
    }
    assert(next == list->links[i]);
  }
  printf("\n");

  for (struct Skiplist *node = list->links[0]; node != 0; node = node->links[0]) {
    printf("%3d", node->value);
    for (size_t i = 0; i < node->height; ++i) {
      printf(" v");
      if (node->links[i] != 0) {
        assert(node->links[i]->height > i);
      }
      struct Skiplist *next = node->links[0];
      while (next != 0 && next->height <= i) {
        next = next->links[0];
      }
      assert(next == node->links[i]);
    }
    printf("\n");
  }

  printf("---");
  for (size_t i = 0; i < list->height; ++i) {
    printf(" -");
  }
  printf("\n");
}

int main() {
  struct Skiplist *list = skiplist_new();
  skiplist_debug(list);

  skiplist_insert(list, 15);
  skiplist_insert(list, 17);
  skiplist_insert(list, 19);
  skiplist_insert(list, 20);
  skiplist_insert(list, 9);
  skiplist_insert(list, 11);
  skiplist_insert(list, 13);
  skiplist_insert(list, 5);
  skiplist_insert(list, 1);
  skiplist_insert(list, 3);
  skiplist_insert(list, 7);
  skiplist_debug(list);

  skiplist_insert(list, 8);
  skiplist_debug(list);

  assert(skiplist_search(list, 1)->value == 1);
  assert(skiplist_search(list, 20)->value == 20);
  assert(skiplist_search(list, 8)->value == 8);
  assert(skiplist_search(list, 6) == 0);
  assert(skiplist_search(list, 10) == 0);
  assert(skiplist_search(list, 0) == 0);
  assert(skiplist_search(list, 21) == 0);

  skiplist_remove(list, 8);
  skiplist_remove(list, 7);
  skiplist_remove(list, 1);
  skiplist_remove(list, 9);
  skiplist_remove(list, 3);
  skiplist_remove(list, 5);
  skiplist_remove(list, 19);
  skiplist_remove(list, 20);
  skiplist_remove(list, 17);
  skiplist_remove(list, 15);
  skiplist_remove(list, 11);
  skiplist_remove(list, 13);

  skiplist_debug(list);

  skiplist_free(list);
}
