import requests
from bs4 import BeautifulSoup
import time
import os
import random
import json
from config import BASE_URL, SECTIONS, TOTAL_DOCS_TO_FETCH, MAX_PAGES_PER_SECTION, HEADERS, LINKS_FILE, PROGRESS_FILE

def load_state():
    """Загружает сохраненное состояние прогресса и ссылок."""
    current_progress = SECTIONS.copy()
    if os.path.exists(PROGRESS_FILE):
        try:
            with open(PROGRESS_FILE, 'r', encoding='utf-8') as f:
                current_progress.update(json.load(f))
        except Exception:
            pass
    
    unique_links = set()
    if os.path.exists(LINKS_FILE):
        with open(LINKS_FILE, 'r', encoding='utf-8') as f:
            unique_links = {line.strip() for line in f}
            
    return current_progress, unique_links

def save_state(current_progress, unique_links):
    """Сохраняет текущее состояние прогресса и ссылок."""
    with open(PROGRESS_FILE, 'w', encoding='utf-8') as f:
        json.dump(current_progress, f)
    with open(LINKS_FILE, 'w', encoding='utf-8') as f:
        f.write('\n'.join(unique_links))

def discover_article_links():
    """
    Основная функция для сбора ссылок. Проходит по страницам разделов,
    собирает ссылки на статьи и сохраняет их.
    """
    print("Сбор ссылок на статьи:")
    progress, unique_links = load_state()
    section_keys = list(progress.keys())
    
    while len(unique_links) < TOTAL_DOCS_TO_FETCH:
        all_sections_finished = True
        for section in section_keys:
            page_num = progress.get(section, 1)

            if page_num > MAX_PAGES_PER_SECTION:
                continue
            
            all_sections_finished = False
            page_url = f"{BASE_URL}{section}/{page_num}"
            
            print(f"\n[{len(unique_links)}/{TOTAL_DOCS_TO_FETCH}] Секция: {section.split('/')[-1]}, Страница: {page_num}")
            
            try:
                time.sleep(random.uniform(3.0, 7.0))
                response = requests.get(page_url, headers=HEADERS, timeout=20)
                response.raise_for_status()

                soup = BeautifulSoup(response.text, 'html.parser')
                page_links = {a['href'] for a in soup.find_all('a', href=True) if a['href'].startswith('/article/n/')}
                
                new_links_count = len(page_links - unique_links)
                unique_links.update(page_links)
                
                print(f"Найдено {new_links_count} новых ссылок. Всего уникальных: {len(unique_links)}")
                
                progress[section] = page_num + 1
                save_state(progress, unique_links)
                
                if len(unique_links) >= TOTAL_DOCS_TO_FETCH:
                    break

            except requests.RequestException as e:
                print(f"Ошибка при доступе к странице {page_url}: {e}")
                time.sleep(10)
        
        if all_sections_finished:
            print("Все доступные разделы обработаны.")
            break

    print(f"\nСбор ссылок завершен. Найдено {len(unique_links)} ссылок.")
    return list(unique_links)


if __name__ == "__main__":
    discover_article_links()