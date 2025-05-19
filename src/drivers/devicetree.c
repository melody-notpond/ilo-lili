#include "drivers/devicetree.h"
#include "console.h"
#include "ints.h"

#define FDT_MAGIC nv2be32(0xd00dfeed)
#define FDT_VERSION nv2be32(17)

// device tree spec:
// https://readthedocs.org/projects/devicetree-specification/downloads/pdf/latest/

struct fdt_header {
  // all fields in big endian
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
};

// a devicetree memory reserve entry. memory writes from `address` to
// `address+size` are invalid and may not occur.
struct fdt_reserve_entry {
  uint64_t address;
  uint64_t size;
};

// definitions related to structure blocks
#define FDT_BEGIN_NODE   nv2be32(0x00000001)
#define FDT_END_NODE     nv2be32(0x00000002)
#define FDT_PROP         nv2be32(0x00000003)
struct fdt_prop {
  uint32_t len;
  uint32_t nameoff;
};

#define FDT_NOP          nv2be32(0x00000004)
#define FDT_END          nv2be32(0x00000009)

// assumption: an fdt_node points to FDT_BEGIN_NODE
// assumption: an fdt_node node is valid iff node != NULL and
//             *node == FDT_BEGIN_NODE
struct fdt_node { };

// validates a device tree. on failure, returns NULL. otherwise, returns a valid
// devicetree value.
devicetree fdt_validate(void *p) {
  struct fdt_header *header = p;

  if (header->magic != FDT_MAGIC)
      return NULL;
  if (header->version != FDT_VERSION)
      return NULL;
  return header;
}

// returns the number of memory reservation entries the device tree has.
int fdt_count_mem_reserve_entries(devicetree tree) {
  struct fdt_reserve_entry *entries =
    (struct fdt_reserve_entry *) (((void *) tree) +
    be2nv32(tree->off_mem_rsvmap));
  int count = 0;
  for (; entries->address != 0 || entries->size != 0; count++, entries++);
  return count;
}

// returns a reference to the root node. this must be valid if the device tree
// is correct.
fdt_node fdt_root_node(devicetree tree) {
  uint32_t *p = (void *) tree + be2nv32(tree->off_dt_struct);

  // skip nops
  for (; *p == FDT_NOP; p++);

  // p must have FDT_BEGIN_NODE or else it's invalid
  if (*p != FDT_BEGIN_NODE)
    return NULL;
  return (fdt_node) p;
}

// checks if an fdt_node handle is valid, returns true iff valid.
bool fdt_node_valid(fdt_node node) {
  return node != NULL && *(int32_t *) node == FDT_BEGIN_NODE;
}

// gets the name of an fdt node.
char *fdt_node_name(fdt_node node) {
  if (!fdt_node_valid(node))
    return NULL;

  // name is immediately after FDT_BEGIN_NODE according to spec
  return (char *) node + 4;
}

// helper function that skips the name of a node. assumes *p == FDT_BEGIN_NODE.
uint32_t *fdt_skip_name(fdt_node node) {
  char *c = (char *) node + 4;
  for (; *c; c++);
  return align4(uint32_t, c + 1);
}

// helper function that skips a property of a node. assumes *p == FDT_PROP.
uint32_t *fdt_skip_property(uint32_t *p) {
  p++;
  p = (void *) p + sizeof(struct fdt_prop) +
      be2nv32(((struct fdt_prop *) p)->len);
  return align4(uint32_t, p);
}

// iterates over nodes depth first in order of definition. returns an invalid
// node when there are no more nodes or when an invalid node exists. example
// usage:
// 
//   for ( fdt_node node = fdt_root_node(tree)
//       ; fdt_node_valid(node)
//       ; node = fdt_node_iter(node)) {
//     ... do stuff with node
//   }
fdt_node fdt_node_iter(fdt_node node) {
  if (!fdt_node_valid(node))
     return node;

  // skip nonnodes
  uint32_t *p = fdt_skip_name(node);
  while (*p != FDT_BEGIN_NODE) {
    switch (*p) {
      case FDT_END_NODE:
      case FDT_NOP:
        p++;
        break;

      case FDT_PROP:
        p = fdt_skip_property(p);
        break;

      case FDT_END:
        return NULL;

      default:
        kprintf("invalid token %x\n", be2nv32(*p));
        return NULL;
    }
  }

  return (fdt_node) p;
}

// helper function that skips a node while iterating. assumes
// *p == FDT_BEGIN_NODE.
uint32_t *fdt_skip_node(uint32_t *p) {
  int deep = 1;
  p = fdt_skip_name((fdt_node) p);
  while (deep > 0) {
    switch (*p) {
      case FDT_BEGIN_NODE:
        p = fdt_skip_name((fdt_node) p);
        deep++;
        break;

      case FDT_END_NODE:
        deep--;
      case FDT_NOP:
        p++;
        break;

      case FDT_PROP:
        p = fdt_skip_property(p);
        break;

      default:
        return p;
    }
  }

  return p;
}

// iterates over the child nodes of a parent in order of definition. returns an
// invalid node when there are no more nodes or when an invalid node exists.
// passing in NULL to the child parameter yields the first child, whereas
// passing in a child node passes in the next child.
// example usage:
// 
//   
//   for ( fdt_node child = fdt_node_child_iter(parent, NULL)
//       ; fdt_node_valid(child)
//       ; child = fdt_node_child_iter(parent, child)) {
//     ... do stuff with node
//   }
fdt_node fdt_node_child_iter(fdt_node parent, fdt_node child) {
  if (!fdt_node_valid(parent))
     return parent;

   // find the first child since we dont have a valid child already
   if (!fdt_node_valid(child)) {
      uint32_t *p = fdt_skip_name(parent);
      while (*p != FDT_BEGIN_NODE) {
        switch (*p) {
          // if the node ended then parent has no children
          case FDT_END_NODE:
          case FDT_END:
            return NULL;

          case FDT_NOP:
            p++;
            break;

          case FDT_PROP:
            p = fdt_skip_property(p);
            break;

          default:
            kprintf("invalid token %x\n", be2nv32(*p));
            return NULL;
        }
      }

      return (fdt_node) p;
   }

   // instead we have to find the child from the given child
    uint32_t *p = fdt_skip_node((uint32_t *) child);
    while (*p != FDT_BEGIN_NODE) {
      switch (*p) {
        case FDT_END_NODE:
        case FDT_END:
          return NULL;

        case FDT_NOP:
          p++;
          break;

        case FDT_PROP:
          p = fdt_skip_property(p);
          break;

        default:
          kprintf("invalid token %x\n", be2nv32(*p));
          return NULL;
      }
    }

    return (fdt_node) p;
 }

// things we want in our api:
// - iterate through all properties of a node
// - get the value of a specific node property
// - get a node by path
// - search for a node
// - dump device tree to serial port for debugging
