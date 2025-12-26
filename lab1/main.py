import os
from config import RAW_DATA_DIR, CLEAN_TEXT_DIR, LINKS_FILE, TOTAL_DOCS_TO_FETCH
from discover import discover_article_links
from downloader import download_articles
from parser import process_all_html_files
from stats import calculate_statistics

def setup_directories():
    """Создает необходимые для работы директории."""
    for dir_path in [RAW_DATA_DIR, CLEAN_TEXT_DIR]:
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)
            print(f"Создана директория: {dir_path}")

def main():
    """
    Главная функция, запускающая все этапы обработки.
    """
    setup_directories()

    num_existing_links = 0
    if os.path.exists(LINKS_FILE):
        with open(LINKS_FILE, 'r', encoding='utf-8') as f:
            num_existing_links = len(f.readlines())

    if num_existing_links < TOTAL_DOCS_TO_FETCH:
        discover_article_links()
    else:
        print(f"Пропущен. Уже собрано {num_existing_links} ссылок.")

    download_articles()
    
    process_all_html_files()
    
    calculate_statistics()
    
    print("\nРабота завершена.")


if __name__ == "__main__":
    main()