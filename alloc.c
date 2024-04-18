#include <stddef.h>
#include <stdio.h>     //printf()
#include <sys/mman.h>  //mmap(), munmap()
#include <unistd.h>    //getpagesize(void)

#include "alloc.h"
#include "memory.h"

void print_LL_info(void);

// BEGINNING OF STRUCTs

typedef struct s_Block {
  size_t bytes_used;
  struct s_Block *next_block;
} Block;

typedef struct s_Map {
  Block *first_block;
  Block *largest_block;
  struct s_Map *next_map;
} Map;

typedef struct {
  Map *first_map;
  Map *largest_map;
} List;

// END OF STRUCTs

// BEGINNING OF GLOBAL VARIABLES DECLARATION

static size_t PAGE = 0;         // Define the size of a page
static List LL = {NULL, NULL};  // Start a list to save all the allocation
                                // spaces to use in malloc, realloc, and calloc

// END OF GLOBAL VARIABLES DECLARATION

/* Use mmap to get a page of memory, use the beginning for the Map object and
 * the rest for a block. This function set all the attributes of the Map, block
 * and add the map to the LL. BUT, the largest_map  pointer for LL are not
 * modified.
 */
static Map *__make_map(void) {
  // Allocate a page of memory for a Map and the rest to Block
  void *ptr = mmap(NULL, PAGE, PROT_WRITE | PROT_READ,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // Make sure that mmap was successful
  if (ptr == MAP_FAILED) return NULL;

  Map *map = (Map *)ptr;

  // Make an Block object right after the Map struct
  Block *block = (Block *)(((void *)map) + sizeof(Map));

  // Set Map attributes
  map->first_block = block;
  map->largest_block = block;
  map->next_map = NULL;

  // Set Block attributes
  block->bytes_used = PAGE - sizeof(Map);
  block->next_block = NULL;

  return map;
}

/* Initialize the linked list to keep the history. */
static int __init_LL(void) {
  // define the size of a page if it isn't already
  if (PAGE == 0) PAGE = (size_t)getpagesize();

  // This function should had not being called because LL was already
  // initialized
  if (LL.first_map != NULL) return 0;

  // This function is never called, unless user is requesting n_bytes that is
  // less than a PAGE+sizeof(Map)
  Map *map = __make_map();

  // Add map to the LL
  LL.first_map = map;
  LL.largest_map = map;

  // LL.first_map would now be pointing to something that it is not null if
  // __make_map was successful
  if (LL.first_map == NULL) return -1;

  return 0;
}

/* Given the map where the block belong to, we add it and marge the block with
 * any other if possible. */
static int __add_block_to_map(Map *map, Block *new_block) {
  Block *curr_block = map->first_block;

  // If there are no blocks in the map
  if (curr_block == NULL) {
    map->first_block = new_block;
    map->largest_block = new_block;
    return 0;
  }

  if (map->largest_block == NULL) {
    map->largest_block = new_block;
  }

  // Check that the block is not already inside the first block
  if ((((void *)new_block) >= ((void *)curr_block)) &&
      (((void *)new_block) < ((void *)curr_block) + curr_block->bytes_used)) {
    return -1;
  }

  // If the new block is before the first one on the map
  if (((void *)new_block) < ((void *)curr_block)) {
    // Update pointers so blocks are organize
    new_block->next_block = curr_block;
    map->first_block = new_block;
    // See if we can merge
    if (((void *)new_block) + new_block->bytes_used == ((void *)curr_block)) {
      new_block->next_block = curr_block->next_block;
      new_block->bytes_used += curr_block->bytes_used;
    }
    // Update pointer to largest block, if needed
    if (new_block->bytes_used > map->largest_block->bytes_used) {
      map->largest_block = new_block;
    }
    return 0;
  }

  /* new_block is not the first one of the map so we try to find where to add it
   */
  // make curr_block the block before new_block
  while ((curr_block->next_block != NULL) &&
         (((void *)new_block) >= ((void *)curr_block->next_block))) {
    curr_block = curr_block->next_block;
  }

  Block *block_after = curr_block->next_block;

  // curr_block is >= than new_block, check that new_block is not inside
  // curr_block
  if (((void *)new_block) < (((void *)curr_block) + curr_block->bytes_used))
    return -1;

  // Update the pointer so the blocks are organize
  new_block->next_block = block_after;
  curr_block->next_block = new_block;

  // Try to merge new_block and block_after
  if ((block_after != NULL) &&
      (((void *)new_block) + new_block->bytes_used == ((void *)block_after))) {
    new_block->next_block = block_after->next_block;
    new_block->bytes_used += block_after->bytes_used;
  }
  // Check is new_block have more space than largest block
  if (new_block->bytes_used > map->largest_block->bytes_used)
    map->largest_block = new_block;

  // Try to merge curr_block and new_block
  if (((void *)curr_block) + curr_block->bytes_used == ((void *)new_block)) {
    curr_block->next_block = new_block->next_block;
    curr_block->bytes_used += new_block->bytes_used;
    // Check is new_block have more space than largest block
    if (curr_block->bytes_used > map->largest_block->bytes_used) {
      map->largest_block = curr_block;
    }
  }
  return 0;
}

/* Iterate over all Maps in LL and we update its largest_map pointer */
static void __update_largest_map(void) {
  Map *map = LL.first_map;
  Block *block;
  size_t largest_size = 0;

  while (map != NULL) {
    block = map->first_block;
    if (block != NULL) {
      if (block->bytes_used > largest_size) {
        LL.largest_map = map;
        largest_size = block->bytes_used;
      }
    }
    map = map->next_map;
  }
}

/* Given a block pointer we add it to the respective map using
 * __add_block_to_map(), if the map got everything back we munmap it and remove
 * it from LL.
 */
static void __add_block(Block *block) {
  // The next block pointer is not meaningful so we set it to null
  block->next_block = NULL;

  Map *curr_map = LL.first_map;
  Map *prev_map = NULL;

  // Find curr_map which is where block belongs to, and prev_map
  while (((void *)block) > (((void *)curr_map) + PAGE)) {
    prev_map = curr_map;
    curr_map = curr_map->next_map;
  }

  // At this point we know that block is inside curr_map, so we need to added it
  // to its list of blocks
  if (__add_block_to_map(curr_map, block) < 0) return;

  Block *temp_block = LL.largest_map->largest_block;

  // If everything that belongs to curr_map has been free, we munmap it
  if (curr_map->first_block->bytes_used == PAGE - sizeof(Map)) {
    // Find if the largest_map is the curr_map
    char curr_is_largest = (curr_map == LL.largest_map ? 1 : 0);
    char curr_is_first = (curr_map == LL.first_map ? 1 : 0);
    char last_map = (curr_is_largest && curr_is_first &&
                     (curr_map->next_map == NULL ? 1 : 0));

    /* Remove map from list */
    // If map was the first one, update list pointers
    if (curr_is_first) {
      LL.first_map = curr_map->next_map;
    }
    // If map was not the first one, update map pointers
    else {
      prev_map->next_map = curr_map->next_map;
    }

    if (munmap(curr_map, PAGE) != 0) {
      // If munmap fail, put curr_map back into LL and because LL have
      // everything free there must not be another Map with more than it
      LL.largest_map = curr_map;
      // Update list by returning pointers as they were
      if (curr_is_first) {
        LL.first_map = curr_map;
      } else {
        prev_map->next_map = curr_map;
      }
      return;
    }

    if (last_map) {
      // LL.first_map is already NULL because of line 271 where we say
      // LL.first_map = curr_map->next_map
      LL.largest_map = NULL;
    }
    // Update LL if the map we munmap was the largest map
    else if (curr_is_largest) {
      __update_largest_map();
    }
  }
  // If by adding the new block we make the map to have a bigger block than what
  // the largest_map have inside the LL
  else if ((temp_block == NULL) ||
           (curr_map->largest_block->bytes_used > temp_block->bytes_used)) {
    LL.largest_map = curr_map;
  }
}

static int __add_map_to_LL(Map *map) {
  if (LL.first_map == NULL) {
    if (__init_LL() != 0) return 1;
  }

  // If map is before the first_map base in memory address
  if (LL.first_map > map) {
    map->next_map = LL.first_map;
    LL.first_map = map;
  } else {
    Map *prev_map = LL.first_map;
    // While we have more maps and the next map is before than the new map in
    // memory
    while ((prev_map->next_map != NULL) && (prev_map->next_map < map)) {
      prev_map = prev_map->next_map;
    }

    map->next_map = prev_map->next_map;
    prev_map->next_map = map;
  }

  return 0;
}

/* We iterate over all the blocks on the map to update the largest_block
 * pointer. */
static void __update_largest_block(Map *map) {
  map->largest_block = NULL;
  Block *block = map->first_block;
  size_t largest_size = 0;

  while (block != NULL) {
    if (block->bytes_used > largest_size) {
      largest_size = block->bytes_used;
      map->largest_block = block;
    }
    block = block->next_block;
  }
}

/* We get size bytes from a block in any map, we first look into the largest_map
 * in LL, if we don't have enough, we update it by calling
 * __update_largest_map() and check with that one. If the new largest map have
 * enough, we use its block, otherwise we call __make_map(), get size from it
 * and update LL if the remaining is more than the largest_map().
 */
static void *__get_allocation(size_t size) {
  Map *map = LL.largest_map;
  Block *block = map->largest_block;
  Block *temp_block;

  if ((block == NULL) || (block->bytes_used < size)) {
    // It could be that the LL is not updated because of a lot of
    // __get_allocation had happen and not enough free() had been done to keep
    // it up to date
    __update_largest_map();

    map = LL.largest_map;
    block = map->largest_block;
  }

  // If the largest block doesn't exist is because all memory had been use so we
  // need to make more OR if the largest block don't have enough bytes
  if ((block == NULL) || (block->bytes_used < size)) {
    map = __make_map();
    if (map == NULL) return NULL;
    if (__add_map_to_LL(map) != 0) return NULL;

    // block is at the beginning of the available memory to be use
    temp_block = map->first_block;
    /* Last bytes of block ends at the end of the map bytes range */
    // Check that we can have a block after return size bytes
    if (size + sizeof(Block) >= temp_block->bytes_used) {
      block = temp_block;
      map->first_block = NULL;
      map->largest_block = NULL;
    } else {
      block = (Block *)(((void *)temp_block) - size + temp_block->bytes_used);
      // update blocks attributes
      block->bytes_used = size;
      temp_block->bytes_used -= size;
      // update LL attributes, if needed
      Block *t_b = LL.largest_map->largest_block;
      if ((t_b == NULL) || (t_b->bytes_used < temp_block->bytes_used)) {
        LL.largest_map = map;
      }
    }
  }
  // We know that the largest block have enough bytes to hold size
  else {
    // If we don't have enough space to get the size and make a block we gave
    // everything back
    if (block->bytes_used < size + sizeof(Block)) {
      /* find the block that is pointing to the largest block so we can
       * eliminate the pointer */
      // If the first block is the largest_block
      if (map->first_block == map->largest_block) {
        map->first_block = block->next_block;
        ;
        __update_largest_block(map);
      } else {
        // We know that the largest_block is not the first block so we find the
        // block that is pointing to it
        temp_block = map->first_block;
        // Set the new largest_block to be the first_block and take advantage of
        // this iteration to update the block information
        map->largest_block = temp_block;
        size_t largest = temp_block->bytes_used;
        while (temp_block->next_block != block) {
          temp_block = temp_block->next_block;
          if (temp_block->bytes_used > largest) {
            map->largest_block = temp_block;
            largest = temp_block->bytes_used;
          }
        }
        // temp_block is pointing to block so we now make it to point to what
        // block was pointing to
        temp_block->next_block = block->next_block;
        temp_block = temp_block->next_block;
        // Finish updating the map for the largest_block
        while (temp_block != NULL) {
          if (temp_block->bytes_used > largest) {
            map->largest_block = temp_block;
            largest = temp_block->bytes_used;
          }
          temp_block = temp_block->next_block;
        }
      }
    }
    // Otherwise, we only get what its asked for and make a new Block in map
    else {
      // temp_block and block point to the same place
      temp_block = block;
      // block finish at the last byte available
      block = (Block *)(((void *)block) - size + block->bytes_used);
      // update blocks attributes
      temp_block->bytes_used = temp_block->bytes_used - size;
      block->bytes_used = size;
      // update largest_block
      __update_largest_block(map);
    }
  }

  return ((void *)block) + sizeof(size_t);
}

/* If size is zero, return NULL. Otherwise, call __get_allocation with size. */
void *__malloc_impl(size_t req_size) {
  if (req_size == ((size_t)0)) return NULL;

  // define the size of a page
  if (PAGE == 0) PAGE = (size_t)getpagesize();

  size_t need_size;

  // Always give at least sizeof(Block *) so we don't get side effects
  if (req_size <= sizeof(Block *)) {
    need_size = sizeof(Block);
  }
  // Check for overflow
  else {
    // Always work on even memory addresses
    need_size = req_size + sizeof(size_t) + (req_size % 2);

    // Check if the summation overflow
    if (need_size < req_size) {
      return NULL;
    }

    // Check if the required size overflows and if it is more than a PAGE
    req_size = need_size + sizeof(Map) + sizeof(Block);
    if (req_size < need_size) {
      return NULL;
    }

    // If we can't make a Map a Block with the size requested and still have
    // space left, we make a pointer only for this request
    if (req_size >= PAGE) {
      // Get the minimum amount of space that is still greater or equal to PAGE
      if (need_size >= PAGE) {
        req_size = need_size;
      }
      void *ptr = mmap(NULL, req_size, PROT_WRITE | PROT_READ,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      if (ptr == MAP_FAILED) {
        return NULL;
      }
      size_t *temp = ((size_t *)ptr);
      *temp = req_size;
      return (void *)&temp[1];
    }
  }

  if (LL.first_map == NULL) {
    if (__init_LL() < 0) {
      return NULL;
    }
  }

  return __get_allocation(need_size);
}

/* If size is less than what already assign to *ptr just lock what is after size
 * and add it using add_block. */
void *__realloc_impl(void *ptr, size_t req_size) {
  // If ptr is NULL, realloc() is identical to a call to malloc() for size
  // bytes.
  if (ptr == NULL) return __malloc_impl(req_size);

  // If size is 0, we free the ptr and return NULL
  if (req_size == ((size_t)0)) {
    __free_impl(ptr);
    return NULL;
  }

  size_t need_size;

  if (req_size <= sizeof(Block *)) {
    need_size = sizeof(Block);
  } else {
    need_size = req_size + sizeof(size_t) + (req_size % 2);
    // Check if the summation overflow
    if (need_size < req_size) {
      return NULL;
    }
  }

  if (LL.first_map == NULL) {
    if (__init_LL() < 0) {
      return NULL;
    }
  }

  Block *curr_block = (Block *)(((void *)ptr) - sizeof(size_t));

  // If the new size is less than before but not enough to make an Block object
  if ((curr_block->bytes_used >= need_size) &&
      (curr_block->bytes_used < (need_size + sizeof(Block)))) {
    return ptr;
  }
  // If the new size is less than before and we can create an Block element to
  // add to LL
  else if (curr_block->bytes_used > need_size) {
    // If what is going to be modify is more than a page we need to individually
    // unmap it because that's how we map it
    if (curr_block->bytes_used >= PAGE) {
      // Save where to start coping from
      const unsigned char *old_ptr = (const unsigned char *)ptr;
      // Get new space to copy to
      req_size = need_size + sizeof(Map);
      if (req_size < need_size) {
        return NULL;
      }
      if (req_size >= PAGE) {
        ptr = mmap(NULL, req_size, PROT_WRITE | PROT_READ,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
          return NULL;
        }
        // Set the bytes_used space and make ptr to point after the size_t
        // bytes_used amount
        size_t *t = ((size_t *)ptr);
        *t = req_size;
        ptr = (void *)&t[1];
      } else {
        ptr = __get_allocation(need_size);
      }
      // We couldn't get enough space
      if (ptr == NULL) {
        return NULL;
      }
      // Copy first need_size characters from curr_block to temp
      size_t t_size =
          ((Block *)(ptr - sizeof(size_t)))->bytes_used - sizeof(size_t);
      for (unsigned char *t = (unsigned char *)ptr; t_size--; t++, old_ptr++) {
        *t = *old_ptr;
      }
      munmap(curr_block, curr_block->bytes_used);
    } else {
      // Save what is left in temp and add it to the LL
      Block *temp_block = (Block *)(((void *)curr_block) + need_size);
      temp_block->bytes_used = curr_block->bytes_used - need_size;
      __add_block(temp_block);
      // Update bytes_used space
      curr_block->bytes_used = need_size;
    }
  }
  // If we are asking for more than what we have in curr_block
  else {
    // Save where to start copying from
    const unsigned char *old_ptr = (const unsigned char *)ptr;
    // Get new space to copy to
    req_size = need_size + sizeof(Map);
    if (req_size < need_size) {
      return NULL;
    }
    if (req_size >= PAGE) {
      ptr = mmap(NULL, req_size, PROT_WRITE | PROT_READ,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      if (ptr == MAP_FAILED) {
        return NULL;
      }
      size_t *t = ((size_t *)ptr);
      *t = req_size;
      ptr = (void *)&t[1];
    } else {
      ptr = __get_allocation(need_size);
    }
    // We couldn't get enough space
    if (ptr == NULL) {
      return NULL;
    }
    // Copy space where to copy to, so we can iterate without losing the
    // beginning
    unsigned char *t = (unsigned char *)ptr;
    // Copy all characters from curr_block to temp
    for (size_t i = ((size_t)sizeof(size_t)); i < curr_block->bytes_used; i++) {
      *t++ = *old_ptr++;
    }
    // Free curr_block, either by munmap() or __add_block()
    if (curr_block->bytes_used >= PAGE) {
      munmap(curr_block, curr_block->bytes_used);
    } else {
      __add_block(curr_block);
    }
  }

  return ptr;
}

/* Check if n1 * n2 can be hold in a size_t type. If so, return 1 and store the
 * value in c. Otherwise, return 0.
 */
static int __try_size_t_multiply(size_t *ans, size_t n1, size_t n2) {
  size_t res, quot;  // res = residual, quot = quotient

  // If any of the arguments a and b is zero, everything works just fine.
  if ((n1 == ((size_t)0)) || (n2 == ((size_t)0))) {
    *ans = ((size_t)0);
    return 1;
  }

  // Neither a nor b is zero
  *ans = n1 * n2;

  // Check that ans is a multiple of n1. Therefore, ans = n1 * quot + res where
  // the residual must be 0
  quot = *ans / n1;
  res = *ans % n1;
  if (res != ((size_t)0)) return 0;

  // Here we know that ans is a multiple of n1, and the quotient must be n2
  if (quot != n2) return 0;

  // Multiplication did not overflow :)
  return 1;
}

/* Call malloc and iterate over the point to change everything to '0's. */
void *__calloc_impl(size_t count, size_t size) {
  size_t n_bytes;
  if (__try_size_t_multiply(&n_bytes, count, size) == 0) return NULL;

  // We either have count or size equal to 0
  if (n_bytes == 0) return NULL;

  void *ptr = __malloc_impl(n_bytes);

  // No memory could be allocated
  if (ptr == NULL) return NULL;

  // Initialize all characters in calloc to '\0';
  size_t *a = (size_t *)(((void *)ptr) - sizeof(size_t));
  n_bytes = *a - sizeof(size_t);

  unsigned char *t = ((unsigned char *)ptr);
  const unsigned char c = ((const unsigned char)'\0');

  while (n_bytes--) *t++ = c;

  return ptr;
}

/* Add space back to List using add_block */
void __free_impl(void *ptr) {
  if (ptr == NULL) return;

  // Adjust ptr by sizeof(size_t) to start where the number of bytes used are
  // allocated for ptr
  Block *temp = (Block *)(((void *)ptr) - sizeof(size_t));

  // define the size of a page
  if (PAGE == 0) PAGE = (size_t)getpagesize();

  // If we allocate this map by itself, we unmap it right away because it
  // doesn't belong to any Map
  if (temp->bytes_used >= PAGE) {
    munmap(temp, temp->bytes_used);
    return;
  }

  // Adjust ptr so size_t before to start on the size of pointer
  __add_block(temp);
}

void print_LL_info(void) {
  printf("\nInside print_LL_info()\n");
  printf("LL.largest_map = %p\n", LL.largest_map);

  Map *m = LL.first_map;
  Block *b;
  size_t tot;
  while (m != NULL) {
    printf("map = %p\n", m);
    b = m->first_block;
    tot = ((size_t)0);
    while (b != NULL) {
      printf("block = %p\n", b);
      printf("block->bytes_used = 0x%zx, block->next_block = %p\n",
             b->bytes_used, b->next_block);
      tot += b->bytes_used;
      if (b == b->next_block) {
        printf("\nERROR: b == b->next_block, b = %p\n\n", b);
        break;
      }
      b = b->next_block;
    }
    printf("map->largest_block = %p\n", m->largest_block);
    printf("Total bytes available = 0x%zx\n", tot);
    if (m == m->next_map) {
      printf("\nERROR: m == m->next_map, m = %p\n\n", m);
      break;
    }
    m = m->next_map;
  }
  printf("(End of function) Exiting print_LL_info()\n\n");
}
