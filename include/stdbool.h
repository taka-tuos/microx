/* copyright(C) 2017 T.Yoda (under Public Domain). */

#if (!defined(STDBOOL_H))

#define STDBOOL_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#if (!defined(NULL))
	#define NULL	((void *) 0)
#endif

#define __bool_true_false_are_defined 1
#define bool char
#define true 1
#define false 0

#if (defined(__cplusplus))
	}
#endif

#endif
