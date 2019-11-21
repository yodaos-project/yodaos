#pragma once

#include <stdint.h>
#include <string.h>

// T = Type
// IT = Index Type
// O = Operator (getIndex, setIndex, compare)
template <typename T, typename IT, typename O>
class MergeSort {
public:
  MergeSort(T *arr, uint32_t size, O &oper) : arrayList(arr), arraySize(size), op(oper) {
  }

  void sort() {
    init();

    uint32_t width = 1;
    while (width < arraySize) {
      doMergeWithWidth(arrayList, arraySize, width);
      width <<= 1;
    }

    release();
  }

private:
  void init() {
    uint32_t i;
    for (i = 0; i < arraySize; ++i) {
      op.setIndex(arrayList[i], i);
    }
    if (arraySize)
      swapSpace = new IT[arraySize];
  }

  void release() {
    delete[] swapSpace;
  }

  static T *spliceArray(T *&arr, uint32_t &size, uint32_t width, uint32_t &rs) {
    T *ra = arr;
    if (size < width)
      rs = size;
    else
      rs = width;
    arr += rs;
    size -= rs;
    return ra;
  }

  void doMergeWithWidth(T *arr, uint32_t size, uint32_t width) {
    T *la;
    T *ra;
    uint32_t ls, rs;
    while (size) {
      la = spliceArray(arr, size, width, ls);
      ra = spliceArray(arr, size, width, rs);
      merge(la, ls, ra, rs);
    }
  }

  void merge(T *left, uint32_t lsize, T *right, uint32_t rsize) {
    uint32_t swapIndex = 0;
    T *arr = left;
    while (lsize || rsize) {
      if (lsize == 0) {
        swapSpace[swapIndex++] = op.getIndex(*right);
        ++right;
        --rsize;
      } else if (rsize == 0) {
        swapSpace[swapIndex++] = op.getIndex(*left);
        ++left;
        --lsize;
      } else {
        if (op.comp(arrayList[op.getIndex(*right)], arrayList[op.getIndex(*left)])) {
          swapSpace[swapIndex++] = op.getIndex(*right);
          ++right;
          --rsize;
        } else {
          swapSpace[swapIndex++] = op.getIndex(*left);
          ++left;
          --lsize;
        }
      }
    }
    uint32_t i;
    for (i = 0; i < swapIndex; ++i) {
      op.setIndex(arr[i], swapSpace[i]);
    }
  }

private:
  O &op;
  T *arrayList;
  IT *swapSpace = nullptr;
  uint32_t arraySize;
};
