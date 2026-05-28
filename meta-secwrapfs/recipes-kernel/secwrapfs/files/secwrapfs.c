#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/mutex.h>
#include "secwrapfs_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("IniyanR");
MODULE_DESCRIPTION("SecWrapFS Stackable Pass-Through Filesystem with IOCTL Concurrency");
MODULE_VERSION("2.0");

#define SECWRAPFS_MAGIC_NUMBER 0x53574653

static const struct super_operations secwrapfs_s_ops;
static const struct inode_operations secwrapfs_dir_i_ops;
static const struct inode_operations secwrapfs_main_i_ops;
static const struct file_operations secwrapfs_dir_f_ops;
static const struct file_operations secwrapfs_main_f_ops;

static DEFINE_MUTEX(telemetry_lock);
static struct secwrapfs_telemetry global_telemetry = {
    .total_reads = 0,
    .total_writes = 0,
    .log_verbosity = 1 
};

struct secwrapfs_sb_info {
    struct path lower_path;
};

static inline struct path *SECWRAPFS_SB_PATH(struct super_block *sb) {
    return &((struct secwrapfs_sb_info *)sb->s_fs_info)->lower_path;
}

static inline struct dentry *SECWRAPFS_D_LOWER(struct dentry *dentry) {
    return (struct dentry *)dentry->d_fsdata;
}

static struct inode *secwrapfs_get_inode(struct super_block *sb, struct inode *lower_inode)
{
    struct inode *inode;
    if (!lower_inode) return NULL;

    inode = new_inode(sb);
    if (!inode) return ERR_PTR(-ENOMEM);

    ihold(lower_inode);
    inode->i_ino = lower_inode->i_ino;
    inode->i_mode = lower_inode->i_mode;
    inode->i_private = lower_inode;
    
    inode_set_ctime_to_ts(inode, inode_get_ctime(lower_inode));
    inode->i_mtime = lower_inode->i_mtime;
    inode->i_atime = lower_inode->i_atime;

    if (S_ISDIR(inode->i_mode)) {
        inode->i_op = &secwrapfs_dir_i_ops;
        inode->i_fop = &secwrapfs_dir_f_ops;
    } else {
        inode->i_op = &secwrapfs_main_i_ops;
        inode->i_fop = &secwrapfs_main_f_ops;
    }
    return inode;
}


static int secwrapfs_create(struct mnt_idmap *idmap, struct inode *dir,
                            struct dentry *dentry, umode_t mode, bool want_excl)
{
    struct dentry *lower_dir_dentry = SECWRAPFS_D_LOWER(dentry->d_parent);
    struct dentry *lower_dentry;
    struct inode *lower_dir_inode = d_inode(lower_dir_dentry);
    struct inode *inode;
    int err;

    if (global_telemetry.log_verbosity)
        printk(KERN_INFO "SecWrapFS: Intercepted file creation request for '%s'\n", dentry->d_name.name);

    inode_lock(lower_dir_inode);
    lower_dentry = lookup_one_len(dentry->d_name.name, lower_dir_dentry, dentry->d_name.len);
    if (IS_ERR(lower_dentry)) {
        err = PTR_ERR(lower_dentry);
        inode_unlock(lower_dir_inode);
        return err;
    }

    err = vfs_create(idmap, lower_dir_inode, lower_dentry, mode, want_excl);
    inode_unlock(lower_dir_inode);

    if (err) {
        dput(lower_dentry);
        return err;
    }

    if (d_really_is_positive(lower_dentry)) {
        inode = secwrapfs_get_inode(dir->i_sb, d_inode(lower_dentry));
        if (IS_ERR(inode)) {
            dput(lower_dentry);
            return PTR_ERR(inode);
        }
        dentry->d_fsdata = lower_dentry;
        d_instantiate(dentry, inode);
    } else {
        dput(lower_dentry);
        return -ENOENT;
    }

    return 0;
}


static struct dentry *secwrapfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    struct dentry *lower_dir_dentry = SECWRAPFS_D_LOWER(dentry->d_parent);
    struct dentry *lower_dentry;
    struct inode *inode = NULL;

    if (global_telemetry.log_verbosity)
        printk(KERN_INFO "SecWrapFS: Intercepting lookup search for: '%s'\n", dentry->d_name.name);

    inode_lock(d_inode(lower_dir_dentry));
    lower_dentry = lookup_one_len(dentry->d_name.name, lower_dir_dentry, dentry->d_name.len);
    inode_unlock(d_inode(lower_dir_dentry));

    if (IS_ERR(lower_dentry)) return ERR_CAST(lower_dentry);

    if (d_really_is_positive(lower_dentry)) {
        inode = secwrapfs_get_inode(dir->i_sb, d_inode(lower_dentry));
        if (IS_ERR(inode)) {
            dput(lower_dentry);
            return ERR_CAST(inode);
        }
    }

    dentry->d_fsdata = lower_dentry;
    return d_splice_alias(inode, dentry);
}

static const struct inode_operations secwrapfs_dir_i_ops = { .lookup = secwrapfs_lookup, .create = secwrapfs_create, };
static const struct inode_operations secwrapfs_main_i_ops = {};

static ssize_t secwrapfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    struct file *lower_file = file->private_data;
    
    mutex_lock(&telemetry_lock);
    global_telemetry.total_reads++;
    mutex_unlock(&telemetry_lock);

    if (global_telemetry.log_verbosity)
        printk(KERN_INFO "SecWrapFS: Intercepted VFS read event logged safely.\n");

    return vfs_read(lower_file, buf, count, pos);
}

static ssize_t secwrapfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    struct file *lower_file = file->private_data;

    mutex_lock(&telemetry_lock);
    global_telemetry.total_writes++;
    mutex_unlock(&telemetry_lock);

    if (global_telemetry.log_verbosity)
        printk(KERN_INFO "SecWrapFS: Intercepted VFS write event logged safely.\n");

    return vfs_write(lower_file, buf, count, pos);
}

static int secwrapfs_open(struct inode *inode, struct file *file)
{
    struct path *lower_sb_path = SECWRAPFS_SB_PATH(inode->i_sb);
    struct file *lower_file;
    struct path lower_file_path;

    lower_file_path.mnt = lower_sb_path->mnt;
    lower_file_path.dentry = SECWRAPFS_D_LOWER(file->f_path.dentry);

    path_get(&lower_file_path);
    lower_file = dentry_open(&lower_file_path, file->f_flags, current_cred());
    path_put(&lower_file_path);

    if (IS_ERR(lower_file)) return PTR_ERR(lower_file);

    file->private_data = lower_file;
    return 0;
}

static int secwrapfs_release(struct inode *inode, struct file *file)
{
    struct file *lower_file = file->private_data;
    if (lower_file) fput(lower_file);
    return 0;
}

static long secwrapfs_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    uint32_t verbosity_val;

    switch (cmd) {
    case SECWRAPFS_SET_VERBOSITY:
        if (copy_from_user(&verbosity_val, (uint32_t __user *)arg, sizeof(verbosity_val)))
            return -EFAULT;
        
        mutex_lock(&telemetry_lock);
        global_telemetry.log_verbosity = verbosity_val;
        mutex_unlock(&telemetry_lock);
        
        printk(KERN_INFO "SecWrapFS: Logging status modified runtime via IOCTL to %u\n", verbosity_val);
        break;

    case SECWRAPFS_GET_TELEMETRY:
        mutex_lock(&telemetry_lock);
        if (copy_to_user((struct secwrapfs_telemetry __user *)arg, &global_telemetry, sizeof(global_telemetry))) {
            mutex_unlock(&telemetry_lock);
            return -EFAULT;
        }
        mutex_unlock(&telemetry_lock);
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

struct secwrapfs_readdir_ctx { struct dir_context ctx; struct dir_context *orig_ctx; };

static bool secwrapfs_filldir(struct dir_context *ctx, const char *name, int namelen,
                              loff_t offset, u64 ino, unsigned int d_type)
{
    struct secwrapfs_readdir_ctx *buf = container_of(ctx, struct secwrapfs_readdir_ctx, ctx);
    buf->orig_ctx->pos = ctx->pos;
    return dir_emit(buf->orig_ctx, name, namelen, ino, d_type);
}

static int secwrapfs_iterate(struct file *file, struct dir_context *ctx)
{
    struct file *lower_file = file->private_data;
    struct secwrapfs_readdir_ctx lower_ctx = { .ctx.actor = secwrapfs_filldir, .orig_ctx = ctx };
    int reset;

    lower_ctx.ctx.pos = ctx->pos;
    reset = iterate_dir(lower_file, &lower_ctx.ctx);
    ctx->pos = lower_ctx.ctx.pos;
    return reset;
}

static const struct file_operations secwrapfs_main_f_ops = {
    .open           = secwrapfs_open,
    .release        = secwrapfs_release,
    .read           = secwrapfs_read,
    .write          = secwrapfs_write,
    .unlocked_ioctl = secwrapfs_unlocked_ioctl, 
};

static const struct file_operations secwrapfs_dir_f_ops = {
    .open           = secwrapfs_open,
    .release        = secwrapfs_release,
    .iterate_shared = secwrapfs_iterate,
    .unlocked_ioctl = secwrapfs_unlocked_ioctl, 
};

static void secwrapfs_put_super(struct super_block *sb)
{
    struct secwrapfs_sb_info *sbi = sb->s_fs_info;
    if (sbi) { path_put(&sbi->lower_path); kfree(sbi); sb->s_fs_info = NULL; }
}

static const struct super_operations secwrapfs_s_ops = { .statfs = simple_statfs, .put_super = secwrapfs_put_super };

static int secwrapfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct secwrapfs_sb_info *sbi = (struct secwrapfs_sb_info *)data;
    struct inode *root_inode;
    struct dentry *lower_root_dentry;

    if (!sbi) return -EINVAL;

    sb->s_maxbytes       = MAX_LFS_FILESIZE;
    sb->s_blocksize      = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic          = SECWRAPFS_MAGIC_NUMBER;
    sb->s_op             = &secwrapfs_s_ops;
    sb->s_fs_info        = sbi;

    lower_root_dentry = sbi->lower_path.dentry;
    root_inode = secwrapfs_get_inode(sb, d_inode(lower_root_dentry));
    if (IS_ERR(root_inode)) return PTR_ERR(root_inode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) return -ENOMEM;

    sb->s_root->d_fsdata = dget(lower_root_dentry);
    return 0;
}

static struct dentry *secwrapfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    struct secwrapfs_sb_info *sbi;
    struct dentry *root_dentry;
    int err;

    if (!dev_name || strlen(dev_name) == 0) return ERR_PTR(-EINVAL);

    sbi = kzalloc(sizeof(struct secwrapfs_sb_info), GFP_KERNEL);
    if (!sbi) return ERR_PTR(-ENOMEM);

    err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &sbi->lower_path);
    if (err) { kfree(sbi); return ERR_PTR(err); }

    root_dentry = mount_nodev(fs_type, flags, sbi, secwrapfs_fill_super);
    if (IS_ERR(root_dentry)) { path_put(&sbi->lower_path); kfree(sbi); }
    return root_dentry;
}

static struct file_system_type secwrap_fs_type = {
    .owner    = THIS_MODULE,
    .name     = "secwrapfs",
    .mount    = secwrapfs_mount,
    .kill_sb  = kill_litter_super,
};

static int __init secwrapfs_init(void)
{
    return register_filesystem(&secwrap_fs_type);
}

static void __exit secwrapfs_exit(void)
{
    unregister_filesystem(&secwrap_fs_type);
}

module_init(secwrapfs_init);
module_exit(secwrapfs_exit);
