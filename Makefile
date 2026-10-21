test:
	cmake -B build
	cmake --build build
	ctest --test-dir build --output-on-failure

usage:
	cmake -B build
	cmake --build build --target usage
	./build/usage