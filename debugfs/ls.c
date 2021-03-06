/*
 * ls.c --- list directories
 *
 * Copyright (C) 1997 Theodore Ts'o.  This file may be redistributed
 * under the terms of the GNU Public License.
 */

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <sys/types.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern int optind;
extern char *optarg;
#endif

#include "debugfs.h"
#include "ext2fs/lfsck.h"

/*
 * list directory
 */

#define LONG_OPT	0x0001
#define DELETED_OPT	0x0002
#define PARSE_OPT	0x0004
#define DIRDATA_OPT	0x0008

struct list_dir_struct {
	FILE	*f;
	int	col;
	int	options;
};

static const char *monstr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static void list_dirdata(struct list_dir_struct *ls,
			 struct ext2_dir_entry_2 *dirent)
{
	unsigned char	*data;
	int		dlen;
	__u8		dirdata_mask;

	data = (unsigned char *)dirent->name + dirent->name_len + 1;

	for (dirdata_mask = EXT2_FT_MASK + 1;
	     dirdata_mask != 0; dirdata_mask <<= 1) {
		if ((dirdata_mask & dirent->file_type) == 0)
			continue;

		dlen = data[0];
		fprintf(ls->f, " ");

		if (dirdata_mask == EXT2_DIRENT_LUFID) {
			struct lu_fid *fid = (struct lu_fid *)(data + 1);

			fid_be_to_cpu(fid, fid);
			fprintf(ls->f, DFID, PFID(fid));
		} else {
			int i;

			for (i = 1; i < dlen; i++)
				fprintf(ls->f, "%02x", data[i]);
		}
		data += dlen;
	}
}

static int list_dir_proc(ext2_ino_t dir EXT2FS_ATTR((unused)),
			 int	entry,
			 struct ext2_dir_entry *dirent,
			 int	offset EXT2FS_ATTR((unused)),
			 int	blocksize EXT2FS_ATTR((unused)),
			 char	*buf EXT2FS_ATTR((unused)),
			 void	*private)
{
	struct ext2_inode	inode;
	ext2_ino_t		ino;
	struct tm		*tm_p;
	time_t			modtime;
	char			name[EXT2_NAME_LEN + 1];
	char			tmp[EXT2_NAME_LEN + 16];
	char			datestr[80];
	char			lbr, rbr;
	int			thislen;
	struct list_dir_struct *ls = (struct list_dir_struct *) private;

	thislen = dirent->name_len & 0xFF;
	strncpy(name, dirent->name, thislen);
	name[thislen] = '\0';
	ino = dirent->inode;

	if (entry == DIRENT_DELETED_FILE) {
		lbr = '<';
		rbr = '>';
		ino = 0;
	} else {
		lbr = rbr = ' ';
	}
	if (ls->options & PARSE_OPT) {
		if (ino) {
			if (debugfs_read_inode(ino, &inode, name))
				return 0;
		} else
			memset(&inode, 0, sizeof(struct ext2_inode));
		fprintf(ls->f,"/%u/%06o/%d/%d/%s/",ino,inode.i_mode,inode.i_uid, inode.i_gid,name);
		if (LINUX_S_ISDIR(inode.i_mode))
			fprintf(ls->f, "/");
		else
			fprintf(ls->f, "%lld/", EXT2_I_SIZE(&inode));
		fprintf(ls->f, "\n");
	} else if (ls->options & LONG_OPT) {
		if (ino) {
			if (debugfs_read_inode(ino, &inode, name))
				return 0;
			modtime = inode.i_mtime;
			tm_p = localtime(&modtime);
			sprintf(datestr, "%2d-%s-%4d %02d:%02d",
				tm_p->tm_mday, monstr[tm_p->tm_mon],
				1900 + tm_p->tm_year, tm_p->tm_hour,
				tm_p->tm_min);
		} else {
			strcpy(datestr, "                 ");
			memset(&inode, 0, sizeof(struct ext2_inode));
		}
		fprintf(ls->f, "%c%6u%c %6o (%d)  %5d  %5d   ", lbr, ino, rbr,
			inode.i_mode, dirent->name_len >> 8,
			inode_uid(inode), inode_gid(inode));
		if (LINUX_S_ISDIR(inode.i_mode))
			fprintf(ls->f, "%5d", inode.i_size);
		else
			fprintf(ls->f, "%5llu", EXT2_I_SIZE(&inode));
		fprintf(ls->f, " %s", datestr);
		if ((ls->options & DIRDATA_OPT) != 0)
			list_dirdata(ls, (struct ext2_dir_entry_2 *)dirent);
		fprintf(ls->f, " %s\n", name);
	} else {
		sprintf(tmp, "%c%u%c (%d) %s   ", lbr, dirent->inode, rbr,
			dirent->rec_len, name);
		thislen = strlen(tmp);

		if (ls->col + thislen > 80) {
			fprintf(ls->f, "\n");
			ls->col = 0;
		}
		fprintf(ls->f, "%s", tmp);
		ls->col += thislen;
	}
	return 0;
}

void do_list_dir(int argc, char *argv[])
{
	ext2_ino_t	inode;
	int		retval;
	int		c;
	int		flags;
	struct list_dir_struct ls;

	ls.options = 0;
	if (check_fs_open(argv[0]))
		return;

	reset_getopt();
	while ((c = getopt (argc, argv, "dDlp")) != EOF) {
		switch (c) {
		case 'l':
			ls.options |= LONG_OPT;
			break;
		case 'D':
			ls.options |= DIRDATA_OPT;
			break;
		case 'd':
			ls.options |= DELETED_OPT;
			break;
		case 'p':
			ls.options |= PARSE_OPT;
			break;
		default:
			goto print_usage;
		}
	}

	if (argc > optind+1) {
	print_usage:
		com_err(0, 0, "Usage: ls [-l] [-d] [-p] file");
		return;
	}

	if (argc == optind)
		inode = cwd;
	else
		inode = string_to_inode(argv[optind]);
	if (!inode)
		return;

	ls.f = open_pager();
	ls.col = 0;
	flags = DIRENT_FLAG_INCLUDE_EMPTY;
	if (ls.options & DELETED_OPT)
		flags |= DIRENT_FLAG_INCLUDE_REMOVED;

	retval = ext2fs_dir_iterate2(current_fs, inode, flags,
				    0, list_dir_proc, &ls);
	fprintf(ls.f, "\n");
	close_pager(ls.f);
	if (retval)
		com_err(argv[1], retval, 0);

	return;
}


