// header file of module ocitest.o 

#define OCI_NOP 1         // ioctl 用のコマンド数値
#define OCI_INT 2         // 一応、カッコ良く定義しておく
#define OCI_STRING 3
#define OCI_STRUCT 4

struct oci_struct 
{
  char *p;
  int len;
};