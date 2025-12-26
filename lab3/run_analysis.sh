#!/bin/bash

if [[ -z "$VIRTUAL_ENV" ]]; then
    echo "Активация виртуального окружения..."
    source venv/bin/activate
fi

echo "Компиляция tokenizer.cpp..."
g++ -O2 -o tokenizer tokenizer.cpp
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции!"
    exit 1
fi
echo "Компиляция успешна."
echo

echo "Запуск пайплайна..."
echo

python3 extractor.py | ./tokenizer | sort | uniq -c | sort -nr > frequencies.txt

if [ $? -ne 0 ]; then
    echo "Ошибка пайплайна!"
    exit 1
fi

echo "Пайплайн успешен. Реузльтаты в frequencies.txt."
echo
echo "Топ 10 самых частых токенов:"
head -n 10 frequencies.txt
