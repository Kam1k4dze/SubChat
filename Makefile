build: 
	cmake -B build .
	cmake --build build

.PHONY: clean
clean: 
	mkdir build
	rm -rf build 

