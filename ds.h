#ifndef DS_H
#define DS_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#if defined(ARR_REALLOC) && !defined(ARR_FREE) || !defined(ARR_REALLOC) && defined(ARR_FREE)
#error "You must define both ARR_REALLOC and ARR_FREE, or neither."
#endif
#if !defined(ARR_REALLOC) && !defined(ARR_FREE)
#include <stdlib.h>
#define ARR_REALLOC(c,p,s) realloc(p,s)
#define ARR_FREE(c,p)      free(p)
#endif

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

// string
DSAPI string s_make(const char* str) {
	string string = {0};
	string.len = str__strlen(str);
	string.value = (char*)malloc(string.len + 1);
	str__strcpy(string.value, str);
	return string;
}

DSAPI int s_print(string str) {
    return printf("%s", str.value);
}

DSAPI void s_free(string str) {
    free(str.value);
}

// string builder
DSAPI string_builder sb_make(const char* str, size_t initial_capacity) {
	string_builder str_builder = { 0 };
	string string = {0};
	string.len = str__strlen(str);
	string.value = (string.len > initial_capacity) ? (char*)malloc(string.len + 1) : (char*)malloc(initial_capacity);
	str__strcpy(string.value, str);
	str_builder.string = string;
	str_builder.capacity = initial_capacity;
	return str_builder;
}

DSAPI void sb_append(string_builder *builder, const char *str) {
	size_t str_len = str__strlen(str);
	size_t required = builder->string.len + str_len + 1; // +1 for '\0'

	if (required > builder->capacity) {
		size_t new_capacity = required;
		void *tmp = realloc(builder->string.value, new_capacity);
		if (!tmp) {
			printf("sb_append(): Failed to acquire memory. Did not append string.");
			return;
		}
		builder->string.value = tmp;
		builder->capacity = new_capacity;
	}

	// Append string
	str__memcpy(builder->string.value + builder->string.len, str, str_len + 1);
	builder->string.len += str_len;
}

DSAPI int sb_print(string_builder *builder) {
    return printf("%s", builder->string.value);
}

DSAPI void sb_free(string_builder *builder) {
    free(builder->string.value);
}

// dynamic arrays
typedef struct arr_header
{
  size_t      length;
  size_t      capacity;
} arr_header;

#define header(t)  ((arr_header *) (t) - 1)

#define arr_set_cap(a,n)   (arr_grow(a,0,n))
#define arr_set_len(a,n)   ((arr_cap(a) < (size_t) (n) ? arr_set_cap((a),(size_t)(n)),0 : 0), (a) ? header(a)->length = (size_t) (n) : 0)
#define arr_cap(a)        ((a) ? header(a)->capacity : 0)
#define arr_len(a)        ((a) ? (ptrdiff_t) header(a)->length : 0)
#define arr_lenu(a)       ((a) ?             header(a)->length : 0)
#define arr_push(a,v)      (arr_maybe_grow(a,1), (a)[header(a)->length++] = (v))
#define arr_pop(a)        (header(a)->length--, (a)[header(a)->length])
#define arr_expand_ptr(a,n)  (arr_maybe_grow(a,n), (n) ? (header(a)->length += (n), &(a)[header(a)->length-(n)]) : (a))
#define arr_expand_index(a,n)(arr_maybe_grow(a,n), (n) ? (header(a)->length += (n), header(a)->length-(n)) : arr_len(a))
#define arr_last(a)       ((a)[header(a)->length-1])
#define arr_free(a)       ((void) ((a) ? ARR_FREE(NULL,header(a)) : (void)0), (a)=NULL)
#define arr_del(a,i)      arr_deln(a,i,1)
#define arr_deln(a,i,n)   (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (header(a)->length-(n)-(i))), header(a)->length -= (n))
#define arr_del_swap(a,i)  ((a)[i] = arr_last(a), header(a)->length -= 1)
#define arr_ins_n(a,i,n)   ((void)arr_expand_index((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (header(a)->length-(n)-(i))))
#define arr_ins(a,i,v)    (arr_ins_n((a),(i),1), (a)[i]=(v))

#define arr_maybe_grow(a,n)  ((!(a) || header(a)->length + (n) > header(a)->capacity) ? (arr_grow(a,n,0),0) : 0)

#define arr_grow(a,b,c)   ((a) = arrgrowf_wrapper((a), sizeof *(a), (b), (c)))

void *arr_grow_f(void *a, size_t elemsize, size_t addlen, size_t min_cap) {
  arr_header temp={0}; // force debugging
  void *b;
  size_t min_len = arr_len(a) + addlen;
  (void) sizeof(temp);

  // compute the minimum capacity needed
  if (min_len > min_cap)
    min_cap = min_len;

  if (min_cap <= arr_cap(a))
    return a;

  // increase needed capacity to guarantee O(1) amortized
  if (min_cap < 2 * arr_cap(a))
    min_cap = 2 * arr_cap(a);
  else if (min_cap < 4)
    min_cap = 4;

  b = ARR_REALLOC(NULL, (a) ? header(a) : 0, elemsize * min_cap + sizeof(arr_header));
  //if (num_prev < 65536) prev_allocs[num_prev++] = (int *) (char *) b;
  b = (char *) b + sizeof(arr_header);
  if (a == NULL) {
      header(b)->length = 0;
  }
  header(b)->capacity = min_cap;

  return b;
}
#endif // DS_H