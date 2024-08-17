#pragma once
#include "index.h"
#include <cmath>
#include <map>
#include <queue>
#include <algorithm>
#include <stack>

using BM25ScoreMap = std::map<double,std::string, std::greater<double>>;

class QueryProcessor {
 private:
  BM25ScoreMap bm_scores;
  std::vector<std::string> query_tokens;
  InvertedIndex inverted_index;
  int max_documents;

  void load_index(const std::string& filename);
  double calculate_avg_length(const InvertedIndex& index);
  double calculate_bm25(const std::string& term, const DocumentInfo& doc_info, const InvertedIndex& index, int size, double avg_length);
  void process_query(const std::string& query);
  bool is_valid_query(const std::string& tokens);
  BM25ScoreMap evaluate_expression(const std::vector<std::string>& tokens);
  BM25ScoreMap search_term(const std::string& term);
  BM25ScoreMap search_and_query(BM25ScoreMap& results1, BM25ScoreMap& results2);
  BM25ScoreMap search_or_query(BM25ScoreMap& results1, BM25ScoreMap& results2);

 public:
  QueryProcessor(const std::string& filename, int max_docs = 10);
  void search_query(const std::string& query);
  void print_query_results();
};