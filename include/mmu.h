#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  // this always contains the physical address of the mmu
  void *mmu;
} mmu_t;

#define M_FLAG_MASK ((1 << 8) - 1)
#define M_DIRTY     (1 << 7)
#define M_ACCESSED  (1 << 6)
#define M_GLOBAL    (1 << 5)
#define M_USER      (1 << 4)
#define M_EXEC      (1 << 3)
#define M_WRITE     (1 << 2)
#define M_READ      (1 << 1)
#define M_VALID     (1 << 0)

#define SV39_MASK 0x7fffffffff
#define phys2virt(phys) ((void *) (((uintptr_t) (phys)) | ~(SV39_MASK >> 1)))
#define virt2phys(virt) ((void *) (((uintptr_t) (virt)) & SV39_MASK))

// creates a new page table with the kernel identity mapped and the upper
// 256gbs of the virtual address space mapped to physical memory.
mmu_t mmu_new_page_table(void);

// maps the given virtual address to the given physical address with the given
// flags on the page table provided.
void mmu_map(mmu_t mmu, void *virt, void *phys, int flags);

// sets the active mmu to the input argument.
void set_mmu(mmu_t mmu);

// gets the currently active page table.
mmu_t get_mmu(void);

// returns true iff the mmu is enabled.
bool mmu_enabled(void);

// converts a physical address to a virtual address in the upper 256gb of the
// virtual address space. does nothing if the mmu is not enabled.
void *make_accessible(void *physical);

// walks the page table to obtain the physical address corresponding to the
// provided virtual address as a valid pointer. returns NULL if there is no
// valid translation for the provided virtual address.
void *mmu_walk(mmu_t mmu, void *virt);
