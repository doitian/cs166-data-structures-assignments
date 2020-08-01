#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct fhs_t {
    int *arr;
    size_t size;
    size_t *summary;
    size_t block_size;
    size_t num_blocks;
};

size_t *fhs_build_summary(int arr[], size_t size, size_t block_size, size_t num_blocks) {
  size_t summary_spans = (size_t) ceil(log2(num_blocks));
  size_t *summary = (size_t *) malloc(sizeof(size_t) * num_blocks * summary_spans);

  size_t offset = 0;
  for (size_t i = 0; i < num_blocks; ++i) {
    int minimum = offset;
    for (size_t j = 1; j < block_size && offset + j < size; ++j) {
      if (arr[offset + j] < arr[minimum]) {
        minimum = offset + j;
      }
    }
    summary[i] = minimum;

    offset += block_size;
  }

  size_t half_span = 1;
  offset = num_blocks;
  for (size_t i = 1; i < summary_spans; ++i) {
    for (size_t j = 0; j + half_span + half_span <= num_blocks; ++j) {
      size_t current = offset + j;
      size_t first_half = current - num_blocks;
      size_t second_half = first_half + half_span;
      if (arr[summary[second_half]] < arr[summary[first_half]]) {
        summary[current] = summary[second_half];
      } else {
        summary[current] = summary[first_half];
      }
    }

    half_span *= 2;
    offset += num_blocks;
  }

  return summary;
}

struct fhs_t *fhs_preprocess(int arr[], size_t size) {
  struct fhs_t *fhs = malloc(sizeof(struct fhs_t));
  fhs->arr = arr;
  fhs->size = size;

  double log2_of_size = log2(size);
  size_t block_size = (size_t) ceil(log2_of_size / 4);
  if (block_size == 0) {
    block_size = 1;
  }

  fhs->block_size = block_size;
  fhs->num_blocks = (size + block_size - 1) / block_size;
  fhs->summary = fhs_build_summary(arr, size, block_size, fhs->num_blocks);

  return fhs;
}

void fhs_free(struct fhs_t *fhs) {
  free(fhs->summary);
  free(fhs);
}

size_t fhs_size(struct fhs_t *fhs) {
  return fhs->size;
}

size_t fhs_query_summary(struct fhs_t *fhs, size_t block_i, size_t block_j) {
  if (block_i >= block_j || block_j > ((fhs->size + fhs->block_size - 1) / fhs->block_size)) {
    return block_j * fhs->block_size;
  }

  if (block_i + 1 == block_j) {
    return fhs->summary[block_i];
  }

  if (block_i + 2 == block_j) {
    return fhs->summary[fhs->num_blocks + block_i];
  }

  size_t size_log2 = (size_t) ceil(log2(block_j - block_i));
  size_t half_span = 1;
  for (size_t i = 1; i < size_log2; ++i) {
    half_span *= 2;
  }
  if (block_j - block_i == half_span + half_span) {
    return fhs->summary[size_log2 * fhs->num_blocks + block_i];
  }
  size_t first_half = fhs->summary[(size_log2 - 1) * fhs->num_blocks + block_i];
  size_t second_half = fhs->summary[(size_log2 - 1) * fhs->num_blocks + block_j - half_span];
  if (fhs->arr[second_half] < fhs->arr[first_half]) {
    return second_half;
  }
  return first_half;
}

size_t fhs_query_block(struct fhs_t *fhs, size_t block, size_t i, size_t j) {
  size_t minimum = block * fhs->block_size + i;
  for (size_t offset = minimum + 1; offset < block * fhs->block_size + j; ++offset) {
    if (fhs->arr[offset] < fhs->arr[minimum]) {
      minimum = offset;
    }
  }

  return minimum;
}

// Finds the index of the minimum element in the range [i, j), returns value greater then or equal to j on error.
size_t fhs_query(struct fhs_t *fhs, size_t i, size_t j) {
  if (i >= j || j > fhs_size(fhs)) {
    return j;
  }

  size_t block_i = i / fhs->block_size;
  size_t block_j = (j + fhs->block_size - 1) / fhs->block_size;
  size_t full_block_i = block_i * fhs->block_size == i ? block_i : block_i + 1;
  size_t full_block_j = block_j * fhs->block_size == j ? block_j : block_j - 1;

  size_t minimum = i;
  if (block_i != full_block_i) {
    minimum = fhs_query_block(fhs, block_i, i % fhs->block_size, fhs->block_size);
  }

  if (full_block_i < full_block_j) {
    size_t summary_minimum = fhs_query_summary(fhs, full_block_i, full_block_j);
    if (fhs->arr[summary_minimum] < fhs->arr[minimum]) {
      minimum = summary_minimum;
    }
  }

  if (block_j != full_block_j && (block_i == full_block_i || block_j > block_i + 1)) {
    size_t last_block_minimum = fhs_query_block(fhs, block_j - 1, 0, j % fhs->block_size);
    if (fhs->arr[last_block_minimum] < fhs->arr[minimum]) {
      minimum = last_block_minimum;
    }
  }

  return minimum;
}

void fhs_assert(struct fhs_t *fhs, size_t i, size_t j, size_t expect) {
  size_t actual = fhs_query(fhs, i, j);
  if (actual == expect) {
    printf("Expect RMQ(%d, %d) = A[%d]: pass\n", i, j - 1, actual);
  } else {
    printf("Expect RMQ(%d, %d) = A[%d]: fail, got %d\n", i, j - 1, expect, actual);
    for (size_t k = i; k < j; ++k) {
      if (k == i) {
        printf("[ %d%s", fhs->arr[k], k == expect ? "*" : (k == actual ? "x" : " "));
      } else {
        printf(", %d%s", fhs->arr[k], k == expect ? "*" : (k == actual ? "x" : " "));
      }
    }
    printf(" ]\n");
  }
}

int main() {
  int arr[] = {31, 41, 59, 26, 53, 58, 97, 23, 93, 84, 33, 64, 62, 83, 27,
               31, 41, 59, 26, 53, 58, 97, 23, 93, 84, 33, 64, 62, 83, 27,
               31, 41, 59, 26, 53, 58, 97, 23, 93, 84, 33, 64, 62, 83, 27,
               31, 41, 59, 26, 53, 58, 97, 23, 93, 84, 33, 64, 62, 83, 27};

  size_t n = sizeof(arr) / sizeof(int);

  printf("===> half fhs\n");
  struct fhs_t *half_fhs = fhs_preprocess(arr, n / 2);

  fhs_assert(half_fhs, 0, 0, 0);
  fhs_assert(half_fhs, 1, 0, 0);
  fhs_assert(half_fhs, 0, 1, 0);
  fhs_assert(half_fhs, 1, 2, 1);
  fhs_assert(half_fhs, 2, 6, 3);
  fhs_assert(half_fhs, 1, 6, 3);
  fhs_assert(half_fhs, 2, 9, 7);
  fhs_assert(half_fhs, 1, 9, 7);

  fhs_assert(half_fhs, 0, n / 2 + 1, n / 2 + 1);

  fhs_free(half_fhs);

  printf("===> full fhs\n");
  struct fhs_t *fhs = fhs_preprocess(arr, n);

  fhs_assert(fhs, 40, 52, 48);

  fhs_assert(fhs, 0, 0, 0);
  fhs_assert(fhs, 1, 0, 0);
  fhs_assert(fhs, 0, 1, 0);
  fhs_assert(fhs, 1, 2, 1);
  fhs_assert(fhs, 2, 6, 3);
  fhs_assert(fhs, 1, 6, 3);
  fhs_assert(fhs, 2, 9, 7);
  fhs_assert(fhs, 1, 9, 7);

  fhs_assert(fhs, 0, n + 1, n + 1);

  printf("===> quick check\n");
  for (size_t k = 0; k < 32; ++k) {
    size_t i = ((size_t) rand()) % n;
    size_t j = i + ((size_t) rand()) % (n - i);
    size_t expect = i;
    for (size_t l = i + 1; l < j; ++l) {
      if (arr[l] < arr[expect]) {
        expect = l;
      }
    }
    fhs_assert(fhs, i, j, expect);
  }

  fhs_free(fhs);

  return 0;
}
