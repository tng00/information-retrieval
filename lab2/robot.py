import argparse
import re
import time
import sys
from urllib.parse import urljoin, urlparse

import requests
import yaml
from bs4 import BeautifulSoup
from pymongo import MongoClient, errors
from pymongo.collection import Collection

class Crawler:
    def __init__(self, config: dict):
        self.config = config
        
        try:
            self.mongo_client = MongoClient(config['db']['host'], config['db']['port'])
            self.db = self.mongo_client[config['db']['db_name']]
            self.docs_collection: Collection = self.db[config['db']['docs_collection']]
            self.queue_collection: Collection = self.db[config['db']['queue_collection']]
        except errors.ConnectionFailure as e:
            print(f"Не удалось подключиться к MongoDB: {e}")
            sys.exit(1)

        self.docs_collection.create_index("url", unique=True)
        self.queue_collection.create_index("url", unique=True)
        
        self.user_agent = config['logic']['user_agent']
        self.delay = config['logic']['delay_sec']
        

    def _seed_queue(self):
        seed_urls = self.config['crawler']['seed_urls']
        for url in seed_urls:
            self.queue_collection.update_one({'url': url}, {'$setOnInsert': {'url': url, 'status': 'new'}}, upsert=True)
        print(f"Очередь заполнена {len(seed_urls)} начальными URL.")

    def _get_next_url_from_queue(self) -> dict | None:
        task = self.queue_collection.find_one_and_update({'status': {'$in': ['new']}}, {'$set': {'status': 'processing'}})
        return task

    def _fetch_page(self, url: str, headers: dict = None) -> requests.Response | None:
        if headers is None:
            headers = {}
        headers['User-Agent'] = self.user_agent
        try:
            response = requests.get(url, headers=headers, timeout=10)
            if response.status_code not in (200, 304):
                response.raise_for_status()
            return response
        except requests.RequestException as e:
            print(f"Ошибка при скачивании {url}: {e}")
            return None

    @staticmethod
    def _extract_links(base_url: str, html_content: str) -> set:
        soup = BeautifulSoup(html_content, 'lxml')
        links = set()
        for a_tag in soup.find_all('a', href=True):
            if a_tag['href'].startswith('/article/'):
                absolute_url = urljoin(base_url, a_tag['href'])
                absolute_url = urlparse(absolute_url)._replace(query="", fragment="").geturl()
                links.add(absolute_url)
        return links

    def _filter_urls(self, urls: set) -> list:
        filtered = []
        allowed_domains = self.config['crawler']['allowed_domains']
        article_pattern = re.compile(r"https://cyberleninka\.ru/article/n/.+")
        category_pattern = re.compile(r"https://cyberleninka\.ru/article/c/.+")
        for url in urls:
            if urlparse(url).netloc not in allowed_domains:
                continue
            if article_pattern.match(url) or category_pattern.match(url):
                filtered.append(url)
        return filtered

    def _add_urls_to_queue(self, urls: list):
        if not urls:
            return
        new_urls_count = 0
        for url in urls:
            result = self.queue_collection.update_one({'url': url}, {'$setOnInsert': {'url': url, 'status': 'new'}}, upsert=True)
            if result.upserted_id:
                new_urls_count += 1
        if new_urls_count > 0:
            print(f"Добавлено {new_urls_count} новых URL в очередь.")

    def run(self):
        """Основной цикл работы робота."""
        print("Запуск поискового робота...")
        self._seed_queue()

        article_pattern = re.compile(r"https://cyberleninka\.ru/article/n/.+")
        task = None

        try:
            while True:
                task = self._get_next_url_from_queue()

                if not task:
                    print("Очередь пуста. Ожидание 10 секунд...")
                    time.sleep(10)
                    continue

                url = task['url']
                print(f"Обработка: {url}")
                
                is_article_page = bool(article_pattern.match(url))
                existing_doc = self.docs_collection.find_one({'url': url})
                request_headers = {}
                if is_article_page and existing_doc:
                    continue
                if is_article_page and existing_doc and 'http_headers' in existing_doc:
                    if 'ETag' in existing_doc['http_headers']:
                        request_headers['If-None-Match'] = existing_doc['http_headers']['ETag']
                    if 'Last-Modified' in existing_doc['http_headers']:
                        request_headers['If-Modified-Since'] = existing_doc['http_headers']['Last-Modified']

                response = self._fetch_page(url, headers=request_headers)

                if not response:
                    self.queue_collection.update_one({'_id': task['_id']}, {'$set': {'status': 'failed'}})
                    continue
                
                if "Вы точно человек?" in response.text:
                    print("Требуется ввод капчи.")
                    print(response.text)
                    break

                if "articleBody" not in response.text:
                    print(f"Страница не содержит текст статьи: {url}. Пропускаем.")
                    self.queue_collection.update_one({'_id': task['_id']}, {'$set': {'status': 'done'}})
                    time.sleep(self.delay)
                    continue
                
                if response.status_code == 304:
                    print(f"Страница не изменилась: {url}. Пропускаем.")
                    self.queue_collection.update_one({'_id': task['_id']}, {'$set': {'status': 'done'}})
                    time.sleep(self.delay)
                    continue


                if is_article_page:
                    response_headers = {'ETag': response.headers.get('ETag'), 'Last-Modified': response.headers.get('Last-Modified')}
                    doc_data = {'url': url, 'html_content': response.text, 'source_name': urlparse(url).netloc, 'crawled_at': int(time.time()), 'http_headers': response_headers}
                    self.docs_collection.update_one({'url': url}, {'$set': doc_data}, upsert=True)
                    print(f"Сохранен/обновлен документ (статья): {url}")
                else:
                    print(f"Обработана страница-список: {url}")

                links = self._extract_links(url, response.text)
                filtered_links = self._filter_urls(links)
                self._add_urls_to_queue(filtered_links)
                
                self.queue_collection.update_one({'_id': task['_id']}, {'$set': {'status': 'done'}})

                time.sleep(self.delay)

        except KeyboardInterrupt:
            print("\nПолучен сигнал прерывания (Ctrl+C). Завершение работы...")
            if task:
                self.queue_collection.update_one({'_id': task['_id']}, {'$set': {'status': 'new'}})
                print(f"URL {task['url']} возвращен в очередь.")
        finally:
            self.mongo_client.close()
            print("Соединение с БД закрыто. Робот остановлен.")

def main():
    parser = argparse.ArgumentParser(description="Поисковый робот для курса 'Информационный поиск'")
    parser.add_argument('config', type=str, help="Путь до YAML-файла с конфигурацией")
    args = parser.parse_args()
    try:
        with open(args.config, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
    except Exception as e:
        print(f"Ошибка при чтении или парсинге конфига: {e}")
        sys.exit(1)
    crawler = Crawler(config)
    crawler.run()



if __name__ == '__main__':
    main()