#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

void print_sa(const size_t *sa, size_t len) {
  for (size_t i = 0; i < len + 1; ++i) {
    printf("%s %lu", i == 0 ? "[" : ",", sa[i]);
  }
  printf(" ]\n");
}

/// Searches pattern in text with assistant of suffix array.
///
/// \param pattern NULL-terminated string to search in the text.
/// \param text NULL-terminated string to search the all occurrences of the pattern.
/// \param sa Suffix array for text
/// \param positions if this is not NULL, it will point to an element in sa, such that starting from poistions, all the consecutive suffixes are starting with the pattern.
/// \return number of occurrences of pattern in text.
size_t sais_search_for(const char *pattern, const char *text, const size_t *sa, const size_t **positions) {
  if (positions != 0 && *positions != 0) {
    free(*positions);
    *positions = 0;
  }

  size_t pattern_len = strlen(pattern);
  size_t sa_len = sa[0] + 1;
  size_t lower = 0;
  size_t upper = sa_len;

  while (lower < upper) {
    size_t middle = (lower + upper) / 2;
    int cmp = strncmp(pattern, text + sa[middle], pattern_len);
    if (cmp < 0) {
      upper = middle;
    } else if (cmp > 0) {
      lower = middle + 1;
    } else {
      lower = middle;
      upper = middle + 1;
      // found one, look around
      while (lower > 0 && strncmp(pattern, text + sa[lower - 1], pattern_len) == 0) {
        --lower;
      }
      while (upper < sa_len && strncmp(pattern, text + sa[upper], pattern_len) == 0) {
        ++upper;
      }
      if (positions != 0) {
        *positions = sa + lower;
      }
      return upper - lower;
    }
  }

  return 0;
}

void test_search_for() {
  const char *text = "ABANANABANDANA";
  size_t sa[] = {14, 13, 0, 6, 11, 4, 2, 8, 1, 7, 10, 12, 5, 3, 9};

  size_t *positions = 0;
  size_t occurrences = sais_search_for("ANA", text, sa, &positions);

  assert(occurrences == 3);
  for (size_t i = 0; i < occurrences; ++i) {
    assert(strncmp("ANA", text + positions[i], 3) == 0);
  }
}

size_t *build_initial_buckets(const char *text, size_t len) {
  size_t *buckets = calloc(256, sizeof(size_t));
  buckets[0] = 1;
  for (size_t i = 0; i < len; ++i) {
    ++buckets[(size_t) text[i]];
  }
  for (size_t i = 1; i < 256; ++i) {
    buckets[i] += buckets[i - 1];
  }

  return buckets;
}

// It is assumed that lms's are already filled in sa.
void induced_sort(const char *text, size_t len, size_t *sa, const char *types, const size_t *initial_buckets) {
  size_t *filled = calloc(256, sizeof(size_t));

  // Fill L preceding the existing suffixes in sa
  for (size_t i = 0; i < len + 1; ++i) {
    if (sa[i] > 0 && types[sa[i] - 1] == 'l') {
      size_t l = sa[i] - 1;
      size_t initial = text[l];
      size_t pos = initial_buckets[initial - 1] + filled[initial];
      ++filled[initial];

      sa[pos] = l;
    }
  }

  // Reset lms at the end of the sa
  for (size_t i = 1; i < 256; ++i) {
    for (size_t j = initial_buckets[i - 1] + filled[i]; j < initial_buckets[i]; ++j) {
      sa[j] = 0;
    }
  }

  bzero(filled, 256 * sizeof(size_t));

  // Fill S preceding the existing suffixes in sa, in reverse order
  for (size_t i = len + 1; i > 0; --i) {
    if (sa[i - 1] > 0 && types[sa[i - 1] - 1] != 'l') {
      size_t s = sa[i - 1] - 1;
      size_t initial = text[s];
      size_t pos = initial_buckets[initial] - 1 - filled[initial];
      ++filled[initial];

      sa[pos] = s;
    }
  }

  free(filled);
}

/// Builds the suffix array using SA-IS.
size_t *sais_build(const char *text) {
  size_t len = strlen(text);
  size_t *sa = calloc(len + 1, sizeof(size_t));

  // Step One: Annotate each character and find the LMS characters.
  char *types = calloc(len + 1, sizeof(char));
  size_t num_lms = 1;
  types[len] = 'S';

  for (size_t i = len; i > 0; --i) {
    if (text[i - 1] < text[i]) {
      types[i - 1] = 's';
    } else if (text[i - 1] > text[i]) {
      types[i - 1] = 'l';
    } else {
      types[i - 1] = types[i];
    }
    if (types[i - 1] == 'l' && types[i] == 's') {
      types[i] = 'S';
      ++num_lms;
    }
  }

  // Step Two: Implement Induced Sorting
  size_t *initial_buckets = build_initial_buckets(text, len);
  size_t *end_filled = calloc(256, sizeof(size_t));
  for (size_t i = len + 1; i > 0; --i) {
    if (types[i - 1] == 'S') {
      size_t initial = text[i - 1];
      size_t pos = initial_buckets[initial] - 1 - end_filled[initial];
      ++end_filled[initial];

      sa[pos] = i - 1;
    }
  }
  induced_sort(text, len, sa, types, initial_buckets);

  size_t block_pos = 0;
  size_t *lms_to_block_pos = calloc(len + 1, sizeof(size_t));
  size_t *block_pos_to_lms = calloc(num_lms, sizeof(size_t));
  for (size_t i = 0; i < len + 1; ++i) {
    if (types[i] == 'S') {
      block_pos_to_lms[block_pos] = i;
      lms_to_block_pos[i] = block_pos;
      ++block_pos;
    }
  }

  char *blocks = calloc(num_lms, sizeof(char));
  blocks[num_lms - 1] = 0;

  size_t block_char = '0';
  size_t has_duplidate = 0;
  size_t prev_len = 0;
  size_t prev_pos = len;
  for (size_t i = 1; i < len + 1; ++i) {
    if (types[sa[i]] == 'S') {
      block_pos = lms_to_block_pos[sa[i]];
      assert(block_pos < num_lms);
      size_t len = block_pos_to_lms[block_pos + 1] - sa[i] + 1;
      if (len == prev_len && strncmp(text + prev_pos, text + sa[i], len) == 0) {
        // Duplicate
        has_duplidate = 1;
      } else {
        ++block_char;
        assert(block_char < 256);
      }

      blocks[block_pos] = block_char;

      prev_len = len;
      prev_pos = sa[i];
    }
  }

  size_t *blocks_sa;
  if (num_lms > 2) {
    // TODO: all unique
    blocks_sa = sais_build(blocks);
  } else {
    blocks_sa = calloc(num_lms, sizeof(size_t));
    blocks_sa[0] = num_lms - 1;
  }

  // fill lms in reversed order
  bzero(end_filled, 256 * sizeof(size_t));
  bzero(sa, (len + 1) * sizeof(size_t));
  for (size_t i = num_lms; i > 0; --i) {
    size_t lms = block_pos_to_lms[blocks_sa[i - 1]];
    size_t initial = text[lms];
    size_t pos = initial_buckets[initial] - 1 - end_filled[initial];
    ++end_filled[initial];

    sa[pos] = lms;
  }
  induced_sort(text, len, sa, types, initial_buckets);

  free(block_pos_to_lms);
  free(lms_to_block_pos);
  free(end_filled);
  free(blocks_sa);
  free(blocks);

  free(initial_buckets);
  free(types);

  return sa;
}

int main() {
  test_search_for();
  const char *text = "ACGTGCCTAGCCTACCGTGCC";
  size_t *sa = sais_build(text);
  print_sa(sa, strlen(text));
  size_t expect_sa[] = {21, 13, 0, 8, 20, 19, 14, 10, 5, 15, 1, 11, 6, 18, 9, 4, 16, 2, 12, 7, 17, 3};
  for (size_t i = 0; i < sizeof(expect_sa) / sizeof(size_t); ++i) {
    assert(expect_sa[i] == sa[i]);
  }
  free(sa);

  for (size_t pass = 0; pass < 32; ++pass) {
    size_t len = rand() % 100;
    char* text = malloc(len + 1);
    text[len] = 0;
    for (size_t i = 0; i < len; ++i) {
      text[i] = 'A' + (rand() % 26);
    }
    
    char* seen = calloc(len + 1, sizeof(char));
    size_t *sa = sais_build(text);
    for (size_t i = 0; i < len + 1; ++i) {
      seen[sa[i]] = 1;
    }
    for (size_t i = 0; i < len + 1; ++i) {
      assert(seen[i]);
    }
    
    for (size_t i = 0; i < len; ++i) {
      assert(strcmp(text + sa[i], text + sa[i + 1]) < 0);
    }
    
    free(sa);
    free(text);
  }
}
