#if !defined(__INVTD_PRIV_H__)
#define __INVTD_PRIV_H__

#define INVTD_DEF_CONF_PATH  "../conf/invterd.xml"

int invtd_getopt(int argc, char **argv, invtd_opt_t *opt);
int invtd_usage(const char *exec);

#endif /*__INVTD_PRIV_H__*/
