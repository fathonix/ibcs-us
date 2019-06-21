/*
 * Various libc and klibc functions we emulate.
 */

/*
 * Define the __builtins before we read in string.h and it redefined them
 * to use the equivalent __builtin.  We aren't going for speed here because
 * we expect gcc will actually use the builtin, but if you don't optimise
 * it won't.
 */
void* memchr(const void* s, int b, unsigned c)
{
    const unsigned char* cs;
    const unsigned char	cb = b;

    for (cs = s; c--; cs += 1) {
	if (*cs == cb) {
	    return (void*)cs;
	}
    }
    return (void*)0;
}


int memcmp(const void* s1, const void* s2, unsigned c)
{
    const unsigned char* cs1 = s1;
    const unsigned char* cs2 = s2;
    int			ret = 0;
    
    for (; c && !(ret = (*cs1 - *cs2)); c -= 1, cs1 += 1, cs2 += 1) {
	continue;
    }
    return ret;
}


void* mempcpy(void* d, const void* s, unsigned c)
{
    unsigned char*	cd = d;
    const unsigned char* cs = s;
    
    for (cd = d, cs = s; c > 0; c -= 1, cd += 1, cs += 1) {
	*cd = *cs;
    }
    return cd;
}


void* memcpy(void* d, const void* s, unsigned c)
{
    unsigned char*	cd = d;
    const unsigned char* cs = s;
    
    for (cd = d, cs = s; c > 0; c -= 1, cd += 1, cs += 1) {
	*cd = *cs;
    }
    return d;
}


void* memset(void* d, int b, unsigned c)
{
    unsigned char*	cd;
    unsigned char	cb;
    
    for (cd = d, cb = b; c; c -= 1, cd += 1) {
	*cd = cb;
    }
    return d;
}


int strcasecmp(const char* c1, const char* c2)
{
    int			ret;
    
    do {
	char i1 = *c1 < 'a' || *c1 > 'z' ? *c1 : *c1 - ('a' - 'A');
	char i2 = *c2 < 'a' || *c2 > 'z' ? *c2 : *c2 - ('a' - 'A');
	ret = i1 - i2;
    } while (ret == 0 && *c1++ && *c2++);
    return ret;
}


char* strcpy(char* d, const char* s)
{
    char*		cs = d;

    for (cs = d; (*cs = *s); cs += 1, s += 1) {
	continue;
    }
    return d;
}


char* strchr(const char* s, int c)
{
    char		cb = c;

    for (cb = c; *s != cb; s += 1) {
	if (!*s) {
	    return (char*)0;
	}
    }
    return (char*)s;
}


char* strcat(char* c1, const char* c2)
{
    strcpy(strchr(c1, '\0'), c2);
    return c1;
}


int strcmp(const char* c1, const char* c2)
{
    int			ret;
    
    do {
	ret = *c1 - *c2;
    } while (ret == 0 && *c1++ && *c2++);
    return ret;
}


int strlen(const char* s)
{
    return strchr(s, '\0') - s;
}


int strncasecmp(const char* c1, const char* c2, unsigned c)
{
    int			ret;
    
    if (!c) {
	return 0;
    }
    do {
	char i1 = *c1 < 'a' || *c1 > 'z' ? *c1 : *c1 - ('a' - 'A');
	char i2 = *c2 < 'a' || *c2 > 'z' ? *c2 : *c2 - ('a' - 'A');
	ret = i1 - i2;
    } while (ret == 0 && *c1++ && *c2++ && --c);
    return ret;
}


char* strncpy(char* d, const char* s, unsigned c)
{
    char*		cd;

    for (cd = d; c && (*cd = *s); c -= 1, cd += 1, s += 1) {
	continue;
    }
    return d;
}


char* strncat(char* c1, const char* c2, unsigned c)
{
    char*		d = strchr(c1, '\0');
    int			l = d - c1;

    if (c > l) {
	strncpy(d, c2, c - l);
    }
    return c1;
}


int strncmp(const char* c1, const char* c2, unsigned c)
{
    int			ret;
    
    if (!c) {
	return 0;
    }
    do {
	ret = *c1 - *c2;
    } while (ret == 0 && *c1++ && *c2++ && --c);
    return ret;
}


char* strrchr(const char* s, int c)
{
    char		cb = c;
    const char*		r = (char*)0;

    do {
	if (*s == cb) {
	    r = s;
	}
    } while (*s++);
    return (char*)r;
}


char* strstr(const char* s, const char* n)
{
    for (; *s; s += 1) {
	unsigned	i;
	for (i = 0; n[i] == s[i] && n[i]; i += 1) {
	    continue;
	}
	if (n[i] == '\0') {
	    return (char*)s;
	}
    }
    return (char*)0;
}

/*
 * Now the __builtin string functions are done we can haul in the
 * definitions.
 */
#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/rwsem.h>
#include <ibcs-us/linux26-compat/linux/signal.h>
#include <ibcs-us/linux26-compat/linux/string.h>

#ifdef	UNIT_TEST
#undef	IBCS_SYSCALL
#define	IBCS_SYSCALL(a, args...)	mock_##a(args)
#define	down_write(x)			mock_down_write(x)
#define	up_write(x)			mock_up_write(x)

static void* mock_brk(void* addr);
static void mock_exit(int status);
static unsigned mock_write(int fd, char* buffer, unsigned size);
static void mock_down_write(struct rw_semaphore* s) {}
static void mock_up_write(struct rw_semaphore* s) {}
#endif

static const char *const ibcs_errno_desc[] = {
    [0]			= "Success",
    [EPERM]		= "Operation not permitted",
    [ENOENT]		= "No such file or directory",
    [ESRCH]		= "No such process",
    [EINTR]		= "Interrupted system call",
    [EIO]		= "Input/output error",
    [ENXIO]		= "No such device or address",
    [E2BIG]		= "Argument list too long",
    [ENOEXEC]		= "Exec format error",
    [EBADF]		= "Bad file descriptor",
    [ECHILD]		= "No child processes",
    [EDEADLK]		= "Resource deadlock avoided",
    [ENOMEM]		= "Cannot allocate memory",
    [EACCES]		= "Permission denied",
    [EFAULT]		= "Bad address",
    [ENOTBLK]		= "Block device required",
    [EBUSY]		= "Device or resource busy",
    [EEXIST]		= "File exists",
    [EXDEV]		= "Invalid cross-device link",
    [ENODEV]		= "No such device",
    [ENOTDIR]		= "Not a directory",
    [EISDIR]		= "Is a directory",
    [EINVAL]		= "Invalid argument",
    [EMFILE]		= "Too many open files",
    [ENFILE]		= "Too many open files in system",
    [ENOTTY]		= "Inappropriate ioctl for device",
    [ETXTBSY]		= "Text file busy",
    [EFBIG]		= "File too large",
    [ENOSPC]		= "No space left on device",
    [ESPIPE]		= "Illegal seek",
    [EROFS]		= "Read-only file system",
    [EMLINK]		= "Too many links",
    [EPIPE]		= "Broken pipe",
    [EDOM]		= "Numerical argument out of domain",
    [ERANGE]		= "Numerical result out of range",
    [EAGAIN]		= "Resource temporarily unavailable",
    [EINPROGRESS]	= "Operation now in progress",
    [EALREADY]		= "Operation already in progress",
    [ENOTSOCK]		= "Socket operation on non-socket",
    [EMSGSIZE]		= "Message too long",
    [EPROTOTYPE]	= "Protocol wrong type for socket",
    [ENOPROTOOPT]	= "Protocol not available",
    [EPROTONOSUPPORT]	= "Protocol not supported",
    [ESOCKTNOSUPPORT]	= "Socket type not supported",
    [EOPNOTSUPP]	= "Operation not supported",
    [EPFNOSUPPORT]	= "Protocol family not supported",
    [EAFNOSUPPORT]	= "Address family not supported by protocol",
    [EADDRINUSE]	= "Address already in use",
    [EADDRNOTAVAIL]	= "Cannot assign requested address",
    [ENETDOWN]		= "Network is down",
    [ENETUNREACH]	= "Network is unreachable",
    [ENETRESET]		= "Network dropped connection on reset",
    [ECONNABORTED]	= "Software caused connection abort",
    [ECONNRESET]	= "Connection reset by peer",
    [ENOBUFS]		= "No buffer space available",
    [EISCONN]		= "Transport endpoint is already connected",
    [ENOTCONN]		= "Transport endpoint is not connected",
    [EDESTADDRREQ]	= "Destination address required",
    [ESHUTDOWN]		= "Cannot send after transport endpoint shutdown",
    [ETOOMANYREFS]	= "Too many references: cannot splice",
    [ETIMEDOUT]		= "Connection timed out",
    [ECONNREFUSED]	= "Connection refused",
    [ELOOP]		= "Too many levels of symbolic links",
    [ENAMETOOLONG]	= "File name too long",
    [EHOSTDOWN]		= "Host is down",
    [EHOSTUNREACH]	= "No route to host",
    [ENOTEMPTY]		= "Directory not empty",
    [EUSERS]		= "Too many users",
    [EDQUOT]		= "Disk quota exceeded",
    [ESTALE]		= "Stale file handle",
    [EREMOTE]		= "Object is remote",
    [ENOLCK]		= "No locks available",
    [ENOSYS]		= "Function not implemented",
    [EILSEQ]		= "Invalid or incomplete multibyte or wide character",
    [EBADMSG]		= "Bad message",
    [EIDRM]		= "Identifier removed",
    [EMULTIHOP]		= "Multihop attempted",
    [ENODATA]		= "No data available",
    [ENOLINK]		= "Link has been severed",
    [ENOMSG]		= "No message of desired type",
    [ENOSR]		= "Out of streams resources",
    [ENOSTR]		= "Device not a stream",
    [EOVERFLOW]		= "Value too large for defined data type",
    [EPROTO]		= "Protocol error",
    [ETIME]		= "Timer expired",
    [ECANCELED]		= "Operation canceled",
    [EOWNERDEAD]	= "Owner died",
    [ENOTRECOVERABLE]	= "State not recoverable",
    [ERESTART]		= "Interrupted system call should be restarted",
    [ECHRNG]		= "Channel number out of range",
    [EL2NSYNC]		= "Level 2 not synchronized",
    [EL3HLT]		= "Level 3 halted",
    [EL3RST]		= "Level 3 reset",
    [ELNRNG]		= "Link number out of range",
    [EUNATCH]		= "Protocol driver not attached",
    [ENOCSI]		= "No CSI structure available",
    [EL2HLT]		= "Level 2 halted",
    [EBADE]		= "Invalid exchange",
    [EBADR]		= "Invalid request descriptor",
    [EXFULL]		= "Exchange full",
    [ENOANO]		= "No anode",
    [EBADRQC]		= "Invalid request code",
    [EBADSLT]		= "Invalid slot",
    [EBFONT]		= "Bad font file format",
    [ENONET]		= "Machine is not on the network",
    [ENOPKG]		= "Package not installed",
    [EADV]		= "Advertise error",
    [ESRMNT]		= "Srmount error",
    [ECOMM]		= "Communication error on send",
    [EDOTDOT]		= "RFS specific error",
    [ENOTUNIQ]		= "Name not unique on network",
    [EBADFD]		= "File descriptor in bad state",
    [EREMCHG]		= "Remote address changed",
    [ELIBACC]		= "Can not access a needed shared library",
    [ELIBBAD]		= "Accessing a corrupted shared library",
    [ELIBSCN]		= ".lib section in a.out corrupted",
    [ELIBMAX]		= "Attempting to link in too many shared libraries",
    [ELIBEXEC]		= "Cannot exec a shared library directly",
    [ESTRPIPE]		= "Streams pipe error",
    [EUCLEAN]		= "Structure needs cleaning",
    [ENOTNAM]		= "Not a XENIX named type file",
    [ENAVAIL]		= "No XENIX semaphores available",
    [EISNAM]		= "Is a named type file",
    [EREMOTEIO]		= "Remote I/O error",
    [ENOMEDIUM]		= "No medium found",
    [EMEDIUMTYPE]	= "Wrong medium type",
    [ENOKEY]		= "Required key not available",
    [EKEYEXPIRED]	= "Key has expired",
    [EKEYREVOKED]	= "Key has been revoked",
    [EKEYREJECTED]	= "Key was rejected by service",
    [ERFKILL]		= "Operation not possible due to RF-kill",
    [EHWPOISON]		= "Memory page has hardware error",
};


/*
 * Print an OS error and die.
 */
void ibcs_fatal(const char* message, ...)
{
    va_list		list;

    va_start(list, message);
    ibcs_vwritef(2, message, list);
    va_end(list);
    ibcs_writef(2, "\n");
    IBCS_SYSCALL(exit, 1);
}


/*
 * Print an OS error and die.
 */
void ibcs_fatal_syscall(int retval, const char* message, ...)
{
    int			errno;
    va_list		list;

    errno = -retval;
    va_start(list, message);
    ibcs_vwritef(2, message, list);
    va_end(list);
    if (
	errno >= sizeof(ibcs_errno_desc) / sizeof(ibcs_errno_desc[0]) ||
	ibcs_errno_desc[errno] == (char*)0
    ) {
	ibcs_writef(2, ": Unknown error %d.\n", errno);
    } else {
	ibcs_writef(2, ": %s.\n", ibcs_errno_desc[errno]);
    }
    IBCS_SYSCALL(exit, 1);
}


/*
 * Write at most 'size' chars of printf formatted null terminated string to
 * 'out'.   'size' includes the null. Return the number of characters written,
 * including the NULL.  It 'out' is NULL no characters are written, and size 0
 * means no size limit.
 */
int ibcs_vfmt(char* out, size_t size, const char* fmt, va_list list)
{
    const char*		b;
    char		buf[20];
    const char*		f;
    const char*		f_start;
    int			fitted_size;
    int			formatted_size;
    int			is_long;
    int			left_justify;
    int			max_width;
    int			min_width;
    char		pad_char;
    int			osize;
    
    if (out == (char*)0 && size == 0) {
	size = (unsigned)-1 >> 1;
    }
    if (size <= 1) {
	return 0;
    }
    osize = size;
    size -= 1;			/* Allow for the mandatory trailing null */
    for (; size > 0 && *fmt != '\0'; fmt = f) {
	/*
	 * Copy everything up to the next '%'.
	 */
	f = strchr(fmt, '%');
	if (!f) {
	    f = strchr(fmt, '\0');
	}
	if (f != fmt) {
	    formatted_size = f - fmt;
	    fitted_size = formatted_size > size ? size : formatted_size;
	    if (out != (char*)0) {
		out = mempcpy(out, fmt, fitted_size);
	    }
	    size -= fitted_size;
	    fmt = f;
	}
	if (size == 0 || *f != '%') {
	    continue;
	}
	f_start = f;
	f += 1;
	/*
	 * Process left justify flag.
	 */
	left_justify = *f == '-';
	if (left_justify) {
	    f += 1;
	}
	/*
	 * Process the '[max_width][.min_width]'.
	 */
	if (*f < '0' || *f > '9') {
	    min_width = 0;
	    pad_char = ' ';
	} else {
	    pad_char = *f == '0' ? '0' : ' ';
	    min_width = 0;
	    do {
		min_width = min_width * 10 + *f++ - '0';
	    } while (*f >= '0' && *f <= '9');
	}
	if (*f != '.') {
	    max_width = 999999999;
	} else {
	    max_width = 0;
	    f += 1;
	    while (*f >= '0' && *f <= '9') {
		max_width = max_width * 10 + *f++ - '0';
	    }
	}
	/*
	 * Process the 'l's.
	 */
	is_long = 0;
	while (*f == 'l' && is_long < 0x200) {
	    is_long += 0x100;
	    f += 1;
	}
	/*
	 * Process the format char.
	 */
	switch ((int)*f | is_long) {
	    case 'c':
		buf[0] = (char)va_arg(list, int);
		formatted_size = 1;
		b = buf;
		break;
	    case 'd':
	    case 'd' | 0x100:
	    case 'u':
	    case 'u' | 0x100:
		{
		    char* bb = &buf[sizeof(buf)];
		    unsigned n = va_arg(list, unsigned);
		    int minus = (*f & 0xff) == 'd' && (int)n < 0;
		    if (minus) {
			n = (unsigned)-(int)n;
		    }
		    *--bb = '0' + (n % 10);
		    while (n /= 10) {
			*--bb = '0' + n % 10;
		    }
		    if (minus) {
			*--bb = '-';
		    }
		    b = bb;
		    formatted_size = &buf[sizeof(buf)] - b;
		}
		f += 1;
		break;
	    case 'o':
	    case 'o' | 0x100:
	    case 'o' | 0x200:
		{
		    char* bb = &buf[sizeof(buf)];
		    unsigned long long n = is_long <= 0x100 ?
			va_arg(list, unsigned) : va_arg(list, unsigned long long);
		    do {
			*--bb = (n & 0x7) + '0';
		    } while (n >>= 3);
		    b = bb;
		    formatted_size = &buf[sizeof(buf)] - b;
		}
		f += 1;
		break;
	    case 's':
		{
		    b = va_arg(list, const char*);
		    formatted_size = strlen(b);
		}
		f += 1;
		break;
	    case 'p':
	    case 'x':
	    case 'x' | 0x100:
	    case 'x' | 0x200:
	    case 'X':
	    case 'X' | 0x100:
	    case 'X' | 0x200:
		{
		    char* bb = &buf[sizeof(buf)];
		    unsigned long long n = is_long <= 0x100 ?
			va_arg(list, unsigned) : va_arg(list, unsigned long long);
		    char a = ((*f & 0xff) == 'x' ? 'a' : 'A') - 10;
		    do {
			unsigned h = n & 0xf;
			*--bb = h + (h < 10 ? '0' : a);
		    } while (n >>= 4);
		    b = bb;
		    formatted_size = &buf[sizeof(buf)] - b;
		}
		f += 1;
		break;
	    case '\0':
		b = fmt;
		formatted_size = f - fmt;
		break;
	    default:
		b = f_start;
		formatted_size = ++f - f_start;
		break;
	}
	if (formatted_size > max_width) {
	    formatted_size = max_width;
	}
	/*
	 * Copy the formatted result to the output, padding as appropriate.
	 */
	int padding = formatted_size >= min_width ? 0 : min_width - formatted_size;
	if (left_justify) {
	    fitted_size = formatted_size > size ? size : formatted_size;
	    if (out != (char*)0) {
		out = mempcpy(out, b, fitted_size);
	    }
	    size -= fitted_size;
	    if (padding > size) {
		padding = size;
	    }
	    if (padding > 0) {
		if (out != (char*)0) {
		    memset(out, pad_char, padding);
		    out += padding;
		}
		size -= padding;
	    }
	} else {
	    if (padding > size) {
		padding = size;
	    }
	    if (padding > 0) {
		if (out != (char*)0) {
		    memset(out, pad_char, padding);
		    out += padding;
		}
		size -= padding;
	    }
	    fitted_size = formatted_size > size ? size : formatted_size;
	    if (out != (char*)0) {
		out = mempcpy(out, b, fitted_size);
	    }
	    size -= fitted_size;
	}
    }
    if (out != (char*)0) {
	*out = '\0';
    }
    return osize - size;
}


/*
 * Write a formatted string to fd.
 */
int ibcs_writef(int fd, const char* fmt, ...)
{
    va_list		list;

    va_start(list, fmt);
    int ret = ibcs_vwritef(fd, fmt, list);
    va_end(list);
    return ret;
}


/*
 * Write a formatted string to fd - variadic version.
 */
int ibcs_vwritef(int fd, const char* fmt, va_list list)
{
    int n = ibcs_vfmt((char*)0, 0, fmt, list);
    char buffer[n];
    n = ibcs_vfmt(buffer, n, fmt, list);
    return IBCS_SYSCALL(write, fd, buffer, n - 1);
}


/*
 * Syscall implementations.
 */
long long ibcs_vsyscall(int syscall_no, va_list args)
{
    long long		ret;
    /*
     * This is a weird way of doing it, but it turns out gcc asm() handling
     * is full of bugs.  This is the only way I could find that works at all
     * -O levels.
     */
    asm volatile (
	"push	%%ebp\n\t"
	"mov	%0,%%ebp\n\t"
	"mov	-4(%%ebp),%%eax\n\t"
	"mov	0(%%ebp),%%ebx\n\t"
	"mov	4(%%ebp),%%ecx\n\t"
	"mov	8(%%ebp),%%edx\n\t"
	"mov	12(%%ebp),%%esi\n\t"
	"mov	16(%%ebp),%%edi\n\t"
	"mov	20(%%ebp),%%ebp\n\t"
	"int	$128\n\t"
	"pop	%%ebp\n\t"
	: "=A" (ret)
	: "r" (args)
	: "ebx", "ecx", "esi", "edi", "memory"	/* Clobbers */);
    return ret;
}


/*
 * We have to implement our own syscall().
 */
long long ibcs_syscall(int syscall_no, ...)
{
    long long		ret;
    va_list		args;

    va_start(args, syscall_no);
    ret = ibcs_vsyscall(syscall_no, args);
    va_end(args);
    return ret;
}


/*
 * This is a bog standard malloc implmentation - slow but compact.  Compact
 * means small, small means less chance of bugs.
 *
 * It keeps all blocks (free or otherwise) is a circular singularly linked
 * list in ascending address order, with ibcs_malloc_first pointing to
 * block with the lowest address and ibcs_malloc_last pointing to the
 * highest.
 *
 * Blocks are flagged as not being free by having the low order address bit
 * (IBCS_MALLOC_BUSY) set.  Free blocks are coalesced as we search the list
 * for free space in ibcs_malloc().   They might "not be free" for several
 * reasons.  They could be allocated by ibcs_malloc(), but they could also
 * be gaps in the chain created by others calling brk(), or they could be
 * the end of the chain marker.
 */
#define	IBCS_MALLOC_BLOCK (PAGE_SIZE * 2);

struct ibcs_malloc_hdr
{
    struct ibcs_malloc_hdr*	next;
};

#define	IBCS_MALLOC_BUSY	1
#define	IBCS_MALLOC_CLR_BUSY(p)	((struct ibcs_malloc_hdr*)((size_t)(p) & ~IBCS_MALLOC_BUSY))
#define	IBCS_MALLOC_SET_BUSY(p)	((struct ibcs_malloc_hdr*)((size_t)(p) | IBCS_MALLOC_BUSY))
#define IBCS_MALLOC_IS_BUSY(p)	((size_t)((p)->next) & IBCS_MALLOC_BUSY)

static struct ibcs_malloc_hdr*	ibcs_malloc_first;
static struct ibcs_malloc_hdr*	ibcs_malloc_last;
static struct ibcs_malloc_hdr*	ibcs_malloc_p;
static struct rw_semaphore	ibcs_malloc_semaphore = { 1 };


void* ibcs_malloc(size_t wanted)
{
    size_t			blk_size;
    struct ibcs_malloc_hdr*	n;
    struct ibcs_malloc_hdr*	p;

    /*
     * Translate wanted into an intergral number of struct ibcs_malloc_hdrs,
     * plus we because of there is a struct ibcs_malloc_hdrs at the front
     * of the block.
     */
    wanted =
	(wanted + sizeof(struct ibcs_malloc_hdr) - 1) /
	sizeof(struct ibcs_malloc_hdr) + 1;
    /*
     * Initialise ourselves if this is the 1st time through here.
     */
    down_write(&ibcs_malloc_semaphore);
    if (ibcs_malloc_p == (struct ibcs_malloc_hdr*)0) {
	p = (struct ibcs_malloc_hdr*)(size_t)IBCS_SYSCALL(brk, 0);
	ibcs_malloc_first = p;
	ibcs_malloc_last = p;
	ibcs_malloc_p = p;
    } else {
	/*
	 * If we been scan every allocated block looking for a free one of the
	 * right size.
	 */
	p = ibcs_malloc_p;
	do {
	    if (!IBCS_MALLOC_IS_BUSY(p)) {
		/*
		 * Coalesce free blocks.
		 */
		while (!IBCS_MALLOC_IS_BUSY(p->next)) {
		    p->next = p->next->next;
		}
		/*
		 * If this block is large enough we are done.
		 */
		blk_size = p->next - p;
		if (blk_size >= wanted) {
		    /*
		     * If it's too big split it.
		     */
		    if (blk_size > wanted) {
			p[wanted].next = p->next;
			p->next = &p[wanted];
		    }
		    ibcs_malloc_p = p->next;
		    p->next = IBCS_MALLOC_SET_BUSY(p->next);
		    goto out;
		}
	    }
	    p = IBCS_MALLOC_CLR_BUSY(p->next);
	} while (p != ibcs_malloc_p);
	p = (struct ibcs_malloc_hdr*)(size_t)IBCS_SYSCALL(brk, 0);
    }
    /*
     * We didn't find a block.  Allocate a new one.  Allow space for sticking
     * a header at the end we so know the allocated blocks size.
     */
    if (IBCS_IS_ERR(p)) {
	ibcs_fatal_syscall((int)p, "ibcs_malloc brk()");
    }
    /*
     * If this new block is adjecent to the previous one if can become part
     * of it, otherwise link it into the chain.
     */
    if (&ibcs_malloc_last[1] == p) {
	p = ibcs_malloc_last;
    }
    /*
     * Ask the OS to give us the memory.
     */
    n = (struct ibcs_malloc_hdr*)(size_t)IBCS_SYSCALL(brk, p + wanted + 1);
    if (IBCS_IS_ERR(n)) {
	ibcs_fatal_syscall(
	    (int)p, "ibcs_malloc brk() for %d bytes failed",
	    (wanted + 1) * sizeof(*p));
    }
    n -= 1;
    blk_size = n - p;
    if (blk_size < wanted) {
	ibcs_fatal_syscall(
	    (int)p, "ibcs_malloc call of brk() returned %d, expecting %d",
	    blk_size * sizeof(*p), wanted * sizeof(*p));
    }
    /*
     * Add this block to the circular linked list of blocks.
     */
    ibcs_malloc_last->next = IBCS_MALLOC_SET_BUSY(n);
    p->next = IBCS_MALLOC_SET_BUSY(n);
    n->next = IBCS_MALLOC_SET_BUSY(ibcs_malloc_first);
    ibcs_malloc_last = n;
out:
    up_write(&ibcs_malloc_semaphore);
    return (void*)&p[1];
}


/*
 * Mark the block as available.
 */
void ibcs_free(void* blk)
{
    struct ibcs_malloc_hdr*	p = (struct ibcs_malloc_hdr*)blk - 1;
    p->next = IBCS_MALLOC_CLR_BUSY(p->next);
    ibcs_malloc_p = p;
}


/*
 * The real Linux API requires a sa_restorer to given to the kernel,
 * who sole job (as far as the kernel is concerned) is to call the
 * rt_sigreturn syscall.  glibc uses it to do fancy nancy things.
 */
extern void ibcs_sigrestorer(void);
void _ibcs_sigrestorer_dummy(void)
{
#define IBCS_STRINGIFY_(x)	#x
#define IBCS_STRINGIFY(x)	IBCS_STRINGIFY_(x)
    asm(
	"ibcs_sigrestorer:\n"
	"   mov $" IBCS_STRINGIFY(__NR_rt_sigreturn) ", %eax\n"
	"   int $128\n"
    );
}

int ibcs_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact)
{
    struct sigaction	kact = *act;

    kact.sa_flags |= SA_RESTORER;
    kact.sa_restorer = ibcs_sigrestorer;
    /*
     * man rt_signal(2) says you pass sizeof(sigset_t) or equivalently
     * sizeof(kact.sa_mask) here.  It's wrong.  _NSIG / 8 is the magic
     * number it wants.  Anything else gets you an EINVAL.
     */
    return IBCS_SYSCALL(rt_sigaction, sig, &kact, oldact, _NSIG / 8);
}


/*
 * Unit testing code.  This is run using "make tests" at the top level or
 * "make test-FILENAME" in the component directory.
 */
#ifdef  UNIT_TEST
#define	_SYS_TYPES_H	1
#define	mode_t		_unwanted_mode_t
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int current_test_line_nr;
#define	MUST_BE_TRUE(line_nr, a)							\
    do { 								\
	current_test_line_nr = line_nr;					\
	if (!(a)) { 							\
	    fprintf(stderr, "%s line %d test failed, %s\n", __FILE__, line_nr, #a);	\
	    abort();							\
	}								\
    } while(0)

static void mock_exit(int status)
{
    MUST_BE_TRUE(current_test_line_nr, 0 == mock_exit - 1);
}

static struct { int fd; char* buffer; int size;} mock_write_result;
static unsigned mock_write(int fd, char* buffer, unsigned int size)
{
    mock_write_result.fd = fd;
    mock_write_result.buffer = buffer;
    mock_write_result.size = size;
    return size;
}

static struct { char* addr; char buffer[4096];} mock_brk_state;
static void* mock_brk(void* addr)
{
    if (mock_brk_state.addr == (void*)0) {
	mock_brk_state.addr = mock_brk_state.buffer;
    }
    if (addr != (void*)0) {
	MUST_BE_TRUE(current_test_line_nr, (char*)addr >= mock_brk_state.addr);
	MUST_BE_TRUE(current_test_line_nr, (char*)addr <= mock_brk_state.buffer + sizeof(mock_brk_state.buffer));
	mock_brk_state.addr = (char*)addr;
    }
    return mock_brk_state.addr;
}

/*
 * Syscall's.
 */
static void test_ibcs_syscall()
{
    char	name_in[64];
    char	name_out[64];
    char	buf[4096];

    MUST_BE_TRUE(__LINE__, (int)ibcs_syscall(__NR_getpid) == getpid());
    open("/dev/non-existant-file-name", 0);
    MUST_BE_TRUE(__LINE__, (int)ibcs_syscall(__NR_open, "/dev/non-existant-file-name", 0) == -errno);
    /*
     * The syscall() copy_file_range() has a lot of parameters, so it makes
     * a good test case.
     */
    sprintf(name_out, "/tmp/test-ibcs-lib-%d-tmp.out", getpid());
    int handle_out = open(name_out, O_RDWR|O_TRUNC|O_CREAT, 0666);
    write(handle_out, "abcd", 4);
    sprintf(name_in, "/tmp/test-ibcs-lib-%d-tmp.in", getpid());
    int handle_in = open(name_in, O_RDWR|O_TRUNC|O_CREAT, 0666);
    write(handle_in, "wxyz", 4);
    loff_t loff_in = 0;
    loff_t loff_out = 1;
    int ret = ibcs_syscall(
	__NR_copy_file_range, handle_in, &loff_in, handle_out, &loff_out, 3, 0);
    MUST_BE_TRUE(__LINE__, ret == 3);
    lseek(handle_out, 0L, 0);
    read(handle_out, buf, 4);
    MUST_BE_TRUE(__LINE__, !memcmp(buf, "awxy", 4));
    close(handle_in);
    close(handle_out);
    unlink(name_out);
    unlink(name_in);
}

static void check_malloc_chain(int line_nr)
{
    struct ibcs_malloc_hdr*	p;

    for (
	p = ibcs_malloc_first;
	p < IBCS_MALLOC_CLR_BUSY(p->next);
	p = IBCS_MALLOC_CLR_BUSY(p->next)
    ) {
	continue;
    }
    MUST_BE_TRUE(line_nr, p == IBCS_MALLOC_CLR_BUSY(ibcs_malloc_last));
    MUST_BE_TRUE(line_nr, p->next == IBCS_MALLOC_SET_BUSY(ibcs_malloc_first));
}

static void test_ibcs_malloc(int line_nr, ...)
{
    int			a;
    va_list		args;
    int			alloc_count;
    void*		mallocs[100];

    memset(mallocs, '\0', sizeof(mallocs));
    mock_brk_state.addr = 0;
    va_start(args, line_nr);
    a = va_arg(args, int);
    ibcs_malloc_first = (struct ibcs_malloc_hdr*)0;
    ibcs_malloc_last = (struct ibcs_malloc_hdr*)0;
    ibcs_malloc_p = (struct ibcs_malloc_hdr*)0;
    alloc_count = 0;
    while (a != (int)(~0U >> 1)) {
	alloc_count += 1;
	if (a >= 0) {
	    mallocs[alloc_count] = ibcs_malloc(a);
	    int i = 0;
	    while (++i < alloc_count) {
		MUST_BE_TRUE(line_nr, mallocs[i] != mallocs[alloc_count]);
	    }
	} else {
	    MUST_BE_TRUE(line_nr, -a < alloc_count);
	    MUST_BE_TRUE(line_nr, mallocs[-a] != (void*)0);
	    MUST_BE_TRUE(line_nr, IBCS_MALLOC_IS_BUSY(((struct ibcs_malloc_hdr*)mallocs[-a]) -1));
	    ibcs_free(mallocs[-a]);
	    mallocs[-a] = (void*)0;
	}
	check_malloc_chain(line_nr);
	a = va_arg(args, int);
    }
    /*
     * All blocks must combined on exit.
     */
    MUST_BE_TRUE(
	line_nr,
	IBCS_MALLOC_CLR_BUSY(ibcs_malloc_first->next) == ibcs_malloc_last);
}

void unit_test_ibcs_ibcs_lib_c()
{
    test_ibcs_syscall();
    /*
     * +n calls malloc(n), -n call free() in the n'th malloc().  The end
     * result must leave one block on the chain (thus proving blockes were
     * combined as expected).
     */
    test_ibcs_malloc(__LINE__, 1, -1, ~0U>>1);
    test_ibcs_malloc(__LINE__, 1, 1, -1, -2, 8, -5, 12, ~0U>>1);
    test_ibcs_malloc(__LINE__, 24, -1, 1, 1, 1, -3, -4, -5, 24, ~0U>>1);
}
#endif
