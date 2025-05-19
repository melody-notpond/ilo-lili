#ifndef DEVICETREE_H
#define DEVICETREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// a devicetree memory reserve entry. memory writes from `address` to
// `address+size` are invalid and may not occur.
struct fdt_reserved {
    void* address;
    size_t size;
};

// a valid devicetree object. do not dereference or modify values of this type.
typedef const struct fdt_header *const devicetree;

// a devicetree node handle. do not modify values of this type.
typedef const struct fdt_node *fdt_node;

typedef const struct fdt_prop *fdt_prop;

// validates a device tree. on failure, returns NULL. otherwise, returns a valid
// devicetree value.
devicetree fdt_validate(void *p);

// returns the number of memory reservation entries the device tree has.
int fdt_count_mem_reserve_entries(devicetree tree);

// returns a reference to the root node. this must be valid if the device tree
// is correct.
fdt_node fdt_root_node(devicetree tree);

// checks if an fdt_node handle is valid, returns true iff valid.
bool fdt_node_valid(fdt_node node);

// gets the name of an fdt node.
char *fdt_node_name(fdt_node node);

// iterates over nodes depth first in order of definition. returns an invalid
// node when there are no more nodes or when an invalid node exists. example
// usage:
// 
//   for ( fdt_node node = fdt_root_node(tree)
//       ; fdt_node_valid(node)
//       ; node = fdt_node_iter(node)) {
//     ... do stuff with node
//   }
fdt_node fdt_node_iter(fdt_node node);

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
//     ... do stuff with child
//   }
fdt_node fdt_node_child_iter(fdt_node parent, fdt_node child);

// checks if an fdt_prop handle is valid, returns true iff valid.
bool fdt_prop_valid(fdt_prop prop);

// gets the name of an fdt prop. tree must be the device tree from which prop
// was obtained.
char *fdt_prop_name(devicetree tree, fdt_prop prop);

// iterates over the properties of a node in order of definition. returns an
// invalid property when there are no more nodes or when an invalid token
// exists. passing in NULL to the prop parameter yields the first property,
// whereas passing in a valid property passes in the next child.
// example usage:
// 
//   
//   for ( fdt_prop prop = fdt_prop_iter(node, NULL)
//       ; fdt_prop_valid(prop)
//       ; prop = fdt_prop_iter(node, prop)) {
//     ... do stuff with prop
//   }
fdt_prop fdt_prop_iter(fdt_node node, fdt_prop prop);

#endif /* DEVICETREE_H */
