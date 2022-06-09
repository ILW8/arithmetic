/*----------------------------------------------------------------------
| polymult.h
|
| This file contains the headers and definitions that are used in the
| polynomial multiplication built upon the gwnum library.
| 
|  Copyright 2021-2022 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

#ifndef _POLYMULT_H
#define _POLYMULT_H

/* This is a C library.  If used in a C++ program, don't let the C++ compiler mangle names. */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct pmhandle_struct pmhandle;
typedef struct polyplan polymult_plan;

// These routines can be used before polymult_init.  Use the information to select a gwnum FFT size with sufficient safety_margin
// to do polymults of the desired size.

// Get the needed safety_margin required for an invec1_size by invec2_size polymult
double polymult_safety_margin (int invec1_size, int invec2_size);

// Get the FFT size that will be used for an n = invec1_size + invec2_size polymult
int polymult_fft_size (int n);

// Get the memory (in bytes) required for an FFT based polymult.  Use the information to ensure over-allocating memory does not occur.
uint64_t polymult_mem_required (int invec1_size, int invec2_size, int options, int cpu_flags, int num_threads);

/* Callers of the polymult routines must first allocate a pmhandle (on the heap or stack) and pass it to all polymult routines. */
// Initialize a polymult handle
void polymult_init (
	pmhandle *pmdata,		// Handle for polymult library that needs initializing
	gwhandle *gwdata);		// Handle for gwnum FFT library

// Control how many threads to use during polymult (or using polymult helper threads with a user-defined callback).  The maximum must be set prior to first
// call to polymult.  One can use fewer than the maximum number of threads -- this might be useful in cases where hyperthreading benefits only some work types.
// NOTE: If gwnum library has a thread_callback routine set, the polymult library will use the same callback routine with action code 20 and 21. */
#define polymult_set_max_num_threads(h,n)	(h)->max_num_threads = (h)->num_threads = (n)
#define polymult_set_num_threads(h,n)		(h)->num_threads = (n)

// Set the cache size to optimize FFTs for
#define polymult_set_cache_size(h,n)	(h)->L2_CACHE_SIZE = (n)

// Terminate use of a polymult handle.  Free up memory.
void polymult_done (
	pmhandle *pmdata);		// Handle for polymult library

/* Multiply two polynomials.  It is safe to use input gwnums in the output vector.  For a normal polynomial multiply outvec_size must be */
/* invec1_size + invec2_size - 1.  For monic polynomial multiply, the leading input coefficients of 1 are omitted as is the leading 1 output */
/* coefficient -- thus outvec_size must be invec1_size + invec2_size. */
/* Entries in outvec can be NULL if caller is not interested in those output coefficients. */
void polymult (
	pmhandle *pmdata,		// Handle for polymult library
	gwnum	*invec1,		// First input poly
	int	invec1_size,		// Size of the first input polynomial
	gwnum	*invec2,		// Second input poly
	int	invec2_size,		// Size of the second input polynomial
	gwnum	*outvec,		// Output poly
	int	outvec_size,		// Size of the output polynomial or if POLYMULT_CIRCULAR is set compute result modulo (X^outvec_size - 1)
					// or if POLYMULT_MULHI or POLYMULT_MULLO is set this is the number of coefficients to return.  This interface
					// does not allow the POLYMULT_CIRCULAR option and POLYMULT_MULHI or POLYMULT_MULLO to both be set -- use
					// polymult2 or polymult_several instead.
	int	options);

/* Multiply two polynomials with a fused multiply add. */
void polymult_fma (
	pmhandle *pmdata,		// Handle for polymult library
	gwnum	*invec1,		// First input poly
	int	invec1_size,		// Size of the first input polynomial
	gwnum	*invec2,		// Second input poly
	int	invec2_size,		// Size of the second input polynomial
	gwnum	*outvec,		// Output poly
	int	outvec_size,		// Size of the output polynomial or if POLYMULT_CIRCULAR is set compute result modulo (X^outvec_size - 1)
					// or if POLYMULT_MULHI or POLYMULT_MULLO is set this is the number of coefficients to return.  This interface
					// does not allow the POLYMULT_CIRCULAR option and POLYMULT_MULHI or POLYMULT_MULLO options to both be set -- use
					// polymult_several instead.
	gwnum	*fmavec,		// FMA poly to add or subtract from poly multiplication result.  Same size as outvec, cannot be preprocessed.
	int	options);

/* Multiply two polynomials.  Supports every possible polymult option.  The original polymult interface overloaded the outvec_size argument. */
void polymult2 (
	pmhandle *pmdata,		// Handle for polymult library
	gwnum	*invec1,		// First input poly
	int	invec1_size,		// Size of the first input polynomial
	gwnum	*invec2,		// Second input poly
	int	invec2_size,		// Size of the second input polynomial
	gwnum	*outvec,		// Output poly
	int	outvec_size,		// Size of the output polynomial.  If POLYMULT_MULHI or POLYMULT_MULLO is set this is the number of coefficients to return.
	gwnum	*fmavec,		// FMA poly to add or subtract from poly multiplication result.  Same size as outvec, cannot be preprocessed.
	int	circular_size,		// If POLYMULT_CIRCULAR set, compute poly result modulo (X^circular_size - 1)
	int	first_mulmid,		// If POLYMULT_MULMID set, this is the number of least significant coefficients that will not be returned
	int	options);

#define POLYMULT_INVEC1_MONIC	0x1	// Invec1 is a monic polynomial.  Leading coefficient of one is implied.
#define POLYMULT_INVEC2_MONIC	0x2	// Invec2 is a monic polynomial.  Leading coefficient of one is implied.
#define POLYMULT_INVEC1_RLP	0x4	// Invec1 is an RLP (Reciprocal Laurent Polynomial).  Needs only half the storage.
#define POLYMULT_INVEC2_RLP	0x8	// Invec2 is an RLP (Reciprocal Laurent Polynomial).  Needs only half the storage.
#define POLYMULT_INVEC1_MONIC_RLP	(POLYMULT_INVEC1_MONIC | POLYMULT_INVEC1_RLP)		// A shorthand
#define POLYMULT_INVEC2_MONIC_RLP	(POLYMULT_INVEC2_MONIC | POLYMULT_INVEC2_RLP)		// A shorthand
#define POLYMULT_INVEC1_NEGATE	0x10	// Invec1 coefficients are negated.  The implied one of a monic polynomial is not negated.
#define POLYMULT_INVEC2_NEGATE	0x20	// Invec2 coefficients are negated.  The implied one of a monic polynomial is not negated.
#define POLYMULT_CIRCULAR	0x100	// Circular convolution based on outvec_size.  Multiplication result is modulo (X^outvec_size - 1).  If using the
					// polymult_several interface, the result is modulo (X^circular_size - 1).
#define POLYMULT_MULHI		0x200	// Return only the outvec_size higher degree coefficients.  Using both POLYMULT_CIRCULAR
					// and POLYMULT_MULHI is only allowed in the polymult_several interface.
#define POLYMULT_MULLO		0x400	// Return only the outvec_size lower degree coefficients.  Using both POLYMULT_CIRCULAR
					// and POLYMULT_MULLO is only allowed in the polymult_several interface.
#define POLYMULT_MULMID		0x800	// Return outvec_size coefficients from middle of the polymult result.  Only allowed in the polymult_several interface.
#define POLYMULT_NO_UNFFT	0x1000	// Do not perform the required unfft on output coefficients.  Caller might use this option to multithread this gwunfft
					// calls or do additional work after the gwunfft call while the gwnum is in the CPU caches.
#define POLYMULT_STARTNEXTFFT	0x2000	// Similar to GWMUL_STARTNEXTFFT.  Applied to all output coefficients.
#define	POLYMULT_NEXTFFT	0x4000	// Perform both the required unfft and a forward FFT on output coefficients.  Caller might use this option because
					// it may perform better because the gwnum is in the CPU caches.
#define POLYMULT_FMADD		0x8000	// Compute invec1 * invec2 + fmavec
#define POLYMULT_FMSUB		0x10000	// Compute invec1 * invec2 - fmavec
#define POLYMULT_FNMADD		0x20000	// Compute fmavec - invec1 * invec2
//#define GWMUL_ADDINCONST	0x10000000		/* Addin the optional gwsetaddin value to the multiplication result */
//#define GWMUL_MULBYCONST	0x20000000		/* Multiply the final result by the small mulbyconst */
// The following options only apply to polymult_preprocess
#define	POLYMULT_PRE_FFT	0x40	// Compute the forward FFT while creating a preprocessed polynomial
#define	POLYMULT_PRE_COMPRESS	0x80	// Compress each double while creating a preprocessed polynomial

/*-----------------------------------------------------------------------------
|	Advanced polymult routines
+----------------------------------------------------------------------------*/

// Accessing the preprocessed header directly is frowned upon, several accessor macros follow
#define is_preprocessed_poly(p)		((p) != NULL && (((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->self_ptr == (p)))
#define is_preffted_poly(p)		(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->options & POLYMULT_PRE_FFT)
#define preprocessed_num_elements(p)	(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->num_lines)
#define preprocessed_element_size(p)	(intptr_t) (((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->padded_element_size)
#define preprocessed_fft_size(p)	(intptr_t) (((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->fft_size)
#define preprocessed_poly_size(p)	(preprocessed_num_elements(p) * preprocessed_element_size(p))
#define preprocessed_monics_included(p)	(((preprocessed_poly_header *)((char *)(p)-sizeof(gwarray_header)))->monic_ones_included)

/* Preprocess a poly that will be used in multiple polymult calls.  Preprocessing can reduce memory consumption or reduce CPU time.  Returns a massaged poly. */
/* Caller should then free the unmassaged poly.  The massaged poly cannot be used in any gwnum calls, it can only be used in future polymult calls with */
/* poly sizes and options that match those passed to this routine. */
/* The POLYMULT_PRE_FFT preprocessing option allows the forward FFT of invec1 to be used over and over again.  The downside to the POLYMULT_PRE_FFT option */
/* is the preprocessed poly consumes more memory. */
gwarray polymult_preprocess (		// Returns a plug-in replacement for the input poly
	pmhandle *pmdata,		// Handle for polymult library
	gwnum	*invec1,		// Input poly
	int	invec1_size,		// Size of the input polynomial
	int	invec2_size,		// Size of the other polynomial that will be used in a future polymult call
	int	outvec_size,		// Size of the output polynomial that will be used in a future polymult call
	int	options);		// Future polymult call options plus preprocessing options (FFT, compress -- see below)

/* These routines allow multiplying one poly with several other polys.  This yields a small performance gain in that the one poly is read and FFTed just once. */

typedef struct pmargument_struct polymult_arg;

struct pmargument_struct {	// Each of the several polys need to describe where their input and output coefficients are located
	gwnum	*invec2;	// Second input poly
	int	invec2_size;	// Size of the second input polynomial
	gwnum	*outvec;	// Output poly
	int	outvec_size;	// Size of the output polynomial
	gwnum	*fmavec;	// Poly to add in if FMA options are requested
	int	circular_size;	// If POLYMULT_CIRCULAR set, compute poly result modulo (X^circular_size - 1)
	int	first_mulmid;	// If POLYMULT_MULMID set, this is the number of least significant coefficients that will not be returned
	int	options;	// Any of the polymult options that are not related to poly #1
};

void polymult_several (		// Multiply one poly with several polys	
	pmhandle *pmdata,	// Handle for polymult library
	gwnum	*invec1,	// First input poly
	int	invec1_size,	// Size of the first input polynomial
	polymult_arg *other_polys, // Array of "other poly descriptors" to multiply with first input poly (describes each second input poly and output poly)
	int	num_other_polys,// Number of other polys to multiply with first input poly
	int	options);	// Poly #1 options.  Options not associated with poly #1 are applied to all other polys.

/* These routines allow the user to use the polymult helper threads and locking mechanisms to craft their own multithreaded helper routines. */
/* The helper_callback and helper_callback_data fields must be set prior to launching the helpers.  Each helper is given its own clone of gwdata */
/* to safely call gwnum operations -- cloning is necessary as independent threads cannot use use same gwdata structure. */

/* This routine launches the polymult helper threads.  The polymult library uses this routine to do multithreading, users can too! */
void polymult_launch_helpers (
	pmhandle *pmdata);		// Handle for polymult library

/* This routine waits for the launched polymult helper threads to finish */
void polymult_wait_on_helpers (
	pmhandle *pmdata);		// Handle for polymult library


/*-----------------------------------------------------------------------------
|	Internal polymult library data structures
+----------------------------------------------------------------------------*/

/* The pmhandle structure containing all of polymult's "global" data. */
struct pmhandle_struct {
	gwhandle *gwdata;		// Handle for gwnum FFT library
	int	max_num_threads;	// Maximum number of threads that can be used to compute polymults
	int	num_threads;		// Number of threads to use computing polymults (must not exceed max_num_threads)
	gwevent work_to_do;		// Event to signal polymult helper threads there is work to do
	gwevent helpers_done;		// Event to signal polymult helper threads are done
	gwevent main_can_wakeup;	// Event to signal polymult helper threads are waiting for work, thus the main thread can resume
	gwmutex	poly_mutex;		// Mutex to make polymult thread safe when multi-threading
	int	next_thread_num;	// Lets us generate a unique id for each helper thread
	int	next_line;		// Next line for a thread to process
	int	helpers_active;		// Count of helper threads that are still active
	int	helpers_waiting_work;	// Count of helper threads thathave reached waiting for work_to_do
	bool	termination_in_progress; // Flag for helper threads to exit
	gwthread *thread_ids;		// Thread ids for the spawned threads
	int	twiddles_initialized;	// Size of the twiddle tables
	bool	twiddles_are_from_cache; // TRUE if the current twiddles came from the twiddle cache
	double	*twiddles1;		// Sin/cos table for radix-3
	double	*twiddles2;		// Sin/cos table for radix-4 and radix-5
	// Brute force from outvec_size [1..KARAT_BREAK), Karatsuba from [KARAT_BREAK..FFT_BREAK), FFT from [FFT_BREAK..infinity)
	int	KARAT_BREAK;		// Output vector size where we switch from brute force to Karatsuba
	int	FFT_BREAK;		// Output vector size where we switch from Karatsuba to FFTs
	int	L2_CACHE_SIZE;		// Optimize FFTs to fit in this size cache (number is in KB).  Default is 256KB.
	// Cached twiddles
	bool	cached_twiddles_enabled;// TRUE if caching twiddles is enabled
	bool	twiddle_cache_additions_disabled; // TRUE if adding new entries to the twiddle cache is temporarily disabled
	struct {
		int	size;		// Size of the twiddle tables
		double	*twiddles1;	// Sin/cos table for radix-3
		double	*twiddles2;	// Sin/cos table for radix-4 and radix-5
	} cached_twiddles[40];
	int	cached_twiddles_count;	// Number of cached twiddles
	// Arguments to the current polymult call.  Copied here so that helper threads can access the arguments.  Also, the plan for implementing the polymult.
	gwnum	*invec1;		// First input poly
	int	invec1_size;		// Size of the first input polynomial
	polymult_arg *other_polys;	// Array of second polys
	int	num_other_polys;	// Number of second polys
	int	options;
	int	alloc_invec1_size;	// Pre-calculated allocation size for invec1
	int	alloc_invec2_size;	// Pre-calculated allocation size for invec2
	int	alloc_outvec_size;	// Pre-calculated allocation size for outvec
	int	alloc_tmpvec_size;	// Pre-calculated allocation size for tmpvec
	polymult_plan *plan;		// Plan for implementing the multiplication of invec1 and several invec2s
	// These items allow users of the polymult library to also use the polymult helper threads for whatever they see fit.
	bool	helpers_doing_polymult;	// TRUE if helpers are doing polymult work, not user work
	bool	helpers_sync_clone_stats; // TRUE if helpers are syncing cloned gwdata stats, not user work
	void	(*helper_callback)(int, gwhandle *, void *); // User-defined helper callback routine
	void	*helper_callback_data;	// User-defined data to pass to the user-defined helper callback routine
	int	saved_gwdata_num_threads; // Used internally to restore gwdata's multithreading after a user defined callback
};

// Header for a preprocessed poly.  Conceptually a preprocessed poly is an array of invec1 or invec2 as returned by read_line called from polymult_line.
// Rather than pulling one cache line out of each gwnum, the cache lines are in contiguous memory (this reduces mem consumption by ~1.5% as it eliminates
// 64 pad bytes every 4KB in most gwnums.  Contiguous memory ought to be quicker to access too.  Each invec1 or invec2 can optionally be FFTed (probably
// increases memory consumption due to zero padding) to reduce CPU time in each future polymult call.  Each invec can also be compressed reducing memory
// consumption by ~12.5% -- we can compress ~8 bits out of each 11-bit exponent in a double.
typedef struct {
	gwarray_header linkage;		// Used to put preprocessed polys on gwarray linked list.  Allows gwdone to free all preprocessed polys.
	gwnum	*self_ptr;		// Ptr to itself.  A unique indicator that this is a preprocessed poly.
	int	num_lines;		// Number of lines returned by read_line
	int	element_size;		// Size of each invec in the array
	int	padded_element_size;	// Size of each invec in the array rounded up to a multiple of 64 (not rounded when compressing)
	int	options;		// Copy of options passed to polymult_line_preprocess
	int	fft_size;		// If POLYMULT_PRE_FFT is set, this is the poly FFT size selected during preprocessing
	bool	monic_ones_included;	// TRUE if monic ones are included in pre-FFTed data
} preprocessed_poly_header;

// Internal description of the plan to multiply two polys
typedef struct polyplan {
	bool	emulate_circular;	// Emulating circular_size is required
	bool	strip_monic_from_invec1;// True if ones are stripped from monic invec1 requiring (99% of the time) invec2 to be added in during post processing
	bool	strip_monic_from_invec2;// True if ones are stripped from monic invec2 requiring (99% of the time) invec1 to be added in during post processing
	bool	post_monic_addin_invec1;// True if ones are stripped from monic invec2 requiring invec1 to be added in during post processing
	bool	post_monic_addin_invec2;// True if ones are stripped from monic invec1 requiring invec2 to be added in during post processing
	int	impl;			// 0 = brute force, 1 = Karatsuba, 2 = poly FFT
	int	fft_size;		// FFT size for impl type 2
	int	adjusted_invec1_size;	// Sizes of the possibly smaller partial polymult result prior to post-monic-adjustment creating the full result
	int	adjusted_invec2_size;
	int	adjusted_outvec_size;
	int	true_invec1_size;	// Sizes of the full polymult inputs and outputs
	int	true_invec2_size;
	int	true_outvec_size;
	int	circular_size;		// Return result modulo (X^circular_size - 1)
	int	LSWs_skipped;		// Least significant coefficients that do not need to be returned
	int	MSWs_skipped;		// Most significant coefficients (of true_outvec_size) that do not need to be returned
	int	addin[4];		// Outvec locations where four 1*1 values may need to be added at the very end of the polymult process
	int	subout;			// Outvec location where 1*1 value may need to be subtracted at the very end of the polymult process
	int	adjusted_shift;		// Number of coefficients to shift left the initial partial poly multiplication to reach true_outvec_size
	int	adjusted_pad;		// Number of coefficients to pad on the left of the initial partial poly multiplication to reach true_outvec_size
} polymult_plan;

#ifdef __cplusplus
}
#endif

#endif
