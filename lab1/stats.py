import os
from config import RAW_DATA_DIR, CLEAN_TEXT_DIR

def calculate_statistics():
    """Собирает и выводит статистику по корпусу."""
    print("\nСбор статистики по корпусу")
    
    try:
        num_docs = len([f for f in os.listdir(RAW_DATA_DIR) if f.endswith('.html')])
        if num_docs == 0:
            print("Документы не найдены. Статистика не может быть собрана.")
            return
            
        raw_total_size = sum(os.path.getsize(os.path.join(RAW_DATA_DIR, f)) for f in os.listdir(RAW_DATA_DIR))
        clean_total_size = sum(os.path.getsize(os.path.join(CLEAN_TEXT_DIR, f)) for f in os.listdir(CLEAN_TEXT_DIR))
        
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

    except FileNotFoundError:
        print("Директории с данными не найдены. Убедитесь, что предыдущие шаги выполнены.")


if __name__ == "__main__":
    calculate_statistics()