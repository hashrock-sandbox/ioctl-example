// gcc -c ocitest.c -Wall -Wstrict-prototypes -O -pipe -m486
// mknod /dev/ocitest c 60 0;  chmod a+rw /dev/ocitest  

#define MODULE
#define __KERNEL__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/string.h>

#include "ocitest.h"

static int devmajor=60;
static char *devname="ocitest";
#if LINUX_VERSION_CODE > 0x20115
MODULE_PARM(devmajor, "i");
MODULE_PARM(devname, "s");
#endif

#if LINUX_VERSION_CODE >= 0x020100
#include <asm/uaccess.h>
#else
// Linux 2.0.x では memcpy_tofs/_fromfs だった関数が
// Linux 2.1.x 以降で copy_to/from_user に 変更。なので合わせる
static inline unsigned long
copy_to_user(void *to, const void *from, unsigned long n)
{
        memcpy_tofs(to,from,n);
        return 0;
}

static inline unsigned long
copy_from_user(void *to, const void *from, unsigned long n)
{
        memcpy_fromfs(to,from,n);
        return 0;
}
#endif

// Linux 2.0/2.2 共通
static int ocitest_open(struct inode * inode, struct file * file)
{
  MOD_INC_USE_COUNT;
  return 0;
}

// Linux 2.1 以降帰り値 int (事実上共通でも可: カーネル内で返り値使わず)
#if LINUX_VERSION_CODE >= 0x020100
static int ocitest_close(struct inode * inode, struct file * file)
#else
static void ocitest_close(struct inode * inode, struct file * file)
#endif
{
  MOD_DEC_USE_COUNT;
#if LINUX_VERSION_CODE >= 0x020100
  return 0;
#endif
}

// ioctl: Linux 2.0/2.2共通
static int ocitest_ioctl(struct inode *inode, struct file *file, 
	      unsigned int cmd, unsigned long arg)
{
  char buff[257];
  struct oci_struct ocis;
  int i;
  printk("ocitest: ioctl: cmd=%04X, arg=%08lX\n",cmd,arg);
  switch(cmd)
    {
    case OCI_NOP: printk("    Cmd: No operation\n");     return 0;
    case OCI_INT: printk("    Cmd: Integer: %ld\n",arg); return 0;
    case OCI_STRING:
      for(i=0;i<256;i++)    // arg をポインタと見なしてユーザデータ取得
	{  // get_user はバージョン依存
#if LINUX_VERSION_CODE >= 0x020100
	  if(get_user(buff[i],(char *)(arg+i))) return -EFAULT;
#else
	  buff[i]=get_user((char *)(arg+i));
#endif
	  if(buff[i]=='\0') break;
	}
      buff[i]='\0';         // 安全止
      printk("    Cmd: String: %s\n",buff);
      for(i=0;buff[i];i++)  // to lower
	{ if((buff[i]>='A')&&(buff[i]<='Z')) buff[i]+=('a'-'A'); }
      copy_to_user((void *)arg,buff,i);  // データ返還
      return 0;
    case OCI_STRUCT:
      copy_from_user(&ocis,(void *)arg,sizeof(ocis));
      for(i=0;;i++)  // 文字列長さカウント
	{
#if LINUX_VERSION_CODE >= 0x020100
	  if(get_user(buff[0],ocis.p+i)) return -EFAULT;
#else
	  buff[0]=get_user(ocis.p+i);
#endif
	  if(buff[0]=='\0') break;
	}
      ocis.len=i;
      copy_to_user((void *)arg,&ocis,sizeof(ocis));  // データ返還
      return 0;
    }
  return -EINVAL;
}


#if LINUX_VERSION_CODE >= 0x020100
static struct file_operations ocitest_fops = {    // Linux 2.2.10 より
  NULL,         // loff_t  llseek(struct file *, loff_t, int)
  NULL,         // ssize_t read(struct file *, char *, size_t, loff_t *)
  NULL,         // ssize_t write(struct file *, const char *, size_t, loff_t *)
  NULL,         // int     readdir(struct file *, void *, filldir_t)
  NULL,         // u. int  poll(struct file *, struct poll_table_struct *)
  ocitest_ioctl,// int     ioctl(struct inode *, struct file *, u.int, u.long)
  NULL,         // int     mmap(struct file *, struct vm_area_struct *)
  ocitest_open, // int     open(struct inode *, struct file *)
  NULL,         // int     flush(struct file *)
  ocitest_close,// int     release(struct inode *, struct file *)
  NULL,         // int     fsync(struct file *, struct dentry *)
  NULL,         // int     fasync(int, struct file *, int)
  NULL,         // int     check_media_change(kdev_t dev)
  NULL,         // int     revalidate(kdev_t dev)
  NULL,         // int     lock(struct file *, int, struct file_lock *)
};
#else
static struct file_operations ocitest_fops = {    // Linux 2.0.36 より
  NULL,         // int  lseek(struct inode *, struct file *, off_t, int)
  NULL,         // int  read(struct inode *, struct file *, char *, int)
  NULL,         // int  write(struct inode *, struct file *, const char *, int)
  NULL,         // int  readdir(struct inode *, struct file *, void *, filldir_t)
  NULL,         // int  select(struct inode *, struct file *, int, select_table *)
  ocitest_ioctl,// int  ioctl(struct inode *, struct file *, u.int, u.long)
  NULL,         // int  mmap(struct inode *, struct file *, struct vm_area_struct *)
  ocitest_open,  // int  open(struct inode *, struct file *)
  ocitest_close, // void release(struct inode *, struct file *)
  NULL,         // int  fsync(struct inode *, struct file *)
  NULL,         // int  fasync(struct inode *, struct file *, int)
  NULL,         // int  check_media_change(kdev_t dev)
  NULL,         // int  revalidate(kdev_t dev)
};
#endif


int init_module(void)
{
  printk("install '%s' into major %d\n",devname,devmajor);
  if(register_chrdev(devmajor,devname,&ocitest_fops))  // 登録
    {
      printk("device registration error\n");
      return -EBUSY;
    }
  return 0;
}

void cleanup_module(void)
{
  printk("remove '%s' from major %d\n",devname,devmajor);
  if (unregister_chrdev(devmajor,devname)) 
    {
      printk ("unregister_chrdev failed\n");
    }
};