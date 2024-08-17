#include "lib/search.h"
#include "lib/index.h"

int main (int argc, char** argv){
  std::string index_file = argv[1];
  int max_documents = 100;
  QueryProcessor q(index_file, max_documents);

  std::string query;
  std::getline(std::cin, query);
  q.search_query(query);
  q.print_query_results();

  return 0;
}