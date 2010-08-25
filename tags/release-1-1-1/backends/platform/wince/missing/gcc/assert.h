/* Header is not present in Windows CE SDK */

// defined in common/util.h
void CDECL _declspec(noreturn) error(const char *s, ...);

#define assert(e) ((e) ? (void) 0 : (::error("Assertion failed " #e " (%s, %d)", __FILE__, __LINE__)))

#define abort() ::error("Abort (%s, %d)", __FILE__, __LINE__)

