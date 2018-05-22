#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/mount.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/init.h>
#include <asm/current.h>
#include <asm-generic/current.h>
#define AUFS_MAGIC 0x64668735
//#define current get_current()

static struct vfsmount *aufs_mount;
static int aufs_mount_count;

static struct inode *aufs_get_inode(struct super_block *sb,int mode, dev_t dev)
{
   struct inode *inode=new_inode(sb);
   if(inode){
	inode->i_mode=mode;
	//inode->i_uid=current->fsuid;
	//inode->i_gid=current->fsgid;
	//inode->i_blksize = PAGE_CACHE_SIZE;
	inode->i_blocks=0;
	inode->i_atime=inode->i_mtime=inode->i_ctime=CURRENT_TIME;
	switch(mode & S_IFMT){
	   default:
		init_special_inode(inode,mode,dev);
		break;
	   case S_IFREG:
		printk("creat a file\n");
		break;
	   case S_IFDIR:
		inode->i_op= &simple_dir_inode_operations;
		inode->i_fop= &simple_dir_operations;
		printk("creat dir file \n");
  		inode->i_nlink++;
		break;
	}
   }
   return inode;
}


static int aufs_mknod(struct inode *dir, struct dentry *dentry,int mode,dev_t dev)
{
  struct inode *inode;
  int error= -EPERM;
  if(dentry->d_inode)
     return -EEXIST;
  inode=aufs_get_inode(dir->i_sb,mode,dev);
  if(inode){
     d_instantiate(dentry,inode);
     dget(dentry);
     error=0;
  }
  return error;
}

static int aufs_mkdir(struct inode *dir, struct dentry *dentry,int mode)
{
   int res;
   res=aufs_mknod(dir,dentry, mode |S_IFDIR, 0);
   if(!res)
     dir->i_nlink++;
   return res;

}

static int aufs_create(struct inode *dir, struct dentry *dentry, int mode)
{
  return aufs_mknod(dir,dentry, mode |S_IFREG,0);
}

static int aufs_fill_super(struct super_block *sb,void *data,int silent)
{
   static struct tree_descr debug_files[]={{""}};
   return simple_fill_super(sb,AUFS_MAGIC,debug_files);
}

static struct super_block *aufs_get_sb(struct file_system_type *fs_type,int flags,
           const char *dev_name,void *data)
{
   return get_sb_single(fs_type,flags,data,aufs_fill_super);
}

static struct file_system_type au_fs_type={
     .owner=THIS_MODULE,
     .name = "aufs",
     .get_sb=aufs_get_sb,
     .kill_sb=kill_litter_super,
};

static int aufs_create_by_name(const char *name, mode_t mode,struct dentry *parent,
       struct dentry **dentry)
{
   int error =0;
   if(!parent){
      if(aufs_mount && aufs_mount->mnt_sb){
	parent = aufs_mount->mnt_sb->s_root;
      }
   }
   if(!parent){
      printk("ah! can not find parent ! \n");
      return -EFAULT;
   }
	
   *dentry=NULL;
   mutex_lock(&parent->d_inode->i_mutex);
   *dentry=lookup_one_len(name,parent,strlen(name));
   if(!IS_ERR(dentry)){
      if((mode & S_IFMT)==S_IFDIR)
        error=aufs_mkdir(parent->d_inode,*dentry,mode);
      else
        error=aufs_create(parent->d_inode,*dentry,mode);
   }
   else
      error=PTR_ERR(dentry);
   mutex_unlock(&parent->d_inode->i_mutex);
   return error;
   
}

struct dentry *aufs_create_file(const char *name,mode_t mode,struct dentry *parent,
        void *data, struct file_operations  *fops)
{
  struct dentry *dentry =NULL;
  int error;
  printk("aufs: creating file '%' \n", name);
  error=aufs_create_by_name(name,mode,parent,&dentry);
  if(error){
     dentry = NULL;
     goto exit;
  }
  if(dentry->d_inode){
    if(data)
       dentry->d_inode->u.generic_ip=data;
    if(fops)
      dentry->d_inode->i_fop=fops;
  }
exit:
  return dentry;
       
}

struct dentry *aufs_create_dir(const char *name,struct dentry *parent)
{
  return aufs_create_file(name,S_IFDIR|S_IRWXU|S_IRUGO|S_IXUGO,parent,NULL,NULL);
}

staticrint __init aufs_init(void)
{
  int ret;
  struct dentry *pslot;
  ret=register_filesystem(&au_fs_type);
  if(!ret){
     aufs_mount = kern_mount(&au_fs_type);
     if(IS_ERR(aufs_mount)){
        printk(KERN_ERR "aufs: can not mount ! \n");
        unregister_filesystem(&au_fs_type);
        return ret;
     }
   }
   pslot=aufs_create_dir("aufs_test",NULL);
   aufs_create_file("lbb",S_IFREG|S_IRUGO,pslot,NULL,NULL);
   aufs_create_file("fbb",S_IFREG|S_IRUGO,pslot,NULL,NULL);
   aufs_create_file("aaa",S_IFREG|S_IRUGO,pslot,NULL,NULL);

   pslot=aufs_create_dir("aufs_test1",NULL);
   aufs_create_file("fdb",S_IFREG|S_IRUGO,pslot,NULL,NULL);
   aufs_create_file("f",S_IFREG|S_IRUGO,pslot,NULL,NULL);
   aufs_create_file("fsdf",S_IFREG|S_IRUGO,pslot,NULL,NULL);
   
   return ret;
}

 void __exit aufs_exit(void)
{
  simple_release_fs(&aufs_mount,&aufs_mount_count);
  unregister_filesystem(&au_fs_type);
}

//module_init(aufs_init);
//module_exit(aufs_exit);
MODULE_INIT(aufs_init);
MODULE_EXIT(aufs_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("this is a simple module");
MODULE_VERSION("ver 1.0");


