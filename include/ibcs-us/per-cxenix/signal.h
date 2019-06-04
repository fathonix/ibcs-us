#ifndef _IBCS_US_CXENIX_SIGNAL_H
#define _IBCS_US_CXENIX_SIGNAL_H

enum {
	SCO_SA_NOCLDSTOP =	0x001,
	SCO_SA_COMPAT =		0x080, /* 3.2.2 compatibilty. */
	SCO_SA_SIGNAL =		0x100,
};

struct sco_sigaction {
	void		(*sco_sa_handler)(int);
	unsigned long	sco_sa_mask;
	int		sco_sa_flags;
};

#endif /* _IBCS_US_CXENIX_SIGNAL_H */
