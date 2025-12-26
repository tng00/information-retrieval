BASE_URL = "https://cyberleninka.ru"

SECTIONS = {
    "/article/c/clinical-medicine": 1,
    "/article/c/health-sciences": 1
}
TOTAL_DOCS_TO_FETCH = 100
MAX_PAGES_PER_SECTION = 50

HEADERS = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.93 Safari/537.36',
    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9',
    'Accept-Language': 'ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7',
}

RAW_DATA_DIR = 'raw_data'
CLEAN_TEXT_DIR = 'clean_text'
LINKS_FILE = 'article_links.txt'
PROGRESS_FILE = 'crawler_progress.json'