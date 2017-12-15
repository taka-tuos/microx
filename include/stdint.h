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
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned char uint_least8_t;
typedef unsigned short uint_least16_t;
typedef unsigned int uint_least32_t;
typedef unsigned long long uint_least64_t;

typedef signed char int_least8_t;
typedef signed short int_least16_t;
typedef signed int int_least32_t;
typedef signed long long int_least64_t;

typedef unsigned long long uintmax_t;
typedef signed long long intmax_t;

typedef signed int intptr_t;
typedef unsigned int uintptr_t;

#define	INT8_C(c)		(c)
#define	INT16_C(c)		(c)
#define	INT32_C(c)		(c)

#define	UINT8_C(c)		(c)
#define	UINT16_C(c)		(c)
#define	UINT32_C(c)		(c ## U)

/* Minimum values of exact-width signed integer types. */
#define	INT8_MIN	(-0x7f-1)
#define	INT16_MIN	(-0x7fff-1)
#define	INT32_MIN	(-0x7fffffff-1)
#define	INT64_MIN	(-INT64_C(0x7fffffffffffffff)-1)

/* Maximum values of exact-width signed integer types. */
#define	INT8_MAX	0x7f
#define	INT16_MAX	0x7fff
#define	INT32_MAX	0x7fffffff
#define	INT64_MAX	INT64_C(0x7fffffffffffffff)

/* Maximum values of exact-width unsigned integer types. */
#define	UINT8_MAX	0xff
#define	UINT16_MAX	0xffff
#define	UINT32_MAX	0xffffffff
#define	UINT64_MAX	UINT64_C(0xffffffffffffffff)

/*
 * ISO/IEC 9899:1999
 * 7.18.2.2  Limits of minimum-width integer types
 */
/* Minimum values of minimum-width signed integer types. */
#define	INT_LEAST8_MIN	INT8_MIN
#define	INT_LEAST16_MIN	INT16_MIN
#define	INT_LEAST32_MIN	INT32_MIN
#define	INT_LEAST64_MIN	INT64_MIN

/* Maximum values of minimum-width signed integer types. */
#define	INT_LEAST8_MAX	INT8_MAX
#define	INT_LEAST16_MAX	INT16_MAX
#define	INT_LEAST32_MAX	INT32_MAX
#define	INT_LEAST64_MAX	INT64_MAX

/* Maximum values of minimum-width unsigned integer types. */
#define	UINT_LEAST8_MAX	 UINT8_MAX
#define	UINT_LEAST16_MAX UINT16_MAX
#define	UINT_LEAST32_MAX UINT32_MAX
#define	UINT_LEAST64_MAX UINT64_MAX

/*
 * ISO/IEC 9899:1999
 * 7.18.2.3  Limits of fastest minimum-width integer types
 */
/* Minimum values of fastest minimum-width signed integer types. */
#define	INT_FAST8_MIN	INT32_MIN
#define	INT_FAST16_MIN	INT32_MIN
#define	INT_FAST32_MIN	INT32_MIN
#define	INT_FAST64_MIN	INT64_MIN

/* Maximum values of fastest minimum-width signed integer types. */
#define	INT_FAST8_MAX	INT32_MAX
#define	INT_FAST16_MAX	INT32_MAX
#define	INT_FAST32_MAX	INT32_MAX
#define	INT_FAST64_MAX	INT64_MAX

/* Maximum values of fastest minimum-width unsigned integer types. */
#define	UINT_FAST8_MAX	UINT32_MAX
#define	UINT_FAST16_MAX	UINT32_MAX
#define	UINT_FAST32_MAX	UINT32_MAX
#define	UINT_FAST64_MAX	UINT64_MAX

/*
 * ISO/IEC 9899:1999
 * 7.18.2.5  Limits of greatest-width integer types
 */
#define	INTMAX_MIN	INT64_MIN
#define	INTMAX_MAX	INT64_MAX
#define	UINTMAX_MAX	UINT64_MAX

/*
 * ISO/IEC 9899:1999
 * 7.18.2.4  Limits of integer types capable of holding object pointers
 */
#define	INTPTR_MIN	INT32_MIN
#define	INTPTR_MAX	INT32_MAX
#define	UINTPTR_MAX	UINT32_MAX

#if (defined(__cplusplus))
	}
#endif

#endif
