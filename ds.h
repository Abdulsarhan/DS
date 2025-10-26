#ifndef DS_H
#define DS_H
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	char* value;
	size_t len;
}string;

typedef struct {
	string string;
	size_t capacity;
}string_builder;

#define DSAPI extern

size_t str__strlen(const char *str) {
    const char *s = str;
    while (*str != '\0')
        str++;
    return str - s;
}

void str__strcpy(char *dst, const char *src) {
	if (!dst || !src)
		return;
	while ((*dst++ = *src++));
}

void *str__memcpy(void *dst, const void *src, size_t n) {
	unsigned char *d = dst;
	const unsigned char *s = src;

	while (n--)
		*d++ = *s++;

	return dst;
}

// str builder
DSAPI string_builder s_str_builder(const char* str, size_t initial_capacity) {
	string_builder str_builder = { 0 };
	string string = {0};
	string.len = str__strlen(str);
	string.value = (char*)malloc(string.len + 1);
	str__strcpy(string.value, str);
	str_builder.string = string;
	str_builder.capacity = initial_capacity;
	return str_builder;
}

DSAPI void s_str_builder_append(string_builder *builder, const char *str) {
	size_t str_len = str__len(str);
	size_t required = builder->string.len + str_len + 1; // +1 for '\0'

	if (required > builder->capacity) {
		size_t new_capacity = required;
		void *tmp = realloc(builder->string.value, new_capacity);
		if (!tmp) {
			printf("s_str_builder_append(): Failed to acquire memory. Did not append string.");
			return;
		}
		builder->string.value = tmp;
		builder->capacity = new_capacity;
	}

	// Append string
	str__memcpy(builder->string.value + builder->string.len, str, str_len + 1);
	builder->string.len += str_len;
}

DSAPI int s_print_string_builder(string_builder *builder) {
    return printf("%s", builder->string.value);
}

DSAPI void s_free_string_builder(string_builder *builder) {
    free(builder->string.value);
}

// str
DSAPI string s_str(const char* str) {
	string string = {0};
	string.len = str__strlen(str);
	string.value = (char*)malloc(string.len + 1);
	str__strcpy(string.value, str);
	return string;
}

DSAPI int s_print_string(string str) {
    return printf("%s", str.value);
}

DSAPI void s_free_string(string str) {
    free(str.value);
}
#endif // DS_H