#include "lib\index.h"

int main(int argc, char** argv) {

  std::string directory = argv[1];
  std::string index_filename = argv[2];
  InvertedIndex inverted_index;

  index_documents(directory, inverted_index);
  save_index(index_filename,inverted_index);

  return 0;
}