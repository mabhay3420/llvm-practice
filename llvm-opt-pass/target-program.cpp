#include <cstdio>

void foo() { printf("This function is dangerous and should be replaced!\n"); }

void bar(int i) {
  printf("This function is safe! we found call site: '%d'.\n", i);
}
void baz() { foo(); }

int main(int argc, char **argv) {
  foo();
  for (int i = 0; i < argc; i++) {
    printf("%s",argv[i]);
  }
  baz();
  return 0;
}