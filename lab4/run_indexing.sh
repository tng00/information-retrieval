#!/bin/bash

source venv/bin/activate

echo "Компиляция токенизатора C++..."
g++ -O2 -o tokenizer tokenizer.cpp
if [ $? -ne 0 ]; then echo "Ошибка компиляции токенизатора!"; exit 1; fi


echo "Компиляция индексатора C++..."
g++ -O2 -o indexer indexer.cpp
if [ $? -ne 0 ]; then echo "Ошибка компиляции индексатора!"; exit 1; fi


echo "Запуск полного конвейера индексации..."
time python3 preparator.py | ./tokenizer | ./indexer


echo "Конвейер завершён."
echo "Созданные файлы: doc_info.tsv, dictionary.dat, postings.dat, forward.idx"
