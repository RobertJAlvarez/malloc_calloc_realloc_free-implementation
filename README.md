# malloc() calloc() realloc() and free() implementation using mmap() OS call

Logic for the implementation is at `alloc.c`. Only the mmap() and munmap() operating
systems calls where use for this implementation.

The structs use are the following:
```c
typedef struct s_Block {
  size_t bytes_used;
  Block *next_block;
} Block;

typedef struct s_Map {
  Block *first_block;
  Block *largest_block;
  Map *next_map;
} Map;

typedef struct s_List {
  Map *first_map;
  Map *largest_map;
} List;
```

The way memory was structure was by a linked list of Maps where each Map had a linked
list of Blocks. If the request was above a PAGE, an individual mmap() was made with
the amount of bytes requested. If the request was below a PAGE, a piece of memory from
the largest block was given to the uer and the *largest_map List filed was updated as
needed.

No unmmap() was made until all the chunks of memory from a single mmap() PAGE was
return by the user, and block were free they were added to the linked list of the beloning
Map and merge with block that were already there if possible.
