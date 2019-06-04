#ifndef _IBCS_US_UW7_ACL_H
#define _IBCS_US_UW7_ACL_H

#include <ibcs-us/linux26-compat/linux/types.h>

//#ident "%W% %G%"

/*
 * UnixWare 7 ACL bits (unused so far).
 */

enum {
	GETACL =	1,
	SETACL =	2,
	GETACLCNT =	3,
};

struct uw7_acl {
	int		a_type;
	uid_t		a_id;
        u_int16_t	a_perm;
};


/* int uw7_acl(char * path, int cmd, int nentries, struct acl * aclp); */

#endif /* _IBCS_US_UW7_ACL_H */
