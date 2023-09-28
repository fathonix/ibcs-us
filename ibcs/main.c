/*
 * SYNOPSIS
 *    ibcs [options] ibcs_executable args...
 *
 * DESCRIPTION
 * 
 *     Run the IBCS program "ibcs_executable", passing it "args...".  This
 *     program must have the CAP_SYS_RAWIO capability, or be run as root, or
 *     have setuid root file permission.  If setuid it will change it's
 *     effective uid to match the program being run before executing it.
 */
#include <asm/ldt.h>
#include <stdarg.h>
#include <stddef.h>

#include <ibcs-us/ibcs/filemap.h>
#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/ibcs/short-inode.h>
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/asm/mman.h>
#include <ibcs-us/linux26-compat/asm/ptrace.h>
#include <ibcs-us/linux26-compat/linux/capability.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/fcntl.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/string.h>
#include <ibcs-us/linux26-compat/linux/types.h>

/*
 * Stuff externally accessible.
 */
static u_long	cmdline_personality;

u_long abi_personality(const char* executable_path)
{
    return cmdline_personality;
}

struct cmdline_options
{
    const char*		argv0;
    const char*		me;
    const char* const*	ibcs_argv;
    u_long		personality;
    unsigned		abi_trage_flg;
    int			abi_trace_fd;
    const char*		filemap_filename;
};


/*
 * Print a usage message and exit.
 */
static void usage(const char* me, const char* fmt, ...)
{
    if (fmt != NULL) {
	va_list ap;
	ibcs_writef(2, "%s: ", me);
	va_start(ap, fmt);
	ibcs_vwritef(2, fmt, ap);
	va_end(ap);
	ibcs_writef(2, "\n");
    } else {
	ibcs_writef(2, "usage: %s [options] ibcs-executable [arg..]\n", me);
	ibcs_writef(2, "options:\n");
        ibcs_writef(2, "  -h,   --help         Print this message & exit.\n");
        ibcs_writef(2, "  -l L, --log=L        Where to write printk output, like trace.\n");
        ibcs_writef(2, "  -m M, --map=M        Rewrite filenames using map file M.\n");
        ibcs_writef(2, "  -p P, --personality=P Emulate personality P.\n");
        ibcs_writef(2, "  -t T, --trace=T      Set tracing bits to T.\n");
    }
    IBCS_SYSCALL(exit, 1);
}


/*
 * Return an environment variable.
 */
static const char* ibcs_getenv(const char* const* envp, const char* name)
{
    int		len = strlen(name);

    for (; *envp; envp += 1) {
	if (strncmp(*envp, name, len) == 0 && (*envp)[len] == '=') {
	    return &(*envp)[len + 1];
	}
    }
    return (const char*)0;
}


/*
 * Poor mans argument parser.  Unlike the GNU's getopt, this is reentrant.
 * Passed is a NULL terminated list of strings, called options, that are
 * the options recognised written as they will appear on the command line
 * (eg "-h" or "--help").  If the option ends with a '=' it requires an
 * argument.  argv and optindex are work variables, which must be initialised
 * before the first call to the first option to be parsed and 0 respectively.
 *
 * The function is intended to be called repeatedly, and on each call it will
 * return the next option found on the command line and place the pointer to
 * is value in arg.  If the option doesn't take an argument arg is NULL.
 * The returned option is the string in options that it matched.  When all
 * options have been parsed NULL is returned, and argv will be pointing at
 * the first non-option command line argument.
 */
static const char* getoption(
    const char* me, const char* options[], const char* const* * argv,
    int* optindex, const char** arg
) {
    const char*		av = **argv;
    const char*		o;
    size_t		l;

    /*
    * Were we processing a list of single letter options, like -xyz?
    */
    if (*optindex != 0) {
	*optindex += 1;
	if (av[*optindex] == '\0') {
	    *optindex = 0;
	    av = *++*argv;
	}
    }
    /*
     * End of options?
     */
    if (av == NULL) {
	*arg = NULL;
	return NULL;
    }
    /*
     * If we are looking for the next option string on the command line,
     * see if we have reached the end of the options. A string not starting
     * with, '-', '-' and '--' terminate options.
     */
    if (*optindex == 0) {
	if (av[0] != '-' || av[1] == '\0' || (av[1] == '-' && av[2] == '\0')) {
	    if (av[1] == '-') {
		*argv += 1;
	    }
	    *arg = NULL;
	    return NULL;
	}
	else if (av[1] != '-') {
	    *optindex = 1;
	}
    }
    /*
     * Search the passed options for one that matches the current one on
     * the command line.
     */
    for (; *options != NULL; options += 1) {
	o = *options;
	if (o[1] != '-') {
	    /*
	     * This is a short option.
	     */
	    if (*optindex != 0 && av[*optindex] == o[1]) {
		if (o[2] != '=') {
		    *arg = NULL;
		    return o;
		}
		av += ++*optindex;
		*optindex = 0;
		if (*av != '\0') {
		    *arg = av;
		    *argv += 1;
		    return o;
		}
		if (*++*argv == NULL) {
		    usage(me, "Option -%c requires an argument.", o[1]);
		}
		*arg = *(*argv)++;
		return o;
	    }
	}
	else {
	    /*
	     * This is a long option.
	     */
	    if (*optindex == 0) {
		l = strlen(o);
		if (o[l - 1] == '=')
		    l -= 1;
		if ((av[l] == '\0' || av[l] == '=') && strncmp(av, o, l) == 0) {
		    if (av[l] == '=') {
			if (o[l] == '\0') {
			    usage(me, "Option %.*s doesn't require an argument.", l, o);
			}
			*arg = &av[l + 1];
			*argv += 1;
			return o;
		    }
		    if (o[l] == '\0') {
			*arg = NULL;
			*argv += 1;
			return o;
		    }
		    *arg = *++*argv;
		    if (*arg == NULL) {
			usage(me, "Option %.*s requires an argument.", l, o);
		    }
		    *argv += 1;
		    return o;
		}
	    }
	}
    }
    if (*optindex != 0) {
	usage(me, "unrecognised option -%c.", av[*optindex]);
    }
    else {
	usage(me, "unrecognised option %s.", av);
    }
    return NULL;
}


/*
 * Translate a comma separated list of flag names to value.
 */
struct flag_lookup
{
    const char*		flag_name;
    unsigned long	flag_value;
};

static struct flag_lookup personality_flag_table[] =
{
    {"abi_y2k_bug",		ABI_Y2K_BUG},
    {"addr_no_randomize",	ADDR_NO_RANDOMIZE},
    {"fdpic_funcptrs",		FDPIC_FUNCPTRS},
    {"mmap_page_zero",		MMAP_PAGE_ZERO},
    {"addr_compat_layout",	ADDR_COMPAT_LAYOUT},
    {"read_implies_exec",	READ_IMPLIES_EXEC},
    {"addr_limit_32bit",	ADDR_LIMIT_32BIT},
    {"short_inode",		SHORT_INODE},
    {"whole_seconds",		WHOLE_SECONDS},
    {"sticky_timeouts",		STICKY_TIMEOUTS},
    {"addr_limit_3gb",		ADDR_LIMIT_3GB},
    {"per_linux",		PER_LINUX},
    {"per_linux_32bit",		PER_LINUX_32BIT},
    {"per_linux_fdpic",		PER_LINUX_FDPIC},
    {"per_svr4",		PER_SVR4},
    {"per_svr3",		PER_SVR3},
    {"per_scosvr3",		PER_SCOSVR3},
    {"per_osr5",		PER_OSR5},
    {"per_wysev386",		PER_WYSEV386},
    {"per_iscr4",		PER_ISCR4},
    {"per_bsd",			PER_BSD},
    {"per_sunos",		PER_SUNOS},
    {"per_xenix",		PER_XENIX},
    {"per_linux32",		PER_LINUX32},
    {"per_linux32_3gb",		PER_LINUX32_3GB},
    {"per_irix32",		PER_IRIX32},
    {"per_irixn32",		PER_IRIXN32},
    {"per_irix64",		PER_IRIX64},
    {"per_riscos",		PER_RISCOS},
    {"per_solaris",		PER_SOLARIS},
    {"per_uw7",			PER_UW7},
    {"per_osf4",		PER_OSF4},
    {"per_hpux",		PER_HPUX},
    {0, 0}
};

static u_long lookup_flag(
    const char* me, const char* arg, struct flag_lookup* table,
    const char* option
) {
    int			i;
    u_long		not;
    u_long		result = 0;
    char		last_sep = '+';
    const char*		orig_arg = arg;

    while (*arg) {
	const char*	end = strchr(arg, '\0');
	const char*	s;
	const char*	sep = "+-";
	u_long		val;

	for (s = sep; *s != '\0'; s += 1) {
	    const char*	e = strchr(arg, *s);
	    if (e != (const char*)0 && e < end) {
		end = e;
	    }
	}
	if (*arg != '~') {
	    not = 0;
	} else {
	    not = -1;
	    arg += 1;
	}
	val = simple_strtoul(arg, &s, 16);
	if (s != end) {
	    int		len = end - arg;
	    val = 0;
	    for (i = 0; table[i].flag_name != (char*)0; i += 1) {
		if (!strncmp(arg, table[i].flag_name, len)) {
		    if (table[i].flag_name[len] == '\0') {
			val = table[i].flag_value;
			break;
		    }
		}
	    }
	    if (val == 0) {
		result = 0;
		break;
	    }
	}
	val ^= not;
	if (last_sep == '+') {
	    result |= val;
	} else {
	    result &= ~val;
	}
	arg = *end == '\0' ? end : end + 1;
    }
    if (result == 0) {
	usage(me, "Can't parse %s\"%s\".", option, orig_arg);
    }
    return result;
}


/*
 * Trace flags from include/util/trace.h.
 */
static struct flag_lookup trace_flags_table[] = {
    {"abi_trace_api",		ABI_TRACE_API},
    {"abi_trace_ioctl",		ABI_TRACE_IOCTL},
    {"abi_trace_ioctl_f",	ABI_TRACE_IOCTL_F},
    {"abi_trace_signal",	ABI_TRACE_SIGNAL},
    {"abi_trace_signal_f",	ABI_TRACE_SIGNAL_F},
    {"abi_trace_socksys",	ABI_TRACE_SOCKSYS},
    {"abi_trace_streams",	ABI_TRACE_STREAMS},
    {"abi_trace_unimpl",	ABI_TRACE_UNIMPL},
    {0,0}
};


/*
 * Command line options we recognise.
 */
static const char* options[] = {
    "-l=",	"--log=",
    "-h",	"--help",
    "-m=",	"--map=",
    "-p=",	"--personality=",
    "-t=",	"--trace=",
    (char*)0
};


static void parse_cmdline_options(
    struct cmdline_options* cmdline_options, const char* const* argv
) {
    const char*		arg;
    const char*		c = strrchr(*argv, '/');
    const char*		end;
    const char*		option;
    int			optindex = 0;

    memset(cmdline_options, '\0', sizeof(*cmdline_options));
    cmdline_options->argv0 = *argv;
    cmdline_options->me = c == (char*)0 ? *argv : c + 1;
    cmdline_options->abi_trace_fd = 2;
    argv += 1;
    while ((option = getoption(cmdline_options->me, options, &argv, &optindex, &arg)) != NULL) {
	if (!strcmp(option, "-l=") || !strcmp(option, "--log=")) {
	    cmdline_options->abi_trace_fd = simple_strtoul(arg, &end, 0);
	    if (*arg != '\0' && *end == '\0') {
		int ret = IBCS_SYSCALL(
		    fcntl, cmdline_options->abi_trace_fd, F_GETFL);
		if (IBCS_IS_ERR(ret)) {
		    ibcs_fatal_syscall(
			cmdline_options->abi_trace_fd, "logging fd %s", arg);
		}
	    } else {
		cmdline_options->abi_trace_fd =
		    IBCS_SYSCALL(open, arg, O_WRONLY|O_APPEND|O_CREAT, 0666);
		if (IBCS_IS_ERR(cmdline_options->abi_trace_fd)) {
		    ibcs_fatal_syscall(
			cmdline_options->abi_trace_fd, "Can not open %s", arg);
		}
	    }
	}
	if (!strcmp(option, "-h=") || !strcmp(option, "--help=")) {
	    usage(cmdline_options->me, (char*)0);
	}
	if (!strcmp(option, "-m=") || !strcmp(option, "--map=")) {
	    cmdline_options->filemap_filename = arg;
	}
	if (!strcmp(option, "-p=") || !strcmp(option, "--personality=")) {
	    cmdline_options->personality = lookup_flag(
		cmdline_options->me, arg, personality_flag_table, option);
	}
	if (!strcmp(option, "-t=") || !strcmp(option, "--trace=")) {
	    cmdline_options->abi_trage_flg = lookup_flag(
		cmdline_options->me, arg, trace_flags_table, option);
	}
    }
    if (*argv == (char*)0) {
        usage(cmdline_options->me, (char*)0);
    }
    cmdline_options->ibcs_argv = argv;
}


/*
 * Set the process security (user, gid, capabilities) to match the loaded
 * program.  Ie, drop all the ones we don't need, and warn if there are
 * credentials we do need but don't have.
 */
static void set_credentials(
    const struct cmdline_options* cmdline_options, const char* executable
) {
    int			his_egid32;
    int			his_euid32;
    int			my_egid32;
    int			my_euid32;
    int			my_gid32;
    int			my_uid32;
    int			retval;
    struct stat64	st64;
    struct file*	efile = (struct file*)0;

    retval = 0;
    /*
     * Read in our credentials.
     */
    my_uid32 = IBCS_SYSCALL(getuid32);
    if (IBCS_IS_ERR(my_uid32)) {
	ibcs_fatal_syscall(my_uid32, "set_credentials getuid32()");
	goto out;
    }
    my_gid32 = IBCS_SYSCALL(getgid32);
    if (IBCS_IS_ERR(my_gid32)) {
	ibcs_fatal_syscall(my_gid32, "set_credentials getgid32()");
	goto out;
    }
    my_euid32 = IBCS_SYSCALL(geteuid32);
    if (IBCS_IS_ERR(my_euid32)) {
	ibcs_fatal_syscall(my_euid32, "set_credentials geteuid32()");
	goto out;
    }
    my_egid32 = IBCS_SYSCALL(getegid32);
    if (IBCS_IS_ERR(my_egid32)) {
	ibcs_fatal_syscall(my_egid32, "set_credentials getegid32()");
	goto out;
    }
    /*
     * Get the credentials of the program we are executing.
     */
    efile = linux26_fopen(executable, O_RDONLY);
    if (IBCS_IS_ERR(efile)) {
        retval = (int)efile;
	ibcs_fatal_syscall(retval, "set_credentials open(\"%s\")", executable);
	efile = (struct file*)0;
	goto out;
    }
    retval = IBCS_SYSCALL(fstat64, efile->fd, &st64);
    if (IBCS_IS_ERR(retval)) {
	ibcs_fatal_syscall(retval, "set_credentials fstat64(\"%s\")", executable);
	goto out;
    }
    /*
     * Do we need to set euid or egid?
     */
    his_egid32 = st64.st_mode & S_ISUID ? st64.st_gid : my_gid32;
    if (his_egid32 != my_egid32) {
	retval = IBCS_SYSCALL(setregid, -1, his_egid32);
	if (IBCS_IS_ERR(retval)) {
	    printk(KERN_ERR "setegid(\"%s\", egid=%d): ", executable, his_egid32);
	}
    }
    his_euid32 = st64.st_mode & S_ISUID ? st64.st_uid : my_uid32;
    if (his_euid32 != my_euid32) {
	retval = IBCS_SYSCALL(setreuid, -1, his_euid32);
	if (IBCS_IS_ERR(retval)) {
	    printk(KERN_ERR "seteuid(\"%s\", euid=%d): ", executable, his_euid32);
	}
    }
    /*
     * Drop all capabilities.  They weren't a thing when the systems we are
     * emulating existed, so they could not have had any set for them.
     */
    struct __user_cap_header_struct	hdr;
    struct __user_cap_data_struct	caps[VFS_CAP_U32_3];
    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    hdr.pid = IBCS_SYSCALL(getpid);
    memset(caps, '\0', sizeof(caps));
    retval = IBCS_SYSCALL(capset, &hdr, caps);
    if (IBCS_IS_ERR(retval)) {
	ibcs_fatal_syscall(retval, "set_credentials capset()");
	goto out;
    }
out:
    if (efile != (struct file*)0) {
	linux26_fclose(efile);
    }
}


/*
 * Search along $PATH looking for the program to load.
 */
static int load_binary(
    const struct cmdline_options* cmdline_options, const char* const* envp
) {
    const char*		ibcs_filename = *cmdline_options->ibcs_argv;
    int			ibcs_filename_len = strlen(ibcs_filename);

    /*
     * Go down the PATH environment variable looking for the program.
     */
    if (strchr(ibcs_filename, '/') != (char*)0) {
	int err = load_binfmt(ibcs_filename);
	if (!IBCS_IS_ERR(err)) {
	    set_credentials(cmdline_options, ibcs_filename);
	}
	return err;
    }
    const char* path_env = ibcs_getenv(envp, "PATH");
    if (path_env == (char*)0) {
	return -ENOENT;
    }
    int	last_err = 0;
    while (1) {
	const char* c = strchr(path_env, ':');
	if (c == (char*)0) {
	    c = strchr(path_env, '\0');
	}
	char buffer[(c - path_env) + 1 + ibcs_filename_len + 1];
	memcpy(buffer, path_env, c - path_env);
	char* b = buffer + (c - path_env);
	if (b > buffer) {
	    *b++ = '/';
	}
	strcpy(b, ibcs_filename);
	int err = load_binfmt(buffer);
	if (err >= 0) {
	    set_credentials(cmdline_options, buffer);
	    return err;
	}
	/*
	 * The conditions here are copied from glibc's execvpe() source.
	 */
	switch (err) {
	    case -EACCES:
		last_err = err;
		break;
	    case -ENOENT:
	    case -ESTALE: 
	    case -ENOTDIR:
	    case -ENODEV:
	    case -ETIMEDOUT:
		if (last_err == 0) {
		    last_err = err;
		}
		break;
	    default:
		return err;
	}
	path_env = b;
	if (*path_env == '\0') {
	    break;
	}
	path_env += 1;
    }
    return last_err;
}


/*
 * Since this program doesn't use glibc this is **not** the entry point.
 * It's not static because that's how gcc thinks the world is supposed to
 * be, and whinges if it isn't.  But it would work perfectly well if it was
 * static.
 *
 * _start() below is the entry point.
 */
void main(int argc, const char* const* argv, const char* const* envp)
{
    /*
     * Beware, everything on this stack disappears when the emulation
     * starts, so don't go taking long lived addresses of local variables.
     */
    struct cmdline_options cmdline_options;
    const char*		ibcs_program_name;

    parse_cmdline_options(&cmdline_options, argv);
    ibcs_program_name = strrchr(*cmdline_options.ibcs_argv, '/');
    if (ibcs_program_name != (const char*)0) {
	ibcs_program_name += 1;
    } else {
	ibcs_program_name = *cmdline_options.ibcs_argv;
    }
    linux26_compat_init(ibcs_program_name);
    cmdline_personality = cmdline_options.personality;
    abi_trace_flg = cmdline_options.abi_trage_flg;
    abi_trace_fd = cmdline_options.abi_trace_fd;
    if ((int)IBCS_SYSCALL(geteuid32) != 0) {
	if (!linux26_capability(CAP_SYS_RAWIO, 0)) {
	    ibcs_writef(
		2, "%s: I must be run as root or have SYS_CAP_RAWIO.\n",
		cmdline_options.me);
	    IBCS_SYSCALL(exit, 1);
	}
    }
    int ret = load_binary(&cmdline_options, envp);
    if (IBCS_IS_ERR(ret)) {
        if (binfmt_mmap_error) {
	    ibcs_fatal_syscall(
		binfmt_mmap_errno, "%s - %s", *cmdline_options.ibcs_argv,
		binfmt_mmap_error);
	}
	ibcs_fatal_syscall(ret, *cmdline_options.ibcs_argv);
    }
    if (current->exec_domain == (struct exec_domain*)0) {
	ibcs_writef(
	    2, "%s: internal error, current->exec_domain not set.\n",
	    cmdline_options.me);
	IBCS_SYSCALL(exit, 1);
    }
    sysent_initialise();
    int retval = short_inode_construct();
    if (IBCS_IS_ERR(retval)) {
	ibcs_fatal_syscall(retval, "Could not initialise short inode table");
    }
    if (cmdline_options.filemap_filename != (const char*)0) {
	filemap_parse(cmdline_options.filemap_filename);
    }
    /*
     * Our final step is a disappearing act.  We've squirreled away all we
     * need to know in static variables, so we can remove all signs we
     * have even been here from the stack so the emulated programs entry
     * point gets the stack it would have got if run natively.   See _start()
     * below for the format.
     */
    argc -= cmdline_options.ibcs_argv - argv;
    asm volatile (
	"mov	%0,%%eax\n\t"
	"mov	%2,%%edx\n\t"
	"mov	%1,%%esp\n\t"
	"mov	%%eax,(%%esp)\n\t"
	"jmp	*%%edx\n"
	: /* No outputs */
	: "r" (argc), "r" (cmdline_options.ibcs_argv - 1), "r" (current->start_addr)
	: "rax", "rdx"
    );
}

/*
 * This _real_ entry point, so real it is in fact the first instruction
 * executed.  We are dealing with the kernel's user space ABI here, not
 * the posix environment set up by glibc.
 */
void _start(char const* arg0)
{
    /*
     *        		Reality		What gcc thinks it is.
     *        		--------	----------------------
     * esp+0x00:   	argc		[return address]
     * esp+0x04:	argv[0]		argv0
     * esp+0x08:	argv[1]
     *    :             argv[...]
     * esp+xxx0:	argv[argc-1]
     * esp+xxx4:	0
     * esp+xxx8:	envp[0]
     *    :     	envp[...]
     * esp+yyyy:	0
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    main(((int*)&arg0)[-1], &arg0, &arg0 + ((int*)&arg0)[-1] + 1);
#pragma GCC diagnostic pop
}
