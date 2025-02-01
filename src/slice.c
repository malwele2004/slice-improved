#include "slice.h" // required
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SLICE_INFO_SIZE (sizeof(SliceInfo))

static SliceInfo *slice__private_info_from_whole(void *sliceWhole) {
  return sliceWhole;
}

static void *slice__private_data_only_from_whole(void *sliceWhole) {
  return sliceWhole + SLICE_INFO_SIZE;
}

static void *slice__private_whole_from_data_only(void *sliceDataOnly) {
  return sliceDataOnly - SLICE_INFO_SIZE;
}

static SliceInfo *slice__private_info_from_data_only(void *sliceDataOnly) {
  return slice__private_info_from_whole(
      slice__private_whole_from_data_only(sliceDataOnly));
}

// @return sliceWhole
static void *slice__private_new(unsigned long bSize) {
  if (bSize == 0) {
    return NULL;
  }
  return malloc(SLICE_INFO_SIZE + bSize);
}

static void *slice__private_init(unsigned long esize, unsigned long cap,
                                 int (*elemDeleter)(unsigned long, void *)) {
  void *sliceWhole = slice__private_new(esize * cap);
  if (!sliceWhole) {
    return NULL;
  }
  *slice__private_info_from_whole(sliceWhole) = (SliceInfo){
      .len = 0, .cap = cap, .esize = esize, .elemDeleter = elemDeleter};
  return slice__private_data_only_from_whole(sliceWhole);
}

void *slice__public_new(unsigned long esize, unsigned long cap,
                        int (*elemDeleter)(unsigned long, void *)) {
  return slice__private_init(esize, cap, elemDeleter);
}

// mode: 1 -> shrink, 0 -> grow
static void *slice__private_modify(void *sliceDataOnly, int mode) {
  SliceInfo *sliceInfo = slice__private_info_from_data_only(sliceDataOnly),
            oldSliceInfo = *sliceInfo;
  unsigned long newCap = ((mode) ? 0.5 : 2) * oldSliceInfo.cap;
  // [mode] avoid trimming, accidently free or unnecessary shrink
  if (mode &&
      (newCap < oldSliceInfo.len || newCap == 0 /* as this may cause free*/ ||
       oldSliceInfo.len == oldSliceInfo.cap /*is full cannot shrink*/)) {
    return NULL;
    // [!mode] must be full to grow
  } else if (!mode &&
             oldSliceInfo.len != oldSliceInfo.cap /*is non-full cannot grow*/) {
    return NULL;
  }

  void *newSliceWhole =
      realloc(slice__private_whole_from_data_only(sliceDataOnly),
              SLICE_INFO_SIZE + (oldSliceInfo.esize * newCap));
  // is reallocated
  if (!newSliceWhole) {
    return NULL;
  }
  sliceInfo = slice__private_info_from_whole(newSliceWhole);
  // changing capacity
  sliceInfo->cap = newCap;
  return slice__private_data_only_from_whole(newSliceWhole);
}

void slice__delete(void *sliceDataOnly) {
  if (!sliceDataOnly) {
    return;
  }
  int (*callback)(unsigned long, void *);
  if ((callback =
           slice__private_info_from_data_only(sliceDataOnly)->elemDeleter)) {
    slice__forEach(sliceDataOnly, callback, 0, 0, 0, 0, 0);
  }
  free(slice__private_whole_from_data_only(sliceDataOnly));
}

void *slice__push(void *sliceDataOnly, void *elemAddr) {
  if (sliceDataOnly) {
    void *newSliceDataOnly =
        (slice__private_modify(sliceDataOnly, 0)) ?: sliceDataOnly;
    SliceInfo *sliceInfo = slice__private_info_from_data_only(newSliceDataOnly);
    if (sliceInfo->len == sliceInfo->cap) {
      return NULL; // failed (reallocate failed and is full)
    }
    memcpy(newSliceDataOnly +
               (sliceInfo->esize * sliceInfo->len++ /*increment length*/),
           elemAddr, sliceInfo->esize);
    return newSliceDataOnly; // ok
  }
  return NULL;
}

void *slice__pop(void *sliceDataOnly, int isShrink) {
  if (sliceDataOnly) {
    if (slice__private_info_from_data_only(sliceDataOnly)->len == 0) {
      return NULL;
    }
    void *newSliceDataOnly =
        (isShrink)
            ? ((slice__private_modify(sliceDataOnly, 1)) ?: sliceDataOnly)
            : sliceDataOnly;
    SliceInfo *sliceInfo = slice__private_info_from_data_only(newSliceDataOnly);
    sliceInfo->len--;
    return newSliceDataOnly;
  }
  return NULL;
}

void slice__debug(void *sliceDataOnly) {
  if (sliceDataOnly) {
    SliceInfo *sliceInfo = slice__private_info_from_data_only(sliceDataOnly);
    fprintf(stderr, "{\ndebug: slice,\nesize: %lu,\nlen: %lu,\ncap: %lu,\n}\n",
            sliceInfo->esize, sliceInfo->len, sliceInfo->cap);
  }
}

unsigned long slice__forEach(void *sliceDataOnly,
                             int (*callback)(unsigned long, void *),
                             int useProvidedRange, unsigned long start,
                             unsigned long n, int exitOnTrue, int exitOnFalse) {
  if (sliceDataOnly) {
    SliceInfo *sliceInfo = slice__private_info_from_data_only(sliceDataOnly);
    unsigned long ustart = 0, un = sliceInfo->len,
                  successCalls = 0; // default to known bounds
    if (useProvidedRange) {
      if (n == 0 || (start + n) > sliceInfo->len)
        return 0; // failed
      ustart = start;
      un = n;
    }

    for (unsigned long i = ustart; i < un; i++) {
      if (callback(i, sliceDataOnly + (i * sliceInfo->esize))) {
        successCalls++;
        if (exitOnTrue)
          break;
      } else {
        if (exitOnFalse)
          break;
      }
    }
    return successCalls;
  }
  return 0;
}

int slice__every(void *sliceDataOnly, int (*callback)(unsigned long, void *),
                 int useProvidedRange, unsigned long start, unsigned long n) {
  return slice__forEach(sliceDataOnly, callback, useProvidedRange, start, n, 0,
                        1) == n;
}

int slice__any(void *sliceDataOnly, int (*callback)(unsigned long, void *),
               int useProvidedRange, unsigned long start, unsigned long n) {
  return slice__forEach(sliceDataOnly, callback, useProvidedRange, start, n, 1,
                        0) > 0;
}

void *slice__getElem(void *sliceDataOnly, unsigned long idx) {
  if (sliceDataOnly) {
    SliceInfo *sliceInfo = slice__private_info_from_data_only(sliceDataOnly);
    if (idx < sliceInfo->len) {
      return sliceDataOnly + (sliceInfo->esize * idx);
    }
  }
  return NULL;
}

const SliceInfo *const slice__getInfo(void *sliceDataOnly) {
  return (sliceDataOnly) ?slice__private_info_from_data_only(sliceDataOnly) : NULL;
}
