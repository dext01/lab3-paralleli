#!/usr/bin/env python3
import matplotlib.pyplot as plt
import sys
import csv

def plot_speedup(csv_file):
    data = {}
    
    # Чтение CSV файла
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            size = int(row['Size'])
            threads = int(row['Threads'])
            calc_time = float(row['CalcTime_ms'])
            
            if size not in data:
                data[size] = {}
            data[size][threads] = calc_time
    
    # Построение графиков
    fig, ax = plt.subplots(figsize=(12, 8))
    
    colors = ['b', 'r', 'g', 'orange']
    
    for i, (size, thread_data) in enumerate(data.items()):
        threads = sorted(thread_data.keys())
        calc_times = [thread_data[t] for t in threads]
        
        # Расчет ускорения относительно 1 потока
        base_time = thread_data[1]
        speedup = [base_time / t for t in calc_times]
        
        # Идеальное ускорение
        ideal_speedup = threads
        
        ax.plot(threads, speedup, marker='o', label=f'Matrix {size}x{size} (Real)', 
                color=colors[i % len(colors)], linewidth=2, markersize=8)
        ax.plot(threads, ideal_speedup, linestyle='--', label=f'Matrix {size}x{size} (Ideal)', 
                color=colors[i % len(colors)], alpha=0.3, linewidth=1)
    
    ax.set_xlabel('Number of Threads', fontsize=14)
    ax.set_ylabel('Speedup', fontsize=14)
    ax.set_title('Matrix-Vector Multiplication Speedup vs Number of Threads', fontsize=16)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=12)
    ax.set_xticks([1, 2, 4, 7, 8, 16, 20, 40])
    
    # Добавление значений на график
    for size, thread_data in data.items():
        threads = sorted(thread_data.keys())
        base_time = thread_data[1]
        speedup = [base_time / thread_data[t] for t in threads]
        for t, s in zip(threads, speedup):
            ax.annotate(f'{s:.2f}', xy=(t, s), textcoords="offset points", 
                       xytext=(0,5), ha='center', fontsize=8)
    
    plt.tight_layout()
    plt.savefig('speedup_graph.png', dpi=300, bbox_inches='tight')
    plt.savefig('speedup_graph.pdf', bbox_inches='tight')
    print("График сохранен в speedup_graph.png и speedup_graph.pdf")
    
    # Вывод таблицы ускорения
    print("\n=== Таблица ускорения ===")
    print(f"{'Size':<8} {'Threads':<8} {'CalcTime(ms)':<14} {'Speedup':<10} {'Efficiency':<12}")
    print("-" * 60)
    
    for size, thread_data in sorted(data.items()):
        for threads in sorted(thread_data.keys()):
            calc_time = thread_data[threads]
            base_time = thread_data[1]
            speedup = base_time / calc_time
            efficiency = (speedup / threads) * 100
            print(f"{size:<8} {threads:<8} {calc_time:<14.2f} {speedup:<10.2f} {efficiency:<12.1f}%")

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Использование: python3 plot_speedup.py <файл_с_данными.csv>")
        print("Пример: python3 plot_speedup.py task1_output.csv")
        sys.exit(1)
    
    plot_speedup(sys.argv[1])
