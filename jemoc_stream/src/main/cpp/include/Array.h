//
// Created on 2025/1/14.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ARRAY_H
#define JEMOC_STREAM_TEST_ARRAY_H

template <typename T> struct Array {
    Array(unsigned long size) {
        this->size = size;
        this->arr = new T[size];
    }
    ~Array() {
        if (data != nullptr) {
            delete[] data;
        }
    }
    const T *operator&() const { return data; }
    T *operator&() { return data; }
    T *data = nullptr;
    signed long size = 0;
};

#endif // JEMOC_STREAM_TEST_ARRAY_H
