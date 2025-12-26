import sys
from pymongo import MongoClient
from bs4 import BeautifulSoup

MONGO_HOST = "localhost"
MONGO_PORT = 27017
DB_NAME = "cyberleninka_db"
COLLECTION_NAME = "documents"
DOC_INFO_FILE = "doc_info.tsv"

def main():
    try:
        client = MongoClient(MONGO_HOST, MONGO_PORT)
        db = client[DB_NAME]
        collection = db[COLLECTION_NAME]
        
        with open(DOC_INFO_FILE, "w", encoding='utf-8') as f_doc_info:
            doc_id_counter = 0
            
            print("Извлечение данных из MongoDB...", file=sys.stderr)
            for doc in collection.find({}, {"url": 1, "html_content": 1}):
                html = doc.get('html_content')
                url = doc.get('url')
                if not html or not url:
                    continue

                soup = BeautifulSoup(html, 'lxml')
                title_tag = soup.find(itemprop="headline")
                title = title_tag.get_text(strip=True).replace('\t', ' ').replace('\n', ' ') if title_tag else "No Title"
                
                content_div = soup.find('div', class_='ocr')
                text = content_div.get_text(separator=' ', strip=True) if content_div else ""

                f_doc_info.write(f"{doc_id_counter}\t{url}\t{title}\n")
                
                print(f"{doc_id_counter}\t{text}")
                
                doc_id_counter += 1
                if doc_id_counter % 1000 == 0:
                    print(f"...извлечено {doc_id_counter} документов.", file=sys.stderr)

            print(f"Извление завершено. Всего документов: {doc_id_counter}", file=sys.stderr)
            
    except Exception as e:
        print(f"Ошибка preparator.py: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()