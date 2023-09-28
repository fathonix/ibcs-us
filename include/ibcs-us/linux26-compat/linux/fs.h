/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/fs.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_FS_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_FS_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/mm_types.h>
#include <ibcs-us/linux26-compat/linux/net.h>
#include <ibcs-us/linux26-compat/linux/statfs.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/types.h>
#include <ibcs-us/linux26-compat/linux/uaccess.h>

#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#define SEEK_MAX	SEEK_END

#define	MAX_NON_LFS	((1UL<<31) - 1)

/*
 * These are the fs-independent mount-flags: up to 32 flags are supported
 */
#define MS_RDONLY	 1	/* Mount read-only */

#if	0
#define MS_NOSUID	 2	/* Ignore suid and sgid bits */
#define MS_NODEV	 4	/* Disallow access to device special files */
#define MS_NOEXEC	 8	/* Disallow program execution */
#define MS_SYNCHRONOUS	16	/* Writes are synced at once */
#define MS_REMOUNT	32	/* Alter flags of a mounted FS */
#define MS_MANDLOCK	64	/* Allow mandatory locks on an FS */
#define MS_DIRSYNC	128	/* Directory modifications are synchronous */
#define MS_NOATIME	1024	/* Do not update access times. */
#define MS_NODIRATIME	2048	/* Do not update directory access times */
#define MS_BIND		4096
#define MS_MOVE		8192
#define MS_REC		16384
#define MS_SILENT	32768
#define MS_POSIXACL	(1<<16)	/* VFS does not apply the umask */
#define MS_UNBINDABLE	(1<<17)	/* change to unbindable */
#define MS_PRIVATE	(1<<18)	/* change to private */
#define MS_SLAVE	(1<<19)	/* change to slave */
#define MS_SHARED	(1<<20)	/* change to shared */
#define MS_RELATIME	(1<<21)	/* Update atime relative to mtime/ctime. */
#define MS_KERNMOUNT	(1<<22) /* this is a kern_mount call */
#define MS_I_VERSION	(1<<23) /* Update inode I_version field */
#define MS_STRICTATIME	(1<<24) /* Always perform atime updates */
#define MS_ACTIVE	(1<<30)
#define MS_NOUSER	(1<<31)
#endif

struct file;
struct file_operations;
struct mutex;
struct path;
struct poll_table_struct;
struct vm_area_struct;

struct files_struct;
typedef struct files_struct* fl_owner_t;

struct file_system_type
{
    char		name[16];	/* The SCO structure allows 16 bytes */
};

struct inode
{
    unsigned long	i_ino;
    dev_t		i_dev;
    dev_t		i_rdev;
    umode_t		i_mode;
    struct mutex*	i_mutex;
    struct dentry*	i_dentry;
    struct socket*	_i_socket;
    struct socket	_i_socket_real;
};

extern struct super_block* inode_get_i_sb(struct inode* i, struct path* path);

struct dentry
{
    struct inode*	d_inode;
    struct inode	_d_inode_real;
    struct file*	_d_file;
};

struct super_block
{
    dev_t		s_dev;		/* search index; _not_ kdev_t */
    struct dentry*	s_root;
    unsigned long	s_flags;
    struct file_system_type* s_type;
    struct file_system_type _s_type_real;
    struct dentry	_s_root_dentry_real;
    struct kstatfs	_s_kstatfs_real;
};

struct file_operations
{
    loff_t		(*llseek)(struct file*, loff_t, int);
    int			(*mmap)(struct file*, struct vm_area_struct*);
    ssize_t		(*read)(struct file*, char*, size_t, loff_t*);
    ssize_t		(*write)(struct file*, const char*, size_t, loff_t*);
    void*		owner;
    int			(*open)(struct inode*, struct file*);
    int			(*release)(struct inode*, struct file*);
    int			(*linux26_chrdev_open)(const char* filename, int mode, int flags);
};

struct file
{
    int			fd;	/* Open file handle */
    int			f_count;
    void*		private_data;
    int			_f_isopen;
    struct file_operations* f_op;
    struct dentry*	f_dentry;
    struct dentry	_f_dentry_real;
    struct file_operations _f_op_real;
};

/*
 * This replaces (struct file*)->f_pos
 */
extern loff_t file_get_f_pos(struct file* file);

/*
 * This replaces (struct file*)->f_flags.
 */
extern unsigned int file_get_f_flags(struct file* file);

/*
 * This replaces (struct file*)->f_flags = .
 */
extern int file_set_f_flags(struct file* file, unsigned int new_flags);

typedef int (*filldir_t)(
    void* dirent, const char* lower_name, int lower_namelen,
    loff_t offset, u64 ino, unsigned int d_type, dev_t dev);

extern int linux26_user_fpath(int fd, struct path* path);
extern int register_chrdev(
    unsigned int major, const char *name, const struct file_operations *fops);
extern void unregister_chrdev(unsigned int major, const char *name);

extern int vfs_fstat(int fd, struct kstat* kstat);
extern int vfs_lstat(const char* name, struct kstat* kstat);
extern int vfs_readdir(struct file* file, filldir_t callback, void* datum);
extern int vfs_stat(const char* name, struct kstat* kstat);
extern int vfs_statfs(const struct dentry*, struct kstatfs* kstatfs);
extern struct file* open_exec(const char* path);


/*
 * These do _not_ match the kernel signatures.  The kernel does not need to
 * return a 'const char*' because it copies the name from userspace.  We
 * are borrowing the user space filename, so it must be const.
 *
 * We could do the copy of course, but it turns out we never need to.
 */
static inline const char* getname(const char* filename)
{
    return filename;
}

static inline void putname(const char* filename)
{
}

static inline void get_file(const struct file* file)
{
}

static inline int kernel_read(struct file* fp, u_long offset, void* buffer, u_long nbytes)
{
    loff_t	loff = offset;
    return fp->f_op->read(fp, buffer, nbytes, &loff);
}
#endif
