#include <assert.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <exception>
#include <iostream>
#include <random>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <vector>

#define BILLION 1000000000l

// edit these params
uint64_t total_attrs = 10;
uint64_t proj_attrs = 4;
float selectivity = 0.1;



// simple mmap'd buffer with randomized contents
struct Buffer {
  const uint64_t size_;  // size in 64 bit words
  const uint64_t size_bytes_;
  uint64_t *buf_;
  uint64_t buf_index_;

  Buffer(uint64_t size) : size_(size), size_bytes_(sizeof(uint64_t) * size_), buf_(nullptr), buf_index_(0) {
    buf_ = reinterpret_cast<uint64_t*>(malloc(size_bytes_));
  }

  ~Buffer() {
    free(buf_);
  }

  // uniformly randomizes the buffer
  void randomize() {
    std::default_random_engine generator;
    std::uniform_int_distribution<uint64_t> distribution(0,UINT64_MAX);
    for (uint64_t i = 0; i < size_; i++) {
      buf_[i] = distribution(generator);
    }
  }

  // appends a value to the buffer
  void append(uint64_t value) {
    if (buf_index_ < size_) {
      buf_[buf_index_] = value;
      buf_index_++;
    } else {
      throw new std::out_of_range("overran append in buffer");
    }
  }
};

// doesn't grow, set size in advance
class BitVector {
  uint64_t capacity_;
  uint64_t size_;
  uint64_t *buf_;

  BitVector(uint64_t capacity) : capacity_(capacity), size_(0), buf_(nullptr) {
    buf_ = reinterpret_cast<uint64_t*>(malloc(capacity_ / sizeof(uint64_t) + 1));
  }

  ~BitVector() {
    free(buf_);
  }

  void append(bool value) {
    if (value) {
      uint64_t word_index = size_ / sizeof(uint64_t);
      uint64_t *word = &buf_[word_index];
      *word |= (1 << (size_ % sizeof(uint64_t)));
    }
    size_++;
  }

    

};

Buffer* rowScan(const Buffer& buf, float selectivity) {
  uint64_t predicate = selectivity * UINT64_MAX;
  uint64_t tuple_length = total_attrs * 8;

  struct timespec timeFirst;
  struct timespec timeLast;
  clock_gettime(CLOCK_MONOTONIC, &timeFirst);


  Buffer *output_buf = new Buffer( (selectivity + 0.1) * buf.size_);
  for (uint64_t tuple_index = 0; tuple_index < buf.size_; tuple_index += tuple_length) {
    if (buf.buf_[tuple_index] < predicate) {
      memcpy(&output_buf->buf_[output_buf->buf_index_], &buf.buf_[buf.buf_index_], proj_attrs);
      output_buf->buf_index_ += proj_attrs;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &timeLast);
  printf("-> %li ns\n", \
            (timeLast.tv_sec - timeFirst.tv_sec) * BILLION + (timeLast.tv_nsec - timeFirst.tv_nsec));

  return output_buf;
}

Buffer* columnScan(const Buffer& buf, float selectivity) {
  uint64_t predicate = selectivity * UINT64_MAX;
  uint64_t size_column = buf.size_/total_attrs;
  std::vector<bool> select(size_column);
  Buffer *output_buf = new Buffer( (selectivity + 0.1) * buf.size_);
  
  struct timespec timeFirst;
  struct timespec timeLast;
  clock_gettime(CLOCK_MONOTONIC, &timeFirst);



  // first, find the selected tuples and mark in bitvector
  for (uint64_t tattr_index = 0; tattr_index < size_column; tattr_index++) {
    if (buf.buf_[tattr_index] < predicate) {
      select[tattr_index] = true;
    }
  }

  // now project into columns
  for (uint64_t col_index = 0; col_index < proj_attrs; col_index++) {
    for (uint64_t tuple_index = 0; tuple_index < size_column; tuple_index++) {
      if (select[tuple_index]) {
        output_buf->append(buf.buf_[col_index * size_column + tuple_index]);
      }
    } 
  }
  clock_gettime(CLOCK_MONOTONIC, &timeLast);
  printf("-> %li ns\n", \
            (timeLast.tv_sec - timeFirst.tv_sec) * BILLION + (timeLast.tv_nsec - timeFirst.tv_nsec));


  return output_buf;
}

int main() {

  // do row style scan
  Buffer buf = Buffer(100000);
  buf.randomize();

  Buffer* output = rowScan(buf, selectivity);
  delete output;
  
  output = columnScan(buf, selectivity);
  delete output;

  return 0;
}
