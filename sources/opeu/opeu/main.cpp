// Operation Europe, native: the application. The entry is the glue's
// opeu_main (sources/opeu/glue/entry.c), crt0's role for the recovered MAIN.
extern "C" auto opeu_main(int argc, char** argv) -> int;

auto main(int argc, char** argv) -> int {
  return opeu_main(argc, argv);
}
