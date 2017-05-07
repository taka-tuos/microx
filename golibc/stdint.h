/* copyright(C) 2016 T.Yoda (under KL-01). */

#if (!defined(STDINT_H))

#define STDINT_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#if (!defined(NULL))
	#define NULL	((void *) 0)
#endif

/* golibc */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

#if (defined(__cplusplus))
	}
#endif

#endif