

#include "storage/pmem.h"
#include "storage/fd.h"


#ifdef HAVE_PMEM
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libpmem.h>
#include <sys/mman.h>
#include <string.h>

int 
PmemFileOpen(char *pathname, int flags, int mode, size_t fsize, void **addr)
{
	int mapped_flag = 0;
	int is_pmem = 0;
	size_t mapped_len = 0, size = 0;
	struct stat	fstat;
	void *ret_addr;

	Assert(addr);

	/* non-zero 'len' not allowed without PMEM_FILE_CREATE */
	if (stat(pathname, &fstat) == 0)
	{
		if ((flags & O_CREAT) && (flags & O_EXCL))
		{
			errno = EEXIST;
			return -1;
		}
	}
	else if (errno == ENOENT) {
		if (flags & O_CREAT) 
		{
			mapped_flag = PMEM_FILE_CREATE;
			size = fsize;
		}
		else {
			errno = ENOENT;
			return -1;
		}

		if ( flags & O_EXCL ) 
			mapped_flag |= PMEM_FILE_EXCL;
	}
	else
		return -1;

	errno = 0;
	ret_addr = pmem_map_file(pathname, size, mapped_flag, mode,  &mapped_len, &is_pmem);

	if (is_pmem == 0) {
		ereport(LOG, 
				(errmsg("xlog file [%s][%p] dosen't consist of persistent memory.", 
								pathname, ret_addr)));
	}

	if (fsize == mapped_len)
	{
		if ( mapped_flag & PMEM_FILE_CREATE )
			if ( msync(ret_addr, mapped_len, MS_SYNC) )
				ereport(PANIC,
						(errcode_for_file_access(),
						 errmsg("could not msync log file %s: %m", pathname)));

		*addr = ret_addr;
		return NO_FD_FOR_MAPPED_FILE;
	}

	ereport(LOG, 
			(errmsg("xlog file [%s(file_len=%d; mapped_len=%d)] couldn't mapped on persistent memory.", 
							pathname, fsize, (int)mapped_len)));

	if (ret_addr != NULL)
		pmem_unmap(ret_addr, mapped_len);

	ereport(LOG, 
			(errmsg("xlog file [%s] was basic opened.", pathname)));
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

