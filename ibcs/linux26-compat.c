/*
 * IBCS was originally a kernel module.  It's now implemented in user space,
 * but is still written like a kernel module using the 2.6.32 kernel ABI.
 *
 * This emulates the kernel 2.6.32 ABI with normal system calls.
 */
#include <linux/elf.h>
#include <linux/futex.h>

/*
 * We don't want the uname() glibc syscall defined.
 */
#define uname	unwanted_glibc_uname_
#include <sys/utsname.h>
#undef uname

#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/ibcs/filemap.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>


/*
 * This has to be done before static inline functions use IBCS_SYSCALL()
 * in the .h files.
 */
#ifdef UNIT_TEST
#undef	IBCS_SYSCALL
#define IBCS_SYSCALL(a, args...)	mock_syscall_##a(args)
static int mock_syscall_getpid();
static int mock_syscall_getuid32(void);
static int mock_syscall_kill(pid_t pid, int sig);
static void* mock_syscall_mmap2(
    void* addr, size_t length, int prot, int flags, int fd, off_t pg_offset);
static int mock_syscall_munmap(void* addr, size_t length);
#endif

#include <ibcs-us/linux26-compat/asm/ptrace.h>
#include <ibcs-us/linux26-compat/linux/binfmts.h>
#include <ibcs-us/linux26-compat/linux/capability.h>
#include <ibcs-us/linux26-compat/linux/cred.h>
#include <ibcs-us/linux26-compat/linux/dirent.h>
#include <ibcs-us/linux26-compat/linux/err.h>
#include <ibcs-us/linux26-compat/linux/fcntl.h>
#include <ibcs-us/linux26-compat/linux/fdtable.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/limits.h>
#include <ibcs-us/linux26-compat/linux/list.h>
#include <ibcs-us/linux26-compat/linux/kdev_t.h>
#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/mount.h>
#include <ibcs-us/linux26-compat/linux/param.h>
#include <ibcs-us/linux26-compat/linux/path.h>
#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/poll.h>
#include <ibcs-us/linux26-compat/linux/rwsem.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/signal.h>
#include <ibcs-us/linux26-compat/linux/socket.h>
#include <ibcs-us/linux26-compat/linux/slab.h>
#include <ibcs-us/linux26-compat/linux/statfs.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/string.h>
#include <ibcs-us/linux26-compat/linux/time.h>
#include <ibcs-us/linux26-compat/linux/types.h>
#include <ibcs-us/linux26-compat/linux/utsname.h>


/*
 * Unit testing prelude - define mock stuff.
 */
#ifdef UNIT_TEST
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#define	abi_trace_fd			mock_abi_trace_fd
#define	ibcs_malloc(x)			malloc(x)
#define	ibcs_fatal(m...)		mock_ibcs_fatal(m)
#define	ibcs_fatal_syscall(r, a...)	mock_ibcs_fatal_syscall(r, a)
#define	ibcs_free(x)			free(x)
#define	ibcs_syscall(s...)		mock_ibcs_syscall(s)
#define	ibcs_vfmt(o, s, m, va)		mock_ibcs_vfmt(o, s, m, va)
#define	ibcs_vsyscall(s, va)		mock_ibcs_vsyscall(s, va)
#define	ibcs_vwritef(fd, fmt...)	mock_ibcs_vwritef(fd, fmt)
#define	filemap_map(d, l, s)		mock_filemap_map(d, l, s)

static int mock_abi_trace_fd;
static int mock_syscall_access(const char* filename, int mode);
static int mock_syscall_close(int fd);
static int mock_syscall_creat(const char* filename, int mode);
static int mock_syscall_capget(cap_user_header_t hdrp, cap_user_data_t datap);
static int mock_syscall_capset(cap_user_header_t hdrp, cap_user_data_t datap);
static void mock_syscall_exit(int status);
static int mock_syscall_fcntl(int fd, int op, void* buf);
static int mock_syscall_fstat64(int fd, struct stat64* st64);
static int mock_syscall_futex(
    int* uaddr, int futex_op, int val,
    const struct timespec* timeout,   /* or: uint32_t val2 */
    int* uaddr2, int val3);
static int mock_syscall_getdents64(
    unsigned int fd, struct linux_dirent64* dirp, unsigned int count);
static int mock_syscall_getsockopt(
    int sockfd, int level, int optname, void* optval, socklen_t* optlen);
static loff_t mock_syscall_lseek(int fd, loff_t loff, int whence);
static int mock_syscall_lstat64(const char* filename, struct stat64* st64);
static void* mock_syscall_mremap(
    void* old_address, size_t old_size, size_t new_size, int flags,
    ... /* void* new_address */);
static int mock_syscall_open(const char* path, int mode, int flags);
static int mock_syscall_poll(
    struct pollfd* fds, int nfds, const struct timespec* tmo_p,
    const sigset_t* sigmask);
static size_t mock_syscall_read(int fd, void* buf, size_t len);
static int mock_syscall_recvmsg(int sockfd, const struct msghdr* msg, int flags);
static int mock_syscall_select(
    int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
    struct timeval* timeout);
static int mock_syscall_sendmsg(int sockfd, const struct msghdr* msg, int flags);
static int mock_syscall_sigaction(
    int signum, const struct sigaction* act, struct sigaction* oldact);
static int mock_syscall_stat64(const char* filename, struct stat64* st64);
static int mock_syscall_statfs(const char* filename, struct statfs* stfs);
static size_t mock_syscall_uname(struct utsname* uts);
static size_t mock_syscall_write(int fd, const void* buf, size_t len);

static void mock_ibcs_fatal(const char* message, ...);
static void mock_ibcs_fatal_syscall(int retval, const char* message, ...);
static int mock_ibcs_vfmt(char* out, size_t len, const char* fmt, va_list list);
static int mock_filemap_map(char* dst, size_t len, const char* path);
static int mock_ibcs_syscall(int syscall_no, ...);
static int mock_ibcs_vsyscall(int syscall_no, va_list list);
static int mock_ibcs_vwritef(int fd, const char* fmt, ...);
#endif


LIST_HEAD(linux_binfmts);

static struct exec_domain* exec_domains;
static struct mm_struct linux26_current_mm;
static struct files_struct linux26_current_files;
static struct task_struct linux26_current = {
    .files = &linux26_current_files,
    .mm = &linux26_current_mm,
};
struct task_struct* current = &linux26_current;
struct uts_namespace init_uts_ns;
struct rw_semaphore uts_sem;

/*
 * The kernal has a number on internal structures associated with the file
 * system.  Some, like "struct kstat" mirror a similar structure exposed to
 * user space, eg "struct stat64" in the case of "struct kstat".
 *
 * Emulate those kernel structures here.
 */
static void statfs_to_kstatfs(struct kstatfs* kstatfs, const struct statfs* statfs)
{
    kstatfs->f_type = statfs->f_type;
    kstatfs->f_bsize = statfs->f_bsize;
    kstatfs->f_blocks = statfs->f_blocks;
    kstatfs->f_bfree = statfs->f_bfree;
    kstatfs->f_bavail = statfs->f_bavail;
    kstatfs->f_files = statfs->f_files;
    kstatfs->f_ffree = statfs->f_ffree;
    kstatfs->f_fsid = statfs->f_fsid;
    kstatfs->f_namelen = statfs->f_namelen;
    kstatfs->f_frsize = statfs->f_frsize;
    memset(kstatfs->f_spare, '\0', sizeof(kstatfs->f_spare));
    kstatfs->f_spare[0] = statfs->f_flags;
}


static void stat64_to_kstat(struct kstat* kstat, const struct stat64* st64)
{
    kstat->ino = st64->st_ino;
    kstat->dev = st64->st_dev;
    kstat->mode = st64->st_mode;
    kstat->nlink = st64->st_nlink;
    kstat->uid = st64->st_uid;
    kstat->gid = st64->st_gid;
    kstat->rdev = st64->st_rdev;
    kstat->size = st64->st_size;
    kstat->atime.tv_sec = st64->st_atime;
    kstat->atime.tv_nsec = st64->st_atime_nsec;
    kstat->mtime.tv_sec = st64->st_mtime;
    kstat->mtime.tv_nsec = st64->st_mtime_nsec;
    kstat->ctime.tv_sec = st64->st_ctime;
    kstat->ctime.tv_nsec = st64->st_ctime_nsec;
    kstat->blksize = st64->st_blksize;
    kstat->blocks = st64->st_blocks;
}


static void kstat_to_inode(struct inode* inode, const struct kstat* kstat)
{
    inode->i_ino = kstat->ino;
    inode->i_dev = kstat->dev;
    inode->i_rdev = kstat->rdev;
    inode->i_mode = kstat->mode;
}


/*
 * Initialise the brk() syscall machinery, so do_brk() and linux26_brk()
 * have the things they need to work with.
 * 
 * The binfmt loader is meant to have set the mm->brk to where it expects
 * the brk() to be.  If the bss is loaded this will be at the end of .bss
 * section.
 */
static void init_brk()
{
    struct mm_struct*	mm = current->mm;

    /*
     * If the highest mmap() was a .bss we can use that.
     */
    if (mm->ibcs_high_mmap_len > 0 && mm->ibcs_bss_mmap_addr == mm->ibcs_high_mmap_addr) {
	mm->ibcs_brk_mmap_addr = mm->ibcs_bss_mmap_addr & PAGE_MASK;
	mm->ibcs_brk_mmap_max = PAGE_ALIGN(mm->ibcs_bss_mmap_addr + mm->ibcs_bss_mmap_len);
	mm->ibcs_brk_mmap_min = mm->brk;
    } else {
	/*
	 * Create a new mmap() in the area they want the mm->brk.
	 */
	unsigned long addr = PAGE_ALIGN(mm->brk);
	unsigned long new_addr = do_mmap(
	    (struct file*)0, addr, PAGE_SIZE,
	    PROT_READ | PROT_WRITE | PROT_EXEC,
	    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_32BIT, 0);
	if (IBCS_IS_ERR(new_addr)) {
	    printk(KERN_ERR "Can not allocate heap");
	} else {
	    mm->ibcs_brk_mmap_addr = new_addr;
	    mm->ibcs_brk_mmap_max = new_addr + PAGE_SIZE;
	    mm->ibcs_brk_mmap_min = new_addr;
	}
    }
    if (mm->brk < mm->ibcs_brk_mmap_min || mm->brk >= mm->ibcs_brk_mmap_max) {
	printk(KERN_ERR "mm->brk does not lie within mmap");
    }
}


/*
 * Emulate the kernel's "struct file->f_op" vtable.  It is a table of functions
 * associated with every file descriptor that says how to open, close, write,
 * read, lseek, yada yada the file.  We provide an emulation that uses the
 * native Linux syscalls to provide the interface.
 */
ssize_t file_operations_read(struct file* file, char* buf, size_t len, loff_t* loff)
{
    loff_t lseek_ret = file->f_op->llseek(file, *loff, SEEK_SET);
    if (lseek_ret != *loff) {
	return lseek_ret;
    }
    ssize_t got = IBCS_SYSCALL(read, file->fd, buf, len);
    if (IBCS_IS_ERR(got)) {
	return got;
    }
    *loff += got;
    return got;
}


ssize_t file_operations_write(struct file* file, const char* buf, size_t len, loff_t* loff)
{
    loff_t lseek_ret = file->f_op->llseek(file, *loff, SEEK_SET);
    if (lseek_ret != *loff) {
	return (ssize_t)lseek_ret;
    }
    ssize_t written = IBCS_SYSCALL(write, file->fd, buf, len);
    if (IBCS_IS_ERR(written)) {
	return written;
    }
    *loff += written;
    return written;
}


loff_t file_operations_llseek(struct file* file, loff_t loff, int origin)
{
    return (loff_t)IBCS_SYSCALL(lseek, file->fd, loff, origin);
}


int file_operations_mmap(struct file* file, struct vm_area_struct* vma)
{
    printk(KERN_ERR "file_operations_map() called, but it isn't implemented\n");
    return -EINVAL;
}


int file_operations_open(struct inode* inode, struct file* file)
{
    /*
     * Allocate file->private_data if you need it.
     */
    return 0;
}


int file_operations_release(struct inode* inode, struct file* file)
{
    /*
     * Undo whatever file_operations_open did.
     */
    return 0;
}


static struct file_operations file_operations = {
    .owner = 0,
    .llseek = file_operations_llseek,
    .mmap = file_operations_mmap,
    .read = file_operations_read,
    .write = file_operations_write,
    .open = file_operations_open,
    .release = file_operations_release,
};

/*
 * Emulate the kernels functions for wraping / unwrapings a file descriptor
 * in a "struct file".
 */
struct file* fget(unsigned int fd)
{
    struct fdtable*	fdt = current->files->fdt;

    /*
     * Grow the file table if it isn't big enough.
     */
    if (fdt->max_fds <= fd) {
        size_t fd_size = (fd + 1) * 2 * sizeof(*fdt->fd);
	void* new_fd = ibcs_malloc(fd_size);
	memcpy(new_fd, fdt->fd, fdt->max_fds * sizeof(*fdt->fd));
	if (fdt->fd) {
	    memset(
		(char*)new_fd + fdt->max_fds * sizeof(*fdt->fd), '\0',
		fd_size - fdt->max_fds * sizeof(*fdt->fd));
	    ibcs_free(fdt->fd);
	}
	fdt->fd = (struct file**)new_fd;
	fdt->max_fds = fd_size / sizeof(*fdt->fd);
    }
    /*
     * Create a new "struct file" we haven't see this one before.
     */
    struct file* file = fdt->fd[fd];
    if (!file) {
	file = (struct file*)ibcs_malloc(sizeof(*file));
	memset(file, '\0', sizeof(*file));
	file->fd = fd;
	file->f_count = 0;
	file->f_op = &file_operations;
	file->f_dentry = &file->_f_dentry_real;
	file->f_dentry->_d_file = file;
	file->f_dentry->d_inode = &file->f_dentry->_d_inode_real;
	struct kstat kstat;
	if (!IBCS_IS_ERR(vfs_fstat(fd, &kstat))) {
	    kstat_to_inode(file->f_dentry->d_inode, &kstat);
	    file->_f_isopen = 1;
	}
	fdt->fd[fd] = file;
    }
    file->f_count += 1;
    return file;
}


static void fdestroy(struct file* file)
{
    if (file->f_count == 0 && !file->_f_isopen) {
	current->files->fdt->fd[file->fd] = (struct file*)0;
	ibcs_free(file);
    }
}

void fput(struct file* file)
{
    file->f_count -= 1;
    fdestroy(file);
}

struct file* linux26_fopen(const char* path, int mode)
{
    int fd = IBCS_SYSCALL(open, path, mode, 0);
    if (IBCS_IS_ERR(fd)) {
	return (struct file*)fd;
    }
    return fget(fd);
}


int linux26_fclose(struct file* file)
{
    if (file->f_op->release) {
        /*
	 * This is only used in anger by our character devices, which always
	 * have d_inode set up.
	 */
	file->f_op->release(file->f_dentry->d_inode, file);
    }
    int ret = IBCS_SYSCALL(close, file->fd);
    file->_f_isopen = 0;
    fput(file);
    return ret;
}

struct file* open_exec(const char* path)
{
    return linux26_fopen(path, O_RDONLY);
}


/*
 * Stuff for the "struct file".
 */
loff_t file_get_f_pos(struct file* file)
{
    return (loff_t)file->f_op->llseek(file, 0, 1);
}


unsigned int file_get_f_flags(struct file* file)
{
    return (unsigned int)IBCS_SYSCALL(fcntl, file->fd, F_GETFD, (void*)0);
}


int file_set_f_flags(struct file* file, unsigned int new_flags)
{
    return (int)IBCS_SYSCALL(fcntl, file->fd, F_SETFD, (void*)new_flags);
}


/*
 * This is a replacement for this code, found in svr4/xti.c:
 *
 *	poll_table* wait = &wait_queue.pt;
 *	while (!(filep->f_op->poll(filep, wait) & mask)
 *			&& !signal_pending(current)) {
 *		current->state = TASK_INTERRUPTIBLE;
 *		wait = NULL;
 *		schedule();
 *	}
 *	current->state = TASK_RUNNING;
 */
int linux26_poll_wait(struct file* file, short poll_events)
{
    struct pollfd poll_struct = {.fd=file->fd, .events=poll_events};
    return IBCS_SYSCALL(poll, &poll_struct, 1, 0, 0);
}


/*
 * Replaces the code:
 *
 *	set_current_state(TASK_INTERRUPTIBLE);
 *	schedule_timeout (period_j);
 *	err = signal_pending(current) ? -EINTR : 0;
 *
 * Found in cxenix/misc.c
 */
int linux26_schedule_timeout(long period_in_jiffies)
{
    fd_set		zfdset;
    struct timeval	tv;

    FD_ZERO(&zfdset);
    tv.tv_sec = period_in_jiffies / HZ;
    tv.tv_usec = (period_in_jiffies - tv.tv_sec * HZ) * 1000 / HZ * 1000;
    return IBCS_SYSCALL(select, 0, &zfdset, &zfdset, &zfdset, &tv);
}


/*
 * Our "struct path" is a custom structure containing inodes, dentry_'s a
 * vfsmount and a super_block, all knitted together the way a kernel would
 * to it.  This does the knitting.
 */
static void path_construct(struct path* path)
{
    memset(path, '\0', sizeof(*path));
    path->mnt = &path->_mnt_real;
    path->dentry = &path->_dentry_real;
    struct inode* path_inode = &path->dentry->_d_inode_real;
    path->dentry->d_inode = path_inode;
    path_inode->i_dentry = path->dentry;
    struct super_block* super_block = &path->mnt->_mnt_sb_real;
    path->mnt->mnt_sb = super_block;
    super_block->s_type = &super_block->_s_type_real;
    struct dentry* root_dentry = &super_block->_s_root_dentry_real;
    super_block->s_root = root_dentry;
    struct inode* root_inode = &root_dentry->_d_inode_real;
    root_dentry->d_inode = root_inode;
    root_inode->i_dentry = root_dentry;
}


/*
 * Construct "struct path" from a "struct inode".
 */
static int inode_to_path(struct path* path, const struct inode* inode)
{
    path_construct(path);
    struct inode*	path_inode = path->dentry->d_inode;
    memcpy(path_inode, inode, sizeof(*path_inode));
    path_inode->i_dentry = path->dentry;
    /*
     * Scan /proc/mounts for all mount points, adding new ones as we go.
     *
     * A typical line from /proc/mounts:
     *     /dev/nvme0n1p1 / ext4 rw,relatime,errors=remount-ro 0 0
     * It the first 3 words we are interested in.
     */
    struct file*	mounts = linux26_fopen("/proc/mounts", O_RDONLY);
    if (IBCS_IS_ERR(mounts)) {
	return (int)mounts;
    }
    /*
     * Read in the new ones.
     */
    int			retval;
    char		buf[PATH_MAX * 3];
    loff_t		loff = 0;
    char*		pos = buf;
    int			remaining = 0;
    char*		words[5];
    struct kstat	sb_kstat;
    do {
        /*
	 * Find the end of the current line, slurping more data if needed.
	 */
	char* nl = memchr(pos, '\n', remaining);
	if (nl == (char*)0) {
	    if (remaining == sizeof(buf)) {
		retval = -ENOENT;
		goto out;
	    }
	    if (pos != buf) {
		memcpy(buf, pos, remaining);
		pos = buf;
	    }
	    int got = mounts->f_op->read(
		mounts, &buf[remaining], sizeof(buf) - remaining, &loff);
	    if (IBCS_IS_ERR(got)) {
	        retval = got;
		goto out;
	    }
	    if (got == 0) {
	        retval = -ENOENT;
		goto out;
	    }
	    remaining += got;
	    continue;
	}
	/*
	 * If we loop beyond this line we will process the next line.
	 */
	char* p = pos;
	pos = nl + 1;
	int linelen = pos - p;
	remaining -= linelen;
	/*
	 * Break the line into words.
	 */
	int i;
	words[0] = p;
	words[ARRAY_SIZE(words) -1] = (char*)0;
	for (i = 1; i < ARRAY_SIZE(words); i += 1) {
	    words[i] = memchr(p, ' ', linelen);
	    if (words[i] == (char*)0 || words[i] >= nl) {
		break;
	    }
	    *words[i]++ = '\0';
	    linelen -= words[i] - p;
	    p = words[i];
	}
	if (!words[ARRAY_SIZE(words) -1]) {
	    continue;
	}
	/*
	 * Do a stat() on the mount point.
	 */
	retval = vfs_stat(words[1], &sb_kstat);
	if (IBCS_IS_ERR(retval)) {
	    continue;
	}
    } while (sb_kstat.dev != inode->i_dev);
    /*
     * We've found it.
     */
    struct super_block*	super_block = path->mnt->mnt_sb;
    super_block->s_dev = sb_kstat.dev;
    struct file_system_type* s_type = &super_block->_s_type_real;
    strncpy(s_type->name, words[2], sizeof(s_type->name) - 1);
    super_block->s_type = s_type;
    struct dentry*	root_dentry = super_block->s_root;
    kstat_to_inode(root_dentry->d_inode, &sb_kstat);
    /*
      * set the flags from the mount options.
      */
    char*		mnt_opt = words[3];
    while (*mnt_opt != '\0') {
	char*		end_opt = strchr(mnt_opt, ',');
	if (end_opt) {
	    *end_opt++ = '\0';
	} else {
	    end_opt = strchr(mnt_opt, '\0');
	}
	if (!strcmp(mnt_opt, "ro")) {
	    super_block->s_flags |= MS_RDONLY;
	}
	if (!strcmp(mnt_opt, "nosuid")) {
	    path->mnt->mnt_flags |= MNT_NOSUID;
	}
	mnt_opt = end_opt;
    }
    struct statfs	statfs;
    retval = IBCS_SYSCALL(statfs, words[1], &statfs);
    if (IBCS_IS_ERR(retval)) {
	goto out;
    }
    statfs_to_kstatfs(&path->mnt->mnt_sb->_s_kstatfs_real, &statfs);
out:
    linux26_fclose(mounts);
    return retval;
}


int user_path(const char* filename, struct path* path)
{
    struct kstat	kstat;
    struct inode	inode;

    int retval = vfs_stat(filename, &kstat);
    if (!IBCS_IS_ERR(retval)) {
	kstat_to_inode(&inode, &kstat);
	retval = inode_to_path(path, &inode);
    }
    return retval;
}


int linux26_user_fpath(int fd, struct path* path)
{
    struct kstat		kstat;
    int retval = vfs_fstat(fd, &kstat);
    if (!IBCS_IS_ERR(retval)) {
	struct inode inode;
	kstat_to_inode(&inode, &kstat);
	retval = inode_to_path(path, &inode);
    }
    return retval;
}


/*
 * The kernel's "vfs" layer is interface between the rest of the kernel
 * and the various file system drivers.  Thus when someone calls "vfs_readdir"
 * the actual file system driver ends up being asked to return a list of
 * the directory contents.  We emulate the same functions here using user
 * space syscalls.
 */
int vfs_readdir(struct file* file, filldir_t filldir, void* dirent)
{
    struct {
	struct linux_dirent64 dirent;
	char		name[PATH_MAX];
    }			buf;
    int			retval;

    for (;;) {
	retval = IBCS_SYSCALL(getdents64, file->fd, &buf.dirent, sizeof(buf));
	if (IBCS_IS_ERR(retval) || retval <= sizeof(buf.dirent)) {
	    break;
	}
	retval = (*filldir)(
	    dirent, buf.dirent.d_name, strlen(buf.dirent.d_name),
	    buf.dirent.d_off, buf.dirent.d_ino, buf.dirent.d_type
	);
	if (IBCS_IS_ERR(retval)) {
	    break;
	}
    }
    return retval;
}


int vfs_fstat(int fd, struct kstat* kstat)
{
    struct stat64	st64;
    int			ret;

    ret = IBCS_SYSCALL(fstat64, fd, &st64);
    if (!IBCS_IS_ERR(ret)) {
	stat64_to_kstat(kstat, &st64);
    }
    return ret;
}


int vfs_lstat(const char* name, struct kstat* kstat)
{
    struct stat64	st64;
    int			ret;
    char		mapped_name[PATH_MAX];

    filemap_map(mapped_name, sizeof(mapped_name), name);
    ret = IBCS_SYSCALL(lstat64, mapped_name, &st64);
    if (!IBCS_IS_ERR(ret)) {
	stat64_to_kstat(kstat, &st64);
    }
    return ret;
}


int vfs_stat(const char* name, struct kstat* kstat)
{
    struct stat64	st64;
    int			ret;
    char		mapped_name[PATH_MAX];

    filemap_map(mapped_name, sizeof(mapped_name), name);
    ret = IBCS_SYSCALL(stat64, mapped_name, &st64);
    if (!IBCS_IS_ERR(ret)) {
	stat64_to_kstat(kstat, &st64);
    }
    return ret;
}


int vfs_statfs(const struct dentry* dentry, struct kstatfs* kstatfs)
{

    struct path		path;
    int			retval;

    retval = inode_to_path(&path, dentry->d_inode);
    if (!IBCS_IS_ERR(retval)) {
	memcpy(kstatfs, &path.mnt->mnt_sb->_s_kstatfs_real, sizeof(*kstatfs));
    }
    return retval;
}


/*
 * Our emulation of the kernel's linux_binfmt system.
 *
 * A linux_binfmt loads an executable file into RAM so it can be executed.
 * There is one for each format the kernel supports (eg, ELF).
 */
int register_binfmt(struct linux_binfmt* binfmt)
{
    list_add(&binfmt->lh, &linux_binfmts);
    return 0;
}


void unregister_binfmt(struct linux_binfmt* binfmt)
{
    list_del(&binfmt->lh, &linux_binfmts);
}


/*
 * Load an executable file. This means asking each linux_binfmt registered
 * with register_binfmt() to load the file, and stopping at the first one
 * that succeeds or returning an error if none succeed.
 */
int load_binfmt(const char* executable_path)
{
    struct linux_binprm	binprm;
    struct file*	file;
    int			retval;
    loff_t		loff;
    struct kstat	kstat;

    memset(&binprm, '\0', sizeof(binprm));
    binprm.mm = current->mm;
    retval = IBCS_SYSCALL(access, executable_path, S_IXOTH);
    if (IBCS_IS_ERR(retval)) {
	goto out;
    }
    file = linux26_fopen(executable_path, O_RDONLY);
    if (IBCS_IS_ERR(file)) {
	retval = (int)file;
	goto out;
    }
    binprm.file = file;
    if (IBCS_IS_ERR(retval = vfs_fstat(binprm.file->fd, &kstat))) {
	goto out;
    }
    if (!S_ISREG(kstat.mode)) {
	retval = -ENOEXEC;
	goto out;
    }
    loff = 0;
    retval = binprm.file->f_op->read(
	binprm.file, binprm.buf, sizeof(binprm.buf), &loff);
    if (IBCS_IS_ERR(retval)) {
	goto out;
    }
    if (retval != sizeof(binprm.buf)) {
	retval = -ENOEXEC;
	goto out;
    }
    struct list_head* binfmt_list;
    struct pt_regs regs;
    list_for_each(binfmt_list, &linux_binfmts) {
	struct linux_binfmt* binfmt =
	    list_entry(binfmt_list, struct linux_binfmt, lh);
	memset(&regs, '\0', sizeof(regs));
	retval = binfmt->load_binary(&binprm, &regs);
	if (retval != -ENOEXEC) {
	    goto out;
	}
    }
out:
    if (binprm.file != (struct file*)0) {
	linux26_fclose(binprm.file);
    }
    if (!IBCS_IS_ERR(retval)) {
	init_brk();
    }
    return retval;
}


/*
 * Implement the brk() syscall.
 *
 * The addr paramater is mostly useless to us: is the current break.
 */
unsigned long do_brk(unsigned long addr, unsigned long len)
{
    struct mm_struct*	mm = current->mm;

    /*
     * Resize the existing brk() mmap to the new size, if it's changed.
     */
    unsigned long	new_brk_end = PAGE_ALIGN(addr + len);
    if (new_brk_end != mm->ibcs_brk_mmap_max) {
	if (new_brk_end < mm->ibcs_brk_mmap_min) {
	    return -ENOMEM;
	}
	if (new_brk_end == mm->ibcs_brk_mmap_min) {
	    new_brk_end = PAGE_ALIGN(mm->ibcs_brk_mmap_min + 1);
	}
	/*
	 * We know where the brk() mmap is, so we just have adjust it to
	 * the new brk().
	 */
	unsigned long	new_addr = (unsigned long)IBCS_SYSCALL(
	    mremap, (void*)mm->ibcs_brk_mmap_addr,
	    mm->ibcs_brk_mmap_max - mm->ibcs_brk_mmap_addr,
	    new_brk_end - mm->ibcs_brk_mmap_addr, 0);
	if (IBCS_IS_ERR(addr)) {
	    return new_addr;
	}
	mm->ibcs_brk_mmap_max = new_brk_end;
    }
    return addr;
}


/*
 * Our emulation of the kernel's exec_domain.  This was provided in the
 * 2.6.32 kernel, but was ripped out by 4.0.  An exec_domain is just a vtable
 * for the kernel's implementation of the user space API.  There is of course
 * a native Linux one, but the kernel used to allow for multiple of them via
 * the exec_domain mechanism.  This was used to run binaries from other
 * systems like Xenis and SysV.
 *
 * The exec_domain is identified by it's exec_domain->personality, which is
 * just an integer make up of a "id" in the lower order byte and some flags
 * that could further refine execution.
 */
static struct exec_domain* lookup_exec_domain(u_long personality)
{
    struct exec_domain*	ep;
    u_long		pers = personality(personality);

    for (ep = exec_domains; ep; ep = ep->next) {
	if (pers >= ep->pers_low && pers <= ep->pers_high) {
	  return ep;
	}
    }
    return (struct exec_domain*)0;
}


int register_exec_domain(struct exec_domain* ep)
{
    struct exec_domain*	tmp;
    int			err = -EBUSY;

    if (ep == NULL) {
	return -EINVAL;
    }
    if (ep->next != NULL) {
	return -EBUSY;
    }
    for (tmp = exec_domains; tmp; tmp = tmp->next) {
	if (tmp == ep)
	    goto out;
    }
    ep->next = exec_domains;
    exec_domains = ep;
    err = 0;
out:
    return err;
}


int unregister_exec_domain(struct exec_domain* ep)
{
    struct exec_domain** epp;

    epp = &exec_domains;
    for (epp = &exec_domains; ep != *epp; epp = &(*epp)->next) {
	if (!*epp) {
	    return -EINVAL;
	}
    }
    *epp = ep->next;
    ep->next = NULL;
    return 0;
}


int set_personality(u_long personality)
{
    current->exec_domain = lookup_exec_domain(personality);
    current->personality = personality;
    return 0;
}


int sprintf(char* out, const char* fmt, ...)
{
    int			ret;
    va_list		list;

    va_start(list, fmt);
    ret = ibcs_vfmt(out, 999999999, fmt, list);
    va_end(list);
    return ret;
}


int snprintf(char* out, size_t size, const char* fmt, ...)
{
    int			ret;
    va_list		list;

    va_start(list, fmt);
    ret = ibcs_vfmt(out, size, fmt, list);
    va_end(list);
    return ret;
}


/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long simple_strtoul(const char* cp, const char** endp, unsigned int base)
{
    unsigned long result = 0;

    if (cp[0] == '0') {
	char lcase = cp[1] | 0x20;
	if (lcase == 'x') {
	    base = 16;
	    cp += 2;
	} else if (lcase == 'o') {
	    base = 8;
	    cp += 2;
	}
    }
    if (base == 0) {
	base = 10;
    }
    for (;; cp += 1) {
	int digit = *cp | 0x20;
	if (digit >= '0' && digit <= '9') {
	    digit -= '0';
	} else if (digit >= 'a' && digit <= 'f') {
	    digit = digit - 'a' + 10;
	} else {
	    break;
	}
	result = result * base + digit;
    }
    if (endp != (const char**)0) {
	*endp = cp;
    }
    return result;
}


/*
 * Our emulation of "printk".
 */
int printk(const char* fmt, ...)
{
    va_list		list;

    va_start(list, fmt);
    int ret = vprintk(fmt, list);
    va_end(list);
    return ret;
}


int vprintk(const char* fmt, va_list list)
{
    char		level = 'd';

    if (fmt[0] == '<' && fmt[1] == '\0' && fmt[2] != '>') {
	level = fmt[1];
    }
    if (abi_trace_fd != -1) {
	ibcs_vwritef(abi_trace_fd, fmt, list);
    }
    if (level >= '0' && level <= '4') {
	ibcs_vwritef(2, fmt, list);
    }
    if (level <= '2') {
	IBCS_SYSCALL(exit, 1);
    }
    return 0;
}


void start_thread(
    struct pt_regs* regs, unsigned long new_ip, unsigned long new_sp
) {
    current->start_addr = (void (*)(void))new_ip;
}


int get_unused_fd(void)
{
    struct file* file = linux26_fopen("/dev/null", O_RDONLY);
    int fd = file->fd;
    fput(file);
    return fd;
}

/*
 * Emulate Link semaphore operations.
 */
static inline int linux26_futex(int* uaddr, int futex_op, int val)
{
    return IBCS_SYSCALL(futex, uaddr, futex_op, val, NULL, NULL, 0);
}


void down_read(struct rw_semaphore* sem)
{
    while (!__sync_bool_compare_and_swap(&sem->rw_semaphore_futex, 1, 0)) {
	linux26_futex(&sem->rw_semaphore_futex, FUTEX_WAIT, 0);
    }
}


void up_read(struct rw_semaphore* sem)
{
    if (__sync_bool_compare_and_swap(&sem->rw_semaphore_futex, 0, 1)) {
	linux26_futex(&sem->rw_semaphore_futex, FUTEX_WAKE, 1);
    }
}


/*
 * Emulate reading the inode.
 */
struct super_block* inode_get_i_sb(struct inode* i, struct path* path)
{
    return path->mnt->mnt_sb;
}


/*
 * Character device emulation.  We see if they are opening /dev/DEVICE_NAME
 * and redirect to socksys_fops if so.
 */
struct linux26_chrdev
{
    unsigned int	chrdev_major;
    const char*		chrdev_name;
    const struct file_operations* chrdev_fops;
};

static struct linux26_chrdev chrdev_list[5];

int register_chrdev(
    unsigned int major, const char* name, const struct file_operations* fops)
{
    int			free = -1;
    int			i;

    for (i = ARRAY_SIZE(chrdev_list) - 1; i >= 0; i -= 1) {
	if (chrdev_list[i].chrdev_major == 0) {
	    free = i;
	    continue;
	}
	if (chrdev_list[i].chrdev_major == major) {
	    return -EEXIST;
	}
	if (!strcmp(chrdev_list[i].chrdev_name, name)) {
	    return -EEXIST;
	}
    }
    if (free == -1) {
	return -ENOMEM;
    }
    chrdev_list[free].chrdev_major = major;
    chrdev_list[free].chrdev_name = name;
    chrdev_list[free].chrdev_fops = fops;
    return 0;
}


void unregister_chrdev(unsigned int major, const char* name)
{
    int			i;

    for (i = ARRAY_SIZE(chrdev_list) - 1; i >= 0; i -= 1) {
	if (
	    chrdev_list[i].chrdev_major == major ||
	    !strcmp(chrdev_list[i].chrdev_name, name)
	) {
	    memset(&chrdev_list[i], 0, sizeof(chrdev_list[i]));
	}
    }
}


/*
 * Return the open file handle if any of the character device drivers
 * are interested.
 *
 * Some background.  Linux now has system calls to handle sockets and stream
 * but on older systems these were created by opening special character
 * devices, and perhaps doing ioctls() on them.  The kernel implementation
 * of iBCS created the character device and then when it knew the type of
 * socket wanted it created the Linux equivalent by doing syscalls.  The user
 * space implementation can't create new kernel devices of course.
 *
 * The approach taken here is register_chrdev() provides an
 * file_operations->linux26_chrdev_open(), which we can call to see if the file
 * name passed to open() is an character device we are emulating.  If so
 * it returns a (struct file*) for a file it has created and put into the
 * file handle table.  It must contain a real file handle, and typically
 * will have the file->f_dentry->d_inode in the way it expects.
 *
 * To support this charade all syscall iBCS syscalls with f_op implementations
 * must go via linux26_fop_xxx equivalents, so we can redirect to the
 * file_operations the character device provides.
*/
int find_chrdev(const char* path, int mode, int flags, struct linux26_chrdev** chrdev)
{
    int			i;

    if (strncmp("/dev/", path, 5) == 0) {
	for (i = 0; i < ARRAY_SIZE(chrdev_list); i += 1) {
	    const struct file_operations* fops = chrdev_list[i].chrdev_fops;
	    if (fops != (struct file_operations*)0) {
		int rdev = fops->linux26_chrdev_open(path + 5, mode, flags);
		if (!IBCS_IS_ERR(rdev)) {
		    if (chrdev) {
			*chrdev = &chrdev_list[i];
		    }
		    return rdev;
		}
	    }
	}
    }
    return -ENOENT;
}


int open_chrdev(const char* path, int mode, int flags)
{
    struct linux26_chrdev* chrdev;
    int			rdev = find_chrdev(path, mode, flags, &chrdev);

    if (IBCS_IS_ERR(rdev)) {
	return rdev;
    }
    /*
    * The chrdev likes it.  Create a file for it.
    */
    struct file* file = linux26_fopen("/dev", 0);
    file->f_dentry->d_inode->i_mode = S_IFCHR | 0666;
    file->f_dentry->d_inode->i_rdev = rdev;
    memcpy(&file->_f_op_real, file->f_op, sizeof(file->_f_op_real));
    const struct file_operations* fops = chrdev->chrdev_fops;
    file->_f_op_real.linux26_chrdev_open = fops->linux26_chrdev_open;
    if (fops->llseek != 0) {
	file->_f_op_real.llseek = fops->llseek;
    }
    if (fops->mmap != 0) {
	file->_f_op_real.mmap = fops->mmap;
    }
    if (fops->read != 0) {
	file->_f_op_real.read = fops->read;
    }
    if (fops->write != 0) {
	file->_f_op_real.write = fops->write;
    }
    if (fops->open != 0) {
	file->_f_op_real.open = fops->open;
    }
    if (fops->release != 0) {
	file->_f_op_real.release = fops->release;
    }
    file->f_op = &file->_f_op_real;
    file->f_op->open(file->f_dentry->d_inode, file);
    fput(file);
    return file->fd;
}


/*
 * kmalloc() and kfree() can just use our malloc().
 */
void* kmalloc(size_t size, gfp_t flags)
{
    return ibcs_malloc(size);

}


void kfree(const void* p)
{
    return ibcs_free((void*)p);
}


/*
 * Implement the cap_raise(), cap_lower(), cap_raised() kernel calls.
 */
int linux26_capability(unsigned int flag, int action)
{
    struct __user_cap_header_struct	hdr;
    struct __user_cap_data_struct	caps[VFS_CAP_U32_3] = { 0 }; /* Shutup gcc warning */
    int					retval;
    u64					old_effective;
    u64					new_effective;
    u64					cap_bit = 1ULL << flag;
    

    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    hdr.pid = current->pid;
    retval = IBCS_SYSCALL(capget, &hdr, caps);
    if (IBCS_IS_ERR(retval)) {
	return retval;
    }
    old_effective = caps[0].effective | ((u64)caps[1].effective << 32);
    switch (action) {
	case -1:
	    new_effective = old_effective & ~cap_bit;
	    break;
	case 0:
	    new_effective = old_effective;
	    break;
	case 1:
	    new_effective = old_effective | cap_bit;
	    break;
	default:
	    return -EINVAL;
    }
    if (new_effective != old_effective) {
        caps[0].effective = new_effective;
        caps[1].effective = new_effective >> 32;
	retval = IBCS_SYSCALL(capset, &hdr, caps);
	if (IBCS_IS_ERR(retval)) {
	    return retval;
	}
    }
    return new_effective & cap_bit;
}


/*
 * The modues implementing the personalities for this program were in a past
 * life kernel modules.  They now are run by us in user space of course, but
 * since I kept changes to their source to a minimum they still think they are
 * kernel modules.  Kernel modules declare their an initialisation function
 * they expect the kernel to call when they are loaded using:
 *
 *     module_init(coff_module_init)
 *
 * We must arrange for those functions to be called just as the kernel used
 * to.  We arrange for the macro above to prefix the function with the gcc
 * __attribute__((constructor)) decorator, which is gcc'ism that puts the
 * passed pointer into a ELF section called ".init_array".
 *
 * Our task here is to extract those function pointers and call them.
 */
static void module_initialise(void);

__attribute__((section(".module_initialise_reloc"))) void (*module_initialise_reloc)() = module_initialise;

static void module_initialise()
{
    const Elf32_Ehdr*	ehdr;
    const char*		exe_filename_fmt = "/proc/%d/exe";
    int			exe_filename_size = ibcs_fmt((char*)0, 0, exe_filename_fmt, current->pid);
    char		exe_filename[exe_filename_size];
    struct file*	exe_file;
    long		exe_map_size;
    int			init_idx;
    int			init_sh_no;
    struct kstat	kstat;
    size_t		reloc;
    int			reloc_sh_no;
    int			ret;
    int			sh_no;
    const Elf32_Shdr*	shdr;
    const char*		strings;

    /*
     * The first step if to open our executable and map it into memory.
     * It is an ELF format file, of course.
     */
    sprintf(exe_filename, exe_filename_fmt, current->pid);
    exe_file = linux26_fopen(exe_filename, O_RDONLY);
    if (IBCS_IS_ERR(exe_file)) {
	ibcs_fatal_syscall((int)exe_file, "module_initialise open(\"%s\")", exe_filename);
    }
    ret = vfs_fstat(exe_file->fd, &kstat);
    if (IBCS_IS_ERR(ret)) {
	ibcs_fatal_syscall(ret, "module_initialise stat(\"%s\")", exe_filename);
    }
    exe_map_size = PAGE_ALIGN(kstat.size);
    ehdr = (const Elf32_Ehdr*)do_mmap(
	exe_file, 0, exe_map_size, PROT_READ, MAP_PRIVATE | MAP_32BIT, 0);
    if (IBCS_IS_ERR(ehdr)) {
	ibcs_fatal_syscall((int)ehdr, "module_initialise mmap(\"%s\")", exe_filename);
    }
    if (ehdr->e_shoff == 0) {
        goto out;
    }
    /*
     * The next step is to find the two ELF sections we need:
     *     .init_array, created when gcc sees __attribute__((constructor))
     *     .module_initialise_reloc, which is created above.
     */
    shdr = (const Elf32_Shdr*)((const char*)ehdr + ehdr->e_shoff);
    strings = (const char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
    init_sh_no = -1;
    reloc_sh_no = -1;
    for (sh_no = 0;  sh_no < ehdr->e_shnum; sh_no += 1) {
	if (!strcmp(".module_initialise_reloc", &strings[shdr[sh_no].sh_name])) {
	   reloc_sh_no = sh_no;
	}
	if (!strcmp(".init_array", &strings[shdr[sh_no].sh_name])) {
	   init_sh_no = sh_no;
	}
    }
    if (init_sh_no == -1) {
        goto out;
    }
    if (reloc_sh_no == -1) {
        ibcs_fatal("internal error: can not find elf section 'module_initialise_reloc'\n");
    }
    /*
     * The kernel has loaded the ELF file at some random address in memory,
     * which means all the code in the ELF file has been relocated by an
     * unknown amount.  This means functions pointed to by the .init_array
     * also have to relocated by that same unknown amount which makes the raw
     * pointers in the ELF file useless.
     *
     * However, everything has been relocated by that amount, including this
     * function.  So above we arrange for a pointer to this function to be
     * stored in the ELF file, and now we can compare to what is in the ELF
     * file to its actual address to derive how far everything has been
     * relocated.
     */
    reloc =
	(size_t)module_initialise -
	*(size_t*)((char*)ehdr + shdr[reloc_sh_no].sh_offset);
    /*
     * Yak shaving done, we can finally call all the module initialisation
     * functions declared with module_init() macro in linix/modules.h.
     */
    char** init_array = (char**)((char*)ehdr + shdr[init_sh_no].sh_offset);
    int init_count = shdr[init_sh_no].sh_size / sizeof(*init_array);
    for (init_idx = 0; init_idx < init_count; init_idx += 1) {
	(*(void (*)(void))(init_array[init_idx] + reloc))();
    }
out:
    do_munmap(current->mm, (unsigned long)ehdr, exe_map_size);
    linux26_fclose(exe_file);
}


/*
 * Initialise the data structures used by this file.
 */
void linux26_compat_init(const char* ibcs_program_name)
{
    struct utsname	uts;

    init_rwsem(&current->mm->mmap_sem);
    init_rwsem(&uts_sem);
    IBCS_SYSCALL(uname, &uts);
    strncpy(init_uts_ns.name.sysname, uts.sysname, sizeof(init_uts_ns.name.sysname) - 1);
    init_uts_ns.name.sysname[sizeof(init_uts_ns.name.sysname) - 1] = '\0';
    strncpy(init_uts_ns.name.nodename, uts.nodename, sizeof(init_uts_ns.name.nodename) - 1);
    init_uts_ns.name.nodename[sizeof(init_uts_ns.name.nodename) - 1] = '\0';
    strncpy(init_uts_ns.name.release, uts.release, sizeof(init_uts_ns.name.release) - 1);
    init_uts_ns.name.release[sizeof(init_uts_ns.name.release) - 1] = '\0';
    strncpy(init_uts_ns.name.version, uts.version, sizeof(init_uts_ns.name.version) - 1);
    init_uts_ns.name.version[sizeof(init_uts_ns.name.version) - 1] = '\0';
    strncpy(init_uts_ns.name.machine, uts.machine, sizeof(init_uts_ns.name.machine) - 1);
    init_uts_ns.name.machine[sizeof(init_uts_ns.name.machine) - 1] = '\0';
    strncpy(init_uts_ns.name.domainname, uts.__domainname, sizeof(init_uts_ns.name.domainname) - 1);
    init_uts_ns.name.domainname[sizeof(init_uts_ns.name.domainname) - 1] = '\0';
    current->pid = IBCS_SYSCALL(getpid);
    strncpy(current->comm, ibcs_program_name, sizeof(current->comm) - 1);
    current->comm[sizeof(current->comm) - 1] = '\0';
    current->files->fdt = &current->files->fdtab;
    for (int i = 1; i < ARRAY_SIZE(current->_linux26_sigtab); i += 1) {
        IBCS_SYSCALL(
	    sigaction, i, (struct sigaction*)0, &current->_linux26_sigtab[i]);
    }
    module_initialise();
}


/*
 * Set up the socket data.
 */
struct socket* inode_get__i_socket(struct inode* inode)
{
    if (inode->_i_socket == (struct socket*)0) {
	size_t optlen = sizeof(inode->_i_socket->type);
	inode->_i_socket = &inode->_i_socket_real;
	inode->_i_socket->type = IBCS_SYSCALL(
	    getsockopt, inode->i_dentry->_d_file->fd, SOL_SOCKET, SO_TYPE,
	    &inode->_i_socket->type, &optlen);
	inode->_i_socket->file = inode->i_dentry->_d_file;
    }
    return inode->_i_socket;
}


/*
 * Get the socket of a fd.
 */
struct socket* sockfd_lookup(int fd, int* err)
{
    struct file*	file = fget(fd);

    *err = 0;
    return inode_get__i_socket(file->f_dentry->d_inode);
}


/*
 * Get the socket of a fd.
 */
void sockfd_put(struct socket* sock)
{
    fput(sock->file);
}


/*
 * Emulate the kernel's sock_sendmsg() API.
 */
int sock_sendmsg(struct socket* sock, struct msghdr* msg, size_t len)
{
    return IBCS_SYSCALL(sendmsg, sock->file->fd, msg, msg->msg_flags);
}


int sock_recvmsg(struct socket* sock, struct msghdr* msg, size_t total_len, int flags)
{
    return IBCS_SYSCALL(recvmsg, sock->file->fd, msg, msg->msg_flags);
}


/*
 * The functions below emulate kernel 2.6.32 user space syscalls.  Most of
 * them can be passed directly through to the kernel we are running on,
 * but some can't and are implemented by the functions named after them
 * below.
 */
typedef long long (*linux26_syscall_t)(int syscall_no, va_list list);

static long long linux26_brk(int syscall_no, va_list list)
{
    struct mm_struct*	mm = current->mm;
    unsigned long	new_brk = va_arg(list, unsigned long);

    if (new_brk < mm->ibcs_brk_mmap_min) {
	return -ENOMEM;
    }
    unsigned long	new_brk_page = PAGE_ALIGN(new_brk);
    unsigned long	old_brk_page = PAGE_ALIGN(mm->brk);
    if (new_brk_page != old_brk_page) {
	unsigned long retval = do_brk(
	    old_brk_page, new_brk_page - old_brk_page);
	if (IBCS_IS_ERR(retval)) {
	    return (int)retval;
	}
    }
    mm->brk = new_brk;
    return new_brk;
}

static long long linux26_link(int syscall_no, va_list list)
{
    /*
     * A syscall that has a path string as its first argument.  All we
     * have to do is map the path.
     */
    const char*		a1 = va_arg(list, const char*);
    const char*		a2 = va_arg(list, const char*);
    char		mapped_name1[PATH_MAX];
    char		mapped_name2[PATH_MAX];
    
    filemap_map(mapped_name1, sizeof(mapped_name1), a1);
    filemap_map(mapped_name2, sizeof(mapped_name2), a2);
    return ibcs_syscall(syscall_no, mapped_name1, mapped_name2);
}

static long long linux26_map_a1_fname(int syscall_no, va_list list)
{
    /*
     * A syscall that has a path string as its first argument.  All we
     * have to do is map the path.
     */
    char		mapped_name[PATH_MAX];
    
    filemap_map(mapped_name, sizeof(mapped_name), *(const char**)list);
    *(const char**)list = mapped_name;
    return ibcs_vsyscall(syscall_no, list);
}

static long long linux26_read(int syscall_no, va_list list)
{
    struct fdtable*	fdt = current->files->fdt;
    int			fd = va_arg(list, int);
    void*		buf = va_arg(list, void*);
    size_t		nbytes = va_arg(list, size_t);

    /*
     * If a chrdev has overridden read() call it's read instead.
     */
    if (
	fd < 0 || fd > fdt->max_fds || !fdt->fd[fd] ||
	!fdt->fd[fd]->f_op->read ||
	fdt->fd[fd]->f_op->read == file_operations_read
    ) {
	return IBCS_SYSCALL(read, fd, buf, nbytes);
    }
    loff_t pos = 0;		/* Our chrdev's don't care about seek pos */
    return fdt->fd[fd]->f_op->read(fdt->fd[fd], buf, nbytes, &pos);
}


static long long linux26_write(int syscall_no, va_list list)
{
    struct fdtable*	fdt = current->files->fdt;
    int			fd = va_arg(list, int);
    void*		buf = va_arg(list, void*);
    size_t		nbytes = va_arg(list, size_t);

    /*
     * If a chrdev has overridden write() call it's write instead.
     */
    if (
	fd < 0 || fd > fdt->max_fds || !fdt->fd[fd] ||
	!fdt->fd[fd]->f_op->write ||
	fdt->fd[fd]->f_op->write == file_operations_write
    ) {
	return IBCS_SYSCALL(write, fd, buf, nbytes);
    }
    loff_t pos = 0;		/* Our chrdev's don't care about seek pos */
    return fdt->fd[fd]->f_op->write(fdt->fd[fd], buf, nbytes, &pos);
}


static long long linux26_open(int syscall_no, va_list list)
{
    const char*		fname = va_arg(list, const char*);
    int			mode = va_arg(list, int);
    int			flags = va_arg(list, int);
    char		mapped_name[PATH_MAX];

    filemap_map(mapped_name, sizeof(mapped_name), fname);
    int	fd = open_chrdev(mapped_name, mode, flags);
    /*
     * Go via open_chrdev() so our fake character devices get to perform
     * their magic.
     */
    if (!IBCS_IS_ERR(fd)) {
	return fd;
    }
    return IBCS_SYSCALL(open, mapped_name, mode, flags);
}


static long long linux26_close(int syscall_no, va_list list)
{
    struct fdtable*	fdt = current->files->fdt;
    int			fd = va_arg(list, int);

    /*
     * Clean up our file table on close.
     */
    if (fd >= 0 && fd < fdt->max_fds && fdt->fd[fd]) {
        fdt->fd[fd]->_f_isopen = 0;
	fdestroy(fdt->fd[fd]);
    }
    return IBCS_SYSCALL(close, fd);
}


static long long linux26_creat(int syscall_no, va_list list)
{
    const char*		fname = va_arg(list, const char*);
    int			mode = va_arg(list, int);
    char		mapped_name[PATH_MAX];

    filemap_map(mapped_name, sizeof(mapped_name), fname);
    int	fd = open_chrdev(mapped_name, mode, 0);
    /*
     * Go via open_chrdev() so our fake character devices get to perform
     * their magic.
     */
    if (!IBCS_IS_ERR(fd)) {
	return fd;
    }
    return IBCS_SYSCALL(creat, mapped_name, mode);

}


static long long linux26_dup(int syscall_no, va_list list)
{
    struct fdtable*	fdt = current->files->fdt;
    int			fd = va_arg(list, int);
    int			fd2 = -1;
    int			new_fd;

    if (syscall_no == __NR_dup2) {
	fd2 = va_arg(list, int);
    }
    new_fd = ibcs_syscall(syscall_no, fd, fd2);
    if (IBCS_IS_ERR(new_fd)) {
	return new_fd;
    }
    if (fd < 0 || fd > fdt->max_fds || !fdt->fd[fd]) {
	return new_fd;
    }
    /*
     * This might be a chrdev file created below.  Copy across the relevant
     * bits.
     */
    struct file* old_file = fdt->fd[fd];
    struct file* new_file = fget(new_fd);
    memcpy(
	&new_file->_f_op_real, &old_file->_f_op_real,
	sizeof(new_file->_f_op_real));
    new_file->f_op = old_file->f_op == &old_file->_f_op_real ?
	&new_file->_f_op_real : old_file->f_op;
    struct inode* old_inode = old_file->f_dentry->d_inode;
    struct inode* new_inode = new_file->f_dentry->d_inode;
    new_inode->i_ino = old_inode->i_ino;
    new_inode->i_dev = old_inode->i_dev;
    new_inode->i_rdev = old_inode->i_rdev;
    new_inode->i_mode = old_inode->i_mode;
    fput(new_file);
    return new_fd;
}


static long long linux26_access(int syscall_no, va_list list)
{
    const char*		fname = va_arg(list, const char*);
    int			mode = va_arg(list, int);
    int			rdev;
    char		mapped_name[PATH_MAX];

    filemap_map(mapped_name, sizeof(mapped_name), fname);
    /*
     * This might be a chrdev regstered with us in which case there won't be
     * a file to check, but we want him to think it exists anyway.
     */
    rdev = find_chrdev(mapped_name, mode, 0, NULL);
    if (!IBCS_IS_ERR(rdev)) {
	return 0;
    }
    return IBCS_SYSCALL(access, mapped_name, mode);
}


static long long linux26_stat(int syscall_no, va_list list)
{
    const char*		fname = va_arg(list, const char*);
    struct stat64*	st64 = va_arg(list, struct stat64*);
    int			rdev;
    int			ret;
    char		mapped_name[PATH_MAX];

    filemap_map(mapped_name, sizeof(mapped_name), fname);
    /*
     * This might be a chrdev regstered with us in which case there won't be
     * a file to stat, but we want him to think it exists anyway.
     */
    rdev = find_chrdev(mapped_name, 0, 0, NULL);
    if (IBCS_IS_ERR(rdev)) {
	ret = IBCS_SYSCALL(stat64, mapped_name, st64);
    } else {
	ret = IBCS_SYSCALL(stat64, "/dev", st64);
	st64->st_mode = S_IFCHR | 0666;
	st64->st_rdev = rdev;
    }
    return ret;
}


static long long linux26_fstat(int syscall_no, va_list list)
{
    struct fdtable*	fdt = current->files->fdt;
    int			fd = va_arg(list, int);
    struct stat64*	st64 = va_arg(list, struct stat64*);
    int			ret;

    ret = IBCS_SYSCALL(fstat64, fd, st64);
    if (IBCS_IS_ERR(ret)) {
	return ret;
    }
    /*
     * If this is a chrdev is will have set a fake mode & rdev.
     */
    if (fd < 0 || fd > fdt->max_fds || !fdt->fd[fd]) {
	return ret;
    }
    st64->st_mode = fdt->fd[fd]->f_dentry->d_inode->i_mode;
    st64->st_rdev = fdt->fd[fd]->f_dentry->d_inode->i_rdev;
    return ret;
}


static long long linux26_rt_sigaction(int syscall_no, va_list list)
{
    int			signum = va_arg(list, int);
    struct sigaction*	act = va_arg(list, struct sigaction*);
    struct sigaction*	oldact = va_arg(list, struct sigaction*);

    /*
     * This is here for sysent.c/lcall_sigsegv().  The emulated program
     * can't be allowed to override the SIGSEGV sigaction() it installed.
     */
    if (signum == SIGSEGV && oldact) {
	*oldact = current->_linux26_sigtab[signum];
    }
    if (signum > 0 && signum < ARRAY_SIZE(current->_linux26_sigtab) && act) {
	current->_linux26_sigtab[signum] = *act;
    }
    if (signum == SIGSEGV) {
	return 0;
    }
    return ibcs_sigaction(signum, act, oldact);
}


static long long linux26_signal(int syscall_no, va_list list)
{
    int			signum = va_arg(list, int);
    __sighandler_t	handler = va_arg(list, __sighandler_t);
    struct sigaction	act;
    struct sigaction	oldact;
    int			err;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    act.sa_flags = SA_ONESHOT | SA_NOMASK;
    err = linux26_syscall(__NR_sigaction, signum, &act, &oldact);
    if (IBCS_IS_ERR(err)) {
        return err;
    }
    return (long)oldact.sa_handler;
}


static linux26_syscall_t linux26_syscall_tab[] = {
    [__NR_read] =	linux26_read,
    [__NR_write] =	linux26_write,
    [__NR_open] =	linux26_open,
    [__NR_close] =	linux26_close,
    [__NR_creat] =	linux26_creat,
    [__NR_link] =	linux26_link,
    [__NR_unlink] =	linux26_map_a1_fname,
    [__NR_execve] =	linux26_map_a1_fname,
    [__NR_chdir] =	linux26_map_a1_fname,
    [__NR_mknod] =	linux26_map_a1_fname,
    [__NR_chmod] =	linux26_stat,
    [__NR_chown] =	linux26_access,
    [__NR_oldfstat] =	linux26_stat,
    [__NR_dup] =	linux26_dup,
    [__NR_brk] =	linux26_brk,
    [__NR_signal] =	linux26_signal,
    [__NR_oldlstat] =	linux26_stat,
    [__NR_dup2] =	linux26_dup,
    [__NR_sigaction] =	linux26_rt_sigaction,
    [__NR_stat] = 	linux26_stat,
    [__NR_lstat] = 	linux26_stat,
    [__NR_fstat] = 	linux26_fstat,
    [__NR_rt_sigaction]=linux26_rt_sigaction,
};


long long linux26_syscall(int syscall_no, ...)
{
    long long		ret;
    va_list		args;

    va_start(args, syscall_no);
    if (
	syscall_no < 0 || syscall_no >= ARRAY_SIZE(linux26_syscall_tab) ||
	!linux26_syscall_tab[syscall_no]
    ) {
	ret = ibcs_vsyscall(syscall_no, args);
    } else {
        ret = linux26_syscall_tab[syscall_no](syscall_no, args);
    }
    va_end(args);
    return ret;
}

/*
 * Unit testing code.  This is run using "make tests" at the top level or
 * "make test-FILENAME" in the component directory.
 */
#ifdef UNIT_TEST
#include <unistd.h>

static int current_test_line_nr;
#define	MUST_BE_TRUE(line_nr, a)					\
    do { 								\
	current_test_line_nr = line_nr;					\
	if (!(a)) { 							\
	    fprintf(stderr, "%s line %d test failed, %s\n", __FILE__, line_nr, #a);	\
	    abort();							\
	}								\
    } while(0)

static int mock_syscall_access(const char* filename, int mode)
{
    if (!strstr(filename, "/test")) {
	extern int access(const char* filename, int mode);
	return access(filename, mode);
    }
    return 0;
}

static int mock_syscall_close(int fd)
{
    if (fd != 111) {
	return close(fd);
    }
    return 0;
}

static int mock_syscall_creat(const char* filename, int mode)
{
    return 0;
}

static int mock_syscall_capget(cap_user_header_t hdrp, cap_user_data_t datap)
{
    return 0;
}

static int mock_syscall_capset(cap_user_header_t hdrp, cap_user_data_t datap)
{
    return 0;
}

static void mock_syscall_exit(int status)
{
}

static int mock_syscall_fcntl(int fd, int op, void* buf)
{
    if (fd != 111) {
	extern int fcntl(int fd, int op, void* buf);
	return fcntl(fd, op, buf);
    }
    return 0;
}

static int mock_syscall_fstat64(int fd, struct stat64* st64)
{
    if (fd != 111) {
	extern int fstat64(int fd, struct stat64* st64);
	return fstat64(fd, st64);
    }
    return mock_syscall_stat64("/etc/passwd", st64);
}

static int mock_syscall_futex(
    int* uaddr, int futex_op, int val,
    const struct timespec* timeout,   /* or: uint32_t val2 */
    int* uaddr2, int val3)
{
    return 0;
}

static int mock_syscall_getdents64(
    unsigned int fd, struct linux_dirent64* dirp, unsigned int count)
{
    return 0;
}

static int mock_syscall_getpid(void)
{
    extern int getpid();
    return getpid();
}

static int mock_syscall_getsockopt(
    int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
    return 0;
}

static loff_t mock_syscall_lseek(int fd, loff_t loff, int whence)
{
    return loff;
}

static int mock_syscall_lstat64(const char* filename, struct stat64* st64)
{
    return 0;
}

static void* mock_syscall_mremap(
    void* old_address, size_t old_size, size_t new_size, int flags,
    ... /* void* new_address */)
{
    return 0;
}

static const char* mock_syscall_open_path;
static int mock_syscall_open(const char* path, int mode, int flags)
{
    mock_syscall_open_path = path;
    if (!strstr(path, "/TEST")) {
        extern int open(const char* pathname, int flags, mode_t mode);
	return open(path, mode, flags);
    }
    return 111;
}

static int mock_syscall_poll(
    struct pollfd* fds, int nfds, const struct timespec* tmo_p,
    const sigset_t* sigmask)
{
    return 0;
}

static const char* mock_syscall_read_buf;
static const char* mock_syscall_read_buf_prev;
static int mock_syscall_read_buf_pos;
static size_t mock_syscall_read(int fd, void* buf, size_t len)
{
    if (mock_syscall_read_buf != mock_syscall_read_buf_prev) {
	mock_syscall_read_buf_pos = 0;
	mock_syscall_read_buf_prev = mock_syscall_read_buf;
    }
    MUST_BE_TRUE(current_test_line_nr, mock_syscall_read_buf != (const char*)0);
    size_t bytes = strlen(mock_syscall_read_buf) - mock_syscall_read_buf_pos;
    if (bytes > 50) {
	bytes = 50;
    }
    memcpy((char*)buf, mock_syscall_read_buf + mock_syscall_read_buf_pos, bytes);
    mock_syscall_read_buf_pos += bytes;
    return bytes;
}

static int mock_syscall_recvmsg(int sockfd, const struct msghdr* msg, int flags)
{
    return 0;
}

static int mock_syscall_select(
    int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
    struct timeval* timeout)
{
    return 0;
}

static int mock_syscall_sendmsg(int sockfd, const struct msghdr* msg, int flags)
{
    return 0;
}

static int mock_syscall_sigaction(
    int signum, const struct sigaction* act, struct sigaction* oldact)
{
    return 0;
}

static int mock_syscall_stat64(const char* filename, struct stat64* st64)
{
    extern int stat64(const char* path, struct stat64* st64);
    return stat64(filename, st64);
}

static int mock_syscall_statfs(const char* filename, struct statfs* stfs)
{
    extern int statfs(const char* path, struct statfs* stfs);
    return statfs(mock_syscall_open_path, stfs);
}

static size_t mock_syscall_uname(struct utsname* uts)
{
    return 0;
}

static size_t mock_syscall_write(int fd, const void* buf, size_t len)
{
    return 0;
}

static void mock_ibcs_fatal(const char* message, ...)
{
}

static int mock_ibcs_vfmt(char* out, size_t len, const char* fmt, va_list list)
{
    char buffer[1024];
    if (out == (char*)0) {
	out = buffer;
    }
    return vsprintf(out, fmt, list) + 1;
}

static int mock_filemap_map(char* dst, size_t len, const char* path)
{
    strcpy(dst, path);
    return 0;
}

static int mock_ibcs_syscall(int syscall_no, ...)
{
    return 0;
}

static int mock_ibcs_vsyscall(int syscall_no, va_list list)
{
    return 0;
}

static int mock_syscall_getuid32(void)
{
    return 0;
}

static int mock_syscall_kill(pid_t pid, int sig)
{
    return 0;
}

static void* mock_syscall_mmap2(
    void* addr, size_t length, int prot, int flags, int fd, off_t pg_offset)
{
    extern void* mmap(
	void* addr, size_t length, int prot, int flags, int fd,
	off_t pg_offset);
    return (void*)mmap(addr, length, prot, flags, fd, pg_offset * PAGE_SIZE);
}

static int mock_syscall_munmap(void* addr, size_t length)
{
    extern int munmap(void* addr, size_t length);
    return munmap(addr, length);
}

static void mock_ibcs_fatal_syscall(int retval, const char* message, ...)
{
}

static int mock_ibcs_vwritef(int fd, const char* fmt, ...)
{
    return 0;
}

/*
 * Parsing /proc/mounts is a little complex, so test that.
 */
static void test_inode_path()
{
    struct path path;
    struct file* file = linux26_fopen("/etc", O_RDONLY);

    mock_syscall_read_buf =
	"tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=1617516k,mode=755 0 0\n"
	"/dev/nvme0n1p1 / ext4 ro,nosuid,relatime,errors=remount-ro 0 0\n";
    int retval = inode_to_path(&path, file->f_dentry->d_inode);
    MUST_BE_TRUE(__LINE__, retval == 0);
    MUST_BE_TRUE(__LINE__, path.mnt->mnt_flags & MNT_NOSUID);
    MUST_BE_TRUE(__LINE__, path.mnt->mnt_sb->s_flags & MS_RDONLY);
    mock_syscall_read_buf =
	"tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=1617516k,mode=755 0 0\n"
	"/dev/nvme0n1p1 / ext4 relatime,errors=remount-ro 0 0\n";
    retval = inode_to_path(&path, file->f_dentry->d_inode);
    MUST_BE_TRUE(__LINE__, retval == 0);
    MUST_BE_TRUE(__LINE__, !(path.mnt->mnt_flags & MNT_NOSUID));
    MUST_BE_TRUE(__LINE__, !(path.mnt->mnt_sb->s_flags & MS_RDONLY));
    linux26_fclose(file);
}

/*
 * chrdev's are rare and complex, so ensure they work.
 */
static ssize_t test_chrdev_fop_read(
    struct file* file, char* buf, size_t len, loff_t* loff
) {
    return len;
}

static int test_chrdev_fop_chrdev_open(const char* path, int mode, int flags)
{
    if (!strcmp(path, "TEST")) {
	return MKDEV(0xde, 99);
    }
    if (!strcmp(path, "TEST1")) {
	return MKDEV(0xab, 98);
    }
    return -ENOENT;
}

static struct file_operations test_chrdev_fops = {
    .read = test_chrdev_fop_read,
    .linux26_chrdev_open = test_chrdev_fop_chrdev_open,
};

static void test_chrdev()
{
    int retval;
    struct linux26_chrdev* pchrdev;
    struct file* f;

    retval = register_chrdev(0xde, "/dev/TEST", &test_chrdev_fops);
    MUST_BE_TRUE(__LINE__, retval == 0);
    retval = register_chrdev(0xde, "/dev/foo", &test_chrdev_fops);
    MUST_BE_TRUE(__LINE__, retval == -EEXIST);
    retval = register_chrdev(0xef, "/dev/TEST", &test_chrdev_fops);
    MUST_BE_TRUE(__LINE__, retval == -EEXIST);
    retval = register_chrdev(0xab, "/dev/TEST1", &test_chrdev_fops);
    MUST_BE_TRUE(__LINE__, retval == 0);
    pchrdev = (struct linux26_chrdev*)1;
    retval = find_chrdev("/dev/foo", 0, 0, &pchrdev);
    MUST_BE_TRUE(__LINE__, retval == -ENOENT);
    MUST_BE_TRUE(__LINE__, pchrdev == (struct linux26_chrdev*)1);
    retval = find_chrdev("/dev/TEST", 0, 0, &pchrdev);
    MUST_BE_TRUE(__LINE__, retval == MKDEV(0xde, 99));
    MUST_BE_TRUE(__LINE__, pchrdev->chrdev_major == 0xde);
    MUST_BE_TRUE(__LINE__, !strcmp(pchrdev->chrdev_name, "/dev/TEST"));
    MUST_BE_TRUE(__LINE__, pchrdev->chrdev_fops == &test_chrdev_fops);
    retval = open_chrdev("/dev/foo", 0, 0);
    MUST_BE_TRUE(__LINE__, retval == -ENOENT);
    retval = open_chrdev("/dev/TEST1", 0, 0);
    MUST_BE_TRUE(__LINE__, mock_syscall_fcntl(retval, F_GETFL, (void*)0) >= 0);
    f = fget(retval);
    MUST_BE_TRUE(__LINE__, f->f_op->llseek == file_operations_llseek);
    MUST_BE_TRUE(__LINE__, f->f_op->read == test_chrdev_fop_read);
    MUST_BE_TRUE(__LINE__, f->f_dentry->d_inode->i_rdev = MKDEV(0xab, 98));
    MUST_BE_TRUE(__LINE__, f->f_dentry->d_inode->i_mode = S_IFCHR | 0666);
    linux26_fclose(f);
    MUST_BE_TRUE(__LINE__, mock_syscall_fcntl(retval, F_GETFL, (void*)0) == -1);
}

void unit_test_ibcs_linux26_compat_c(int ac, const char** av, const char** ep)
{
    linux26_compat_init("unit-test");
    module_initialise();
    test_inode_path();
    test_chrdev();
}
#endif
