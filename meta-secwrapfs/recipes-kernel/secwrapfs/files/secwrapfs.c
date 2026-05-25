#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/fs_context.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("IniyanR");
MODULE_DESCRIPTION("SecWrapFS Stackable Cryptographic Filesystem");
MODULE_VERSION("0.1");

#define SECWRAPFS_MAGIC_NUMBER 0x53574653

static const struct super_operations secwrapfs_s_ops = {
	.statfs = simple_statfs,
	.drop_inode = generic_delete_inode,
};

static int secwrapfs_fill_super(struct super_block *sb, struct fs_context *fc){
	struct inode *root_inode;
	struct timespec64 now;
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = SECWRAPFS_MAGIC_NUMBER;
	
	sb->s_op = &secwrapfs_s_ops;

	root_inode = new_inode(sb);
	if(!root_inode){
		return -ENOMEM;
	}

	root_inode->i_ino = 1;
	root_inode->i_mode = S_IFDIR | 0755;
	
	now = current_time(root_inode);
	root_inode->i_atime = now;
	root_inode->i_mtime = now ;
	inode_set_ctime_to_ts(root_inode, now);
	
	root_inode->i_op = &simple_dir_inode_operations;
	root_inode->i_fop = &simple_dir_operations;
	
	sb->s_root = d_make_root(root_inode);
	if(!sb->s_root){
		return -ENOMEM;
	}
	return 0;
}

static int secwrapfs_get_tree(struct fs_context *fc){
	return get_tree_nodev(fc,secwrapfs_fill_super);
}

static const struct fs_context_operations secwrapfs_context_ops = {
	.get_tree = secwrapfs_get_tree,
};

static int secwrap_init_fs_context(struct fs_context *fc){
	fc->ops = &secwrapfs_context_ops;
	return 0;
}

static struct file_system_type secwrap_fs_type = {
	.owner = THIS_MODULE,
	.name = "secwrapfs",
	.init_fs_context = secwrap_init_fs_context,
	.kill_sb = kill_litter_super,
};

static int __init secwrapfs_init(void){	
	int ret;
	printk(KERN_INFO "Registering Device\n");
	ret=register_filesystem(&secwrap_fs_type);
	if(ret){
		printk(KERN_ERR "SecWrapFS : Failed to Register filesystem framework\n");
		return ret;
	}
	printk(KERN_INFO "SecWrapFS Filesystem Registered Successfully.\n");
	return 0;
}

static void __exit secwrapfs_exit(void){
	unregister_filesystem(&secwrap_fs_type);
	printk(KERN_INFO "SecWrapFS Skeleton Module Unloaded.\n");
}

module_init(secwrapfs_init);
module_exit(secwrapfs_exit);

