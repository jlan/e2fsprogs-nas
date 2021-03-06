#ifndef LFSCK_H
#define LFSCK_H

/* This is unfortunately needed for older lustre_user.h to be usable */
#define LASSERT(cond)		do { } while (0)

#include "../lib/ext2fs/ext2fsP.h"
#include <ext2fs/ext2_ext_attr.h>
#ifdef HAVE_LUSTRE_LUSTREAPI_H
#include <lustre/lustreapi.h>
#elif HAVE_LUSTRE_LIBLUSTREAPI_H
#include <lustre/liblustreapi.h>
#endif

#ifndef DFID
#define DFID "[%#llx:0x%x:0x%x]"
#define PFID(fid) (unsigned long long)fid_seq(fid), fid_oid(fid), fid_ver(fid)
struct lu_fid {
	__u64   f_seq;
	__u32   f_oid;
	__u32   f_ver;
};
#endif /* !DFID */

/* Unfortunately, neither the 1.8 or 2.x lustre_idl.h file is suitable
 * for inclusion by userspace programs because of external dependencies.
 * Define the minimum set of replacement functions here until that is fixed. */
#ifndef HAVE_LUSTRE_LUSTRE_IDL_H
#define fid_seq(fid) ((fid)->f_seq)
#define fid_oid(fid) ((fid)->f_oid)
#define fid_ver(fid) ((fid)->f_ver)

static inline void fid_be_to_cpu(struct lu_fid *dst, struct lu_fid *src)
{
	dst->f_seq = ext2fs_be64_to_cpu(src->f_seq);
	dst->f_oid = ext2fs_be32_to_cpu(src->f_oid);
	dst->f_ver = ext2fs_be32_to_cpu(src->f_ver);
}

static inline void fid_le_to_cpu(struct lu_fid *dst, struct lu_fid *src)
{
	dst->f_seq = ext2fs_le64_to_cpu(src->f_seq);
	dst->f_oid = ext2fs_le32_to_cpu(src->f_oid);
	dst->f_ver = ext2fs_le32_to_cpu(src->f_ver);
}
#endif /* HAVE_LUSTRE_LUSTRE_IDL_H */

#define LUSTRE_XATTR_MDT_LOV	"lov"
#define LUSTRE_XATTR_MDT_LMA	"lma"
#define LUSTRE_XATTR_MDT_LINK	"link"
#define LUSTRE_XATTR_OST_FID	"fid"

#ifndef LMA_OLD_SIZE
#ifndef LMA_INCOMPAT_SUPP
struct lustre_mdt_attrs {
	__u32		lma_compat;
	__u32		lma_incompat;
	struct lu_fid	lma_self_fid;
};
#endif

struct filter_fid_old {
	struct lu_fid	ff_parent;
	__u64		ff_objid;
	__u64		ff_seq;
};

#define LMA_OLD_SIZE 64
#endif /* !LMA_OLD_SIZE */

#ifdef HAVE_LFSCK	/* compiling with Lustre lfsck support */

#ifdef DOSTID
#define lmm_oi lmm_oi.oi
#define l_ost_oi l_ost_oi.oi
#else
#define DOSTID LPX64":"LPX64
#define POSTID(oi) (oi)->oi_seq, (oi)->oi_id

struct ostid {
	__u64	oi_id;
	__u64	oi_seq;
};
#define l_ost_oi       l_object_id             /* for lov_ost_data_v1 */
#define lmm_oi         lmm_object_id           /* for lov_mds_md_v1/3 */
#endif /* DOSTID */

#ifndef HAVE_LUSTRE_LUSTRE_IDL_H
#define OBD_CONNECT_FID		0x40000000ULL

enum fid_seq {
	FID_SEQ_OST_MDT0	= 0ULL,
	FID_SEQ_IGIF		= 12ULL,
	FID_SEQ_IGIF_MAX	= 0x0ffffffffULL,
	FID_SEQ_IDIF		= 0x100000000ULL,
};

static inline int fid_seq_is_igif(const __u64 seq)
{
	return seq >= FID_SEQ_IGIF && seq <= FID_SEQ_IGIF_MAX;
}

static inline int fid_is_igif(const struct lu_fid *fid)
{
	return fid_seq_is_igif(fid_seq(fid));
}

static inline __u64 ostid_id(const void *oip)
{
	const struct ostid *oi = oip;

	if (oi->oi_seq == FID_SEQ_OST_MDT0)
		return oi->oi_id;

	return oi->oi_seq;
}

static inline __u64 ostid_seq(const void *oip)
{
	const struct ostid *oi = oip;

	if (oi->oi_seq == FID_SEQ_OST_MDT0)
		return FID_SEQ_OST_MDT0;

	return oi->oi_id;
}

/* convert an OST objid + index into an IDIF FID SEQ number */
static inline __u64 fid_idif_seq(__u64 id, __u32 ost_idx)
{
	return FID_SEQ_IDIF | (ost_idx << 16) | ((id >> 32) & 0xffff);
}

/* convert ost_id from 1.x compatible OST protocol into FID for future usage */
static inline void ostid_idif_unpack(struct ostid *oi, struct lu_fid *fid,
				     __u32 idx)
{
	fid->f_seq = fid_idif_seq(ostid_id(oi), idx);
	fid->f_oid = ostid_id(oi);	/* truncate to 32 bits by assignment */
	fid->f_ver = ostid_id(oi) >> 48;/* in theory, not currently used */
}
#endif /* HAVE_LUSTRE_LUSTRE_IDL_H */

#ifndef LPU64
#if (__WORDSIZE == 32) || defined(__x86_64__)
# define LPU64 "%llu"
# define LPD64 "%lld"
# define LPX64 "%#llx"
# define LPSZ  "%u"
# define LPSSZ "%d"
#elif (__WORDSIZE == 64)
# define LPU64 "%lu"
# define LPD64 "%ld"
# define LPX64 "%#lx"
# define LPSZ  "%lu"
# define LPSSZ "%ld"
#endif
#endif /* !LPU64 */

/* Get O/R or O/0 dir */
#define OBJECT_DIR  "O"
#define OBJECT_DIR_V1 "R"
#define OBJECT_DIR_V2 "0"
#define LOG_DIR "1"
#define PENDING_DIR "PENDING"
#define OBJECTS "OBJECTS"
#define CATLIST "CATALOGS"
#define LAST_ID "LAST_ID"
#define LAST_RCVD "last_rcvd"
#define LOV_OBJID "lov_objid"

/* Database names */
#define MDS_HDR       "mdshdr"
#define MDS_DIRINFO   "mds_dirinfo"
#define MDS_SIZEINFO  "mds_sizeinfo"
#define MDS_OSTDB     "mds_ostdb"
#define OST_HDR       "osthdr"
#define OST_OSTDB     "ost_db"

#define MDS_MAGIC     0xDBABCD01
#define OST_MAGIC     0xDB123402

#define OBD_COMPAT_OST		0x00000002 /* this is an OST (1.6+) */
#define OBD_COMPAT_MDT		0x00000004 /* this is an MDT (1.6+) */

#define OBD_INCOMPAT_OST	0x00000002 /* this is an OST (1.8+) */
#define OBD_INCOMPAT_MDT	0x00000004 /* this is an MDS (1.8+) */

#define LOV_MAX_OSTS 2048       /* Arbitrary limit, can be increased */
#define LOV_EA_SIZE(lum, num) (sizeof(*lum) + num * sizeof(*lum->lmm_objects))
#define LOV_EA_MAX(lum) LOV_EA_SIZE(lum, LOV_MAX_OSTS)

struct lustre_server_data {
	__u8  lsd_uuid[40];        /* server UUID */
	__u64 lsd_last_transno;    /* last completed transaction ID */
	__u64 lsd_compat14;        /* reserved - compat with old last_rcvd */
	__u64 lsd_mount_count;     /* incarnation number */
	__u32 lsd_feature_compat;  /* compatible feature flags */
	__u32 lsd_feature_rocompat;/* read-only compatible feature flags */
	__u32 lsd_feature_incompat;/* incompatible feature flags */
	__u32 lsd_server_size;     /* size of server data area */
	__u32 lsd_client_start;    /* start of per-client data area */
	__u16 lsd_client_size;     /* size of per-client data area */
	__u16 lsd_subdir_count;    /* number of subdirectories for objects */
	__u64 lsd_catalog_oid;     /* recovery catalog object id */
	__u32 lsd_catalog_ogen;    /* recovery catalog inode generation */
	__u8  lsd_peeruuid[40];    /* UUID of LOV/OSC associated with MDS */
	__u32 lsd_ost_index;       /* index number of OST in LOV */
	__u32 lsd_mdt_index;       /* index number of MDT in LMV */
};

struct lfsck_mds_hdr {
	__u64 mds_magic;
	__u64 mds_flags;
	__u64 mds_max_files;
	__u32 mds_num_osts;
	__u32 mds_unused;
	__u64 mds_max_ost_id[LOV_MAX_OSTS];
	struct obd_uuid mds_uuid;
	struct obd_uuid mds_ost_info[LOV_MAX_OSTS];
};

struct lfsck_ost_hdr  {
	__u64 ost_magic;
	__u64 ost_flags;
	__u64 ost_num_files;
	__u64 ost_last_id;
	__u32 ost_index;
	__u32 ost_unused;
	struct obd_uuid ost_mds_uuid;
	struct obd_uuid ost_uuid;
};

struct lfsck_mds_dirent {
	struct lu_fid mds_dirfid;
	struct lu_fid mds_fid;
};

struct lfsck_mds_szinfo {
	__u64 mds_fid;
	__u64 mds_seq;
	__u64 mds_size;
	__u64 mds_calc_size;
	__u32 mds_stripe_size;
	__u32 mds_stripe_pattern;
	__u16 mds_stripe_count;
	__u16 mds_stripe_start;
};

struct lfsck_mds_objent {
	struct lu_fid	mds_fid;
	struct ostid	mds_oi;
	__u32		mds_ostidx;
	__u32		mds_ostoffset;
};

struct lfsck_ost_objent {
	struct ostid	ost_oi;
	__u64		ost_size;
	__u64		ost_bytes;
};

#ifdef HAVE_DB_H
#include <db.h>
#endif

struct lfsck_ofile_ctx {
	DB *dbp;
	__u64 max_id;
	int have_max_id;
};

struct lfsck_outdb_info {
	__u32 ost_count;
	int have_ost_count;
	DB *mds_sizeinfo_dbp;
	struct lfsck_ofile_ctx *ofile_ctx;
};

/* pass6.c */
#ifdef FSCK_OK		/* compiling for e2fsck or lfsck */
extern int e2fsck_lfsck_find_ea(e2fsck_t ctx, struct ext2_inode_large *inode,
				struct ext2_ext_attr_entry *entry, void *value,
				struct lov_user_md_v1 **lmm,
				struct lustre_mdt_attrs **lma);
extern int e2fsck_lfsck_save_ea(e2fsck_t ctx, ext2_ino_t ino, __u32 generation,
				 struct lov_user_md_v1 *lmm,
				 struct lustre_mdt_attrs *lma);
extern int e2fsck_lfsck_flush_ea(e2fsck_t ctx);
extern int e2fsck_lfsck_cleanupdb(e2fsck_t ctx);
extern int e2fsck_lfsck_remove_pending(e2fsck_t ctx, char *block_buf);

/* lfsck_common.c */
extern char *my_dirname(char *path);
extern const char *my_basename(const char *path);
extern int lfsck_create_dbenv(const char *progname);
extern int lfsck_opendb(const char *fname, const char *dbname, DB **dbpp,
			int allow_dup, int keydata_size, int num_files);
extern void cputole_mds_hdr(struct lfsck_mds_hdr *mds_hdr);
extern void letocpu_mds_hdr(struct lfsck_mds_hdr *mds_hdr);
extern void cputole_ost_hdr(struct lfsck_ost_hdr *ost_hdr);
extern void letocpu_ost_hdr(struct lfsck_ost_hdr *ost_hdr);
extern void cputole_fid(struct lu_fid *fid);
extern void letocpu_fid(struct lu_fid *fid);
extern void cputole_mds_dirent(struct lfsck_mds_dirent *mds_dirent);
extern void letocpu_mds_dirent(struct lfsck_mds_dirent *mds_dirent);
extern void cputole_mds_szinfo(struct lfsck_mds_szinfo *mds_szinfo);
extern void letocpu_mds_szinfo(struct lfsck_mds_szinfo *mds_szinfo);
extern void cputole_mds_objent(struct lfsck_mds_objent *mds_objent);
extern void letocpu_mds_objent(struct lfsck_mds_objent *mds_objent);
extern void cputole_ost_objent(struct lfsck_ost_objent *ost_objent);
extern void letocpu_ost_objent(struct lfsck_ost_objent *ost_objent);
extern void letocpu_lov_user_md(struct lov_user_md *lmm);

int lfsck_get_fid(ext2_filsys fs, ino_t ino, struct lu_fid *fid);
int lfsck_is_dirfid_root(const struct lu_fid *dirfid);
int lfsck_fidcmp(const struct lu_fid *fid1, const struct lu_fid *fid2);
#endif /* FSCK_OK */

#define MDS_START_DIRENT_TABLE sizeof(struct lfsck_mds_hdr)

#define MDS_START_SZINFO_TABLE(numfiles) \
  (sizeof(struct lfsck_mds_hdr) + (sizeof(struct lfsck_mds_dirent) * numfiles))

#define MDS_START_OST_TABLE_OFFSET(idx, numfiles)			       \
  (sizeof(struct lfsck_mds_hdr) + (sizeof(struct lfsck_mds_dirent) * numfiles)+\
   (sizeof(struct lfsck_mds_szinfo) * numfiles) +			       \
   (sizeof(struct lfsck_mds_objent_hdr) +				       \
   ((sizeof(struct lfsck_mds_objent) * numfiles)) * (idx)) +		       \
   sizeof(struct lfsck_mds_objent_hdr))

#define MDS_START_OST_HDR_OFFSET(idx, numfiles)				       \
  (sizeof(struct lfsck_mds_hdr) + (sizeof(struct lfsck_mds_dirent) * numfiles)+\
   (sizeof(struct lfsck_mds_szinfo) * numfiles) +			       \
   (sizeof(struct lfsck_mds_objent_hdr) +				       \
   ((sizeof(struct lfsck_mds_objent) * numfiles)) * (idx)))

#define OST_START_OFFSET  sizeof(struct lfsck_ost_hdr)

#else /* !HAVE_LFSCK */
#define e2fsck_lfsck_find_ea(ctx, inode, entry, value, lmm, lma) (0)
#define e2fsck_lfsck_save_ea(ctx, ino, generation, lmm, lma) do {} while(0)
#define e2fsck_lfsck_flush_ea(ctx) (0)
#define e2fsck_lfsck_cleanupdb(ctx) (0)
#define e2fsck_lfsck_remove_pending(ctx, block_buf) (0)
#endif /* HAVE_LFSCK */

#endif /* LFSCK_H */
