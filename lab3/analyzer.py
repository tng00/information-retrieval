import matplotlib.pyplot as plt

def plot_zipf(filename="frequencies.txt"):
    """
    Читает файл с частотами и строит график распределения по закону Ципфа.
    """
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Ошибка: файл {filename} не найден. Запустите сначала ./run_analysis.sh")
        return

    ranks = []
    frequencies = []
    
    for rank, line in enumerate(lines, 1):
        parts = line.strip().split()
        if len(parts) == 2:
            freq, _ = parts
            ranks.append(rank)
            frequencies.append(int(freq))

    if not frequencies:
        print("Нет данных для построения графика.")
        return

    C = frequencies[0]
    zipf_line = [C / r for r in ranks]

    plt.figure(figsize=(10, 6))
    
    plt.loglog(ranks, frequencies, label='Эмпирическое распределение', marker='.', linestyle='none')
    plt.loglog(ranks, zipf_line, label=f'Идеальный закон Ципфа (C={C})', color='red', linestyle='--')
    
    plt.title('Распределение частот токенов (Закон Ципфа)')
    plt.xlabel('Ранг (log scale)')
    plt.ylabel('Частота (log scale)')
    plt.grid(True, which="both", ls="--")
    plt.legend()
    
    plt.savefig('zipf_plot.png')
    print("График сохранен в файл zipf_plot.png")
    
    plt.show()


if __name__ == "__main__":
    plot_zipf()