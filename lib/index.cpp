#include "index.h"

std::ostream& operator<<(std::ostream& os, const DocumentInfo& doc) {
  size_t length = doc.name.size();
  os.write(reinterpret_cast<const char*>(&length), sizeof(length));
  os.write(doc.name.c_str(), length);
  os.write(reinterpret_cast<const char*>(&doc.term_frequency), sizeof(doc.term_frequency));
  return os;
}

std::istream& operator>>(std::istream& is, DocumentInfo& doc) {
  size_t length;
  is.read(reinterpret_cast<char*>(&length), sizeof(length));
  doc.name.resize(length);
  is.read(&doc.name[0], length);
  is.read(reinterpret_cast<char*>(&doc.term_frequency), sizeof(doc.term_frequency));
  return is;
}

void update_inverted_index(const std::string& document_name, const std::string& text, InvertedIndex& inverted_index) {
  std::vector<std::string> tokens;
  std::stringstream ss(text);
  std::string token;
  while (ss >> token) {
	tokens.push_back(token);
  }
  std::unordered_map<std::string, int> term_frequency_map;
  for (const std::string& token : tokens) {
	term_frequency_map[token]++;
  }

  size_t document_length = text.size();
  for (const auto& entry : term_frequency_map) {
	DocumentInfo doc_info;
	doc_info.name = document_name;
	doc_info.term_frequency = entry.second;
	doc_info.length = document_length;
	inverted_index[entry.first].push_back(doc_info);
  }
}


void index_documents(const std::string& directory, InvertedIndex& inverted_index) {
  for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
	if (entry.is_regular_file()) {
	  std::ifstream file(entry.path());
	  if (file.is_open()) {
		std::string document_name = entry.path().string();
		std::string line;
		std::stringstream document_content;
		while (getline(file, line)) {
		  document_content << line << " ";
		}
		update_inverted_index(document_name, document_content.str(), inverted_index);
		file.close();
	  } else {
		std::cerr << "Error: Unable to open file " << entry.path() << std::endl;
	  }
	}
  }
}

void save_index(const std::string& filename, const InvertedIndex& inverted_index) {
  std::ostringstream oss;
  for (const auto& entry : inverted_index) {
	size_t term_length = entry.first.size();
	oss.write(reinterpret_cast<const char*>(&term_length), sizeof(term_length));
	oss.write(entry.first.c_str(), term_length);

	size_t num_docs = entry.second.size();
	oss.write(reinterpret_cast<const char*>(&num_docs), sizeof(num_docs));

	for (const DocumentInfo& doc_info : entry.second) {
	  size_t doc_length = doc_info.length;
	  oss.write(reinterpret_cast<const char*>(&doc_length), sizeof(doc_length));
	  oss << doc_info;
	}
  }
  std::string data = oss.str();
  std::map<std::string, int> dictionary;
  for (int i = 0; i < 256; i++) {
	dictionary[std::string(1, i)] = i;
  }

  std::string current;
  std::vector<int> compressed;
  for (char ch : data) {
	std::string current_with_char = current + ch;
	if (dictionary.find(current_with_char) != dictionary.end()) {
	  current = current_with_char;
	} else {
	  compressed.push_back(dictionary[current]);
	  dictionary[current_with_char] = dictionary.size();
	  current = std::string(1, ch);
	}
  }
  if (!current.empty()) {
	compressed.push_back(dictionary[current]);
  }

  std::ofstream outfile(filename, std::ios::binary);
  if (outfile.is_open()) {
	for (int code : compressed) {
	  outfile.write(reinterpret_cast<const char*>(&code), sizeof(code));
	}
	outfile.close();
	std::cout << "Index saved to file: " << filename << std::endl;
  } else {
	std::cerr << "Error: Unable to open file for writing: " << filename << std::endl;
  }
}
