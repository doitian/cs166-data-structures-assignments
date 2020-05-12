#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

typedef struct Node {
  int value;
  struct Node* left;
  struct Node* right;
};

void insert_into(struct Node** root, int value) {
  struct Node** insert_at = root;
  while (*insert_at != NULL) {
    int pivotal_value = (*insert_at)->value;
    if (value < pivotal_value) {
      insert_at = &(*insert_at)->left;
    } else {
      insert_at = &(*insert_at)->right;
    }
  }

  // Found the insert point, create the node
  *insert_at = (struct Node*)malloc(sizeof(struct Node));

  (*insert_at)->value = value;
  (*insert_at)->left = (*insert_at)->right = NULL;
}

void free_tree(struct Node* root) {
  if (root->left != NULL) {
    free_tree(root->left);
  }
  if (root->right != NULL) {
    free_tree(root->right);
  }
  printf("free %d\n", root->value);
  free(root);
}

size_t size_of(const struct Node* root) {
  if (root != NULL) {
    return size_of(root->left) + size_of(root->right) + 2;
  }
  return 1;
}

int* write_contents_into(const struct Node* root, int* remaining_contents) {
  if (root != NULL) {
    remaining_contents = write_contents_into(root->left, remaining_contents);
    *remaining_contents = root->value;
    return write_contents_into(root->right, remaining_contents + 2);
  }

  return remaining_contents;
}

int* contents_of(const struct Node* root) {
  size_t size = size_of(root);
  int* contents = malloc(sizeof(int) * size);

  write_contents_into(root, contents);
  return contents;
}

void print_contents(int* contents, size_t size) {
  for (size_t i = 1; i < size; ++i) {
    if (i == 1) {
      printf("[ %d", contents[i]);
    } else {
      printf(", %d", contents[i]);
    }
  }
  printf(" ]\n");
}

const struct Node* second_min_in(const struct Node* root) {
  if (root != NULL) {
    if (root->left != NULL) {
      if (root->left->left != NULL || root->left->right != NULL) {
        // if left tree has at least two elements, then the second min of left tree is the second min of the whole tree.
        return second_min_in(root->left);
      }
      // if the left tree only has one element, then the root must be the second min.
      return root;
    }
    if (root->right != NULL) {
      // left tree is empty, the second min must be the minimal of the right tree
      struct Node* right_min = root->right;
      while (right_min->left != NULL) {
        right_min = right_min->left;
      }
      return right_min;
    }
  }

  return NULL;
}

int main() {
  struct Node* root = NULL;
  insert_into(&root, 6);
  insert_into(&root, 7);
  insert_into(&root, 4);
  insert_into(&root, 6);
  insert_into(&root, 3);
  insert_into(&root, 2);

  int* contents = contents_of(root);
  print_contents(contents, size_of(root));

  struct Node* second_min = second_min_in(root);
  printf("second min is %d\n", second_min->value);

  free_tree(root);
  free(contents);
}