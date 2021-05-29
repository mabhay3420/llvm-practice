LLVM_FLAGS := `llvm-config --cppflags --ldflags`
LLVM_LIBS := `llvm-config --system-libs --libs all`

.PHONY: clean

all: target
	clang++ -std=c++17 -Wall -Wextra -O0 -g $(LLVM_FLAGS) transformation.cpp $(LLVM_LIBS) -o transforamtion.out

target:
	clang++ -std=c++17 -Wall -Wextra -emit-llvm -S -fno-discard-value-names target-program.cpp

clean:
	rm -f transforamtion.out
	rm -f target-program.ll
