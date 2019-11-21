#pragma once

#include <stdint.h>
#include <string.h>

// T = Type
// O = Operator (getIndex, setIndex, compare)
template <typename T, typename O>
class HeapSort {
public:
  HeapSort(T *arr, uint32_t size, O &oper) : arrayList(arr), arraySize(size), op(oper) {
  }

  void sort() {
    initIndex();
    heapSize = arraySize;
    buildHeap();

    uint32_t i;
    for (i = arraySize - 1; i > 0; --i) {
      swap(0, i);
      --heapSize;
      heapify(0);
    }
  }

private:
  void initIndex() {
    uint32_t i;
    for (i = 0; i < arraySize; ++i) {
      op.setIndex(arrayList[i], i);
    }
  }

  void swap(uint32_t i1, uint32_t i2) {
    uint32_t t = op.getIndex(arrayList[i1]);
    op.setIndex(arrayList[i1], op.getIndex(arrayList[i2]));
    op.setIndex(arrayList[i2], t);
  }

  void heapify(uint32_t idx) {
    uint32_t left = (idx << 1) + 1;
    uint32_t right = (idx << 1) + 2;
    uint32_t largest = idx;
    if (left < heapSize) {
      if (op.comp(arrayList[op.getIndex(arrayList[largest])], arrayList[op.getIndex(arrayList[left])])) {
        largest = left;
      }
    }
    if (right < heapSize) {
      if (op.comp(arrayList[op.getIndex(arrayList[largest])], arrayList[op.getIndex(arrayList[right])])) {
        largest = right;
      }
    }
    if (largest != idx) {
      swap(largest, idx);
      heapify(largest);
    }
  }

  void buildHeap() {
    uint32_t i = heapSize / 2;
    while (true) {
      heapify(i);
      if (i == 0)
        break;
      --i;
    }
  }

private:
  O &op;
  T *arrayList;
  uint32_t arraySize;
  uint32_t heapSize;
};
