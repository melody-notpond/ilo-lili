#include "drivers/devicetree.h"
#include "console.h"
#include "ints.h"
#include "string.h"

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
struct fdt_node;

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
  if (!tree)
    return 0;
  struct fdt_reserve_entry *entries =
    (struct fdt_reserve_entry *) (((uint8_t *) tree) +
    be2nv32(tree->off_mem_rsvmap));
  int count = 0;
  for (; entries->address != 0 || entries->size != 0; count++, entries++);
  return count;
}

// returns a reference to the root node. this must be valid if the device tree
// is correct.
fdt_node fdt_root_node(devicetree tree) {
  if (!tree)
    return NULL;
  uint32_t *p = (uint32_t *) ((uint8_t *) tree + be2nv32(tree->off_dt_struct));

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
  p = (uint32_t *) ((uint8_t *) p + sizeof(struct fdt_prop) +
      be2nv32(((struct fdt_prop *) p)->len));
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
        p++;
        break;

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
// passing in a child node passes in the next child. example usage:
//
//   for ( fdt_node child = fdt_node_child_iter(parent, NULL)
//       ; fdt_node_valid(child)
//       ; child = fdt_node_child_iter(parent, child)) {
//     ... do stuff with child
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

// checks if an fdt_prop handle is valid, returns true iff valid.
bool fdt_prop_valid(fdt_prop prop) {
  return prop != NULL && *((uint32_t *) prop - 1) == FDT_PROP;
}

// gets the name of an fdt prop. tree must be the device tree from which prop
// was obtained.
char *fdt_prop_name(devicetree tree, fdt_prop prop) {
  if (!tree || !fdt_prop_valid(prop))
    return NULL;
  return (char *) tree + be2nv32(tree->off_dt_strings) + be2nv32(prop->nameoff);
}

// iterates over the properties of a node in order of definition. returns an
// invalid property when there are no more nodes or when an invalid token
// exists. passing in NULL to the prop parameter yields the first property,
// whereas passing in a valid property passes in the next child. example usage:
//
//   for ( fdt_prop prop = fdt_prop_iter(node, NULL)
//       ; fdt_prop_valid(prop)
//       ; prop = fdt_prop_iter(node, prop)) {
//     ... do stuff with prop
//   }
fdt_prop fdt_prop_iter(fdt_node node, fdt_prop prop) {
  if (!fdt_node_valid(node))
     return NULL;

  // find the first property since we dont have a valid property already
  if (!fdt_prop_valid(prop)) {
    uint32_t *p = fdt_skip_name(node);
    while (*p != FDT_PROP) {
      switch (*p) {
        case FDT_BEGIN_NODE:
          p = fdt_skip_node(p);
          break;

        // if the node ended then parent has no properties
        case FDT_END_NODE:
        case FDT_END:
          return NULL;

        case FDT_NOP:
          p++;
          break;

        default:
          kprintf("invalid token %x\n", be2nv32(*p));
          return NULL;
      }
    }

    return (fdt_prop) (p + 1);
  }
 
  // instead we have to find the child from the given child
  uint32_t *p = fdt_skip_property((uint32_t *) prop - 1);
  while (*p != FDT_PROP) {
    switch (*p) {
      case FDT_BEGIN_NODE:
        p = fdt_skip_node(p);
        break;

    case FDT_END_NODE:
    case FDT_END:
      return NULL;

    case FDT_NOP:
      p++;
      break;

    default:
      kprintf("invalid token %x\n", be2nv32(*p));
      return NULL;
    }
  }

  return (fdt_prop) (p + 1);
}

// valid node names match the following regex:
// /[a-zA-Z][a-zA-Z0-9,._+\-]{,31}(@[0-9a-fA-F]+)?/
// 
// the part before the @ is the actual name and the part after is the unit
// address. returns the length of the name within the path (+ 1 for the slash)
// and 0 if invalid.
int fdt_name_iter(char *path) {
  // cant be empty
  if (!path || !*path)
    return 0;

  // first character must match /[a-zA-Z]/
  if (!('a' <= path[0] && path[0] <= 'z') &&
      !('A' <= path[0] && path[0] <= 'Z'))
    return 0;

  // validate the name part
#define MAX_NAME_LENGTH 31
  int length;
  for ( length = 1
      ; length < MAX_NAME_LENGTH && path[length] && path[length] != '/' &&
        path[length] != '@'
      ; length++) {
    if (!('a' <= path[length] && path[length] <= 'z') &&
        !('A' <= path[length] && path[length] <= 'Z') &&
        !('0' <= path[length] && path[length] <= '9') &&
        path[length] != '.' && path[length] != ',' &&
        path[length] != '_' && path[length] != '+' &&
        path[length] != '-')
      return 0;
  }

  // no address, at end of name
  if (!path[length] || path[length] == '/')
    return length + (path[length] == '/');

  // we should be validating the address now
  if (path[length++] != '@')
    return 0;
  for (; path[length] && path[length] != '/'; length++) {
    if (!('a' <= path[length] && path[length] <= 'f') &&
        !('A' <= path[length] && path[length] <= 'F') &&
        !('0' <= path[length] && path[length] <= '9'))
      return 0;
  }

#undef MAX_NAME_LENGTH
  return length + (path[length] == '/');
}

// valid path names consist of valid node names separated by forward slashes.
bool fdt_validate_path(char *path) {
  // cant be empty, has to start with '/'
  if (!path || !*path)
    return false;
  if (path[0] != '/')
    return false;
  path++;

  // root path valid
  if (!*path)
    return true;

  // check for valid names separated by slashes by iterating through each path
  // component
  for ( int length = fdt_name_iter(path)
      ; *path && length
      ; path += length
      , length = fdt_name_iter(path) );

  // valid iff we have reached the null terminator
  return !*path;
}

// return true iff the two names passed in are equal.
bool fdt_name_equal(char *n, char *m) {
  while ( !(!*n || *n == '/' || *n == '@') &&
          !(!*m || *m == '/' || *m == '@') ) {
    if (*n++ != *m++)
      return false;
  }

  // if both names have addresses then check the addresses of both
  if (*n == '@' && *m == '@') {
    ++n; ++m;
    while ( !(!*n || *n == '/') &&
            !(!*m || *m == '/') ) {
      if (*n++ != *m++)
        return false;
    }
  }

  // both names must have terminated (the right one must have its address
  // considered since its the path)
  return (!*n || *n == '/' || *n == '@') && (!*m || *m == '/');
}

// searches for a node by its path, starting from the passed in root node. if
// no root is provided then the device tree root node is used. returns an
// invalid node if cannot be found.
fdt_node fdt_node_path(devicetree tree, fdt_node root, char *path) {
  // validation and get root node
  if (!tree)
    return NULL;
  if (!fdt_validate_path(path++))
    return NULL;
  if (!fdt_node_valid(root))
    root = fdt_root_node(tree);
  if (!fdt_node_valid(root))
    return NULL;
  if (!*path)
    return root;

  // search layer by layer
  for ( fdt_node child = fdt_node_child_iter(root, NULL)
      ; fdt_node_valid(child) && *path
      ; child = fdt_node_child_iter(root, child) ) {
    if (!fdt_name_equal(fdt_node_name(child), path))
      continue;

    // pop a name from the path; path empty means we found the node we wanted
    path += fdt_name_iter(path);
    if (!*path)
      return child;

    root = child;
    child = NULL;
  }

  return NULL;
}

// iterates over the nodes with a given name in order of definition. returns an
// invalid node when there are no more nodes or when an invalid node exists.
// passing in NULL to the nodeparameter yields the first node, whereas
// passing in a node node passes in the next node. example usage:
//
//   for ( fdt_node node = fdt_node_search_iter(tree, NULL, "virtio_mmio")
//       ; fdt_node_valid(node)
//       ; node = fdt_node_search_iter(parent, node)) {
//     ... do stuff with child
//   }
//
// in the above example, if virtio_mmio@10001000 and virtio_mmio@10002000 both
// exist in the device tree, then both of those will be yielded by the iterator.
fdt_node fdt_node_search_iter(devicetree tree, fdt_node node, char *name) {
  // validation and get root node (assum e we arent searching for the root node)
  if (!tree)
    return NULL;
  if (!fdt_node_valid(node))
    node = fdt_root_node(tree);

  // we can use the preexisting node iteration functions
  for ( node = fdt_node_iter(node)
      ; fdt_node_valid(node)
      ; node = fdt_node_iter(node) ) {
    if (fdt_name_equal(fdt_node_name(node), name))
      return node;
  }

  return NULL;
}

// gets the property with the given name from the given node. returns an
// invalid property if cant be found.
fdt_prop fdt_node_prop(devicetree tree, fdt_node node, char *name) {
  if (!tree)
    return NULL;
  if (!fdt_node_valid(node))
    return NULL;

  for ( fdt_prop prop = fdt_prop_iter(node, NULL)
      ; fdt_prop_valid(prop)
      ; prop = fdt_prop_iter(node, prop) ) {
    if (streq(name, fdt_prop_name(tree, prop)))
      return prop;
  }

  return NULL;
}

// helper function to determine whether to print as a string list or ints.
bool fdt_prolly_string(void *p, size_t len) {
  char *s = (char *) p;
  bool all_zeroes = true;
  for (size_t i = 0; i < len; i++) {
    all_zeroes &= s[i] == 0;
    if (s[i] != 0 && !(32 <= s[i] && s[i] <= 127))
      return false;
  }

  return !all_zeroes;
}

// helper function for fdt_dump that does the actual recursive print.
void fdt_dump_helper(devicetree tree, fdt_node node, int indent) {
  // check if valid node
  if (!fdt_node_valid(node))
    node = fdt_root_node(tree);
  if (!fdt_node_valid(node))
    return;

  // name of node, start of block
  #define INDENT(indent) for (int i = 0; i < indent; i++) kprintf("    ")
  INDENT(indent);
  kprintf("%s/ {\n", fdt_node_name(node));

  // print properties
  fdt_prop prop = fdt_prop_iter(node, NULL);
  bool has_prop = prop != NULL;
  for (; fdt_prop_valid(prop) ; prop = fdt_prop_iter(node, prop)) {
    INDENT(indent + 1);
    kprintf("%s = ", fdt_prop_name(tree, prop));

    if (fdt_prolly_string((void *) (prop + 1), be2nv32(prop->len))) {
      kprintf("|");
      kputx(((void *) (prop + 1)), be2nv32(prop->len));
      kprintf("|\n");
    } else {
      kprintf("<");
      bool not_first = false;
      for (uint32_t i = 0; i < be2nv32(prop->len); i += 4) {
        if (not_first)
          kputc(' ');
        not_first = true;
        kprintf("[%x]", be2nv32(*(uint32_t *) ((uint8_t *) (prop + 1) + i)));
      }
      kprintf(">\n");
    }
  }

  // print children
  fdt_node child = fdt_node_child_iter(node, NULL);
  bool has_children = child != NULL;
  for (; fdt_node_valid(child) ; child = fdt_node_child_iter(node, child)) {
    if (has_prop && has_children)
      kprintf("\n");
    has_children = true;
    fdt_dump_helper(tree, child, indent + 1);
  }

  // yay we are done! close the block
  INDENT(indent);
  kprintf("}\n");
  #undef INDENT
}

// dumps the given device tree to the uart console.
void fdt_dump(devicetree tree) {
  if (!tree)
    return;
  fdt_dump_helper(tree, NULL, 0);
}

// TODO: things we want in our api:
// - actual memory reservations api
// - get the value of a node property
// - aliases
// - search by phandle for a node
