#include <assert.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <exception>
#include <iostream>
#include <random>
#include <string.h>
#include <sys/mman.h>
#include <vector>
#include <memory>

#include "scans.hpp"

#define BILLION 1000000000l

// edit these params
uint64_t total_attrs = 10;
uint64_t proj_attrs = 4;
float selectivity = 0.1;

/**
 * returns time (s) it takes to run a row scan
 */
double rowScan(const Buffer& buf, float selectivity) {
  uint64_t predicate = selectivity * UINT64_MAX;
  uint64_t tuple_length = total_attrs * 8;
  std::unique_ptr<Buffer> output_buf(new Buffer( (selectivity + 0.1) * buf.size_));

  std::chrono::time_point<std::chrono::high_resolution_clock> start_time, end_time;
  start_time = std::chrono::high_resolution_clock::now();

  for (uint64_t tuple_index = 0; tuple_index < buf.size_; tuple_index += tuple_length) {
    if (buf.buf_[tuple_index] < predicate) {
      memcpy(&output_buf->buf_[output_buf->buf_index_], &buf.buf_[buf.buf_index_], proj_attrs);
      output_buf->buf_index_ += proj_attrs;
    }
  }
  
  end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;
  
  return elapsed.count();
}

double columnScan(const Buffer& buf, float selectivity) {
  uint64_t predicate = selectivity * UINT64_MAX;
  uint64_t size_column = buf.size_/total_attrs;
  std::vector<bool> select(size_column);
  std::unique_ptr<Buffer> output_buf(new Buffer( (selectivity + 0.1) * buf.size_));
  
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time, end_time;
  start_time = std::chrono::high_resolution_clock::now();

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

  end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;
  
  return elapsed.count();
}

int main() {

  uint64_t size_buffer_bytes = ((uint64_t)1) << 28; // 2 gb
  Buffer buf = Buffer(size_buffer_bytes);
  buf.randomize();

  std::cout << "rs: " << rowScan(buf, selectivity) << " s" << std::endl;
  std::cout << "cs: " << columnScan(buf, selectivity) << " s" << std::endl; 

  return 0;
}
