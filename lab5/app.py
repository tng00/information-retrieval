import subprocess
from flask import Flask, render_template, request
from markupsafe import Markup
import re
import time


app = Flask(__name__)
SEARCHER_PATH = "./searcher"
TOKENIZER_PATH = "../lab4/tokenizer"

def preprocess_query(query):
    tokens = re.split(r'(&&|\|\||[()!])', query)
    
    processed_parts = []
    for token in tokens:
        token = token.strip()
        if not token:
            continue

        if token in ['&&', '||', '!', '(', ')']:
            processed_parts.append(token)
        else:
            try:
                proc = subprocess.run(
                    [TOKENIZER_PATH], 
                    input=f"0\t{token}", 
                    capture_output=True, text=True, encoding='utf-8', check=True
                )
                stemmed_terms = [line.split('\t')[0] for line in proc.stdout.strip().split('\n') if line]
                if stemmed_terms:
                    processed_parts.append(' '.join(stemmed_terms))
            except (subprocess.CalledProcessError, FileNotFoundError):
                processed_parts.append(token.lower())

    return ' '.join(processed_parts)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/search')
def search():
    start_time = time.time()
    query = request.args.get('q', '')
    page = request.args.get('p', 1, type=int)
    
    if not query:
        return render_template('results.html', results=[], query='', page=page, total_pages=0)

    processed_query = preprocess_query(query)
    if not processed_query:
        return render_template('results.html', results=[], query=query, page=page, total_pages=0, error="Плохой запрос.")
    print(processed_query)
    try:
        cmd = [SEARCHER_PATH] + processed_query.split()
        process = subprocess.run(
            cmd, capture_output=True, text=True, encoding='utf-8', check=True
        )
        doc_ids_str = process.stdout.strip().split('\n')
        doc_ids = [int(id) for id in doc_ids_str if id.isdigit()]
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        return render_template('results.html', results=[], query=query, page=page, total_pages=0, error=f"Ошибка поискового движка: {e}")
    
    per_page = 50
    start = (page - 1) * per_page
    end = start + per_page
    paginated_ids = doc_ids[start:end]
    total_pages = (len(doc_ids) + per_page - 1) // per_page

    results = []
    if paginated_ids:
        cmd_lookup = [SEARCHER_PATH, '--lookup'] + [str(id) for id in paginated_ids]
        process_info = subprocess.run(cmd_lookup, capture_output=True, text=True, encoding='utf-8')
        
        parsed_results = {}
        for line in process_info.stdout.strip().split('\n'):
            if not line:
                continue
            parts = line.split('\t', 2)
            if len(parts) == 3:
                parsed_results[int(parts[0])] = {'url': parts[1], 'title': parts[2]}
        
        for doc_id in paginated_ids:
            if doc_id in parsed_results:
                results.append(parsed_results[doc_id])

    total_time = (time.time() - start_time) * 1000
    print(f"Время выполнения: {total_time:.2f}мс")


    return render_template('results.html', results=results, query=Markup.escape(query), page=page, total_pages=total_pages, found_count=len(doc_ids))

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')