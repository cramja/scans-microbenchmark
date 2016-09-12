#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include "scans.hpp"

#define BILLION 1000000000l
#define MILLION 1000000l

// edit these params
uint64_t total_attrs = 10;
uint64_t proj_attrs = 4;
float selectivity = 0.01;


double scan(const RowTable& src, RowTable &dst, uint64_t predicate) {
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
      end_time;
  start_time = std::chrono::high_resolution_clock::now();

  for (uint64_t row = 0; row < src.size_tuples(); row++) {
		if (src.getValue(row, 0) < predicate) {
			dst.copyRow(src.getTupleAddress(row));
		}
	}

  end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  return elapsed.count();
}

double scan(const ColumnTable &src, ColumnTable &dst, uint64_t predicate) {
  BitVector bv(src.size_tuples());
	
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
      end_time;
  start_time = std::chrono::high_resolution_clock::now();

  for (uint64_t row = 0; row < src.size_tuples(); row++) {
		bv.append(src.getValue(row, 0) < predicate);
	}
	bv.finalize();

	dst.beginInsert();
	for (uint64_t col = 0; col < total_attrs; col++) {
		for (uint64_t row = 0; row < src.size_tuples(); row++) {
		  if (bv.next()) {
				dst.appendToCurrentColumn(src.getValue(row, col));
			}	
		}
		dst.insertNewColumn();
		bv.finalize();		
	}


  end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  return elapsed.count();
}

void run(std::string scan_type) {
	uint64_t size_src_table_tuples = MILLION;
	uint64_t size_dst_table_tuples = size_src_table_tuples * selectivity + 1000;
	uint64_t num_attributes = 10;
	uint64_t predicate = (uint64_t) (selectivity * (float)(UINT64_MAX));


	double exetime = 0.0;
	if (scan_type == "rs") {
		std::unique_ptr<RowTable> rt(GetRandomRT(size_src_table_tuples, num_attributes));
		std::unique_ptr<RowTable> rtd(new RowTable(size_dst_table_tuples, num_attributes));
		exetime = scan(*rt, *rtd, predicate);
	} else if (scan_type == "cs") {
		std::unique_ptr<ColumnTable> ct(GetRandomCT(size_src_table_tuples, num_attributes));
		std::unique_ptr<ColumnTable> ctd(new ColumnTable(size_dst_table_tuples, num_attributes));
		exetime = scan(*ct, *ctd, predicate);
	} else if (scan_type == "dm") {
		exetime = 0;
	} else {
		std::cout << "Unrecongized scan type: " << scan_type << std::endl;
		return;
	}
  std::cout << scan_type << ":\t" << std::to_string(exetime) << std::endl;
}


int main(int argc, char **argv) {
	assert(argc > 1);
	run(std::string(argv[1]));
  return 0;
}
