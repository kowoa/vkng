.PHONY: build run clean

build:
	cmake -S . -B build
	cmake --build build

run:
	./bin/vulkan_guide

clean:
	rm -rf build