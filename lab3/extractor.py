import sys
from pymongo import MongoClient
from bs4 import BeautifulSoup

MONGO_HOST = "localhost"
MONGO_PORT = 27017
DB_NAME = "cyberleninka_db"
COLLECTION_NAME = "documents"

def extract_text():
    """
    Извлекает чистый текст из HTML документов в MongoDB и выводит его в stdout.
    """
    try:
        client = MongoClient(MONGO_HOST, MONGO_PORT)
        db = client[DB_NAME]
        collection = db[COLLECTION_NAME]
        
        for doc in collection.find():
            html_content = doc.get('html_content')
            if not html_content:
                continue
            
            soup = BeautifulSoup(html_content, 'lxml')
            
            content_div = soup.find('div', class_='ocr')
            if not content_div:
                content_div = soup.find('article')

            if content_div:
                text = content_div.get_text(separator=' ', strip=True)
                print(text)
                
    except Exception as e:
        print(f"Ошибка при работе с MongoDB: {e}", file=sys.stderr)
    finally:
        if 'client' in locals():
            client.close()

if __name__ == "__main__":
    extract_text()