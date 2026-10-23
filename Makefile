test:
	cmake -B build -DBUILD_BENCHMARK=OFF
	cmake --build build
	ctest --test-dir build --output-on-failure

usage:
	cmake -B build -DBUILD_BENCHMARK=OFF
	cmake --build build --target usage
	./build/usage

benchmark:
	cmake -B build -DBUILD_BENCHMARK=ON
	cmake --build build --target benchmark
	./build/benchmark