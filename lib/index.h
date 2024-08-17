#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <filesystem>

struct DocumentInfo {
  std::string name;
  int term_frequency;
  int length;
  friend std::ostream& operator<<(std::ostream& os, const DocumentInfo& doc);
  friend std::istream& operator>>(std::istream& is, DocumentInfo& doc);
};

using InvertedIndex = std::unordered_map<std::string, std::vector<DocumentInfo>>;

void update_inverted_index(const std::string& document_name, const std::string& text, InvertedIndex& inverted_index);
void index_documents(const std::string& directory, InvertedIndex& inverted_index);
void save_index(const std::string& filename, const InvertedIndex& inverted_index);