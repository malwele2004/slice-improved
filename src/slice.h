#ifndef SLICE_H
#define SLICE_H

/**
 * @author: Malwele
 * @purpose: Store a contiguous list of elements in a dynamic list, minimally
 * @date: 29 jan, 2025
 *
 * */

typedef struct {
        int (*elemDeleter)(unsigned long, void *);
        unsigned long len, cap, esize;
} SliceInfo;

void *slice__public_new(unsigned long esize, unsigned long cap,
                        int (*elemDeleter)(unsigned long, void *));
#define slice__new(T, cap) slice__public_new(sizeof(T), cap, NULL)
#define slice__new_with_deleter(T, cap, deleter)                               \
  slice__public_new(sizeof(T), cap, deleter)

void slice__delete(void *sliceDataOnly);

void *slice__push(void *sliceDataOnly, void *elemAddr);

void *slice__pop(void *sliceDataOnly, int isShrink);

void slice__debug(void *sliceDataOnly);

// returns number of callback that returned 1
unsigned long
slice__forEach(void *sliceDataOnly,
               int (*callback)(unsigned long, void *), // 1 -> true, 0 -> false
               int useProvidedRange, unsigned long start, 
               unsigned long n, int exitOnTrue, int exitOnFalse);

int slice__every(void *sliceDataOnly, int (*callback)(unsigned long, void *),
                 int useProvidedRange, unsigned long start, unsigned long n);

int slice__any(void *sliceDataOnly, int (*callback)(unsigned long, void *),
               int useProvidedRange, unsigned long start, unsigned long n);

void *slice__getElem(void *sliceDataOnly, unsigned long idx);

const SliceInfo *const slice__getInfo(void *sliceDataOnly);

#endif
