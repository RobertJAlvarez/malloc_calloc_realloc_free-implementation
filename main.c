#include <stdio.h>  //printf()
#include <stdlib.h>
#include <unistd.h>

#include "alloc.h"

#define SIZE (4200000)

void print_LL_info(void);

void testing(void);
void clear(void);
void ls(void);
void gcc(void);
void random_alloc(size_t n);
void infinite_alloc(void);
void zsh(void);

void check_calloc(void *ptr, size_t size);
void ten1d8malloc(void);

void check_calloc(void *ptr, size_t size) {
  // printf("\tENTERING CHECK_CALLOC\n");
  // printf("size = %zu\n", size);
  const unsigned char *p = (const unsigned char *)ptr;
  for (size_t i = ((size_t)0); i < size; i++) {
    if (*p != '\0') {
      printf("p[%zu] = 0x%x\n", i, *p);
    }
  }
  // printf("Check done\n");
}

int main(void) {
  testing();
  clear();
  ls();
  gcc();
  zsh();
  random_alloc(10000);

  return 0;
}

void random_alloc(size_t n) {
  printf("\tENTERING RANDOM_ALLOC:\n");

  void **ptr = (void **)malloc(n * sizeof(void *));
  size_t page = (size_t)getpagesize();
  size_t size[n];
  size_t count[(n / 5) + 1];

  size_t n_bytes;
  char c;
  char *p;

  printf("\tALLOCATING:\n");
  for (size_t i = ((size_t)0); i < n; i++) {
    // print call number if it is divisible by 1000
    if (i % (n / 20) == 0) {
      printf("i = %zd\n", i);
    }
    // Set the amount of memory to request
    size[i] = ((size_t)rand()) % page;
    // If the function call is divisible by 5 we use calloc
    if (i % 5 == 0) {
      count[i / 5] = ((size_t)rand() % 10);
      size[i] /= 5;
      ptr[i] = __calloc_impl(count[i / 5], size[i]);
      n_bytes = count[i / 5] * size[i];
    }
    // Otherwise we use malloc
    else {
      ptr[i] = __malloc_impl(size[i]);
      n_bytes = size[i];
    }
    // If the function call is divisible by 7, we call realloc
    if (i % 7 == 0) {
      // If we are on the second half, do realloc at the pointer minus half of
      // the list maximum number
      if (i > (n / 2)) {
        size[i - (n / 2)] = ((size_t)rand()) % page;
        ptr[i - (n / 2)] = __realloc_impl(ptr[i - (n / 2)], size[i]);
      }
      // If we are on the first half, do realloc at the pointer
      else {
        size[i] = ((size_t)rand()) % page;
        ptr[i] = __realloc_impl(ptr[i], size[i]);
        n_bytes = size[i];
      }
    }

    // Place random characters inside the recently allocated pointer
    c = ((char)((rand() % 96) + 32));
    p = ((char *)ptr[i]);
    while (n_bytes--) {
      *p++ = c;
      if (c >= ((char)127)) {
        c = ((char)32);
      } else {
        c++;
      }
    }
  }

  // printf("After allocating memory:\n");
  // print_LL_info();

  printf("\tFREEING:\n");
  for (size_t i = ((size_t)0); i < n; i++) {
    if (i % (n / 20) == 0) {
      printf("i = %zd\n", i);
    }
    __free_impl(ptr[i]);
  }

  printf("After freeing all the memory:\n");
  print_LL_info();

  free(ptr);

  printf("\tLEAVING RANDOM_ALLOC:\n");
}

void testing(void) {
  printf("\tENTERING TESTING\n");

  // TODO: Write here all the testing
  void *s1 = __malloc_impl((size_t)(getpagesize() / 2));
  void *s2 = __malloc_impl((size_t)(getpagesize() / 4));
  void *s3 = __malloc_impl((size_t)(getpagesize() / 2));
  __free_impl(s3);
  __free_impl(s2);
  __free_impl(s1);
  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  s1 = __malloc_impl((size_t)getpagesize());

  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  s1 = __realloc_impl(s1, (size_t)(3 * getpagesize() / 4));  // Less than before
  __free_impl(s1);
  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  s1 = __malloc_impl((size_t)getpagesize() / 2);
  printf("Next two print_LL_info should only have one thing in LL\n");
  print_LL_info();
  s1 = __realloc_impl(s1, (size_t)(getpagesize() / 4));  // Less than before
  __free_impl(s1);
  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();
  s1 = __calloc_impl(2, sizeof(int));
  check_calloc(s1, 2 * sizeof(int));
  __free_impl(s1);
  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  short *ptr = (short *)__calloc_impl(((size_t)5), sizeof(short));
  check_calloc((char *)ptr, 5 * sizeof(short));
  __free_impl(ptr);
  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  char *p = (char *)__calloc_impl(((size_t)SIZE), sizeof(char));
  check_calloc(p, SIZE * sizeof(char));
  __free_impl(p);
  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  printf("\tEXITING TESTING\n");
}

void clear(void) {
  // printf("0x%zx\n", ((size_t) getpagesize()));
  printf("\tENTERING CLEAR\n");

  void *p1 = __malloc_impl((size_t)0xf);
  void *p2 = __calloc_impl((size_t)0x1, (size_t)0x118);
  check_calloc(p2, 0x1 * 0x118);
  __free_impl(NULL);
  void *p3 = __malloc_impl((size_t)0x1c);
  void *p4 = __malloc_impl((size_t)0x10);
  __free_impl(NULL);
  void *p5 = __malloc_impl((size_t)0x61);
  void *p6 = __calloc_impl((size_t)0x6, (size_t)0x8);
  check_calloc(p6, 0x6 * 0x8);
  void *p7 = __calloc_impl((size_t)0x6, (size_t)0x80);
  check_calloc(p7, 0x6 * 0x80);
  __free_impl(p7);
  void *p8 = __malloc_impl((size_t)0x1d8);
  printf("On bash execution, I got a Bus error right here.\n");
  print_LL_info();
  void *p9 = __malloc_impl((size_t)0x1000);
  void *p10 = __malloc_impl((size_t)0x642);
  void *p11 = __calloc_impl((size_t)0x2c, (size_t)0x1);
  check_calloc(p11, 0x2c * 0x1);
  void *p12 = __calloc_impl((size_t)0x27, (size_t)0x4);
  check_calloc(p12, 0x27 * 0x4);
  void *p13 = __calloc_impl((size_t)0x19e, (size_t)0x8);
  check_calloc(p13, 0x19e * 0x8);

  printf("After allocating everything needed for main.\n");
  print_LL_info();

  __free_impl(p1);
  __free_impl(p2);
  __free_impl(p3);
  __free_impl(p4);
  __free_impl(p5);
  __free_impl(p6);
  __free_impl(p8);
  __free_impl(p9);
  __free_impl(p10);
  __free_impl(p11);
  __free_impl(p12);
  __free_impl(p13);

  printf("Next call to print_LL_info() should print nothing.\n");
  print_LL_info();

  printf("\tLEAVING CLEAR\n");
}

void ls(void) {
  printf("\tENTERING LS\n");

  void *p1 = __malloc_impl((size_t)0x1d8);
  void *p2 = __malloc_impl((size_t)0x78);
  void *p3 = __malloc_impl((size_t)0x400);
  __free_impl(p2);
  __free_impl(p3);
  __free_impl(p1);
  __free_impl(NULL);
  void *p4 = __malloc_impl((size_t)0x5);
  __free_impl(p4);
  void *p5 = __malloc_impl((size_t)0x78);
  void *p6 = __malloc_impl((size_t)0xc);
  void *p7 = __malloc_impl((size_t)0x308);
  void *p8 = __malloc_impl((size_t)0x70);
  void *p9 = __malloc_impl((size_t)0x538);
  void *p10 = __malloc_impl((size_t)0xd8);
  void *p11 = __malloc_impl((size_t)0x1b0);
  void *p12 = __malloc_impl((size_t)0x68);
  void *p13 = __malloc_impl((size_t)0x58);
  void *p14 = __malloc_impl((size_t)0x78);
  void *p15 = __malloc_impl((size_t)0xa8);
  void *p16 = __malloc_impl((size_t)0x68);
  void *p17 = __malloc_impl((size_t)0x50);
  void *p18 = __malloc_impl((size_t)0xc0);
  void *p19 = __malloc_impl((size_t)0xc);
  void *p20 = __malloc_impl((size_t)0xc);
  void *p21 = __malloc_impl((size_t)0xc);
  void *p22 = __malloc_impl((size_t)0xc);
  void *p23 = __malloc_impl((size_t)0xc);
  void *p24 = __malloc_impl((size_t)0xc);

  printf("After allocating memory:\n");
  print_LL_info();

  __free_impl(p5);
  __free_impl(p6);
  __free_impl(p7);
  __free_impl(p8);
  __free_impl(p9);
  __free_impl(p10);
  __free_impl(p11);
  __free_impl(p12);
  __free_impl(p13);
  __free_impl(p14);
  __free_impl(p15);
  __free_impl(p16);
  __free_impl(p17);
  __free_impl(p18);
  __free_impl(p19);
  __free_impl(p20);
  __free_impl(p21);
  __free_impl(p22);
  __free_impl(p23);
  __free_impl(p24);

  printf("After freeing all the memory:\n");
  print_LL_info();

  printf("\tEXITING LS\n");
}

void gcc(void) {
  printf("\tENTERING GCC\n");
  void *p1 = __malloc_impl((size_t)0x30);
  void *p2 = __calloc_impl((size_t)0xd, (size_t)0x10);
  check_calloc(p2, 0xd * 0x10);
  void *p3 = __malloc_impl((size_t)0x30);
  void *p4 = __calloc_impl((size_t)0xd, (size_t)0x18);
  check_calloc(p2, 0xd * 0x18);
  void *p5 = __malloc_impl((size_t)0x30);
  void *p6 = __calloc_impl((size_t)0xd, (size_t)0x18);
  check_calloc(p2, 0xd * 0x18);
  void *p7 = __malloc_impl((size_t)0x16);
  void *p8 = __malloc_impl((size_t)0x16);
  void *p9 = __malloc_impl((size_t)0x16);
  void *p10 = __malloc_impl((size_t)0x16);
  void *p11 = __malloc_impl((size_t)0x16);
  void *p12 = __malloc_impl((size_t)0x16);
  void *p13 = __malloc_impl((size_t)0x11c00);
  void *p14 = __malloc_impl((size_t)0x10000);
  void *p15 = __malloc_impl((size_t)0x280);
  __free_impl(p15);
  void *p16 = __malloc_impl((size_t)0x2c0);
  void *p17 = __malloc_impl((size_t)0x268);
  __free_impl(p16);
  p17 = __realloc_impl(p17, (size_t)0x268);
  void *p18 = __malloc_impl((size_t)0x5);
  __free_impl(p18);
  void *p19 = __malloc_impl((size_t)0x78);
  void *p20 = __malloc_impl((size_t)0xc);
  void *p21 = __malloc_impl((size_t)0x308);
  void *p22 = __malloc_impl((size_t)0x70);
  void *p23 = __malloc_impl((size_t)0x538);
  void *p24 = __malloc_impl((size_t)0xd8);
  void *p25 = __malloc_impl((size_t)0x1b0);
  void *p26 = __malloc_impl((size_t)0x68);
  void *p27 = __malloc_impl((size_t)0x58);
  void *p28 = __malloc_impl((size_t)0x78);
  void *p29 = __malloc_impl((size_t)0xa8);
  void *p30 = __malloc_impl((size_t)0x68);
  void *p31 = __malloc_impl((size_t)0x50);
  void *p32 = __malloc_impl((size_t)0xc0);
  void *p33 = __malloc_impl((size_t)0xc);
  void *p34 = __malloc_impl((size_t)0xab);
  __free_impl(NULL);
  void *p35 = __malloc_impl((size_t)0xc);
  void *p36 = __malloc_impl((size_t)0xb5);
  __free_impl(p34);
  __free_impl(NULL);
  void *p37 = __malloc_impl((size_t)0x1f);
  void *p38 = __malloc_impl((size_t)0x7);
  void *p39 = __malloc_impl((size_t)0x34);
  __free_impl(p39);
  void *p40 = __malloc_impl((size_t)0x1d8);

  // BUZZ ERROR

  printf("After allocating memory:\n");
  print_LL_info();

  // ten1d8malloc();

  __free_impl(p1);
  __free_impl(p2);
  __free_impl(p3);
  __free_impl(p4);
  __free_impl(p5);
  __free_impl(p6);
  __free_impl(p7);
  __free_impl(p8);
  __free_impl(p9);
  __free_impl(p10);
  __free_impl(p11);
  __free_impl(p12);
  __free_impl(p13);
  __free_impl(p14);
  __free_impl(p17);
  __free_impl(p19);
  __free_impl(p20);
  __free_impl(p21);
  __free_impl(p22);
  __free_impl(p23);
  __free_impl(p24);
  __free_impl(p25);
  __free_impl(p26);
  __free_impl(p27);
  __free_impl(p28);
  __free_impl(p29);
  __free_impl(p30);
  __free_impl(p31);
  __free_impl(p32);
  __free_impl(p33);
  __free_impl(p35);
  __free_impl(p36);
  __free_impl(p37);
  __free_impl(p38);
  __free_impl(p40);

  printf("After freeing all the memory:\n");
  print_LL_info();

  printf("\tEXITING GCC\n");
}

void ten1d8malloc(void) {
  void *p1 = __malloc_impl((size_t)0x1d8);
  void *p2 = __malloc_impl((size_t)0x1d8);
  void *p3 = __malloc_impl((size_t)0x1d8);
  void *p4 = __malloc_impl((size_t)0x1d8);
  void *p5 = __malloc_impl((size_t)0x1d8);
  void *p6 = __malloc_impl((size_t)0x1d8);
  void *p7 = __malloc_impl((size_t)0x1d8);
  void *p8 = __malloc_impl((size_t)0x1d8);
  void *p9 = __malloc_impl((size_t)0x1d8);

  printf("After allocating memory:\n");
  print_LL_info();

  __free_impl(p1);
  __free_impl(p2);
  __free_impl(p3);
  __free_impl(p4);
  __free_impl(p5);
  __free_impl(p6);
  __free_impl(p7);
  __free_impl(p8);
  __free_impl(p9);

  printf("After freeing all the memory:\n");
  print_LL_info();
}

void zsh(void) {
  printf("\tENTERING ZSH\n");
  void *t1 = __malloc_impl((size_t)0x5);

  printf("After allocating memory:\n");
  print_LL_info();

  __free_impl(t1);

  printf("After freeing all the memory:\n");
  print_LL_info();

  void *t2 = __malloc_impl((size_t)0x78);
  void *t3 = __malloc_impl((size_t)0xc);
  void *t4 = __malloc_impl((size_t)0x308);
  void *t5 = __malloc_impl((size_t)0x70);
  void *t6 = __malloc_impl((size_t)0x538);
  void *t7 = __malloc_impl((size_t)0xd8);
  void *t8 = __malloc_impl((size_t)0x1b0);
  void *t9 = __malloc_impl((size_t)0x68);
  void *t10 = __malloc_impl((size_t)0x58);
  void *t11 = __malloc_impl((size_t)0x78);
  void *t12 = __malloc_impl((size_t)0xa8);
  void *t13 = __malloc_impl((size_t)0x68);
  void *t14 = __malloc_impl((size_t)0x50);
  void *t15 = __malloc_impl((size_t)0xc0);
  void *t16 = __malloc_impl((size_t)0xc);
  void *t17 = __malloc_impl((size_t)0xc);
  void *t18 = __malloc_impl((size_t)0xc);
  void *t19 = __malloc_impl((size_t)0xc);
  void *t20 = __malloc_impl((size_t)0xc);
  void *t21 = __malloc_impl((size_t)0xc);
  void *t22 = __malloc_impl((size_t)0xc);
  void *t23 = __malloc_impl((size_t)0xc);
  void *t24 = __malloc_impl((size_t)0xc);
  void *t25 = __malloc_impl((size_t)0xc);
  void *t26 = __malloc_impl((size_t)0xc);
  void *t27 = __malloc_impl((size_t)0xc);
  void *t28 = __malloc_impl((size_t)0xc);

  printf("After allocating memory:\n");
  print_LL_info();

  __free_impl(t2);
  __free_impl(t3);
  __free_impl(t4);
  __free_impl(t5);
  __free_impl(t6);
  __free_impl(t7);
  __free_impl(t8);
  __free_impl(t9);
  __free_impl(t10);
  __free_impl(t11);
  __free_impl(t12);
  __free_impl(t13);
  __free_impl(t14);
  __free_impl(t15);
  __free_impl(t16);
  __free_impl(t17);
  __free_impl(t18);
  __free_impl(t19);
  __free_impl(t20);
  __free_impl(t21);
  __free_impl(t22);
  __free_impl(t23);
  __free_impl(t24);
  __free_impl(t25);
  __free_impl(t26);
  __free_impl(t27);
  __free_impl(t28);

  printf("After freeing all the memory:\n");
  print_LL_info();

  void *t29 = __malloc_impl((size_t)0x35);
  void *t30 = __malloc_impl((size_t)0xf);
  void *t31 = __malloc_impl((size_t)0x50);
  void *t32 = __malloc_impl((size_t)0x10);

  __free_impl(NULL);

  void *t33 = __malloc_impl((size_t)0x1f);
  void *t34 = __malloc_impl((size_t)0x4);
  void *t35 = __malloc_impl((size_t)0x50);
  void *t36 = __malloc_impl((size_t)0xd);

  __free_impl(NULL);

  void *t37 = __malloc_impl((size_t)0x11);
  void *t38 = __malloc_impl((size_t)0x2);
  void *t39 = __malloc_impl((size_t)0x50);
  void *t40 = __malloc_impl((size_t)0x1c);

  __free_impl(NULL);

  void *t41 = __malloc_impl((size_t)0x1e);
  void *t42 = __malloc_impl((size_t)0x3a);
  void *t43 = __malloc_impl((size_t)0x50);

  printf("After allocating memory:\n");
  print_LL_info();

  __free_impl(t29);
  __free_impl(t30);
  __free_impl(t31);
  __free_impl(t32);

  __free_impl(t33);
  __free_impl(t34);
  __free_impl(t35);
  __free_impl(t36);

  __free_impl(t37);
  __free_impl(t38);
  __free_impl(t39);
  __free_impl(t40);

  __free_impl(t41);
  __free_impl(t42);
  __free_impl(t43);
  __free_impl(NULL);

  printf("After freeing all the memory:\n");
  print_LL_info();

  printf("\tEXITING ZSH\n");
}
