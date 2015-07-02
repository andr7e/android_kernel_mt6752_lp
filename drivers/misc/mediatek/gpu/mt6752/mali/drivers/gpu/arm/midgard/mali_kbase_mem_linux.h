/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */






#ifndef _KBASE_MEM_LINUX_H_
#define _KBASE_MEM_LINUX_H_

typedef struct kbase_hwc_dma_mapping {
	void       *cpu_va;
	dma_addr_t  dma_pa;
	size_t      size;
} kbase_hwc_dma_mapping;

struct kbase_va_region * kbase_mem_alloc(struct kbase_context * kctx, u64 va_pages, u64 commit_pages, u64 extent, u64 * flags, u64 * gpu_va, u16 * va_alignment);
mali_error kbase_mem_query(struct kbase_context *kctx, mali_addr64 gpu_addr, int query, u64 * const pages);
int kbase_mem_import(struct kbase_context *kctx, enum base_mem_import_type type, int handle, mali_addr64 * gpu_va, u64 * va_pages, u64 * flags);
u64 kbase_mem_alias(struct kbase_context *kctx, u64* flags, u64 stride, u64 nents, struct base_mem_aliasing_info* ai, u64 * num_pages);
mali_error kbase_mem_flags_change(struct kbase_context *kctx, mali_addr64 gpu_addr, unsigned int flags, unsigned int mask);
int kbase_mem_commit(struct kbase_context * kctx, mali_addr64 gpu_addr, u64 new_pages, enum base_backing_threshold_status * failure_reason);
int kbase_mmap(struct file *file, struct vm_area_struct *vma);

struct kbase_vmap_struct {
	mali_addr64 gpu_addr;
	struct kbase_mem_phy_alloc *alloc;
	phys_addr_t *pages;
	void *addr;
	size_t size;
	bool is_cached;
};
void *kbase_vmap(struct kbase_context *kctx, mali_addr64 gpu_addr, size_t size,
		struct kbase_vmap_struct *map);
void kbase_vunmap(struct kbase_context *kctx, struct kbase_vmap_struct *map);

void *kbase_va_alloc(struct kbase_context *kctx, u32 size, struct kbase_hwc_dma_mapping *handle);

void kbase_va_free(struct kbase_context *kctx, struct kbase_hwc_dma_mapping *handle);

#endif				
