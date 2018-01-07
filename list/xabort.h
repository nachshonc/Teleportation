
#ifndef _ABORT_CODES
#define _ABORT_CODES
enum xabort_t {XABORT_RETRIES = 1 << 2,
	       XABORT_NOT_LINKED = 1 << 3,
	       XABORT_LOCKED = 1 << 4,
	       XABORT_INVALID = 1 << 5
};


#endif /* abort codes */
