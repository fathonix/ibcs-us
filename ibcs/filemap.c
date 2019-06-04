/*
 * Implement the ibcs --map option.
 */
#include <ibcs-us/ibcs/filemap.h>
#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/fcntl.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/limits.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/string.h>

#ifdef UNIT_TEST
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#undef	IBCS_SYSCALL
#define IBCS_SYSCALL(a, args...)	mock_##a(args)
#undef	abi_printk
#define	abi_printk(f, msg, args...)	mock_abi_printk(f, msg, args)
#define	ibcs_malloc(x)			malloc(x)
#define	ibcs_fatal(m, a...)		mock_ibcs_fatal(m, a)
#define	ibcs_free(x)			free(x)
#define	linux26_fopen(name, mode)	mock_linux26_fopen(name, mode)
#define	linux26_fclose(file)		mock_linux26_fclose(file)
#define	vfs_fstat(fd, kst)		mock_vfs_fstat(fd, kst)

static void mock_abi_printk(unsigned flgs, const char* message, ...);
static void mock_ibcs_fatal(const char* message, ...);
static int mock_linux26_fclose(struct file* f);
static struct file* mock_linux26_fopen(const char* name, int mode);
static int mock_vfs_fstat(int fd, struct kstat* kst);


static const char* mock_getcwd_cwd;
static int mock_getcwd_retval;
static int mock_getcwd(char* buf, int len)
{
    strcpy(buf, mock_getcwd_cwd);
    int i = mock_getcwd_retval;
    mock_getcwd_retval = 0;
    return i;
}
#endif

struct filemap
{
    unsigned short	from_off;
    unsigned short	to_off;
    unsigned short	next_off;
} __attribute__((packed));

static struct filemap*	filemap;


/*
 * Copy src to dst, removing redundant "//", "/." and "/.." sequences.
 * If is_abs is true convert src to an absolute path name.  dst must have
 * at least dst_size chars available.  On success the return value will
 * be the length of the result including the trailing null which will
 * always be greater than 1.
 */
static int normalise_path(char* dst, size_t dst_size, const char* src, int is_abs)
{
    const char*		s = src;
    char*		d = dst;
    
    /*
     * If an absolute path is wanted and src isn't absolute start with 
     * the current working directory name.
     */
    if (*s == '/') {
	*d++ = *s++;
    } else if (is_abs) {
        int ret = IBCS_SYSCALL(getcwd, d, dst_size);
	if (IBCS_IS_ERR(ret)) {
	    return ret;
	}
	d = strchr(d, '\0');
	if (d - dst + 2 >= dst_size) {
	    return -ENAMETOOLONG;
	}
	*d++ = '/';
    }
    /*
     * Each interation processes one path component in src.
     */
    while (*s) {
	/*
	 * Eliminate consequtive '/'s.
	 */
	if (s[0] == '/') {
	    s += 1;
	    continue;
	}
	/*
	 * Eliminate "/.".
	 */
	if (s[0] == '.' && s[1] == '/') {
	    s += 2;
	    continue;
	}
	if (s[0] == '.' && s[1] == '\0') {
	    s += 1;
	    continue;
	}
	/*
	 * Handle "/..".
	 */
	if (s[0] == '.' && s[1] == '.' && (s[2] == '/' || s[2] == '\0')) {
	    /*
	     * If the previous component is "../" then we can't get rid of it.
	     */
	    if (
		d == dst ||
		(d == dst + 3 && d[-2] == '.' && d[-3] == '.') ||
		(d >= dst + 4 && d[-2] == '.' && d[-3] == '.' && d[-4] == '/')
	    ) {
		if (d - dst + 3 >= dst_size) {
		    return -ENAMETOOLONG;
		}
		*d++ = s[0];
		*d++ = s[1];
		if (s[2] != '\0') {
		    *d++ = '/';
		}
	    }
	    else if (d != dst + 1) {
		/*
		 * It's of the form "name/..", so erase "name".
		 */
		for (d -= 1; d > dst && d[-1] != '/'; d -= 1) {
		    continue;
		}
	    }
	    s += s[2] == '\0' ? 2 : 3;
	    continue;
	}
	/*
	 * A normal file name component.  Copy it, if there is room.
	 */
	for (; (*d = *s) != '\0'; d += 1, s += 1) {
	    if (d - dst + 1 >= dst_size) {
		return -ENAMETOOLONG;
	    }
	    if (*s == '/') {
		d += 1;
		s += 1;
		break;
	    }
	}
    }
    if (d == dst) {
	*d++ = '.';
    } else if (d != dst + 1 && d[-1] == '/') {
	d -= 1;
    }
    *d++ = '\0';
    return d - dst;
}


/*
 * Some string crap.
 */
#define IS_BLANK(c)		({char cc = (c); cc == ' ' || cc == '\t';})

#define SKIP_STR(str, literal)						\
    ({									\
	char* s = (str);						\
	const size_t z = sizeof(literal) - 1;				\
	(char*)(!strncmp(s, literal, z) ? s + z : 0);			\
    })

#define	TYPE_ALIGN(ptr, type)						\
    ((void*)(((size_t)ptr + sizeof(type) - 1) & ~(sizeof(type) - 1)))

static char* skip_blanks(const char* b)
{
    while (IS_BLANK(*b)) {
	b += 1;
    }
    return (char*)b;
}


/*
 * Read in a filemap.  The format of the lines is:
 *
 *     # comment
 *     prefixmap: /from/path --> /to/path
 *
 * Comments and blank lines are ignored.  The "-->" must be surrounded by
 * white space.  Leading and trailing spaces are stripped from the paths.
 * All paths must be absolute, and start with a "/".  If /from/path/ ends
 * with a slash and is a directory name matches the files under the directory
 * but not the directory itself.
 */
int filemap_parse(const char* map_filename)
{
    char*		b;
    int			bad_line_no;
    char*		buffer;
    char*		c;
    char*		end;
    char*		end_line;
    char		from_end_char;
    struct kstat	kst;
    int			line_no;
    struct filemap*	map;
    struct file*	mapfile;
    char*		next_line;
    loff_t		off;
    struct filemap*	prev;
    int			ret;

    buffer = (char*)0;
    mapfile = (struct file*)0;
    mapfile = linux26_fopen(map_filename, O_RDONLY);
    bad_line_no = 0;
    filemap = (struct filemap*)0;
    if (IBCS_IS_ERR(mapfile)) {
	ret = (int)mapfile;
	goto out;
    }
    ret = vfs_fstat(mapfile->fd, &kst);
    if (IBCS_IS_ERR(ret)) {
	goto out;
    }
    if (kst.size == 0) {
	goto out;
    }
    /*
     * This way this file is parsed is kinky to avoid memory allocations.
     * The entire file is read in, then it is converted into place to a
     * linked list of struct filemap.  This is possible before the token
     * "prefixmap:" was chosen so that:
     *     strlen("prefixmap:") >= sizeof(struct filemap),
     * naturally because that makes it big enough to safely replace it
     * with a struct filemap.  The path strings are then null terminated and
     * used in place.
     *
     * Step one is to read the file into memory and null terminate it.
     */
    buffer = (char*)ibcs_malloc(kst.size + 1);
    if (IBCS_IS_ERR(buffer)) {
	goto out;
    }
    off = 0;
    ret = mapfile->f_op->read(mapfile, (char*)buffer, kst.size, &off);
    if (IBCS_IS_ERR(ret)) {
	goto out;
    }
    buffer[kst.size] = '\0';
    /*
     * Loop for each line in the file.
     */
    line_no = 0;
    prev = 0;
    for (b = buffer; b; b = next_line) {
	end_line = strchr(b, '\n');
	if (end_line != (char*)0) {
	    next_line = end_line + 1;
	} else {
	    end_line = strchr(b, '\0');
	    next_line = (char*)0;
	}
	line_no += 1;
	/*
	 * Ignore blank lines and comments.
	 */
	b = skip_blanks(b);
	if (*b == '#' || b == end_line) {
	    continue;
	}
	/*
	 * Now parse "^prefixmap: /from/path --> /to/path",
	 * position is at the ^.
	 */
	end = SKIP_STR(b, "prefixmap:");
	if (!end) {
	    bad_line_no = line_no;
	    goto bad_line;
	}
	/*
	 * Overwrite "prefixmap:" with a "struct filemap".
	 */
	map = (struct filemap*)TYPE_ALIGN(b, typeof(map->from_off));
	/*
	 * Parse "prefixmap:^ /from/path --> /to/path" - looking for the "-->"
	 * sourrounded by blanks.  Null terminate /from/path.
	 */
	b = skip_blanks(end);
	if (*b != '/') {
	    bad_line_no = line_no;
	    goto bad_line;
	}
	map->from_off = b - (char*)map;
	for (c = b; ; c += 3) {
	    c = strstr(c, "-->");
	    if (c == (char*)0 || c >= end_line) {
		bad_line_no = line_no;
		goto bad_line;
	    }
	    if (IS_BLANK(c[-1]) && IS_BLANK(c[3])) {
		break;
	    }
	}
	for (b = c - 1; IS_BLANK(*b); b -= 1) {
	    continue;
	}
	from_end_char = *b;
	*++b = '\0';
	/*
	 * Parse "prefixmap: /from/path --> ^ /to/path".  Null terminate
	 * /to/path.
	 */
	b = skip_blanks(c + 4);
	if (*b != '/') {
	    bad_line_no = line_no;
	    goto bad_line;
	}
	c = b;
	map->to_off = b - (char*)map;
	for (b = end_line - 1; *b == '\r' || IS_BLANK(*b); b -= 1) {
	    continue;
	}
	if ((from_end_char == '/' || *b == '/') && *b != from_end_char) {
	    bad_line_no = line_no;
	    goto bad_line;
	}
	*++b = '\0';
	/*
	 * Add the new map into the chain.
	 */
	map->next_off = 0;
	if (prev == (struct filemap*)0) {
	    filemap = map;
	} else {
	    prev->next_off = (char*)map - (char*)prev;
	}
	prev = map;
    }

bad_line:
out:
    if (mapfile != (struct file*)0) {
	linux26_fclose(mapfile);
    }
    if (IBCS_IS_ERR(ret) || bad_line_no != 0) {
	filemap = (struct filemap*)0;
    }
    if (filemap == (struct filemap*)0 && buffer != (char*)0) {
	ibcs_free(buffer);
    }
    if (IBCS_IS_ERR(ret)) {
	ibcs_fatal_syscall(ret, "%s", map_filename);
    } else if (bad_line_no != 0) {
	ibcs_fatal("%s line %d: can't parse line", map_filename, bad_line_no);
	ret = -EILSEQ;
    }
    return IBCS_IS_ERR(ret) ? ret : 0;
}


/*
 * Convert path to an absolute filename, then see if there is a matching
 * mapping in the filemap.  If there is no match put absolue filename into
 * dst and return 0, if there is a map put the translated filename into dst
 * and return the length of the resulting path including the tailing null.
 * This can fail if the resulting path would exceed dst_size.   dst must
 * be at least dst_size long.
 */
int filemap_map(char* dst, size_t dst_size, const char* path)
{
    struct filemap*	map;
    const char*		c;
    char*		d;
    int			ret;

    ret = normalise_path(dst, dst_size, path, 1);
    if (IBCS_IS_ERR(ret) || filemap == (struct filemap*)0) {
	return 0;
    }
    c = (char*)0;
    for (map = filemap; map; map = (struct filemap*)((char*)map + map->next_off)) {
	c = (char*)map + map->from_off;
	for (d = dst; *c && *c == *d; c += 1, d += 1) {
	    continue;
	}
	if (*c == *d) {
	    break;
	}
	if (c[0] == '\0' && c[-1] == '/') {
	    break;
	}
	if (map->next_off == 0) {
	    return 0;
	}
    }
    if (c == (char*)0) {
	return 0;
    }
    /*
     * Found it.  Handle the simple filename match first.
     */
    if (c[-1] != '/') {
	strcpy(dst, (char*)map + map->to_off);
    } else {
	/*
	 * A directory match.
	 *
	 * First move the tail end of the path so the new directory name
	 * fits neatly.
	 */
	size_t from_len = strlen((char*)map + map->from_off);
	size_t to_len = strlen((char*)map + map->to_off);
	if (to_len > from_len) {
	    size_t pad = to_len - from_len;
	    char* end = dst + from_len;
	    d = strchr(dst, '\0');
	    if (d - dst + pad + 1 > dst_size) {
		return -ENAMETOOLONG;
	    }
	    for (; d >= end; d -= 1) {
		d[pad] = *d;
	    }
	} else if (to_len < from_len) {
	    size_t pad = from_len - to_len;
	    for (d = dst + to_len; (*d = d[pad]) != '\0'; d += 1) {
		continue;
	    }
	}
	memcpy(dst, (char*)map + map->to_off, to_len);
    }
    abi_printk(ABI_TRACE_FILEMAP, "filemap: %s --> %s\n", path, dst);
    return strlen(dst) + 1;
}

/*
 * Unit testing code.  This is run using "make tests" at the top level or
 * "make test-FILENAME" in the component directory.
 */
#ifdef  UNIT_TEST
static int current_test_line_nr;
#define	MUST_BE_TRUE(line_nr, a)							\
    do { 								\
	current_test_line_nr = line_nr;					\
	if (!(a)) { 							\
	    fprintf(stderr, "%s line %d test failed, %s\n", __FILE__, line_nr, #a);	\
	    abort();							\
	}								\
    } while(0)

static void test_normalise_path(
	int line_no, const char* src, int is_abs, const char* dst, int retval,
	const char* cwd, int cwd_result)
{
    mock_getcwd_cwd = cwd;
    mock_getcwd_retval = cwd_result;
    char normalise_path_dst[PATH_MAX];
    memset(normalise_path_dst, '\0', sizeof(normalise_path_dst));
    int normalise_path_result = normalise_path(
	    normalise_path_dst, sizeof(normalise_path_dst), src, is_abs);
    MUST_BE_TRUE(normalise_path_result == retval, line_no);
    MUST_BE_TRUE(!strcmp(normalise_path_dst, dst), line_no);
}

static const char* fop_read_result;
static ssize_t fop_read(struct file* f, char* buf, size_t bytes, loff_t* loff)
{
    size_t len = strlen(fop_read_result);
    if (bytes != len) {
	fprintf(
	    stderr, "read of %d bytes, expecting read of %d bytes",
	    bytes, len);
	abort();
    }
    memcpy(buf, fop_read_result, len);
    fop_read_result = (const char*)0;
    return len;
}

static int mock_vfs_fstat(int fd, struct kstat* kst)
{
    kst->size = strlen(fop_read_result);
    return 0;
}

static struct file* linux26_fopen(const char* name, int mode)
{
    static struct file		f;
    static struct file_operations fo;

    f.f_op = &fo;
    fo.read = fop_read;
    return &f;
}

static int mock_linux26_fclose(struct file* f)
{
    return 0;
}

static void mock_abi_printk(unsigned flgs, const char* message, ...)
{
    va_list list;

    va_start(list, message);
    vfprintf(stderr, message, list);
    va_end(list);
    fprintf(stderr, "\n");
}

static char ibcs_fatal_message[4096];
static void mock_ibcs_fatal(const char* message, ...)
{
    va_list list;

    va_start(list, message);
    vsprintf(ibcs_fatal_message, message, list);
    va_end(list);
}

static void test_filemap_parse(
    int line_no, int ret, const char* error, const char* data
) {
    filemap = (struct filemap*)1;
    fop_read_result = data;
    ibcs_fatal_message[0] = '\0';
    int retval = filemap_parse("test-filename");
    MUST_BE_TRUE(line_no, filemap != (struct filemap*)1);
    MUST_BE_TRUE(line_no, !IBCS_IS_ERR(ret) || filemap == (struct filemap*)0);
    MUST_BE_TRUE(line_no, ibcs_fatal_message[0] == '\0' || IBCS_IS_ERR(ret));
    MUST_BE_TRUE(line_no, ret == retval);
    MUST_BE_TRUE(line_no, !strcmp(error, ibcs_fatal_message));
}

static void test_filemap_map(
    int line_no, int ret, const char* path, const char* result
) {
    char buffer[PATH_MAX];
    int retval = filemap_map(buffer, sizeof(buffer), path);
    MUST_BE_TRUE(line_no, ret == retval);
    MUST_BE_TRUE(line_no, !strcmp(result, buffer));
}

void unit_test_ibcs_filemap_c()
{
    /*
     * Test normalise_path().
     */
    test_normalise_path(__LINE__, "", 0, ".", 2, "", -1);
    test_normalise_path(__LINE__, "..", 0, "..", 3, "", -1);
    test_normalise_path(__LINE__, ".././..", 0, "../..", 6, "", -1);
    test_normalise_path(__LINE__, "x/..", 0, ".", 2, "", -1);
    test_normalise_path(__LINE__, "x/.", 0, "x", 2, "", -1);
    test_normalise_path(__LINE__, "./x", 0, "x", 2, "", -1);
    test_normalise_path(__LINE__, "x//y/./../", 0, "x", 2, "", -1);
    test_normalise_path(__LINE__, "../../x", 0, "../../x", 8, "", -1);
    test_normalise_path(__LINE__, "//../../x", 0, "/x", 3, "", -1);
    test_normalise_path(__LINE__, "/x/../../../a/./b///", 0, "/a/b", 5, "", -1);
    test_normalise_path(__LINE__, "", 1, "/x/y", 5, "/x/y", 0);
    test_normalise_path(__LINE__, "/a", 1, "/a", 3, "/x/y", 0);
    test_normalise_path(__LINE__, "./../b///", 1, "/x/b", 5, "/x/y", 0);
    test_normalise_path(__LINE__, "./../../../../b///", 1, "/b", 3, "/x/y", 0);
    /*
     * Filemap parse.
     */
    test_filemap_parse(__LINE__, 0, "", "");
    test_filemap_parse(__LINE__, 0, "", " \t# comment - empty file\n \t \n");
    test_filemap_parse(__LINE__,
	0, "",
	"# comment\nprefixmap: /a f --> /b e\nprefixmap:\t/c/\t \t-->\t \t/d/\t  \t \n");
    MUST_BE_TRUE(__LINE__, filemap != (struct filemap*)0);
    MUST_BE_TRUE(__LINE__, !strcmp("/a f", (char*)filemap + filemap->from_off));
    MUST_BE_TRUE(__LINE__, !strcmp("/b e", (char*)filemap + filemap->to_off));
    MUST_BE_TRUE(__LINE__, filemap->next_off != 0);
    struct filemap* f1 = (struct filemap*)((size_t)filemap + filemap->next_off);
    MUST_BE_TRUE(__LINE__, !strcmp("/c/", (char*)f1 + f1->from_off));
    MUST_BE_TRUE(__LINE__, !strcmp("/d/", (char*)f1 + f1->to_off));
    MUST_BE_TRUE(__LINE__, f1->next_off == 0);
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 1: can't parse line",
	"prefixmap ");
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 2: can't parse line",
	"# comment \nprefixmap:");
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 2: can't parse line",
	"# comment \nprefixmap: x --> y\n");
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 3: can't parse line",
	"# comment \n\nprefixmap: /x");
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 3: can't parse line",
	"# comment \n\nprefixmap: /x --> \n");
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 3: can't parse line",
	"# comment \n\nprefixmap: /x --> zzz \n");
    test_filemap_parse(__LINE__, 0, "", "# comment \n\nprefixmap: /x --> /y ");
    MUST_BE_TRUE(__LINE__, filemap != (struct filemap*)0);
    MUST_BE_TRUE(__LINE__, !strcmp("/x", (char*)filemap + filemap->from_off));
    MUST_BE_TRUE(__LINE__, !strcmp("/y", (char*)filemap + filemap->to_off));
    MUST_BE_TRUE(__LINE__, filemap->next_off == 0);
    test_filemap_parse(__LINE__, 0, "", "# comment \n\nprefixmap: /a --> /b");
    MUST_BE_TRUE(__LINE__, filemap != (struct filemap*)0);
    MUST_BE_TRUE(__LINE__, !strcmp("/a", (char*)filemap + filemap->from_off));
    MUST_BE_TRUE(__LINE__, !strcmp("/b", (char*)filemap + filemap->to_off));
    MUST_BE_TRUE(__LINE__, filemap->next_off == 0);
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 1: can't parse line",
	"prefixmap: /x --> /zzz/");
    test_filemap_parse(__LINE__,
	-EILSEQ, "test-filename line 1: can't parse line",
	"prefixmap: /x/ --> /zzz");
    /*
     * Filemap map.
     */
    filemap = (struct filemap*)0;
    test_filemap_map(__LINE__, 0, "/x", "/x");
    test_filemap_parse(__LINE__,
	0, "",
	"prefixmap: /a/b --> /x\n"
	"prefixmap: /c/d/ --> /y/\n"
	"prefixmap: /c/ --> /z/z/x/\n"
	"prefixmap: /a --> /ttt\n");
    test_filemap_map(__LINE__, 0, "/x/../y/./", "/y");
    test_filemap_map(__LINE__, 5, "/a", "/ttt");
    test_filemap_map(__LINE__, 3, "/a/b", "/x");
    test_filemap_map(__LINE__, 9, "/c/d/x/foo", "/y/x/foo");
    test_filemap_map(__LINE__, 13, "/c/x/foo", "/z/z/x/x/foo");
}
#endif
