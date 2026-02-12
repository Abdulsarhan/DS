#ifndef DS_H
#define DS_H

#include <stddef.h>

#if defined(DS_STATIC)
    #define DSAPI static
#elif defined(_WIN32) || defined(_WIN64)
    #if defined(DS_BUILD_DLL)
        #define DSAPI __declspec(dllexport)
    #elif defined(DS_USE_DLL)
        #define DSAPI __declspec(dllimport)
    #else
        #define DSAPI extern
    #endif
#else
    #define DSAPI extern
#endif // DSAPI

#ifdef DEBUG /* NDEBUG means "not debug", so by doing ifndef, we want these things to be defined only in debug mode */

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED printf("Unimplemented function on line: %d in file: %s" __LINE__, __FILE__);
#endif /* UNIMPLEMENTED */

#ifndef UNREACHABLE
#define UNREACHABLE() printf("Error: Reached unreachable code on line: %d in file: %s", __LINE__, __FILE__);
#endif /* UNREACHABLE */

#ifndef TODO
#define TODO(...) \
    do { \
        printf("[TODO] "); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while(0)
#define TODO_INTERNAL(...) TODO(__VA_ARGS__)
#else
#define TODO(...) ((void)0)
#define TODO_INTERNAL(...)((void)0)
#endif

#ifndef STATIC_ASSERT
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define STATIC_ASSERT(cond) \
    typedef char CONCAT(static_assert_line_, __LINE__)[(cond) ? 1 : -1]
#endif /* STATIC_ASSERT */

/* Compiler-specific debug break */
#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
    #define DEBUG_BREAK() __builtin_debugtrap()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    #define DEBUG_BREAK() __asm__ volatile("int $3")
#elif defined(__GNUC__) && defined(__arm__)
    #define DEBUG_BREAK() __asm__ volatile("bkpt #0")
#elif defined(__GNUC__) && defined(__aarch64__)
    #define DEBUG_BREAK() __asm__ volatile("brk #0")
#else
    #define DEBUG_BREAK() abort()
#endif

/* Runtime assert */
    #define ASSERT(cond) \
        ((cond) ? (void)0 : (fprintf(stderr, "Assertion failed: %s\nFile: %s, Line: %d\n", \
                                      #cond, __FILE__, __LINE__), DEBUG_BREAK()))
#endif /* NDEBUG */

#define defer(start, end) for (int _i = (start, 1); _i; _i = 0, end)

#define KiB(n) ((size_t)(n) << 10)
#define MiB(n) ((size_t)(n) << 20)
#define GiB(n) ((size_t)(n) << 30)

#define ARENA_PUSH_STRUCT(arena, T) (T*)arena_push(arena, sizeof(T), 0)
#define ARENA_PUSH_ARRAY(arena, T, n) (T*)arena_push(arena, sizeof(T) * n, 0)
#define ARENA_PUSH_STRUCT_ZERO(arena, T) (T*)arena_push(arena, sizeof(T), 1)
#define ARENA_PUSH_ARRAY_ZERO(arena, T, n) (T*)arena_push(arena, sizeof(T) * n, 1)

typedef struct {
    size_t pos;
    size_t committed_size;
    size_t page_size;
    size_t reserved_size;
} mem_arena;

typedef struct {
    mem_arena *arena;
    size_t start_pos;
}arena_temp;

typedef int arena_bool;

typedef struct da_header {
  size_t length;
  size_t capacity;
} da_header;

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

DSAPI mem_arena *arena_init(size_t size);
DSAPI void *arena_push(mem_arena *arena, size_t size, arena_bool zero_out_the_memory);
DSAPI void arena_pop(mem_arena *arena, size_t size);
DSAPI void arena_pop_to(mem_arena *arena, size_t pos);
DSAPI void arena_clear(mem_arena *arena);
DSAPI void arena_destroy(mem_arena *arena);
DSAPI int arena_reset_region(const mem_arena *arena, void *region_start, size_t region_size);

DSAPI string str_make(const char* str);

/* this function slices from left if a positive from is provided, and from the right if a negative from is provided */
DSAPI string str_from(string str, size_t from);
DSAPI string str_from_to(string str, size_t from, size_t to);
DSAPI string str_copy(char *dst, char *str);
DSAPI string str_format(char *buffer, const char *format, ...);
DSAPI int str_print(string str);

DSAPI string ds_to_stri(mem_arena *arena, int value);
DSAPI char *ds_to_cstri(mem_arena *arena, int value);
DSAPI int ds_itoa(int value, char *string);

DSAPI string_builder sb_make(mem_arena *arena, const char* str, size_t max_capacity);
DSAPI string sb_append(string_builder *builder, const char *str);
DSAPI void sb_reset(string_builder *builder);
DSAPI int sb_print(string_builder *builder);
DSAPI void sb_free(string_builder *builder);

#define header(t)  ((da_header *) (t) - 1)

#define da_reserve(a,n)          (da_grow(a,0,n)) /* reserve n bytes for the entire array. You can do this to avoid calling realloc() multiple times */
#define da_set_len(a,n)          ((da_cap(a) < (size_t) (n) ? da_reserve((a),(size_t)(n)),0 : 0), (a) ? header(a)->length = (size_t) (n) : 0)
#define da_cap(a)                ((a) ? header(a)->capacity : 0)
#define da_len(a)                ((a) ? (ptrdiff_t) header(a)->length : 0)
#define da_lenu(a)               ((a) ?             header(a)->length : 0)
#define da_push(a,v)             (da_maybe_grow(a,1), (a)[header(a)->length++] = (v))
#define da_push_many(a,v,n) \
    do { \
        da_maybe_grow(a,n); \
        for(int i = 0; i < (n); i++) { \
            (a)[header(a)->length++] = (v)[i]; \
        } \
    } while(0)
#define da_pop(a)                (header(a)->length--, (a)[header(a)->length])
#define da_expand_ptr(a,n)       (da_maybe_grow(a,n), (n) ? (header(a)->length += (n), &(a)[header(a)->length-(n)]) : (a))
#define da_expand_index(a,n)     (da_maybe_grow(a,n), (n) ? (header(a)->length += (n), header(a)->length-(n)) : da_len(a))
#define da_last(a)               ((a)[header(a)->length-1])
#define da_free(a)               ((void) ((a) ? ds_free(NULL,header(a)) : (void)0), (a)=NULL)
#define da_delete(a,i)           da_delete_many(a,i,1)
#define da_delete_many(a,i,n)    (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (header(a)->length-(n)-(i))), header(a)->length -= (n))
#define da_swapback(a,i)         ((a)[i] = da_last(a), header(a)->length -= 1)
#define da_ins_n(a,i,n)          ((void)da_expand_index((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (header(a)->length-(n)-(i))))
#define da_ins(a,i,v)            (da_ins_n((a),(i),1), (a)[i]=(v))

#define da_maybe_grow(a,n)  ((!(a) || header(a)->length + (n) > header(a)->capacity) ? (da_grow(a,n,0),0) : 0)

#define da_grow(a,b,c)   ((a) = da_arrgrowf_wrapper((a), sizeof *(a), (b), (c)))

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// in C we use implicit assignment from these void*-returning functions to T*.
// in C++ these templates make the same code work
template<class T> static T * da_arrgrowf_wrapper(T *a, size_t elemsize, size_t addlen, size_t min_cap) {
  return (T*)da_arrgrowf((void *)a, elemsize, addlen, min_cap);
}
#else
#define da_arrgrowf_wrapper            da_arrgrowf
#endif


#endif // DS_H

#ifdef DS_IMPLEMENTATION
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Not to be confused with da_free */
#if defined(ds_realloc) && !defined(ds_free) || !defined(ds_realloc) && defined(ds_free)
#error "You must define both da_realloc and da_free, or neither."
#endif
#if !defined(ds_realloc) && !defined(ds_free)
#include <stdlib.h>
#define ds_realloc(c,p,s) realloc(p,s)
#define ds_free(c,p)      free(p)
#endif


#ifndef ARENA_ALIGNMENT
#define ARENA_ALIGNMENT 16
#endif

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define ARENA_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ARENA_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ARENA_BASE_POS sizeof(mem_arena)

static void *ds_memset(void *buf, int value, size_t count) {
    unsigned char *p = buf;
    unsigned char v = (unsigned char)value;
	size_t i = 0;

    for (; i < count; i++) {
        p[i] = v;
    }

    return buf;
}

void *ds_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    size_t i = 0;

    for (; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

static char *ds_strcpy(char *dst, const char *src) {
    char *start = dst;
    while((*start++ = *src++)){};
    return dst;
}

/* ===========================================================
   ===============   WINDOWS IMPLEMENTATION   =================
   =========================================================== */
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

DSAPI mem_arena *arena_init(size_t size) {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    size_t page_size = sys_info.dwPageSize;
    size = ALIGN_UP(size, page_size);

    mem_arena *arena = (mem_arena*)VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
    if (!arena) {
        fprintf(stderr, "VirtualAlloc reserve failed: %lu\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    size_t initial_commit = ARENA_MAX(page_size, ARENA_BASE_POS);
    initial_commit = ALIGN_UP(initial_commit, page_size);
    
    void *commit = VirtualAlloc(arena, initial_commit, MEM_COMMIT, PAGE_READWRITE);
    if (!commit) {
        fprintf(stderr, "VirtualAlloc initial commit failed: %lu\n", GetLastError());
        VirtualFree(arena, 0, MEM_RELEASE);
        exit(EXIT_FAILURE);
    }

    arena->page_size = page_size;
    arena->reserved_size = size;
    arena->pos = ARENA_BASE_POS;
    arena->committed_size = initial_commit;

    return arena;
}

DSAPI void* arena_push(mem_arena* arena, size_t size, arena_bool zero_out_the_memory) {
    size_t base = (size_t)arena + arena->pos;
    uintptr_t aligned = ALIGN_UP(base, ARENA_ALIGNMENT);
    size_t padding = aligned - base;
    size_t total_size = padding + size;
    size_t required = arena->pos + total_size;

    if (required > arena->reserved_size) {
        fprintf(stderr, "ERROR: Allocation exceeds arena reserved_size!\n");
        return NULL;
    }

    if (required > arena->committed_size) {
        size_t new_commit_end = ALIGN_UP(required, arena->page_size);
        size_t commit_amount = new_commit_end - arena->committed_size;

        void* result = VirtualAlloc((char*)arena + arena->committed_size,
                                    commit_amount, MEM_COMMIT, PAGE_READWRITE);
        if (!result) {
            fprintf(stderr, "VirtualAlloc commit failed: %lu\n", GetLastError());
            exit(EXIT_FAILURE);
        }

        arena->committed_size = new_commit_end;
    }

    arena->pos += total_size;

    if(zero_out_the_memory) {
        ds_memset((void*)aligned, 0, total_size);
    }

    return (void*)aligned;
}

DSAPI void arena_pop(mem_arena *arena, size_t size) {
    if (arena->pos - ARENA_BASE_POS < size) {
        arena->pos = ARENA_BASE_POS;
    } else {
        arena->pos -= size;
    }
}

DSAPI void arena_pop_to(mem_arena *arena, size_t pos) {
    pos = ARENA_MAX(pos, ARENA_BASE_POS);
    if (pos < arena->pos) {
        arena->pos = pos;
    }
}

DSAPI void arena_clear(mem_arena *arena) {
    if (arena) {
        size_t reset_size = arena->committed_size - ARENA_BASE_POS;
        if (reset_size > 0) {
            VirtualAlloc((char*)arena + ARENA_BASE_POS, reset_size, MEM_RESET, PAGE_READWRITE);
        }
        arena->pos = ARENA_BASE_POS;
    }
}

DSAPI void arena_destroy(mem_arena *arena) {
    if (arena) {
        arena->pos = 0;
        arena->committed_size = 0;
        arena->reserved_size = 0;
        arena->page_size = 0;
        VirtualFree(arena, 0, MEM_RELEASE);
    }
}

DSAPI int arena_reset_region(const mem_arena *arena, void *region_start, size_t region_size) {
    uintptr_t arena_start = (uintptr_t)arena + ARENA_BASE_POS;
    uintptr_t arena_end = (uintptr_t)arena + arena->reserved_size;
    uintptr_t region_addr = (uintptr_t)region_start;

    if (region_addr >= arena_start && region_addr + region_size <= arena_end) {
        ds_memset(region_start, 0, region_size);
        return 0;
    }
    return -1;
}

#else
/* ===========================================================
   ===============   POSIX IMPLEMENTATION   ===================
   =========================================================== */

#include <sys/mman.h>
#include <unistd.h>

DSAPI mem_arena *arena_init(size_t size) {
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    size = ALIGN_UP(size, page_size);

    mem_arena *arena = (mem_arena*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    arena->page_size = page_size;
    arena->reserved_size = size;
    arena->pos = ARENA_BASE_POS;
    arena->committed_size = size; // mmap commits all at once on most systems

    return arena;
}

DSAPI void* arena_push(mem_arena* arena, size_t size, arena_bool zero_out_the_memory) {
    size_t base = (size_t)arena + arena->pos;
    uintptr_t aligned = ALIGN_UP(base, ARENA_ALIGNMENT);
    size_t padding = aligned - base;
    size_t total_size = padding + size;
    size_t required = arena->pos + total_size;

    if (required > arena->reserved_size) {
        fprintf(stderr, "ERROR: Allocation exceeds arena reserved_size!\n");
        return NULL;
    }

    arena->pos += total_size;

    if(zero_out_the_memory) {
        ds_memset((void*)aligned, 0, total_size);
    }

    return (void*)aligned;
}

DSAPI void arena_pop(mem_arena *arena, size_t size) {
    if (arena->pos - ARENA_BASE_POS < size) {
        arena->pos = ARENA_BASE_POS;
    } else {
        arena->pos -= size;
    }
}

DSAPI void arena_pop_to(mem_arena *arena, size_t pos) {
    pos = ARENA_MAX(pos, ARENA_BASE_POS);
    if (pos < arena->pos) {
        arena->pos = pos;
    }
}

DSAPI void arena_clear(mem_arena *arena) {
    if (arena) {
        size_t reset_size = arena->pos - ARENA_BASE_POS;
        if (reset_size > 0) {
            madvise((char*)arena + ARENA_BASE_POS, reset_size, MADV_DONTNEED);
        }
        arena->pos = ARENA_BASE_POS;
    }
}

DSAPI void arena_destroy(mem_arena *arena) {
    if (arena) {
        munmap(arena, arena->reserved_size);
    }
}

DSAPI int arena_reset_region(const mem_arena *arena, void *region_start, size_t region_size) {
    uintptr_t arena_start = (uintptr_t)arena + ARENA_BASE_POS;
    uintptr_t arena_end = (uintptr_t)arena + arena->reserved_size;
    uintptr_t region_addr = (uintptr_t)region_start;

    if (region_addr >= arena_start && region_addr + region_size <= arena_end) {
        ds_memset(region_start, 0, region_size);
        return 0;
    }
    return -1;
}

#endif // _WIN32 || _WIN64

/* string */
DSAPI string str_make(const char* str) {
    string s;
    s.value = (char*)str;
    s.len = strlen(str);
    return s;
}

DSAPI string str_from(string str, size_t from) {
    if (from == 0) {
        return str;
    }

    if (from < 0) {
        from += (int)str.len;
    }

    if (from < 0) {
        from = 0;
    }

    if ((size_t)from > str.len) {
        from = (int)str.len;
    }

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
    ds_strcpy(dst, str);
    string string;
    string.value = str;
    string.len = strlen(str);
    return string;
}

DSAPI int ds_itoa(int value, char *string) {
    char tmp[16];
    char *tp = tmp;
    unsigned int absolute_value;

    unsigned int is_negative = ((unsigned int)(value >> 31)) & 1;

    absolute_value = is_negative ? (unsigned int)(-value) : (unsigned int)value;

    do {
        *tp++ = (absolute_value % 10) + '0';
        absolute_value /= 10;
    } while (absolute_value);

    int len = tp - tmp;

    if (is_negative) {
        *string++ = '-';
        len++;
    }

    while (tp > tmp) {
        *string++ = *--tp;
    }

    return len;
}

DSAPI string ds_to_stri(mem_arena *arena, int value) {
    
    char tmp[16] = {0};
    int len = ds_itoa(value, tmp);
    
    string str;
    str.value = (char*)arena_push(arena, len, 1);
    str.len = len;
    ds_strcpy(str.value, tmp);
    
    return str;
}

DSAPI char *ds_to_cstri(mem_arena *arena, int value) {
    
    char tmp[16] = {0};
    int len = ds_itoa(value, tmp);
    
    char *str = (char*)arena_push(arena, len, 1);
    ds_strcpy(str, tmp);
    
    return str;
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
                ds_strcpy(buffer, s);
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
DSAPI string_builder sb_make(mem_arena *arena, const char* str, size_t max_capacity) {
	string_builder str_builder = { 0 };
	string string = {0};
	string.len = strlen(str);
    string.value = (char*)arena_push(arena, string.len + 1, 1);

    ds_memset(string.value, 0, max_capacity);
	ds_strcpy(string.value, (char*)str);
	str_builder.string = string;
	str_builder.capacity = max_capacity;
	return str_builder;
}


DSAPI string sb_append(string_builder *builder, const char *str) {
	size_t str_len = strlen(str);
	size_t required = builder->string.len + str_len + 1; // +1 for '\0'

	if (required > builder->capacity) {
	}

	/* Append string */
	ds_memcpy(builder->string.value + builder->string.len, str, str_len);
	builder->string.len += str_len;
    return builder->string;
}

DSAPI void sb_reset(string_builder *builder) {
    ds_memset(builder->string.value, 0, builder->string.len);
    builder->string.len = 0;
}

DSAPI int sb_print(string_builder *builder) {
    return printf("%s", builder->string.value);
}

DSAPI void sb_free(string_builder *builder) {
    free(builder->string.value);
}

/* dynamic arrays */
void *da_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap) {
  da_header temp={0}; // force debugging
  void *b;
  size_t min_len = da_len(a) + addlen;
  (void) sizeof(temp);

  /* compute the minimum capacity needed */
  if (min_len > min_cap) {
    min_cap = min_len;
  }

  if (min_cap <= da_cap(a)) {
    return a;
  }

  /* increase needed capacity to guarantee O(1) amortized */
  if (min_cap < 2 * da_cap(a)) {
    min_cap = 2 * da_cap(a);
  } else if (min_cap < 4) {
    min_cap = 4;
  }

  b = ds_realloc(NULL, (a) ? header(a) : 0, elemsize * min_cap + sizeof(da_header));
  /* if (num_prev < 65536) prev_allocs[num_prev++] = (int *) (char *) b; */
  b = (char *) b + sizeof(da_header);
  if (a == NULL) {
      header(b)->length = 0;
  }
  header(b)->capacity = min_cap;

  return b;
}
#endif /* DS_IMPLEMENTATION */
