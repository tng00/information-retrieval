import os
from bs4 import BeautifulSoup
from config import RAW_DATA_DIR, CLEAN_TEXT_DIR

def extract_text_from_html(raw_html_path):
    """Извлекает заголовок и текст статьи из одного HTML-файла."""
    with open(raw_html_path, 'r', encoding='utf-8') as f:
        html_content = f.read()

    soup = BeautifulSoup(html_content, 'html.parser')
    
    title_tag = soup.find(itemprop="headline")
    title = title_tag.get_text(strip=True) if title_tag else "No Title"
    
    article_body_tag = soup.find(itemprop='articleBody')
    
    article_text = article_body_tag.get_text(separator='\n', strip=True)[1:] if article_body_tag else "No Body"

    return f"{title}\n\n{article_text}"

def process_all_html_files():
    """Проходит по всем сырым HTML и извлекает из них чистый текст."""
    print("\nИзвлечение чистого текста из HTML")
    
    if not os.path.exists(RAW_DATA_DIR):
        print(f"Директория {RAW_DATA_DIR} не найдена. Нечего парсить.")
        return
        
    raw_files = [f for f in os.listdir(RAW_DATA_DIR) if f.endswith('.html')]
    total_files = len(raw_files)
    
    if total_files == 0:
        print("Не найдено HTML-файлов для обработки.")
        return

    print(f"Начинаем обработку {total_files} файлов...")
    processed_count = 0
    for i, filename in enumerate(raw_files):
        raw_path = os.path.join(RAW_DATA_DIR, filename)
        clean_path = os.path.join(CLEAN_TEXT_DIR, os.path.splitext(filename)[0] + '.txt')
        
        if os.path.exists(clean_path) and os.path.getsize(clean_path) > 0:
            continue
            
        try:
            clean_content = extract_text_from_html(raw_path)
            with open(clean_path, 'w', encoding='utf-8') as f:
                f.write(clean_content)
            processed_count += 1
            print(f"[{i+1}/{total_files}] Обработан и сохранен: {os.path.basename(clean_path)}")
        except Exception as e:
            print(f"Ошибка при обработке файла {filename}: {e}")
                 
    print(f"\nИзвлечение текста завершено. Обработано {processed_count} новых файлов.")


if __name__ == "__main__":
    process_all_html_files()