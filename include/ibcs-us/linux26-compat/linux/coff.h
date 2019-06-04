/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/coff.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_COFF_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_COFF_H
#include <sys/cdefs.h>

/*
 * These defines are byte order independent. There is no alignment of fields
 * permitted in the structures. Therefore they are declared as characters
 * and the values loaded from the character positions. It also makes it
 * nice to have it "endian" independent.
 */
 
/*
 * Load a short int from the following tables with little-endian formats
 */
#define COFF_SHORT_L(ps) (						\
	(short)(((unsigned short)((unsigned char)ps[1]) << 8)|		\
		((unsigned short)((unsigned char)ps[0]))))

/*
 * Load a long int from the following tables with little-endian formats
 */
#define COFF_LONG_L(ps) (						\
	((long)(((unsigned long)((unsigned char)ps[3]) << 24) |		\
		((unsigned long)((unsigned char)ps[2]) << 16) |		\
		((unsigned long)((unsigned char)ps[1]) <<  8) |		\
		((unsigned long)((unsigned char)ps[0])))))
 
/*
 * Load a short int from the following tables with big-endian formats.
 */
#define COFF_SHORT_H(ps) (						\
	(short)(((unsigned short)((unsigned char)ps[0]) << 8) |		\
		((unsigned short)((unsigned char)ps[1]))))

/*
 * Load a long int from the following tables with big-endian formats.
 */
#define COFF_LONG_H(ps) (						\
	((long)(((unsigned long)((unsigned char)ps[0]) << 24) |		\
		((unsigned long)((unsigned char)ps[1]) << 16) |		\
		((unsigned long)((unsigned char)ps[2]) <<  8) |		\
		((unsigned long)((unsigned char)ps[3])))))

/*
 * These may be overridden later by brain dead implementations which generate
 * a big-endian header with little-endian data. In that case, generate a
 * replacement macro which tests a flag and uses either of the two above
 * as appropriate.
 */
#define COFF_LONG(v)   COFF_LONG_L(v)
#define COFF_SHORT(v)  COFF_SHORT_L(v)

/********************** FILE HEADER **********************/

struct COFF_filehdr
{
    char		f_magic[2];	/* magic number			*/
    char		f_nscns[2];	/* number of sections		*/
    char		f_timdat[4];	/* time & date stamp		*/
    char		f_symptr[4];	/* file pointer to symtab	*/
    char		f_nsyms[4];	/* number of symtab entries	*/
    char		f_opthdr[2];	/* sizeof(optional hdr)		*/
    char		f_flags[2];	/* flags			*/
};

/*
 *   Bits for f_flags:
 *
 *	F_RELFLG	relocation info stripped from file
 *	F_EXEC		file is executable  (i.e. no unresolved external
 *			references)
 *	F_LNNO		line numbers stripped from file
 *	F_LSYMS		local symbols stripped from file
 *	F_MINMAL	this is a minimal object file (".m") output of fextract
 *	F_UPDATE	this is a fully bound update file, output of ogen
 *	F_SWABD		this file has had its bytes swabbed (in names)
 *	F_AR16WR	this file has the byte ordering of an AR16WR
 *			(e.g. 11/70) machine
 *	F_AR32WR	this file has the byte ordering of an AR32WR machine
 *			(e.g. vax and iNTEL 386)
 *	F_AR32W		this file has the byte ordering of an AR32W machine
 *			(e.g. 3b,maxi)
 *	F_PATCH		file contains "patch" list in optional header
 *	F_NODF		(minimal file only) no decision functions for
 *			replaced functions
 */
#define  COFF_F_RELFLG		0000001
#define  COFF_F_EXEC		0000002
#define  COFF_F_LNNO		0000004
#define  COFF_F_LSYMS		0000010
#define  COFF_F_MINMAL		0000020
#define  COFF_F_UPDATE		0000040
#define  COFF_F_SWABD		0000100
#define  COFF_F_AR16WR		0000200
#define  COFF_F_AR32WR		0000400
#define  COFF_F_AR32W		0001000
#define  COFF_F_PATCH		0002000
#define  COFF_F_NODF		0002000

#define	COFF_I386MAGIC	        0x14c	/* Linux's system    */

#define COFF_I386BADMAG(x)	(COFF_SHORT((x).f_magic) != COFF_I386MAGIC)

#define	COFF_FILHDR		struct COFF_filehdr
#define	COFF_FILHSZ		sizeof(COFF_FILHDR)

/********************** AOUT "OPTIONAL HEADER" **********************/

/* Linux COFF must have this "optional" header. Standard COFF has no entry
   location for the "entry" point. They normally would start with the first
   location of the .text section. This is not a good idea for linux. So,
   the use of this "optional" header is not optional. It is required.

   Do not be tempted to assume that the size of the optional header is
   a constant and simply index the next byte by the size of this structure.
   Use the 'f_opthdr' field in the main coff header for the size of the
   structure actually written to the file!!
*/

typedef struct 
{
    char 		magic[2];	/* type of file				 */
    char		vstamp[2];	/* version stamp			 */
    char		tsize[4];	/* text size in bytes, padded to FW bdry */
    char		dsize[4];	/* initialized   data "   "		 */
    char		bsize[4];	/* uninitialized data "   "		 */
    char		entry[4];	/* entry pt.				 */
    char 		text_start[4];	/* base of text used for this file       */
    char 		data_start[4];	/* base of data used for this file       */
} COFF_AOUTHDR;

#define COFF_AOUTSZ	(sizeof(COFF_AOUTHDR))

#define COFF_STMAGIC	0401
#define COFF_OMAGIC     0404
#define COFF_JMAGIC     0407    /* dirty text and data image, can't share  */
#define COFF_DMAGIC     0410    /* dirty text segment, data aligned        */
#define COFF_ZMAGIC     0413    /* The proper magic number for executables  */
#define COFF_SHMAGIC	0443	/* shared library header                   */

/********************** SECTION HEADER **********************/

struct COFF_scnhdr
{
    char		s_name[8];	/* section name			    */
    char		s_paddr[4];	/* physical address, aliased s_nlib */
    char		s_vaddr[4];	/* virtual address		    */
    char		s_size[4];	/* section size			    */
    char		s_scnptr[4];	/* file ptr to raw data for section */
    char		s_relptr[4];	/* file ptr to relocation	    */
    char		s_lnnoptr[4];	/* file ptr to line numbers	    */
    char		s_nreloc[2];	/* number of relocation entries	    */
    char		s_nlnno[2];	/* number of line number entries    */
    char		s_flags[4];	/* flags			    */
};

#define	COFF_SCNHDR	struct COFF_scnhdr
#define	COFF_SCNHSZ	sizeof(COFF_SCNHDR)

/*
 * names of "special" sections
 */
#define COFF_TEXT	".text"
#define COFF_DATA	".data"
#define COFF_BSS	".bss"
#define COFF_COMMENT    ".comment"
#define COFF_LIB        ".lib"

#define COFF_SECT_TEXT  0	/* Section for instruction code             */
#define COFF_SECT_DATA  1	/* Section for initialized globals          */
#define COFF_SECT_BSS   2	/* Section for un-initialized globals       */
#define COFF_SECT_REQD	3	/* Minimum number of sections for good file */

#define COFF_STYP_REG	0x000	/* regular segment                          */
#define COFF_STYP_DSECT	0x001	/* dummy segment                            */
#define COFF_STYP_NOLOAD 0x002	/* no-load segment                          */
#define COFF_STYP_GROUP	0x004	/* group segment                            */
#define COFF_STYP_PAD	0x008	/* .pad segment                             */
#define COFF_STYP_COPY	0x010	/* copy section                             */
#define COFF_STYP_TEXT	0x020	/* .text segment                            */
#define COFF_STYP_DATA	0x040	/* .data segment                            */
#define COFF_STYP_BSS	0x080	/* .bss segment                             */
#define COFF_STYP_INFO	0x200	/* .comment section                         */
#define COFF_STYP_OVER	0x400	/* overlay section                          */
#define COFF_STYP_LIB	0x800	/* library section                          */

/*
 * Shared libraries have the following section header in the data field for
 * each library.
 */
struct COFF_slib
{
    char		sl_entsz[4];	/* Size of this entry               */
    char		sl_pathndx[4];	/* size of the header field         */
};

#define	COFF_SLIBHD	struct COFF_slib
#define	COFF_SLIBSZ	sizeof(COFF_SLIBHD)

#endif
