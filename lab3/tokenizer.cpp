#include <stdio.h>
#include <time.h>

#define MAX_TOKEN_LEN 256

enum State {
    IN_WORD,
    OUT_OF_WORD
};


int strcmp_unsigned(const unsigned char* s1, const char* s2) {
    while (*s1 != '\0' && *s2 != '\0' && *s1 == (unsigned char)*s2) {
        s1++;
        s2++;
    }
    return *s1 - (unsigned char)*s2;
}



bool is_valid_token(const unsigned char* token, int len) {
    if (len == 0) return false;
    bool all_digits = true;
    bool all_hyphens = true;
    for (int i = 0; i < len; ++i) {
        if (token[i] < '0' || token[i] > '9') all_digits = false;
        if (token[i] != '-') all_hyphens = false;
    }
    if (all_digits || all_hyphens) return false;
    if (token[0] == '-' || token[len - 1] == '-') return false;
    int char_count = 0;
    for(int i = 0; i < len; ++i) {
        if ((token[i] & 0xC0) != 0x80) char_count++;
    }
    if (char_count < 2) return false;
    return true;
}


bool is_word_char(unsigned char c1) {
    if ((c1 >= 'a' && c1 <= 'z') || (c1 >= 'A' && c1 <= 'Z') || (c1 >= '0' && c1 <= '9') || c1 == '-') {
        return true;
    }
    if (c1 == 0xD0 || c1 == 0xD1) {
        return true;
    }
    return false;
}

void to_lower_utf8(unsigned char* token_buffer, int len) {
    for (int i = 0; i < len; ++i) {
        if (token_buffer[i] >= 'A' && token_buffer[i] <= 'Z') {
            token_buffer[i] += 32;
        } else if (token_buffer[i] == 0xD0 && i + 1 < len) {
            if (token_buffer[i+1] >= 0x90 && token_buffer[i+1] <= 0x9F) {
                token_buffer[i+1] += 0x20;
            } else if (token_buffer[i+1] >= 0xA0 && token_buffer[i+1] <= 0xAF) {
                token_buffer[i] = 0xD1;
                token_buffer[i+1] -= 0x20;
            } else if (token_buffer[i+1] == 0x81) {
                token_buffer[i] = 0xD1;
                token_buffer[i+1] = 0x91;
            }
        }
    }
}

bool is_vowel(const unsigned char* c) {
    if (*c == 0xD0) {
        switch (*(c+1)) {
            case 0xB0: case 0xB5: case 0xB8: case 0xBE: case 0x81: case 0x84: case 0x86: case 0x87: return true;
        }
    } else if (*c == 0xD1) {
         switch (*(c+1)) {
            case 0x91: case 0x94: case 0x99: return true;
        }
    }
    return false;
}

int find_rv(const unsigned char* word, int len) {
    for (int i = 0; i < len - 1; ++i) {
        if (is_vowel(&word[i])) {
            return i + 2;
        }
        if (word[i] == 0xD0 || word[i] == 0xD1) i++;
    }
    return len;
}

int get_suffix_pos(const unsigned char* word, int len, const char* suffix) {
    int suffix_len = 0;
    while(suffix[suffix_len]) suffix_len++;
    if (len < suffix_len) return -1;
    for (int i = 0; i < suffix_len; ++i) {
        if (word[len - suffix_len + i] != (unsigned char)suffix[i]) return -1;
    }
    return len - suffix_len;
}

void remove_suffix(int* len, int suffix_pos) {
    if (suffix_pos != -1) *len = suffix_pos;
}

void stem_word(unsigned char* token, int* len) {
    if (*len == 0) return;
    int rv = find_rv(token, *len);
    int suffix_pos;

    suffix_pos = get_suffix_pos(token, *len, "\xD0\xB2\xD1\x88\xD0\xB8\xD1\x81\xD1\x8C");
    if (suffix_pos == -1) suffix_pos = get_suffix_pos(token, *len, "\xD0\xB2\xD1\x88\xD0\xB8");
    if (suffix_pos == -1) suffix_pos = get_suffix_pos(token, *len, "\xD0\xB2");
    if (suffix_pos != -1 && suffix_pos >= rv) {
        if (get_suffix_pos(token, suffix_pos, "\xD0\xB0") != -1 || get_suffix_pos(token, suffix_pos, "\xD1\x8F") != -1) {
            remove_suffix(len, suffix_pos); rv = find_rv(token, *len);
        }
    }
    
    const char* ADJECTIVAL[] = { "\xD0\xB5\xD0\xB5", "\xD0\xB8\xD0\xB5", "\xD1\x8B\xD0\xB5", "\xD0\xBE\xD0\xB5", "\xD0\xB5\xD0\xB3\xD0\xBE", "\xD0\xBE\xD0\xB3\xD0\xBE", "\xD0\xB5\xD0\xBC\xD1\x83", "\xD0\xBE\xD0\xBC\xD1\x83", "\xD0\xB8\xD1\x85", "\xD1\x8B\xD1\x85", "\xD0\xB8\xD0\xBC", "\xD1\x8B\xD0\xBC", "\xD0\xB5\xD0\xB9", "\xD0\xBE\xD0\xB9", "\xD1\x83\xD1\x8E", "\xD0\xB0\xD1\x8F", "\xD1\x8F\xD1\x8F", "\xD0\xBE\xD1\x8E", "\xD0\xB5\xD1\x8E", "\xD0\xB8\xD0\xBC\xD0\xB8", "\xD1\x8B\xD0\xBC\xD0\xB8", "\xD1\x8B\xD0\xB9", "\xD0\xB8\xD0\xB9", "\xD0\xBE\xD0\xB9", NULL };
    for(int i = 0; ADJECTIVAL[i]; ++i) {
        suffix_pos = get_suffix_pos(token, *len, ADJECTIVAL[i]);
        if (suffix_pos != -1 && suffix_pos >= rv) {
            remove_suffix(len, suffix_pos);
            const char* PARTICIPLE[] = { "\xD0\xB2\xD1\x88", "\xD1\x8E\xD1\x89", "\xD1\x89", NULL };
            for(int j = 0; PARTICIPLE[j]; ++j) {
                int part_pos = get_suffix_pos(token, *len, PARTICIPLE[j]);
                if (part_pos != -1 && part_pos >= rv) {
                     if (get_suffix_pos(token, part_pos, "\xD0\xB0") != -1 || get_suffix_pos(token, part_pos, "\xD1\x8F") != -1) {
                         remove_suffix(len, part_pos);
                     }
                }
            }
            rv = find_rv(token, *len); goto step3;
        }
    }

step3:
    const char* VERB[] = { "\xD0\xBB\xD0\xB0", "\xD0\xBB\xD0\xB8", "\xD0\xBB\xD0\xBE", "\xD0\xBB", "\xD0\xB5\xD1\x82", "\xD1\x83\xD0\xB5\xD1\x82", "\xD0\xB8\xD1\x82", "\xD1\x8B\xD0\xBB", "\xD0\xB5\xD0\xB9\xD1\x82\xD0\xB5", "\xD0\xB8\xD1\x82\xD0\xB5", "\xD1\x82\xD1\x8C", "\xD1\x82\xD1\x8C\xD1\x81\xD1\x8F", NULL };
    for(int i = 0; VERB[i]; ++i) {
        suffix_pos = get_suffix_pos(token, *len, VERB[i]);
        if (suffix_pos != -1 && suffix_pos >= rv) {
            if (get_suffix_pos(token, suffix_pos, "\xD0\xB0") != -1 || get_suffix_pos(token, suffix_pos, "\xD1\x8F") != -1) {
                remove_suffix(len, suffix_pos); rv = find_rv(token, *len); goto step4;
            }
        }
    }

step4:
    const char* NOUN[] = { "\xD0\xB0", "\xD1\x8F", "\xD0\xB5\xD0\xB2", "\xD0\xB5\xD0\xB9", "\xD0\xB8", "\xD0\xB5\xD0\xB9", "\xD0\xBE\xD0\xB2", "\xD0\xBE\xD0\xBC", "\xD0\xB5\xD0\xBC", "\xD0\xB0\xD0\xBC", "\xD1\x8F\xD0\xBC", "\xD1\x8C", NULL };
    for(int i = 0; NOUN[i]; ++i) {
        suffix_pos = get_suffix_pos(token, *len, NOUN[i]);
        if (suffix_pos != -1 && suffix_pos >= rv) {
            remove_suffix(len, suffix_pos); break;
        }
    }

    suffix_pos = get_suffix_pos(token, *len, "\xD0\xB5\xD0\xB9\xD1\x88");
    if (suffix_pos == -1) suffix_pos = get_suffix_pos(token, *len, "\xD0\xB0\xD0\xB9\xD1\x88");
    if (suffix_pos != -1 && suffix_pos >= rv) remove_suffix(len, suffix_pos);
    
    suffix_pos = get_suffix_pos(token, *len, "\xD0\xBD\xD0\xBD");
    if (suffix_pos != -1 && suffix_pos >= rv) remove_suffix(len, suffix_pos + 2);

    if (*len > 0 && token[*len - 1] == 0x8C && token[*len - 2] == 0xD1) {
       if (*len - 2 >= rv) *len -= 2;
    }
}



int main() {
    State state = OUT_OF_WORD;
    unsigned char token_buffer[MAX_TOKEN_LEN];
    int token_len = 0;
    
    long total_tokens = 0;
    long total_token_length = 0;
    long total_bytes_processed = 0;
    
    clock_t start_time = clock();

    int c;
    while ((c = getchar()) != EOF) {
        total_bytes_processed++;
        unsigned char current_char = (unsigned char)c;
        unsigned char next_char = 0;

        if (is_word_char(current_char)) {
            state = IN_WORD;
            if (current_char == 0xD0 || current_char == 0xD1) {
                next_char = (unsigned char)getchar();
                if (next_char == (unsigned char)EOF) break;
                total_bytes_processed++;
            }
            if (token_len < MAX_TOKEN_LEN - 2) {
                token_buffer[token_len++] = current_char;
                if (next_char != 0) token_buffer[token_len++] = next_char;
            }
        } else {
            if (state == IN_WORD) {
                token_buffer[token_len] = '\0';
                to_lower_utf8(token_buffer, token_len);
                
            
                stem_word(token_buffer, &token_len);
                token_buffer[token_len] = '\0';
            
                if (is_valid_token(token_buffer, token_len)) {
                    printf("%s\n", token_buffer);
                    total_tokens++;
                    total_token_length += token_len;
                }
                
                token_len = 0;
            }
            state = OUT_OF_WORD;
        }
    }
    
    if (state == IN_WORD && token_len > 0) {
        token_buffer[token_len] = '\0';
        to_lower_utf8(token_buffer, token_len);
        stem_word(token_buffer, &token_len);
        token_buffer[token_len] = '\0';
        if (is_valid_token(token_buffer, token_len)) {
            printf("%s\n", token_buffer);
            total_tokens++;
            total_token_length += token_len;
        }
    }
    
    clock_t end_time = clock();
    double elapsed_time_sec = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    double speed_kbs = (elapsed_time_sec > 0) ? (total_bytes_processed / 1024.0) / elapsed_time_sec : 0;
    double avg_len = (total_tokens > 0) ? (double)total_token_length / total_tokens : 0;

    fprintf(stderr, "--- Статистика токенизации ---\n");
    fprintf(stderr, "Количество токенов: %ld\n", total_tokens);
    fprintf(stderr, "Средняя длина токена: %.2f\n", avg_len);
    fprintf(stderr, "Время: %.2f секунд\n", elapsed_time_sec);
    fprintf(stderr, "Объем данных: %.2f кБ\n", total_bytes_processed / 1024.0);
    fprintf(stderr, "Скорость: %.2f кБ/с\n", speed_kbs);

    return 0;
}