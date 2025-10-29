.PHONY: all build test clean format format-check size

# Default target
all: build test

# Build
build:
	@echo "🔨 Building..."
	@cmake -B build -DCMAKE_BUILD_TYPE=Debug -DV4_BUILD_TESTS=ON
	@cmake --build build -j

# Release build
release:
	@echo "🚀 Building release..."
	@cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DV4_BUILD_TESTS=ON
	@cmake --build build-release -j

# Run tests
test: build
	@echo "🧪 Running tests..."
	@cd build && ctest --output-on-failure

# Clean
clean:
	@echo "🧹 Cleaning..."
	@rm -rf build build-release build-debug build-asan build-ubsan

# Apply formatting
format:
	@echo "✨ Formatting C/C++ code..."
	@find src include tests -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) \
		-not -path "*/vendor/*" -exec clang-format -i {} \;
	@echo "✨ Formatting CMake files..."
	@find . -name 'CMakeLists.txt' -o -name '*.cmake' | xargs cmake-format -i
	@echo "✅ Formatting complete!"

# Format check
format-check:
	@echo "🔍 Checking C/C++ formatting..."
	@find src include tests -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) \
		-not -path "*/vendor/*" | xargs clang-format --dry-run --Werror || \
		(echo "❌ C/C++ formatting check failed." && exit 1)
	@echo "🔍 Checking CMake formatting..."
	@find . -name 'CMakeLists.txt' -o -name '*.cmake' | xargs cmake-format --check || \
		(echo "❌ CMake formatting check failed." && exit 1)
	@echo "✅ All formatting checks passed!"

# Sanitizer build
asan: clean
	@echo "🛡️  Building with AddressSanitizer..."
	@cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug -DV4_BUILD_TESTS=ON \
		-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
	@cmake --build build-asan -j
	@echo "🧪 Running tests with AddressSanitizer..."
	@cd build-asan && ctest --output-on-failure

ubsan: clean
	@echo "🛡️  Building with UndefinedBehaviorSanitizer..."
	@cmake -B build-ubsan -DCMAKE_BUILD_TYPE=Debug -DV4_BUILD_TESTS=ON \
		-DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g"
	@cmake --build build-ubsan -j
	@echo "🧪 Running tests with UndefinedBehaviorSanitizer..."
	@cd build-ubsan && ctest --output-on-failure

# Build release and show stripped binary sizes
size:
	@echo "📦 Building release with size optimization..."
	@cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DV4_BUILD_TESTS=ON -DV4_OPTIMIZE_SIZE=ON
	@cmake --build build-release -j
	@echo ""
	@echo "📊 Stripping and measuring binary sizes..."
	@echo ""
	@echo "=== Library ==="
	@if [ -f build-release/libv4vm.a ]; then \
		cp build-release/libv4vm.a build-release/libv4vm.stripped.a && \
		strip --strip-debug build-release/libv4vm.stripped.a && \
		ls -lh build-release/libv4vm.a build-release/libv4vm.stripped.a | awk '{print $$9 ": " $$5}'; \
	fi
	@echo ""
	@echo "=== Test Executables (stripped) ==="
	@for binary in build-release/test_*; do \
		if [ -f "$$binary" ] && [ -x "$$binary" ]; then \
			cp "$$binary" "$${binary}.stripped" && \
			strip "$${binary}.stripped" && \
			echo "$$(basename $$binary): $$(ls -lh $${binary}.stripped | awk '{print $$5}')"; \
		fi \
	done
	@echo ""
	@echo "✅ Size measurement complete!"