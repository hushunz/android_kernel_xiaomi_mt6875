/*
 * Copyright (C) 2016 MediaTek Inc.
 * Copyright (C) 2021 XiaoMi, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "ccu_cmn.h"
#include "ccu_mva.h"
#include <linux/timekeeping.h>

static struct ion_client *_ccu_ion_client;

int ccu_config_m4u_port(void);
static struct ion_handle *_ccu_ion_alloc(struct ion_client *client,
		unsigned int heap_id_mask, size_t align, unsigned int size,
		unsigned int flags);
static int _ccu_ion_get_mva(struct ion_client *client,
	struct ion_handle *handle,
		unsigned int *mva, int port);
static void _ccu_ion_free_handle(struct ion_client *client,
	struct ion_handle *handle);

int ccu_ion_init(void)
{
	MBOOL need_init = MFALSE;

	ccu_lock_ion_client_mutex();
	if (!_ccu_ion_client && g_ion_device) {
		LOG_INF_MUST("CCU ION_client need init\n");
		need_init = MTRUE;
	} else {
		LOG_INF_MUST("ION Client exist: 0x%p | Device NULL: 0x%p\n",
			_ccu_ion_client, g_ion_device);
	}

	if (need_init == MTRUE) {
		_ccu_ion_client = ion_client_create(g_ion_device, "ccu");
		LOG_INF_MUST("CCU ION_client create success: 0x%p\n",
			_ccu_ion_client);
	}

	ccu_unlock_ion_client_mutex();
	return 0;
}

int ccu_ion_uninit(void)
{
	MBOOL need_uninit = MFALSE;

	ccu_lock_ion_client_mutex();
	if (_ccu_ion_client && g_ion_device) {
		LOG_INF_MUST("CCU ION_client need uninit\n");
		need_uninit = MTRUE;
	}

	if (need_uninit == MTRUE) {
		ion_client_destroy(_ccu_ion_client);
		LOG_INF_MUST("CCU ION_client destroy done.\n");
		_ccu_ion_client = NULL;
	}

	ccu_unlock_ion_client_mutex();
	return 0;
}

int ccu_deallocate_mva(struct ion_handle **handle)
{
	LOG_DBG("X-:%s\n", __func__);
	if (_ccu_ion_client == NULL) {
		LOG_ERR("%s: _ccu_ion_client is null!\n", __func__);
		return -1;
	}

	if (*handle != NULL) {
		_ccu_ion_free_handle(_ccu_ion_client, *handle);
		*handle = NULL;
	}
	return 0;
}

int ccu_allocate_mva(uint32_t *mva, void *va,
	struct ion_handle **handle, int buffer_size)
{
	int ret = 0;
	/*int buffer_size = 4096;*/

	if (_ccu_ion_client == NULL) {
		LOG_ERR("%s: _ccu_ion_client is null!\n", __func__);
		return -1;
	}

	ret = ccu_config_m4u_port();
	if (ret) {
		LOG_ERR("fail to config m4u port!\n");
		return ret;
	}

	*handle = _ccu_ion_alloc(_ccu_ion_client,
			ION_HEAP_MULTIMEDIA_MAP_MVA_MASK,
			(unsigned long)va, buffer_size, 0);

	/*i2c dma buffer is PAGE_SIZE(4096B)*/

	if (!(*handle)) {
		LOG_ERR("Fatal Error, ion_alloc for size %d failed\n", 4096);
		return -1;
	}

	ret = _ccu_ion_get_mva(_ccu_ion_client, *handle, mva, 0);

	if (ret) {
		LOG_ERR("ccu ion_get_mva failed\n");
		ccu_deallocate_mva(handle);
		return -1;
	}

	return ret;
}



int ccu_config_m4u_port(void)
{
	int ret = 0;

#if defined(CONFIG_MTK_M4U)
	struct M4U_PORT_STRUCT port;

	port.ePortID = M4U_PORT_CCU0;
	port.Virtuality = 1;
	port.Security = 0;
	port.domain = 3;
	port.Distance = 1;
	port.Direction = 0;

	ret = m4u_config_port(&port);
#endif
	return ret;
}

static struct ion_handle *_ccu_ion_alloc(struct ion_client *client,
		unsigned int heap_id_mask, size_t align, unsigned int size,
		unsigned int flags)
{
	struct ion_handle *disp_handle = NULL;

	disp_handle = ion_alloc(client, size, align, heap_id_mask, flags);
	if (IS_ERR(disp_handle)) {
		LOG_ERR("disp_ion_alloc 1error %p\n", disp_handle);
		return NULL;
	}

	LOG_DBG("disp_ion_alloc 1 %p\n", disp_handle);

	return disp_handle;

}

static int _ccu_ion_get_mva(struct ion_client *client,
	struct ion_handle *handle,
		unsigned int *mva, int port)
{
	struct ion_mm_data mm_data;
	size_t mva_size;
	ion_phys_addr_t phy_addr = 0;

	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER_EXT;
	mm_data.config_buffer_param.kernel_handle = handle;
	mm_data.config_buffer_param.module_id   = port;
	mm_data.config_buffer_param.security    = 0;
	mm_data.config_buffer_param.coherent    = 1;
	mm_data.config_buffer_param.reserve_iova_start  = 0x10000000;
	mm_data.config_buffer_param.reserve_iova_end    = 0xFFFFFFFF;

	if (ion_kernel_ioctl(client, ION_CMD_MULTIMEDIA,
		(unsigned long)&mm_data) < 0) {
		LOG_ERR("disp_ion_get_mva: config buffer failed.%p -%p\n",
			client, handle);

		ion_free(client, handle);
		return -1;
	}
	*mva = 0;
	*mva = (port<<24) | ION_FLAG_GET_FIXED_PHYS;
	mva_size = ION_FLAG_GET_FIXED_PHYS;

	phy_addr = *mva;
	ion_phys(client, handle, &phy_addr, &mva_size);
	*mva = (unsigned int)phy_addr;
	LOG_DBG_MUST("alloc mmu addr hnd=0x%p,mva=0x%08x\n",
		handle, (unsigned int)*mva);
	return 0;
}

static void _ccu_ion_free_handle(struct ion_client *client,
	struct ion_handle *handle)
{
	if (!client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}
	if (!handle)
		return;

	ion_free(client, handle);

	LOG_DBG("free ion handle 0x%p\n", handle);
}

/* 官核: ccu_buffer_handle[2]，步长 0x30，按 meminfo.cached 索引 */
static struct CcuMemHandle ccu_buffer_handle[2];

struct CcuMemInfo *ccu_get_binary_memory(void)
{
	if (ccu_buffer_handle[0].meminfo.va != NULL)
		return &ccu_buffer_handle[0].meminfo;

	LOG_ERR("ccu ddr va not found!\n");
	return NULL;
}

/*
 * 官核 ccu_allocate_mem (0xffffff8008ca79c0):
 *   __ion_alloc(client, size, align=0, heap=0x400,
 *               flags=(cached ? 7 : 4), 0)
 *   -> ion_map_kernel -> _ccu_ion_get_mva(&mva, cached)
 *   -> 存入 ccu_buffer_handle[cached & 1]；不回写用户内存
 *   dump_time(meminfo+0x24) 且 size > 0xa00000(10MB) 时打印分配耗时
 */
int ccu_allocate_mem(struct CcuMemHandle *memHandle, int size, bool cached)
{
	int ret = 0;
	struct timespec64 ts;
	long long start_ns = 0;
	bool dump_time = memHandle->meminfo.dump_time != 0;

	LOG_DBG_MUST("_ccuAllocMem+ size(%d) cached(%d) handle(%p)\n",
		size, cached, memHandle->ionHandleKd);

	if (_ccu_ion_client == NULL) {
		LOG_ERR("%s: _ccu_ion_client is null!\n", __func__);
		return -EINVAL;
	}

	if (ccu_buffer_handle[cached & 1].ionHandleKd != NULL) {
		LOG_ERR("ccu buffer already allocated, cached(%d)\n",
			cached & 1);
		return -EINVAL;
	}

	if (dump_time) {
		getnstimeofday64(&ts);
		start_ns = -(ts.tv_sec * 1000000000LL + ts.tv_nsec);
	}

	memHandle->ionHandleKd = _ccu_ion_alloc(_ccu_ion_client,
		ION_HEAP_MULTIMEDIA_MASK, 0, (unsigned int)size,
		(cached & 1) ? 7 : 4);

	if ((0xa00000 < (unsigned int)size) && dump_time) {
		getnstimeofday64(&ts);
		LOG_INF_MUST("ccu alloc time %lld ns, size(%d)\n",
			ts.tv_sec * 1000000000LL + ts.tv_nsec + start_ns, size);
	}

	if (IS_ERR(memHandle->ionHandleKd)) {
		LOG_ERR("fail to get ion buffer handle (size=%d)\n", size);
		return -EINVAL;
	}
	if (memHandle->ionHandleKd == NULL) {
		LOG_ERR("fail to get ion buffer handle (size=%d)\n", size);
		return -EINVAL;
	}

	memHandle->meminfo.size = size;
	memHandle->meminfo.cached = cached & 1;

	memHandle->meminfo.va = (char *)ion_map_kernel(_ccu_ion_client,
		memHandle->ionHandleKd);
	if (memHandle->meminfo.va == NULL) {
		LOG_ERR("fail to get buffer kernel virtual address\n");
		return -EINVAL;
	}
	LOG_DBG_MUST("memHandle->va(0x%lx)\n",
		(unsigned long)memHandle->meminfo.va);

	ret = _ccu_ion_get_mva(_ccu_ion_client, memHandle->ionHandleKd,
		&memHandle->meminfo.mva, cached & 1);
	if (ret) {
		LOG_ERR("ccu ion_get_mva failed\n");
		return -1;
	}
	LOG_DBG_MUST("memHandle->mva(0x%x)\n", memHandle->meminfo.mva);

	ccu_buffer_handle[cached & 1] = *memHandle;

	LOG_DBG_MUST("_ccuAllocMem-\n");

	return 0;
}

/*
 * 官核 ccu_deallocate_mem (0xffffff8008ca7d90):
 *   取 ccu_buffer_handle[cached != 0].ionHandleKd
 *   -> ion_unmap_kernel -> ion_free
 *   -> memset 槽位(0x30)；dump_time 且 size > 10MB 打印释放日志
 *   官核不做 __close_fd / shareFd 释放
 */
int ccu_deallocate_mem(struct CcuMemHandle *memHandle)
{
	int cached = memHandle->meminfo.cached != 0;
	struct ion_handle *handle;

	LOG_DBG_MUST("free ccu ion: cached(%d) size(%d) share_fd(%d)\n",
		cached, memHandle->meminfo.size, memHandle->meminfo.shareFd);

	if (_ccu_ion_client == NULL) {
		LOG_ERR("%s: _ccu_ion_client is null!\n", __func__);
		return -EINVAL;
	}

	if (ccu_buffer_handle[cached].ionHandleKd == NULL) {
		LOG_ERR("ccu buffer not allocated, cached(%d)\n", cached);
		return -EINVAL;
	}

	handle = ccu_buffer_handle[cached].ionHandleKd;
	ion_unmap_kernel(_ccu_ion_client, handle);
	ion_free(_ccu_ion_client, handle);

	if ((memHandle->meminfo.dump_time != 0) &&
		(0xa00000 < memHandle->meminfo.size))
		LOG_INF_MUST("ccu dealloc done, size(%d)\n",
			memHandle->meminfo.size);

	memset(&ccu_buffer_handle[cached], 0, sizeof(struct CcuMemHandle));

	return 0;
}

void ccu_ion_free_import_handle(struct ion_handle *handle)
{
	if (!_ccu_ion_client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}
	if (handle == NULL) {
		LOG_ERR("invalid ion handle!\n");
		return;
	}

	ion_free(_ccu_ion_client, handle);
}

struct ion_handle *ccu_ion_import_handle(int fd)
{
	struct ion_handle *handle = NULL;

	if (!_ccu_ion_client) {
		LOG_ERR("ccu invalid ion client!\n");
		return handle;
	}
	if (fd == -1) {
		LOG_ERR("ccu invalid ion fd!\n");
		return handle;
	}

	handle = ion_import_dma_buf_fd(_ccu_ion_client, fd);
	LOG_INF_MUST("ccu_ion_import_fd : %d, %s : 0x%p\n",
		fd, __func__, handle);
	if (!(handle)) {
		LOG_ERR("ccu import ion handle failed!\n");
		return NULL;
	}

	return handle;
}
