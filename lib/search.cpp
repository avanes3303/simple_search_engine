#include "search.h"


QueryProcessor::QueryProcessor(const std::string& filename, int max_docs) {
  max_documents = max_docs;
  load_index(filename);
}

void QueryProcessor::load_index(const std::string& filename) {
  std::ifstream infile(filename, std::ios::binary);
  if (infile.is_open()) {
	std::vector<int> compressed;
	int code;
	while (infile.read(reinterpret_cast<char*>(&code), sizeof(code))) {
	  compressed.push_back(code);
	}
	infile.close();

	std::map<int, std::string> dictionary;
	for (int i = 0; i < 256; i++) {
	  dictionary[i] = std::string(1, i);
	}

	std::string decompressed;
	int previous_code = compressed[0];
	decompressed = dictionary[previous_code];
	std::string current = dictionary[previous_code];

	for (size_t i = 1; i < compressed.size(); i++) {
	  int current_code = compressed[i];
	  std::string entry;

	  if (dictionary.find(current_code) != dictionary.end()) {
		entry = dictionary[current_code];
	  } else if (current_code == dictionary.size()) {
		entry = current + current[0];
	  } else {
		throw std::runtime_error("Invalid compressed data");
	  }

	  decompressed += entry;

	  dictionary[dictionary.size()] = current + entry[0];
	  current = entry;
	}

	std::istringstream iss(decompressed, std::ios::binary);

	inverted_index.clear();
	while (iss.peek() != EOF) {

	  size_t term_length;
	  iss.read(reinterpret_cast<char*>(&term_length), sizeof(term_length));
	  std::string term(term_length, ' ');
	  iss.read(&term[0], term_length);

	  size_t num_docs;
	  iss.read(reinterpret_cast<char*>(&num_docs), sizeof(num_docs));

	  std::vector<DocumentInfo> docs;
	  for (size_t i = 0; i < num_docs; ++i) {
		DocumentInfo doc_info;
		size_t doc_length;
		iss.read(reinterpret_cast<char*>(&doc_length), sizeof(doc_length));
		iss >> doc_info;
		doc_info.length = doc_length;
		docs.push_back(doc_info);
	  }

	  inverted_index[term] = docs;
	}
	std::cout << "Index loaded from file: " << filename << std::endl;
  } else {
	std::cerr << "Error: Unable to open file for reading: " << filename << std::endl;
  }
}


bool QueryProcessor::is_valid_query(const std::string& query) {
  std::vector<std::string> tokens;
  std::istringstream stream(query);
  std::string word;

  while (stream >> word) {
	tokens.push_back(word);
  }
  if (tokens.size() % 2) {
	for (size_t i = 0; i < tokens.size(); ++i) {
	  if (!(i % 2)) {
		if (tokens[i] == "AND" || tokens[i] == "OR") {
		  return false;
		}
	  } else {
		if (tokens[i] != "AND" && tokens[i] != "OR") {
		  return false;
		}
	  }
	}
	return true;
  } else {
	return false;
  }
}


void QueryProcessor::process_query(const std::string& query) {
  std::stack<std::string> operators;
  std::string term;

  auto precedence = [](const std::string& op) {
	if (op == "AND") return 2;
	if (op == "OR") return 1;
	return 0;
  };

  for (size_t i = 0; i < query.length(); ++i) {
	char c = query[i];

	if (c == ' ') {
	  if (!term.empty()) {
		query_tokens.push_back(term);
		term.clear();
	  }
	} else if (c == '(') {
	  operators.push("(");
	} else if (c == ')') {
	  if (!term.empty()) {
		query_tokens.push_back(term);
		term.clear();
	  }
	  while (!operators.empty() && operators.top() != "(") {
		query_tokens.push_back(operators.top());
		operators.pop();
	  }
	  if (!operators.empty() && operators.top() == "(") {
		operators.pop();
	  }
	} else if (c == 'A' || c == 'O') {
	  if (!term.empty()) {
		query_tokens.push_back(term);
		term.clear();
	  }
	  std::string op;
	  if (c == 'A' && query.substr(i, 3) == "AND") {
		op = "AND";
		i += 2;
	  } else if (c == 'O' && query.substr(i, 2) == "OR") {
		op = "OR";
		i += 1;
	  }
	  if (!op.empty()) {
		while (!operators.empty() && precedence(operators.top()) >= precedence(op)) {
		  query_tokens.push_back(operators.top());
		  operators.pop();
		}
		operators.push(op);
	  }
	} else {
	  term.push_back(c);
	}
  }

  if (!term.empty()) {
	query_tokens.push_back(term);
  }

  while (!operators.empty()) {
	query_tokens.push_back(operators.top());
	operators.pop();
  }
}



BM25ScoreMap QueryProcessor::evaluate_expression(const std::vector<std::string>& tokens) {
  std::stack<BM25ScoreMap> results_stack;

  for (const std::string& token : tokens) {
	if (token == "AND" || token == "OR") {
	  BM25ScoreMap right = results_stack.top();
	  results_stack.pop();
	  BM25ScoreMap left = results_stack.top();
	  results_stack.pop();
	  if (token == "AND") {
		results_stack.push(search_and_query(left, right));
	  } else if (token == "OR") {
		results_stack.push(search_or_query(left, right));
	  }
	} else {
	  results_stack.push(search_term(token));
	}
  }

  return results_stack.top();
}

void QueryProcessor::search_query(const std::string& query) {
  if (!is_valid_query(query)){
	throw std::runtime_error("invalid query");
  }

  process_query(query);
  bm_scores = evaluate_expression(query_tokens);
}


double QueryProcessor::calculate_avg_length(const InvertedIndex& index) {
  double total_length = 0;
  size_t total_docs = 0;

  for (const auto& entry : index) {
	total_length += entry.second.size();
	total_docs++;
  }

  if (total_docs == 0) {
	return 0;
  }
  return total_length / total_docs;
}

double QueryProcessor::calculate_bm25(const std::string& term, const DocumentInfo& doc_info, const InvertedIndex& index, int size, double avg_length) {
  double k1 = 2.0;
  double b = 0.75;

  int document_frequency = index.at(term).size();
  double idf = std::log((size - document_frequency + 0.5) / (document_frequency + 0.5));
  double tf = doc_info.term_frequency;
  double doc_length = doc_info.length;
  double score = idf * ((tf * (k1 + 1)) / (tf + k1 * (1 - b + b * (doc_length / avg_length))));

  return score;
}

void QueryProcessor::print_query_results() {
  if (!bm_scores.empty()) {
	for (const auto& pair : bm_scores) {
	  const std::string& document_name = pair.second;
	  double bm25_score = pair.first;

	  std::cout << "File: " << document_name << std::endl;
	  std::cout << "BM25 Score: " << bm25_score << std::endl;

	  std::ifstream file(document_name);
	  if (file.is_open()) {
		std::string line;
		int line_number = 1;
		while (std::getline(file, line)) {
		  for (const auto& term : query_tokens) {
			if (term != "AND" && term != "OR" && line.find(term) != std::string::npos) {
			  std::cout << "term: " << term << ", line " << line_number << std::endl;
			}
		  }
		  ++line_number;
		}
		std::cout << std::endl;
		file.close();
	  } else {
		std::cerr << "Error: Unable to open file " << document_name << std::endl;
	  }
	}
  } else {
	std::cout << "No documents found " << std::endl;
  }
}

BM25ScoreMap QueryProcessor::search_term(const std::string& term) {
  BM25ScoreMap bm25_scores;

  auto it = inverted_index.find(term);
  if (it == inverted_index.end()) {
	return bm25_scores;
  }

  const std::vector<DocumentInfo>& term_docs = it->second;
  int size = inverted_index.size();
  double avg_length = calculate_avg_length(inverted_index);

  for (const DocumentInfo& doc_info : term_docs) {
	double bm25_score = calculate_bm25(term, doc_info, inverted_index, size, avg_length);
	bm25_scores.emplace(bm25_score, doc_info.name);

	if (bm25_scores.size() > max_documents) {
	  bm25_scores.erase(bm25_scores.begin());
	}
  }

  return bm25_scores;
}

BM25ScoreMap QueryProcessor::search_and_query(BM25ScoreMap& results1 , BM25ScoreMap& results2) {
  BM25ScoreMap intersection_results;

  for (const auto& pair1 : results1) {
	const std::string& doc1 = pair1.second;
	double score1 = pair1.first;

	for (const auto& pair2 : results2) {
	  const std::string& doc2 = pair2.second;
	  double score2 = pair2.first;

	  if (doc1 == doc2) {
		double combined_score = score1 + score2;
		if (intersection_results.size() < max_documents) {
		  intersection_results[combined_score] = doc1;
		} else {
		  auto min_it = intersection_results.begin();
		  if (combined_score > min_it->first) {
			intersection_results.erase(min_it);
			intersection_results[combined_score] = doc1;
		  }
		}
	  }
	}
  }
  return intersection_results;
}


BM25ScoreMap QueryProcessor::search_or_query(BM25ScoreMap& results1, BM25ScoreMap& results2) {
  BM25ScoreMap union_results;

  auto add = [&union_results, this](double score, const std::string& doc) {
	if (union_results.size() < max_documents) {
	  union_results.emplace(score, doc);
	} else if (score > union_results.begin()->first) {
	  union_results.erase(union_results.begin());
	  union_results.emplace(score, doc);
	}
  };

  for (const auto& pair : results1) {
	add(pair.first, pair.second);
  }

  for (const auto& pair : results2) {
	auto it = std::find_if(union_results.begin(), union_results.end(), [&pair](const auto& union_pair) {
	  return union_pair.second == pair.second;
	});

	if (it != union_results.end()) {
	  double new_score = std::max(it->first, pair.first);
	  union_results.erase(it);
	  add(new_score, pair.second);
	} else {
	  add(pair.first, pair.second);
	}
  }

  return union_results;
}
