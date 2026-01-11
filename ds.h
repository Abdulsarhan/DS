#ifndef DS_H
#define DS_H

#include <stddef.h>

#define DSAPI extern


typedef struct {
	char* value;
	size_t len;
}string;

typedef struct {
	string string;
	size_t capacity;
}string_builder;

#ifdef __cplusplus
extern "C" {
#endif // extern "C" {

DSAPI string str_make(const char* str);

/* this function slices from left if a positive from is provided, and from the right if a negative from is provided */
DSAPI string str_from(string str, size_t from);
DSAPI string str_from_to(string str, size_t from, size_t to);
DSAPI string str_copy(char *dst, char *str);
DSAPI string str_format(char *buffer, const char *format, ...);
DSAPI int str_print(string str);

DSAPI string_builder sb_make(const char* str, size_t initial_capacity);
DSAPI string sb_append(string_builder *builder, const char *str);
DSAPI void sb_reset(string_builder *builder);
DSAPI int sb_print(string_builder *builder);
DSAPI void sb_free(string_builder *builder);

#ifdef __cplusplus
}
#endif

typedef struct arr_header {
  size_t length;
  size_t capacity;
} arr_header;

#define header(t)  ((arr_header *) (t) - 1)

#define arr_set_cap(a,n)          (arr_grow(a,0,n))
#define arr_set_len(a,n)          ((arr_cap(a) < (size_t) (n) ? arr_set_cap((a),(size_t)(n)),0 : 0), (a) ? header(a)->length = (size_t) (n) : 0)
#define arr_cap(a)                ((a) ? header(a)->capacity : 0)
#define arr_len(a)                ((a) ? (ptrdiff_t) header(a)->length : 0)
#define arr_lenu(a)               ((a) ?             header(a)->length : 0)
#define arr_push(a,v)             (arr_maybe_grow(a,1), (a)[header(a)->length++] = (v))
#define arr_pop(a)                (header(a)->length--, (a)[header(a)->length])
#define arr_expand_ptr(a,n)       (arr_maybe_grow(a,n), (n) ? (header(a)->length += (n), &(a)[header(a)->length-(n)]) : (a))
#define arr_expand_index(a,n)     (arr_maybe_grow(a,n), (n) ? (header(a)->length += (n), header(a)->length-(n)) : arr_len(a))
#define arr_last(a)               ((a)[header(a)->length-1])
#define arr_free(a)               ((void) ((a) ? ARR_FREE(NULL,header(a)) : (void)0), (a)=NULL)
#define arr_del(a,i)              arr_deln(a,i,1)
#define arr_deln(a,i,n)           (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (header(a)->length-(n)-(i))), header(a)->length -= (n))
#define arr_del_swap(a,i)         ((a)[i] = arr_last(a), header(a)->length -= 1)
#define arr_ins_n(a,i,n)          ((void)arr_expand_index((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (header(a)->length-(n)-(i))))
#define arr_ins(a,i,v)            (arr_ins_n((a),(i),1), (a)[i]=(v))

#define arr_maybe_grow(a,n)  ((!(a) || header(a)->length + (n) > header(a)->capacity) ? (arr_grow(a,n,0),0) : 0)

#define arr_grow(a,b,c)   ((a) = arrgrowf_wrapper((a), sizeof *(a), (b), (c)))

#endif // DS_H

#ifdef DS_IMPLEMENTATION
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if defined(ARR_REALLOC) && !defined(ARR_FREE) || !defined(ARR_REALLOC) && defined(ARR_FREE)
#error "You must define both ARR_REALLOC and ARR_FREE, or neither."
#endif
#if !defined(ARR_REALLOC) && !defined(ARR_FREE)
#include <stdlib.h>
#define ARR_REALLOC(c,p,s) realloc(p,s)
#define ARR_FREE(c,p)      free(p)
#endif

/* string */
DSAPI string str_make(const char* str) {
    string s;
    s.value = (char*)str;
    s.len = strlen(str);
    return s;
}

DSAPI string str_from(string str, size_t from) {
    if (from == 0) return str;

    if (from < 0) from += (int)str.len;

    if (from < 0) from = 0;
    if ((size_t)from > str.len) from = (int)str.len;

    str.value += from;
    str.len -= from;

    return str;
}

DSAPI string str_from_to(string str, size_t from, size_t to) {
    if (from < 0) from += (int)str.len;
    if (to   < 0) to   += (int)str.len;

    if (from < 0) from = 0;
    if (to   < 0) to   = 0;
    if ((size_t)from > str.len) from = (int)str.len;
    if ((size_t)to   > str.len) to   = (int)str.len;

    string result = {0};

    if (from >= to) {
        result.value = str.value + str.len;
        result.len = 0;
    } else {
        result.value = str.value + from;
        result.len = to - from;
    }

    return result;
}

DSAPI string str_copy(char *dst, char *str) {
    strcpy(dst, str);
    string string;
    string.value = str;
    string.len = strlen(str);
    return string;
}

char eat_char(char **at) {
    if (**at != '\0') {
        char c = **at;
        (*at)++;
        return c;
    }
    return '\0';
}

DSAPI string str_format(char *buffer, const char *format, ...) {
    va_list ap;
    char *at = (char*)format;
    char c;

    char *out = buffer;   // keep pointer to start

    va_start(ap, format);

    while ((c = eat_char(&at)) != '\0') {
        if (c == '%') {
            char spec = eat_char(&at);
            switch (spec) {

            case 's': {
                char *s = va_arg(ap, char *);
                strcpy(buffer, s);
                buffer += strlen(s);
            } break;

            case 'd': {
                int d = va_arg(ap, int);
                int written = sprintf(buffer, "%d", d);
                buffer += written;
            } break;

            case 'z': {
                char next = eat_char(&at);  // expect 'u'
                if (next == 'u') {
                    size_t zu = va_arg(ap, size_t);
                    int written = sprintf(buffer, "%zu", zu);
                    buffer += written;
                }
            } break;

            default:
                *buffer++ = spec;
                break;
            }
        } else {
            *buffer++ = c;
        }
    }

    *buffer = '\0';
    va_end(ap);

    string result;
    result.value = out;
    result.len = buffer - out;
    return result;
}

DSAPI int str_print(string str) {
    return printf("%.*s", (int)str.len, str.value);
}

/* string builder */
DSAPI string_builder sb_make(const char* str, size_t initial_capacity) {
	string_builder str_builder = { 0 };
	string string = {0};
	string.len = strlen(str);
    if(string.len > initial_capacity) {
        string.value = (char*)malloc(string.len + 1);
    } else {
        string.value = (char*)malloc(initial_capacity);
    }
	strcpy(string.value, str);
	str_builder.string = string;
	str_builder.capacity = initial_capacity;
	return str_builder;
}

DSAPI string sb_append(string_builder *builder, const char *str) {
	size_t str_len = strlen(str);
	size_t required = builder->string.len + str_len + 1; // +1 for '\0'

	if (required > builder->capacity) {
		size_t new_capacity = required;
		char *tmp = (char*)realloc(builder->string.value, new_capacity);
		if (!tmp) {
			printf("sb_append(): Failed to acquire memory. Did not append string.");
			return builder->string;
		}
		builder->string.value = tmp;
		builder->capacity = new_capacity;
	}

	/* Append string */
	memcpy(builder->string.value + builder->string.len, str, str_len + 1);
	builder->string.len += str_len;
    return builder->string;
}

DSAPI void sb_reset(string_builder *builder) {
    builder->string.value = '\0';
    builder->string.len = 0;
}

DSAPI int sb_print(string_builder *builder) {
    return printf("%s", builder->string.value);
}

DSAPI void sb_free(string_builder *builder) {
    free(builder->string.value);
}

/* dynamic arrays */
void *arr_grow_f(void *a, size_t elemsize, size_t addlen, size_t min_cap) {
  arr_header temp={0}; // force debugging
  void *b;
  size_t min_len = arr_len(a) + addlen;
  (void) sizeof(temp);

  /* compute the minimum capacity needed */
  if (min_len > min_cap) {
    min_cap = min_len;
  }

  if (min_cap <= arr_cap(a)) {
    return a;
  }

  /* increase needed capacity to guarantee O(1) amortized */
  if (min_cap < 2 * arr_cap(a)) {
    min_cap = 2 * arr_cap(a);
  } else if (min_cap < 4) {
    min_cap = 4;
  }

  b = ARR_REALLOC(NULL, (a) ? header(a) : 0, elemsize * min_cap + sizeof(arr_header));
  /* if (num_prev < 65536) prev_allocs[num_prev++] = (int *) (char *) b; */
  b = (char *) b + sizeof(arr_header);
  if (a == NULL) {
      header(b)->length = 0;
  }
  header(b)->capacity = min_cap;

  return b;
}
#endif /* DS_IMPLEMENTATION */
