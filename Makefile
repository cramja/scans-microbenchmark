CXX=clang++-3.8

all:
	$(CXX) -g -std=c++14 scans.cpp -o scans

profile:
	$(CXX) -g -Wl,--no-as-needed -lprofiler -std=c++14 scans.cpp -o scans
