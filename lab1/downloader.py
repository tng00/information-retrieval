import requests
import os
import time
import random
from config import BASE_URL, HEADERS, RAW_DATA_DIR, LINKS_FILE

def download_articles():
    """
    Скачивает HTML-содержимое статей по ссылкам из файла.
    """
    print("\nСкачивание HTML-страниц")
    
    if not os.path.exists(LINKS_FILE):
        print(f"Файл со ссылками {LINKS_FILE} не найден. Запустите сначала сбор ссылок.")
        return

    with open(LINKS_FILE, 'r', encoding='utf-8') as f:
        links = [line.strip() for line in f]
    
    total_links = len(links)
    print(f"Начинаем скачивание {total_links} статей...")
    downloaded_count = 0
    
    for i, link in enumerate(links):
        try:
            filename = link.split('/')[-1] + ".html"
            filepath = os.path.join(RAW_DATA_DIR, filename)
            
            if os.path.exists(filepath):
                continue

            full_url = BASE_URL + link
            time.sleep(random.uniform(2.0, 5.0))
            response = requests.get(full_url, headers=HEADERS, timeout=20)
            response.raise_for_status()

            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(response.text)
            
            downloaded_count += 1
            print(f"[{i+1}/{total_links}] Скачан и сохранен: {filename}")

        except requests.RequestException as e:
            print(f"Ошибка при скачивании {link}: {e}")
    
    print(f"\nСкачивание завершено. Загружено {downloaded_count} новых файлов.")


if __name__ == "__main__":
    download_articles()