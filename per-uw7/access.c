/*
 *      abi/uw7/access.c - support for UnixWare access(2) system call.
 *
 *      We handle the non-POSIX EFF_ONLY_OK/EX_OK flags.
 *      This software is under GPL.
 */
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/trace.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/fcntl.h>
#include <ibcs-us/linux26-compat/linux/namei.h>
#include <ibcs-us/linux26-compat/linux/sched.h>



#undef DEBUG

#ifdef DEBUG
#define DBG(x...)	printk(x)
#else
#define DBG(x...)
#endif

#define UW7_R_OK	004
#define UW7_W_OK	002
#define UW7_X_OK	001
#define UW7_F_OK	000
#define UW7_EFF_ONLY_OK	010
#define UW7_EX_OK	020

#define UW7_MODE_MSK	(UW7_R_OK|UW7_W_OK|UW7_X_OK|UW7_F_OK|UW7_EFF_ONLY_OK|UW7_EX_OK)

int uw7_access(char * filename, int mode)
{
#if _KSL > 26
	struct path      path;
#else
	struct nameidata nd;
#endif
	int error;
#ifndef _KSL_IBCS_US
#if _KSL > 28
        struct cred *cred = (struct cred *)(current->cred);
#else
        struct task_struct *cred = current;
#endif
#endif

	DBG(KERN_ERR "UW7[%d]: access(%p,%o)\n", current->pid, filename, mode);

	if (mode & ~UW7_MODE_MSK)
		return -EINVAL;

	if (mode & UW7_EX_OK) {
#if _KSL > 26
		error = user_path(filename, &path);
#else
		error = user_path_walk(filename, &nd);
#endif
		if (!error) {
#ifndef	_KSL_IBCS_US
#if _KSL > 24
#if _KSL > 26
			error = do_revalidate(path.dentry);
#else
			error = do_revalidate(nd.path.dentry);
#endif
#else
			error = do_revalidate(nd.dentry);
#endif
#endif
			if (error) {
#if _KSL < 25
				path_release(&nd);
#else
#if _KSL > 26
				path_put(&path);
#else
				path_put(&nd.path);
#endif
#endif
				return -EIO;
			}
#if _KSL > 24
#if _KSL > 26
			if (!S_ISREG(path.dentry->d_inode->i_mode)) {
				path_put(&path);
#else
			if (!S_ISREG(nd.path.dentry->d_inode->i_mode)) {
				path_put(&nd.path);
#endif
#else
			if (!S_ISREG(nd.dentry->d_inode->i_mode)) {
				path_release(&nd);
#endif
				return -EACCES;
			}
#if _KSL < 25
			path_release(&nd);
#else
#if _KSL > 26
			path_put(&path);
#else
			path_put(&nd.path);
#endif
#endif
		}
		mode &= ~UW7_EX_OK;
		mode |= UW7_X_OK;
	}
	if (mode & UW7_EFF_ONLY_OK) {
#ifdef	_KSL_IBCS_US
		error = SYS(faccessat, AT_FDCWD, path, mode, AT_EACCESS);
#else
		uid_t old_uid = cred->uid, old_gid = cred->gid;
		cred->uid = cred->euid;
		cred->gid = cred->egid;
		mode &= ~UW7_EFF_ONLY_OK;
        	error =  SYS(access,filename, mode);
		cred->uid = old_uid;
		cred->gid = old_gid;
#endif
	} else
        	error = SYS(access,filename, mode);

	return error;
}
