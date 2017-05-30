

#include "storage/pmem.h"
#include "storage/fd.h"


#ifdef HAVE_PMEM
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libpmem.h>
#include <sys/mman.h>

int 
PmemFileOpen(char *pathname, int flags, int mode, size_t fsize, void **addr)
{
	int mapped_flag = 0;
	int is_pmem = 0;
	size_t mapped_len, size = 0;

	Assert(addr);

	/* non-zero 'len' not allowed without PMEM_FILE_CREATE */
	if ( flags & O_CREAT ) 
	{
		mapped_flag |= PMEM_FILE_CREATE;
		size = fsize;
	}
		
	if ( flags & O_EXCL ) 
		mapped_flag |= PMEM_FILE_EXCL; // Ensure create file ;; EEXIST

	*addr = NULL;
	*addr = pmem_map_file(pathname, size, flags, mode,  &mapped_len, &is_pmem);

	if (!is_pmem)
		ereport(NOTICE, 
				(errmsg("xlog file [%s] dosen't consist of persistent memory.", 
								pathname)));

	if (mapped_len == fsize)
	{
		if ( flags & O_CREAT )
			if ( msync(*addr, mapped_len, MS_SYNC) )
				ereport(PANIC,
						(errcode_for_file_access(),
						 errmsg("could not msync log file %s: %m", pathname)));

		return NO_FD_FOR_MAPPED_FILE;
	}

	ereport(NOTICE, 
			(errmsg("xlog file [%s] couldn't mapped on persistent memory.", 
							pathname)));

	if (*addr)
	{
		pmem_unmap(addr, mapped_len);
		*addr = NULL;
	}

	return BasicOpenFile(pathname, flags, mode);
}

void 
PmemFileWrite(void *dest, void *src, size_t len)
{
	Assert(dest != NULL && src != NULL);

	pmem_memcpy_nodrain(dest, src, len);

	return;
}

void 
PmemFileRead(void *map_addr, void *buf, size_t len)
{
	Assert(dest != NULL && src != NULL);

	memcpy(buf, map_addr, len);

	return;
}

void 
PmemFileSync(void)
{
	return pmem_drain();
}

int 
PmemFileClose(void *addr, size_t fsize)
{
	Assert(addr);
	return pmem_unmap(addr, fsize);
}


#else
int 
PmemFileOpen(char *pathname, int flags, int mode, size_t fsize, void **addr)
{

	return BasicOpenFile(pathname, flags, mode);
}

void 
PmemFileWrite(void *dest, void *src, size_t len) { 
	ereport(PANIC, (errmsg("don't have the pmem device")));
}

void 
PmemFileRead(void *map_addr, void *buf, size_t len)
	ereport(PANIC, (errmsg("don't have the pmem device")));
}

void 
PmemFileSync(void)
{
	ereport(PANIC, (errmsg("don't have the pmem device")));
}

int 
PmemFileClose(void *addr, size_t fsize)
{
	ereport(PANIC, (errmsg("don't have the pmem device")));
	return -1;
}
#endif

