
#ifndef PMEM_H
#define PMEM_H

#include "postgres.h"

#ifdef HAVE_PMEM
#include <libpmem.h>
#endif


#define NO_FD_FOR_MAPPED_FILE -1

extern int	PmemFileOpen(char *pathname, int flags, int mode,
	 	size_t fsize, void **addr);
extern void	PmemFileWrite(void *dest, void *src, size_t len);
extern void PmemFileRead(void *map_addr, void *buf, size_t len);
extern void	PmemFileSync(void);
extern int	PmemFileClose(void *addr, size_t fsize);

#endif /* PMEM_H */
