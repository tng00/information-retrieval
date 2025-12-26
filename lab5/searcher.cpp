#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


template <typename T>
struct MyVector {
    T* data; size_t size; size_t capacity;
    void init() { data = NULL; size = 0; capacity = 0; }
    void free_mem() { free(data); init(); }
    void push_back(const T& value) {
        if (size == capacity) {
            capacity = (capacity == 0) ? 8 : capacity * 2;
            data = (T*)realloc(data, capacity * sizeof(T));
        }
        data[size++] = value;
    }
    T pop_back() { return data[--size]; }
    T& top() { return data[size - 1]; }
};

struct TermInfo {
    char term[256];
    unsigned int doc_freq;
    unsigned long long postings_offset;
};
struct DocInfo { char* url; char* title; };

enum TokenType { WORD, AND, OR, NOT, LPAREN, RPAREN, END };
struct Token { TokenType type; char value[256]; };

MyVector<TermInfo> g_dictionary;
MyVector<DocInfo> g_forward_index;
unsigned long long g_total_docs = 0;
FILE* g_postings_file = NULL;


void load_forward_index() {
    FILE* f = fopen("../lab4/forward.idx", "rb");
    if (!f) { fprintf(stderr, "Ошибка: forward.idx не найден.\n"); exit(1); }
    fread(&g_total_docs, sizeof(g_total_docs), 1, f);
    unsigned long long* offsets = (unsigned long long*)malloc(g_total_docs * sizeof(unsigned long long));
    fread(offsets, sizeof(unsigned long long), g_total_docs, f);
    
    g_forward_index.init();
    for (size_t i = 0; i < g_total_docs; ++i) {
        fseek(f, offsets[i], SEEK_SET);
        unsigned short url_len, title_len;
        fread(&url_len, sizeof(url_len), 1, f);
        char* url = (char*)malloc(url_len + 1);
        fread(url, url_len, 1, f); url[url_len] = '\0';
        
        fread(&title_len, sizeof(title_len), 1, f);
        char* title = (char*)malloc(title_len + 1);
        fread(title, title_len, 1, f); title[title_len] = '\0';

        DocInfo info = {url, title};
        g_forward_index.push_back(info);
    }
    free(offsets);
    fclose(f);
}

void load_dictionary() {
    FILE* f = fopen("../lab4/dictionary.dat", "rb");
    if (!f) { fprintf(stderr, "Ошибка: dictionary.dat не найден.\n"); exit(1); }
    unsigned long long total_terms;
    fread(&total_terms, sizeof(total_terms), 1, f);
    g_dictionary.init();
    for (size_t i = 0; i < total_terms; ++i) {
        TermInfo info;
        unsigned char term_len;
        fread(&term_len, sizeof(term_len), 1, f);
        fread(info.term, term_len, 1, f);
        info.term[term_len] = '\0';
        fread(&info.doc_freq, sizeof(info.doc_freq), 1, f);
        fread(&info.postings_offset, sizeof(info.postings_offset), 1, f);
        g_dictionary.push_back(info);
    }
    fclose(f);
    g_postings_file = fopen("../lab4/postings.dat", "rb");
}

int term_comparator(const void* a, const void* b) {
    return strcmp(((TermInfo*)a)->term, (const char*)b);
}

int find_term_id(const char* term) {
    TermInfo* result = (TermInfo*)bsearch(term, g_dictionary.data, g_dictionary.size, sizeof(TermInfo), 
                                     (int(*)(const void*, const void*))term_comparator);
    if (!result) return -1;
    return result - g_dictionary.data;
}

MyVector<unsigned int> get_postings(int term_id) {
    MyVector<unsigned int> postings; postings.init();
    if (term_id < 0) return postings;
    TermInfo* info = &g_dictionary.data[term_id];
    fseek(g_postings_file, info->postings_offset, SEEK_SET);
    unsigned int last_doc_id = 0;
    for (unsigned int i = 0; i < info->doc_freq; ++i) {
        unsigned int d_gap;
        fread(&d_gap, sizeof(d_gap), 1, g_postings_file);
        unsigned int current_doc_id = last_doc_id + d_gap;
        postings.push_back(current_doc_id);
        last_doc_id = current_doc_id;
    }
    return postings;
}


MyVector<unsigned int> intersect_lists(MyVector<unsigned int>& a, MyVector<unsigned int>& b) {
    MyVector<unsigned int> result; result.init();
    size_t i = 0, j = 0;
    while (i < a.size && j < b.size) {
        if (a.data[i] == b.data[j]) { result.push_back(a.data[i]); i++; j++; } 
        else if (a.data[i] < b.data[j]) { i++; } 
        else { j++; }
    }
    return result;
}

MyVector<unsigned int> unite_lists(MyVector<unsigned int>& a, MyVector<unsigned int>& b) {
    MyVector<unsigned int> result; result.init();
    size_t i = 0, j = 0;
    while (i < a.size || j < b.size) {
        if (i < a.size && (j >= b.size || a.data[i] < b.data[j])) { result.push_back(a.data[i++]); } 
        else if (j < b.size && (i >= a.size || b.data[j] < a.data[i])) { result.push_back(b.data[j++]); }
        else if (i < a.size && j < b.size) { result.push_back(a.data[i]); i++; j++; }
    }
    return result;
}

MyVector<unsigned int> invert_list(MyVector<unsigned int>& a) {
    MyVector<unsigned int> result; result.init();
    size_t ptr_a = 0;
    for (unsigned int doc_id = 0; doc_id < g_total_docs; ++doc_id) {
        if (ptr_a < a.size && a.data[ptr_a] == doc_id) { ptr_a++; } 
        else { result.push_back(doc_id); }
    }
    return result;
}


MyVector<Token> tokenize_query(const char*& p) {
    MyVector<Token> tokens; tokens.init();
    while (*p) {
        while (isspace(*p)) p++;
        if (*p == '\0') break;

        Token t;
        if (*p == '(') { t.type = LPAREN; p++; }
        else if (*p == ')') { t.type = RPAREN; p++; }
        else if (*p == '!') { t.type = NOT; p++; }
        else if (strncmp(p, "&&", 2) == 0) { t.type = AND; p += 2; }
        else if (strncmp(p, "||", 2) == 0) { t.type = OR; p += 2; }
        else {
            t.type = WORD;
            int i = 0;
            while (*p && !isspace(*p) && *p != ')' && *p != '(') {
                if (i < 255) t.value[i++] = *p;
                p++;
            }
            t.value[i] = '\0';
        }
        tokens.push_back(t);
    }
    return tokens;
}

int get_precedence(TokenType type) {
    if (type == NOT) return 3;
    if (type == AND) return 2;
    if (type == OR) return 1;
    return 0;
}

MyVector<Token> shunting_yard(MyVector<Token>& input_tokens) {
    MyVector<Token> output_queue; output_queue.init();
    MyVector<Token> operator_stack; operator_stack.init();

    MyVector<Token> tokens; tokens.init();
    for (size_t i = 0; i < input_tokens.size; ++i) {
        tokens.push_back(input_tokens.data[i]);
        if (i < input_tokens.size - 1) {
            TokenType current = input_tokens.data[i].type;
            TokenType next = input_tokens.data[i+1].type;
            if ((current == WORD || current == RPAREN) && (next == WORD || next == LPAREN || next == NOT)) {
                Token and_token; and_token.type = AND;
                tokens.push_back(and_token);
            }
        }
    }
    
    for (size_t i = 0; i < tokens.size; ++i) {
        Token t = tokens.data[i];
        if (t.type == WORD) {
            output_queue.push_back(t);
        } else if (t.type == AND || t.type == OR || t.type == NOT) {
            while (operator_stack.size > 0 && operator_stack.top().type != LPAREN &&
                   get_precedence(operator_stack.top().type) >= get_precedence(t.type)) {
                output_queue.push_back(operator_stack.pop_back());
            }
            operator_stack.push_back(t);
        } else if (t.type == LPAREN) {
            operator_stack.push_back(t);
        } else if (t.type == RPAREN) {
            while (operator_stack.size > 0 && operator_stack.top().type != LPAREN) {
                output_queue.push_back(operator_stack.pop_back());
            }
            if (operator_stack.size > 0) operator_stack.pop_back();
        }
    }

    while (operator_stack.size > 0) {
        output_queue.push_back(operator_stack.pop_back());
    }

    tokens.free_mem();
    operator_stack.free_mem();
    return output_queue;
}

MyVector<unsigned int> evaluate_rpn(MyVector<Token>& rpn_queue) {
    MyVector<MyVector<unsigned int>> operand_stack; operand_stack.init();

    for (size_t i = 0; i < rpn_queue.size; ++i) {
        Token t = rpn_queue.data[i];
        if (t.type == WORD) {
            operand_stack.push_back(get_postings(find_term_id(t.value)));
        } else if (t.type == NOT) {
            MyVector<unsigned int> operand = operand_stack.pop_back();
            MyVector<unsigned int> result = invert_list(operand);
            operand_stack.push_back(result);
            operand.free_mem();
        } else { // AND, OR
            MyVector<unsigned int> op2 = operand_stack.pop_back();
            MyVector<unsigned int> op1 = operand_stack.pop_back();
            MyVector<unsigned int> result;
            if (t.type == AND) result = intersect_lists(op1, op2);
            else if (t.type == OR) result = unite_lists(op1, op2);
            operand_stack.push_back(result);
            op1.free_mem();
            op2.free_mem();
        }
    }
    
    MyVector<unsigned int> final_result;
    if (operand_stack.size > 0) {
        final_result = operand_stack.pop_back();
    } else {
        final_result.init();
    }
    operand_stack.free_mem();
    return final_result;
}

void execute_search(const char* query) {
    const char* p = query;
    MyVector<Token> tokens = tokenize_query(p);
    MyVector<Token> rpn = shunting_yard(tokens);
    MyVector<unsigned int> result = evaluate_rpn(rpn);

    for(size_t i = 0; i < result.size; ++i) {
        printf("%u\n", result.data[i]);
    }
    
    result.free_mem();
    tokens.free_mem();
    rpn.free_mem();
}


void execute_lookup(int argc, char* argv[]) {
    for (int i = 2; i < argc; ++i) {
        unsigned int doc_id = atoi(argv[i]);
        if (doc_id < g_total_docs) {
            DocInfo* info = &g_forward_index.data[doc_id];
            printf("%u\t%s\t%s\n", doc_id, info->url, info->title);
        }
    }
}

int main(int argc, char* argv[]) {
    load_forward_index();
    load_dictionary();

    if (argc > 1) {
        if (strcmp(argv[1], "--lookup") == 0 && argc > 2) {
            execute_lookup(argc, argv);
        } else {
            char query[4096] = {0};
            for (int i = 1; i < argc; ++i) { 
                strcat(query, argv[i]); 
                if (i < argc - 1) strcat(query, " "); 
            }
            execute_search(query);
        }
    } else {
        char line[4096];
        while(fgets(line, sizeof(line), stdin)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;
            fprintf(stderr, "Запрос: [%s]\n", line);
            execute_search(line);
            fprintf(stderr, "---\n");
        }
    }
    
    fclose(g_postings_file);
    for (size_t i = 0; i < g_forward_index.size; ++i) {
        free(g_forward_index.data[i].url);
        free(g_forward_index.data[i].title);
    }
    g_forward_index.free_mem();
    g_dictionary.free_mem();

    return 0;
}