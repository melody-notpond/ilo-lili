#include "mmu.h"

#include "pages.h"

// everything is sv39 cuz im lazy
static const int PTE_SIZE = 9;
static const int PTE_MASK = 0x1f;
static const int ENTRY_COUNT = PAGE_SIZE / sizeof(uintptr_t);

extern int text_start;
extern int ro_data_start;
extern int data_start;
extern int pages_bottom;

// identity maps the range of memory [start, end) on the provided page table
// with the given flags. used for identity mapping the kernel
static void identity_map(mmu_t mmu, void *start, void *end, int flags) {
  start = PAGE_ALIGN_PREV(start);
  end = PAGE_ALIGN(end);
  for (uint8_t *p = start; p < (uint8_t *) end; p += PAGE_SIZE) {
    mmu_map(mmu, p, p, flags);
  }
}

// walks the page table to obtain a pointer to the page table entry
// corresponding to the provided virtual address. returns NULL if there is no
// entry. if alloc is true, then any missing page table levels will be allocated
// and added to the mmu.
static uintptr_t *mmu_entry(mmu_t mmu, void *virt, bool alloc) {
  // i pretty much just copied this from the risc v spec ngl, go read the
  // psuedocode if you cant read this code
  uintptr_t *table = make_accessible(mmu.mmu);
  uintptr_t virti = (uintptr_t) virt;
  for (int i = 2; i > 0; i--) {
    int offset = PAGE_BITS + PTE_SIZE * i;
    uintptr_t index = (virti & (PTE_MASK << offset)) >> offset;
    uintptr_t entry = table[index];

    if (!(entry & M_VALID) || (!(entry & M_READ) && (entry & M_WRITE))) {
      if (!alloc)
        return NULL;
      void *page = alloc_page(true);
      table[index] = ((uintptr_t) page >> 2) | M_VALID;
      table = make_accessible(page);
      continue;
    }

    if ((entry & (M_READ | M_EXEC)))
      return &table[index];
    table = make_accessible((void *) ((entry << 2) & ~PAGE_MASK));
  }

  uintptr_t index = (virti & (PAGE_MASK << PAGE_BITS)) >> PAGE_BITS;
  return &table[index];
}

// creates a new page table with the kernel identity mapped and the upper
// 256gbs of the virtual address space mapped to physical memory.
mmu_t mmu_new_page_table(void) {
  mmu_t mmu = { .mmu = alloc_page(true) };

  // identity map the kernel
  identity_map(mmu, &text_start, &ro_data_start, M_GLOBAL | M_EXEC);
  identity_map(mmu, &ro_data_start, &data_start, M_GLOBAL | M_READ);
  identity_map(mmu, &data_start, &pages_bottom,  M_GLOBAL | M_READ | M_WRITE);

  // set the top half of the virtual address space to map physical memory
  // for read/write in S mode
  volatile uintptr_t *p = make_accessible(mmu.mmu);
  for (int i = ENTRY_COUNT / 2; i < ENTRY_COUNT; i++) {
    p[i] = ((uintptr_t) (i - ENTRY_COUNT / 2) << (PTE_SIZE * 2 + PAGE_BITS - 2))
      | M_GLOBAL | M_READ | M_WRITE | M_VALID;
  }

  return mmu;
}

// maps the given virtual address to the given physical address with the given
// flags on the page table provided.
void mmu_map(mmu_t mmu, void *virt, void *phys, int flags) {
  flags = (flags & M_FLAG_MASK) | M_VALID;
  uintptr_t *entry = mmu_entry(mmu, virt, true);
  *entry = ((uintptr_t) PAGE_ALIGN_PREV(phys) >> 2) | flags;
}

// sets the active page table to the input argument.
void set_mmu(mmu_t mmu) {
  uintptr_t satp = 0x8000000000000000 | ((uintptr_t) mmu.mmu >> PAGE_BITS);
  asm volatile
    ( "csrw satp, %0\n"
      "sfence.vma" : : "r" (satp));
}

// gets the currently active page table.
mmu_t get_mmu(void) {
  uintptr_t satp = 0;
  asm volatile ("csrr %0, satp" : "=r" (satp));
  if (satp & 0x8000000000000000)
    return (mmu_t) { .mmu = (void *) ((satp & SV39_MASK) << PAGE_BITS) };
  return (mmu_t) { .mmu = NULL };
}

// returns true iff the mmu is enabled.
bool mmu_enabled(void) {
  uintptr_t satp = 0;
  asm volatile ("csrr %0, satp" : "=r" (satp));
  return satp & 0x8000000000000000;
}

// converts a physical address to a virtual address in the upper 256gb of the
// virtual address space. does nothing if the mmu is not enabled.
void *make_accessible(void *physical) {
  if (mmu_enabled()) {
    return phys2virt(physical);
  }

  return physical;
}

// walks the page table to obtain the physical address corresponding to the
// provided virtual address as a valid pointer. returns NULL if there is no
// valid translation for the provided virtual address.
void *mmu_walk(mmu_t mmu, void *virt) {
  uintptr_t *entry = mmu_entry(mmu, virt, false);
  if (!entry || !(*entry & M_VALID))
    return NULL;
  uintptr_t offset = (uintptr_t) virt & PAGE_MASK;
  void *physical = (void *) ((((uintptr_t) *entry << 2) & ~PAGE_MASK) | offset);
  return make_accessible(physical);
}
