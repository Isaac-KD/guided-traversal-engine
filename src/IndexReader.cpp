#include "IndexReader.hpp"
#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

IndexReader::IndexReader() : posting_fd(-1), posting_size(0), posting_map(nullptr) {}

IndexReader::~IndexReader() {
    if (posting_map && posting_map != MAP_FAILED) {
        munmap(posting_map, posting_size);
    }
    if (posting_fd != -1) {
        close(posting_fd);
    }
}

bool IndexReader::load(const std::string& vocab_path, const std::string& posting_path) {
    // 1. Load vocab.bin
    std::ifstream v_file(vocab_path, std::ios::binary);
    if (!v_file) {
        std::cerr << "Cannot open vocab file: " << vocab_path << std::endl;
        return false;
    }
    
    while (v_file.peek() != EOF) {
        uint32_t term_id;
        float ub_bm25;
        uint64_t offset;
        uint32_t nb_postings;
        
        v_file.read(reinterpret_cast<char*>(&term_id), 4);
        if (v_file.eof()) break;
        v_file.read(reinterpret_cast<char*>(&ub_bm25), 4);
        v_file.read(reinterpret_cast<char*>(&offset), 8);
        v_file.read(reinterpret_cast<char*>(&nb_postings), 4);
        
        vocab[term_id] = {ub_bm25, offset, nb_postings};
    }
    v_file.close();

    // 2. Mmap posting.bin
    posting_fd = open(posting_path.c_str(), O_RDONLY);
    if (posting_fd == -1) {
        std::cerr << "Cannot open posting file: " << posting_path << std::endl;
        return false;
    }

    struct stat sb;
    if (fstat(posting_fd, &sb) == -1) {
        std::cerr << "Cannot fstat posting file" << std::endl;
        return false;
    }
    posting_size = sb.st_size;

    posting_map = static_cast<uint8_t*>(mmap(nullptr, posting_size, PROT_READ, MAP_PRIVATE, posting_fd, 0));
    if (posting_map == MAP_FAILED) {
        std::cerr << "mmap failed" << std::endl;
        return false;
    }

    return true;
}

bool IndexReader::get_term_info(uint32_t term_id, TermInfo& info) const {
    auto it = vocab.find(term_id);
    if (it != vocab.end()) {
        info = it->second;
        return true;
    }
    return false;
}

PostingIterator IndexReader::get_posting_iterator(const TermInfo& info) const {
    if (info.offset >= posting_size) {
        throw std::out_of_range("Offset is out of posting file bounds.");
    }
    return PostingIterator(posting_map + info.offset, info.nb_postings);
}
