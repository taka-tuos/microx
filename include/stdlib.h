/* copyright(C) 2003 H.Kawai (under KL-01). */

#if (!defined(STDLIB_H))

#define STDLIB_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <stddef.h>		/* size_t */

#define	RAND_MAX	0x7fff
#define srand(seed)			(void) (rand_seed = (seed))

#define EXIT_SUCCESS		0
#define EXIT_FAILURE		1

int abs(int n);
int atoi(const char *s);
void qsort(void *base, size_t n, size_t size,
	int (*cmp)(const void *, const void *));
int rand(void);
extern unsigned int rand_seed;
int strtol(const char *s, const char **endp, int base);
unsigned int strtoul(const char *s, const char **endp, int base);

/* for strdup() */
void *malloc(unsigned int nbytes);
void free(void *ap);

#if (defined(__cplusplus))
	}
#endif

#endif
