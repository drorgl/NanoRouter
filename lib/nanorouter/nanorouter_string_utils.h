#include <stddef.h> // For size_t

char* nr_trim_whitespace(char *str);
void nr_trim_string(char *str, size_t len);

typedef void (*nr_string_split_callback_t)(const char *token, size_t token_len, size_t token_index, void *user_data);
void nr_string_split(const char *str, size_t str_len, const char *delimiter, nr_string_split_callback_t callback, void *user_data);
