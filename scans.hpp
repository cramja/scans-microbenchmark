#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

/**
 * Simple malloc'd buffer
 */
struct Buffer {
  const uint64_t size_;  // size in 64 bit words
  const uint64_t size_bytes_;
  uint64_t *buf_;
  uint64_t buf_index_;

  Buffer(uint64_t size)
      : size_(size),
        size_bytes_(sizeof(uint64_t) * size_),
        buf_(nullptr),
        buf_index_(0) {
    buf_ = reinterpret_cast<uint64_t *>(malloc(size_bytes_));
  }

  ~Buffer() { free(buf_); }

  /*
   * uniformly randomizes the buffer
   */
  void randomize() {
    if (load_random()) {
      return;
    }
    uint64_t x = 213523143, y = 5423323388, z = 321543134;

    for (uint64_t i = 0; i < size_; i++) {
      uint64_t t;
      x ^= x << 16;
      x ^= x >> 5;
      x ^= x << 1;
      t = x;
      x = y;
      y = z;
      z = t ^ x ^ y;

      buf_[i] = z;
    }
    save_random();
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

 private:
  bool file_exists(const std::string &fname) {
    return (access(fname.c_str(), F_OK) != -1);
  }

  // try to find a random buffer file.
  bool load_random() {
    std::string fname = "rand_" + std::to_string(size_bytes_) + ".bin";
    if (!file_exists(fname)) {
      return false;
    }
    std::ifstream infile;
    infile.open(fname.c_str(), std::ios::binary | std::ios::in);
    uint64_t chunk_size = 1 << 20;  // 1mb reads
    uint64_t remaining = size_bytes_;
    char *rptr = (char *)buf_;
    while (remaining > 0) {
      if (remaining < chunk_size) chunk_size = remaining;
      infile.read(rptr, chunk_size);
      rptr += chunk_size;
      remaining -= chunk_size;
    }
    infile.close();
    return true;
  }

  void save_random() {
    std::string fname = "rand_" + std::to_string(size_bytes_) + ".bin";
    std::ofstream outfile;
    outfile.open(fname.c_str(), std::ios::binary | std::ios::out);
    uint64_t chunk_size = 1 << 10;  // 1kb reads
    uint64_t remaining = size_bytes_;
    char *rptr = (char *)buf_;
    while (remaining > 0) {
      if (remaining < chunk_size) chunk_size = remaining;
      outfile.write(rptr, chunk_size);
      rptr += chunk_size;
      remaining -= chunk_size;
    }
    outfile.close();
  }
};

// doesn't grow, set size in advance
// can only append and iterate forward
// todo: incomplete, not used
class BitVector {
  uint64_t capacity_;
  uint64_t size_;
  uint64_t *buf_;

  uint64_t itr_pos_word_;
  uint64_t itr_pos_array_;

  bool finalized_;

 public:
  BitVector(uint64_t capacity)
      : capacity_(capacity),
        size_(0),
        buf_(nullptr),
        itr_pos_word_(1),
        itr_pos_array_(0),
        finalized_(false) {
    buf_ =
        reinterpret_cast<uint64_t *>(malloc(capacity_ / sizeof(uint64_t) + 1));
  }

  ~BitVector() { free(buf_); }

  void append(bool value) {
    assert(!finalized_);

    if (value) {
      buf_[itr_pos_array_] = (buf_[itr_pos_array_] << 1) + 1;
    } else {
      buf_[itr_pos_array_] = buf_[itr_pos_array_] << 1;
    }

    itr_pos_word_ = itr_pos_word_ << 1;
    if (itr_pos_word_ == 0) {
      itr_pos_array_++;
      itr_pos_word_ = 1;
    }
    size_++;
  }

  void finalize() {
    finalized_ = true;
    itr_pos_word_ = 1;
    itr_pos_array_ = 0;
  }

  bool next() {
    bool result = (buf_[itr_pos_array_] & itr_pos_word_) > 0;
    itr_pos_word_ = itr_pos_word_ << 1;
    if (itr_pos_word_ == 0) {
      itr_pos_word_ = 1;
      itr_pos_array_++;
    }
    return result;
  }
};
