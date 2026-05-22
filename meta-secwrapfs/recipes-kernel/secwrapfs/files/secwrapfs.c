#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init secwrapfs_init(void){
	printk(KERN_INFO "SecWrapFS Skeleton Module Loaded Successfully.");
	return 0;
}

static void __exit secwrapfs_exit(void){
	printk(KERN_INFO "SecWrapFS Skeleton Module Unloaded.\n");
}

module_init(secwrapfs_init);
module_exit(secwrapfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("IniyanR");
MODULE_DESCRIPTION("SecWrapFS Skeleton out-of-tree driver.");
