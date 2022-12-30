#include <gio/gio.h>

#if defined (__ELF__) && ( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
# define SECTION __attribute__ ((section (".gresource.resources"), aligned (8)))
#else
# define SECTION
#endif

#ifdef _MSC_VER
static const SECTION union { const guint8 data[409]; const double alignment; void * const ptr;}  resources_resource_data = { {
  0107, 0126, 0141, 0162, 0151, 0141, 0156, 0164, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 
  0030, 0000, 0000, 0000, 0220, 0000, 0000, 0000, 0000, 0000, 0000, 0050, 0004, 0000, 0000, 0000, 
  0000, 0000, 0000, 0000, 0003, 0000, 0000, 0000, 0003, 0000, 0000, 0000, 0003, 0000, 0000, 0000, 
  0324, 0265, 0002, 0000, 0377, 0377, 0377, 0377, 0220, 0000, 0000, 0000, 0001, 0000, 0114, 0000, 
  0224, 0000, 0000, 0000, 0230, 0000, 0000, 0000, 0214, 0324, 0260, 0006, 0003, 0000, 0000, 0000, 
  0230, 0000, 0000, 0000, 0007, 0000, 0114, 0000, 0240, 0000, 0000, 0000, 0244, 0000, 0000, 0000, 
  0020, 0371, 0343, 0230, 0001, 0000, 0000, 0000, 0244, 0000, 0000, 0000, 0013, 0000, 0166, 0000, 
  0260, 0000, 0000, 0000, 0215, 0001, 0000, 0000, 0113, 0120, 0220, 0013, 0000, 0000, 0000, 0000, 
  0215, 0001, 0000, 0000, 0004, 0000, 0114, 0000, 0224, 0001, 0000, 0000, 0230, 0001, 0000, 0000, 
  0057, 0000, 0000, 0000, 0003, 0000, 0000, 0000, 0167, 0157, 0157, 0146, 0145, 0162, 0057, 0000, 
  0002, 0000, 0000, 0000, 0151, 0143, 0157, 0156, 0062, 0065, 0066, 0056, 0163, 0166, 0147, 0000, 
  0315, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0074, 0077, 0170, 0155, 0154, 0040, 0166, 0145, 
  0162, 0163, 0151, 0157, 0156, 0075, 0042, 0061, 0056, 0060, 0042, 0040, 0145, 0156, 0143, 0157, 
  0144, 0151, 0156, 0147, 0075, 0042, 0125, 0124, 0106, 0055, 0070, 0042, 0040, 0163, 0164, 0141, 
  0156, 0144, 0141, 0154, 0157, 0156, 0145, 0075, 0042, 0156, 0157, 0042, 0077, 0076, 0012, 0074, 
  0163, 0166, 0147, 0040, 0167, 0151, 0144, 0164, 0150, 0075, 0042, 0062, 0065, 0066, 0042, 0040, 
  0150, 0145, 0151, 0147, 0150, 0164, 0075, 0042, 0062, 0065, 0066, 0042, 0040, 0166, 0151, 0145, 
  0167, 0102, 0157, 0170, 0075, 0042, 0060, 0040, 0060, 0040, 0062, 0065, 0066, 0040, 0062, 0065, 
  0066, 0042, 0040, 0170, 0155, 0154, 0156, 0163, 0075, 0042, 0150, 0164, 0164, 0160, 0072, 0057, 
  0057, 0167, 0167, 0167, 0056, 0167, 0063, 0056, 0157, 0162, 0147, 0057, 0062, 0060, 0060, 0060, 
  0057, 0163, 0166, 0147, 0042, 0076, 0012, 0040, 0040, 0074, 0164, 0151, 0164, 0154, 0145, 0076, 
  0120, 0162, 0157, 0152, 0145, 0143, 0164, 0040, 0127, 0157, 0157, 0146, 0145, 0162, 0040, 0111, 
  0143, 0157, 0156, 0040, 0062, 0065, 0066, 0170, 0062, 0065, 0066, 0074, 0057, 0164, 0151, 0164, 
  0154, 0145, 0076, 0012, 0040, 0040, 0074, 0147, 0076, 0074, 0057, 0147, 0076, 0012, 0074, 0057, 
  0163, 0166, 0147, 0076, 0012, 0000, 0000, 0050, 0165, 0165, 0141, 0171, 0051, 0157, 0162, 0147, 
  0057, 0000, 0000, 0000, 0001, 0000, 0000, 0000
} };
#else /* _MSC_VER */
static const SECTION union { const guint8 data[409]; const double alignment; void * const ptr;}  resources_resource_data = {
  "\107\126\141\162\151\141\156\164\000\000\000\000\000\000\000\000"
  "\030\000\000\000\220\000\000\000\000\000\000\050\004\000\000\000"
  "\000\000\000\000\003\000\000\000\003\000\000\000\003\000\000\000"
  "\324\265\002\000\377\377\377\377\220\000\000\000\001\000\114\000"
  "\224\000\000\000\230\000\000\000\214\324\260\006\003\000\000\000"
  "\230\000\000\000\007\000\114\000\240\000\000\000\244\000\000\000"
  "\020\371\343\230\001\000\000\000\244\000\000\000\013\000\166\000"
  "\260\000\000\000\215\001\000\000\113\120\220\013\000\000\000\000"
  "\215\001\000\000\004\000\114\000\224\001\000\000\230\001\000\000"
  "\057\000\000\000\003\000\000\000\167\157\157\146\145\162\057\000"
  "\002\000\000\000\151\143\157\156\062\065\066\056\163\166\147\000"
  "\315\000\000\000\000\000\000\000\074\077\170\155\154\040\166\145"
  "\162\163\151\157\156\075\042\061\056\060\042\040\145\156\143\157"
  "\144\151\156\147\075\042\125\124\106\055\070\042\040\163\164\141"
  "\156\144\141\154\157\156\145\075\042\156\157\042\077\076\012\074"
  "\163\166\147\040\167\151\144\164\150\075\042\062\065\066\042\040"
  "\150\145\151\147\150\164\075\042\062\065\066\042\040\166\151\145"
  "\167\102\157\170\075\042\060\040\060\040\062\065\066\040\062\065"
  "\066\042\040\170\155\154\156\163\075\042\150\164\164\160\072\057"
  "\057\167\167\167\056\167\063\056\157\162\147\057\062\060\060\060"
  "\057\163\166\147\042\076\012\040\040\074\164\151\164\154\145\076"
  "\120\162\157\152\145\143\164\040\127\157\157\146\145\162\040\111"
  "\143\157\156\040\062\065\066\170\062\065\066\074\057\164\151\164"
  "\154\145\076\012\040\040\074\147\076\074\057\147\076\012\074\057"
  "\163\166\147\076\012\000\000\050\165\165\141\171\051\157\162\147"
  "\057\000\000\000\001\000\000\000" };
#endif /* !_MSC_VER */

static GStaticResource static_resource = { resources_resource_data.data, sizeof (resources_resource_data.data) - 1 /* nul terminator */, NULL, NULL, NULL };

G_GNUC_INTERNAL
GResource *resources_get_resource (void);
GResource *resources_get_resource (void)
{
  return g_static_resource_get_resource (&static_resource);
}
/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a usable way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#ifndef __GTK_DOC_IGNORE__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
/* Visual studio 2008 and later has _Pragma */

#include <stdlib.h>

#define G_HAS_CONSTRUCTORS 1

/* We do some weird things to avoid the constructors being optimized
 * away on VS2015 if WholeProgramOptimization is enabled. First we
 * make a reference to the array from the wrapper to make sure its
 * references. Then we use a pragma to make sure the wrapper function
 * symbol is always included at the link stage. Also, the symbols
 * need to be extern (but not dllexport), even though they are not
 * really used from another object file.
 */

/* We need to account for differences between the mangling of symbols
 * for x86 and x64/ARM/ARM64 programs, as symbols on x86 are prefixed
 * with an underscore but symbols on x64/ARM/ARM64 are not.
 */
#ifdef _M_IX86
#define G_MSVC_SYMBOL_PREFIX "_"
#else
#define G_MSVC_SYMBOL_PREFIX ""
#endif

#define G_DEFINE_CONSTRUCTOR(_func) G_MSVC_CTOR (_func, G_MSVC_SYMBOL_PREFIX)
#define G_DEFINE_DESTRUCTOR(_func) G_MSVC_DTOR (_func, G_MSVC_SYMBOL_PREFIX)

#define G_MSVC_CTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
  __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define G_MSVC_DTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
   __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#elif defined (_MSC_VER)

#define G_HAS_CONSTRUCTORS 1

/* Pre Visual studio 2008 must use #pragma section */
#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func ## _wrapper;

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __GTK_DOC_IGNORE__ */

#ifdef G_HAS_CONSTRUCTORS

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(resource_constructor)
#endif
G_DEFINE_CONSTRUCTOR(resource_constructor)
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(resource_destructor)
#endif
G_DEFINE_DESTRUCTOR(resource_destructor)

#else
#warning "Constructor not supported on this compiler, linking in resources will not work"
#endif

static void resource_constructor (void)
{
  g_static_resource_init (&static_resource);
}

static void resource_destructor (void)
{
  g_static_resource_fini (&static_resource);
}
