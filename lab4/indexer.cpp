#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


template <typename T>
struct MyVector {
    T* data;
    size_t size;
    size_t capacity;

    void init() { data = NULL; size = 0; capacity = 0; }
    void free_mem() { free(data); init(); }

    void push_back(const T& value) {
        if (size == capacity) {
            capacity = (capacity == 0) ? 8 : capacity * 2;
            data = (T*)realloc(data, capacity * sizeof(T));
        }
        data[size++] = value;
    }
};

struct TermInfo {
    char* term;
    MyVector<unsigned int> postings;
};

struct DocInfo {
    char* url;
    char* title;
};

struct HashNode {
    char* key;
    int value;
    HashNode* next;
};

#define HASH_TABLE_SIZE 2097152
HashNode* hash_table[HASH_TABLE_SIZE];

unsigned int hash_function(const char* key) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

int find_term_in_hash(const char* term) {
    unsigned int index = hash_function(term);
    HashNode* current = hash_table[index];
    while (current != NULL) {
        if (strcmp(current->key, term) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return -1;
}

void insert_term_into_hash(const char* term, int value_idx) {
    unsigned int index = hash_function(term);
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    new_node->key = strdup(term);
    new_node->value = value_idx;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
}


int compare_terminfo(const void* a, const void* b) {
    return strcmp(((TermInfo*)a)->term, ((TermInfo*)b)->term);
}

void build_inverted_index(MyVector<TermInfo>& term_index) {
    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        char* term = strtok(line, "\t\n");
        char* doc_id_str = strtok(NULL, "\t\n");
        if (!term || !doc_id_str) continue;
        
        unsigned int doc_id = atoi(doc_id_str);

        int term_idx = find_term_in_hash(term);

        if (term_idx != -1) {
            MyVector<unsigned int>* postings = &term_index.data[term_idx].postings;
            if (postings->size == 0 || postings->data[postings->size - 1] != doc_id) {
                postings->push_back(doc_id);
            }
        } else {
            TermInfo new_info;
            new_info.term = strdup(term);
            new_info.postings.init();
            new_info.postings.push_back(doc_id);
            term_index.push_back(new_info);
            insert_term_into_hash(term, term_index.size - 1);
        }
    }
}

void write_inverted_index(MyVector<TermInfo>& term_index) {
    qsort(term_index.data, term_index.size, sizeof(TermInfo), compare_terminfo);

    FILE* dict_file = fopen("dictionary.dat", "wb");
    FILE* post_file = fopen("postings.dat", "wb");
    
    unsigned long long total_terms = term_index.size;
    fwrite(&total_terms, sizeof(total_terms), 1, dict_file);

    unsigned long long current_postings_offset = 0;
    for (size_t i = 0; i < term_index.size; ++i) {
        TermInfo* info = &term_index.data[i];
        
        unsigned char term_len = strlen(info->term);
        fwrite(&term_len, sizeof(term_len), 1, dict_file);
        fwrite(info->term, term_len, 1, dict_file);
        
        unsigned int doc_freq = info->postings.size;
        fwrite(&doc_freq, sizeof(doc_freq), 1, dict_file);
        fwrite(&current_postings_offset, sizeof(current_postings_offset), 1, dict_file);

        unsigned int last_doc_id = 0;
        for (size_t j = 0; j < info->postings.size; ++j) {
            unsigned int d_gap = info->postings.data[j] - last_doc_id;
            fwrite(&d_gap, sizeof(d_gap), 1, post_file);
            last_doc_id = info->postings.data[j];
        }
        current_postings_offset += info->postings.size * sizeof(unsigned int);
    }
    
    fclose(dict_file);
    fclose(post_file);
}

void build_and_write_forward_index(const char* doc_info_path) {
    MyVector<DocInfo> fwd_index;
    fwd_index.init();

    FILE* f = fopen(doc_info_path, "r");
    char line[8192];
    while(fgets(line, sizeof(line), f)) {
        strtok(line, "\t");
        char* url = strtok(NULL, "\t");
        char* title = strtok(NULL, "\n");
        
        DocInfo info;
        info.url = strdup(url ? url : "");
        info.title = strdup(title ? title : "");
        fwd_index.push_back(info);
    }
    fclose(f);

    FILE* fwd_file = fopen("forward.idx", "wb");
    unsigned long long total_docs = fwd_index.size;
    fwrite(&total_docs, sizeof(total_docs), 1, fwd_file);

    unsigned long long data_offset_start = sizeof(total_docs) + total_docs * sizeof(unsigned long long);
    fseek(fwd_file, data_offset_start, SEEK_SET);

    MyVector<unsigned long long> offsets;
    offsets.init();
    unsigned long long current_offset = data_offset_start;

    for (size_t i = 0; i < fwd_index.size; ++i) {
        offsets.push_back(current_offset);
        unsigned short url_len = strlen(fwd_index.data[i].url);
        unsigned short title_len = strlen(fwd_index.data[i].title);

        fwrite(&url_len, sizeof(url_len), 1, fwd_file);
        fwrite(fwd_index.data[i].url, url_len, 1, fwd_file);
        fwrite(&title_len, sizeof(title_len), 1, fwd_file);
        fwrite(fwd_index.data[i].title, title_len, 1, fwd_file);
        
        current_offset += sizeof(url_len) + url_len + sizeof(title_len) + title_len;
    }
    
    fseek(fwd_file, sizeof(total_docs), SEEK_SET);
    fwrite(offsets.data, sizeof(unsigned long long), offsets.size, fwd_file);
    
    fclose(fwd_file);
}


int main() {
    clock_t start = clock();
    
    MyVector<TermInfo> term_index;
    term_index.init();
    
    fprintf(stderr, "Построение инвертированного индекса в памяти...\n");
    build_inverted_index(term_index);

    fprintf(stderr, "Запись инвертированного индекса на диск...\n");
    write_inverted_index(term_index);

    fprintf(stderr, "Построение и запись прямого индекса...\n");
    build_and_write_forward_index("doc_info.tsv");

    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    size_t total_term_length = 0;
    for (size_t i = 0; i < term_index.size; ++i) {
        total_term_length += strlen(term_index.data[i].term);
    }
    double avg_term_length = term_index.size > 0 ? (double)total_term_length / term_index.size : 0.0;

    double docs_per_second = elapsed_time > 0.0 ? (double)30000 / elapsed_time : 0.0;

    fprintf(stderr, "Индексирование завершено!\n");
    fprintf(stderr, "Всего уникальных термов: %zu\n", term_index.size);
    fprintf(stderr, "Средняя длина терма: %.2f символов\n", avg_term_length);
    fprintf(stderr, "Общее время: %.2f секунд\n", elapsed_time);
    fprintf(stderr, "Средняя скорость индексации: %.2f документов/сек\n", docs_per_second);

    return 0;
}