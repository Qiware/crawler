#if !defined(__MEM_REF_H__)
#define __MEM_REF_H__

int mem_ref_init(void);
int mem_ref_incr(void *addr);
int mem_ref_sub(void *addr);

#endif /*__MEM_REF_H__*/