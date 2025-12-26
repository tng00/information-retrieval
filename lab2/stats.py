import sys
from pymongo import MongoClient
from bs4 import BeautifulSoup

MONGO_HOST = "localhost"
MONGO_PORT = 27017
DB_NAME = "cyberleninka_db"
COLLECTION_NAME = "documents"

def calculate_statistics():
    """
    Извлекает чистый текст из HTML документов в MongoDB и выводит его в stdout.
    """
    try:
        client = MongoClient(MONGO_HOST, MONGO_PORT)
        db = client[DB_NAME]
        collection = db[COLLECTION_NAME]
        num_docs = collection.count_documents({})
        raw_total_size = 0
        clean_total_size = 0
        
        for doc in collection.find():
            html_content = doc.get('html_content')  
            raw_total_size += len(html_content)  

            soup = BeautifulSoup(html_content, 'html.parser')
            article_body_tag = soup.find(itemprop='articleBody')
            article_text = article_body_tag.get_text(separator='\n', strip=True)[1:] if article_body_tag else "No Body"
            clean_total_size += len(article_text)
        
        print("\n======================================")
        print("    СТАТИСТИКА КОРПУСА ДОКУМЕНТОВ")
        print("======================================")
        print(" Источник:                  cyberleninka.ru")
        print(f" Количество документов:     {num_docs}")
        print(f" Общий размер 'сырых' HTML: {raw_total_size / 1024 / 1024:.2f} MB")
        print(f" Общий размер текста (TXT): {clean_total_size / 1024 / 1024:.2f} MB")
        print("--------------------------------------")
        print(f" Средний размер 'сырого' документа: {raw_total_size / num_docs / 1024:.2f} KB")
        print(f" Средний объём текста в документе:  {clean_total_size / num_docs / 1024:.2f} KB")
        print("======================================")

    except Exception as e:
        print(f"Ошибка при работе с MongoDB: {e}", file=sys.stderr)
    finally:
        if 'client' in locals():
            client.close()

if __name__ == "__main__":
    calculate_statistics()