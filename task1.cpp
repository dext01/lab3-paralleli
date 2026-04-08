#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>
#include <cmath>
#include <future>

// Тип данных для элементов матрицы
using ValueType = double;

// Храним матрицу в виде плоского массива для лучшей локальности и экономии памяти
// matrix[i * n + j] соответствует элементу [i][j]
class Matrix {
public:
    Matrix(size_t size) : n_(size), data_(size * size) {}
    
    ValueType& operator()(size_t i, size_t j) { return data_[i * n_ + j]; }
    const ValueType& operator()(size_t i, size_t j) const { return data_[i * n_ + j]; }
    
    size_t size() const { return n_; }
    ValueType* data() { return data_.data(); }
    const ValueType* data() const { return data_.data(); }
    
private:
    size_t n_;
    std::vector<ValueType> data_;
};

// Функция инициализации массива (параллельная версия)
void init_array_parallel(std::vector<ValueType>& arr, size_t start, size_t end, unsigned int seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<ValueType> dist(0.0, 1.0);
    
    for (size_t i = start; i < end; ++i) {
        arr[i] = dist(gen);
    }
}

// Функция умножения матрицы на вектор (один поток обрабатывает строки от start до end)
void mat_vec_mult(const Matrix& matrix,
                  const std::vector<ValueType>& vec,
                  std::vector<ValueType>& result,
                  size_t start_row, size_t end_row) {
    size_t n = matrix.size();
    for (size_t i = start_row; i < end_row; ++i) {
        ValueType sum = 0;
        for (size_t j = 0; j < n; ++j) {
            sum += matrix(i, j) * vec[j];
        }
        result[i] = sum;
    }
}

// Многопоточная версия умножения матрицы на вектор
void mat_vec_mult_parallel(const Matrix& matrix,
                           const std::vector<ValueType>& vec,
                           std::vector<ValueType>& result,
                           size_t num_threads) {
    size_t n = matrix.size();
    size_t rows_per_thread = (n + num_threads - 1) / num_threads;
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start_row = t * rows_per_thread;
        size_t end_row = std::min(start_row + rows_per_thread, n);
        
        if (start_row >= n) break;
        
        threads.emplace_back(mat_vec_mult, 
                            std::ref(matrix), std::ref(vec), std::ref(result),
                            start_row, end_row);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

// Асинхронная версия с использованием std::async
void mat_vec_mult_async(const Matrix& matrix,
                        const std::vector<ValueType>& vec,
                        std::vector<ValueType>& result,
                        size_t num_threads) {
    size_t n = matrix.size();
    size_t rows_per_thread = (n + num_threads - 1) / num_threads;
    
    std::vector<std::future<void>> futures;
    
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start_row = t * rows_per_thread;
        size_t end_row = std::min(start_row + rows_per_thread, n);
        
        if (start_row >= n) break;
        
        futures.push_back(std::async(std::launch::async,
                                     mat_vec_mult,
                                     std::ref(matrix), std::ref(vec), std::ref(result),
                                     start_row, end_row));
    }
    
    for (auto& f : futures) {
        f.get();
    }
}

// Однопоточная версия для сравнения
void mat_vec_mult_serial(const Matrix& matrix,
                         const std::vector<ValueType>& vec,
                         std::vector<ValueType>& result) {
    size_t n = matrix.size();
    for (size_t i = 0; i < n; ++i) {
        ValueType sum = 0;
        for (size_t j = 0; j < n; ++j) {
            sum += matrix(i, j) * vec[j];
        }
        result[i] = sum;
    }
}

int main() {
    // Размеры матриц для тестирования - уменьшены для работы с ограниченной памятью
    // 5000x5000 требует ~200MB, 10000x10000 требует ~800MB
    const std::vector<size_t> matrix_sizes = {2000, 4000};
    
    // Количество потоков для тестирования
    const std::vector<size_t> thread_counts = {1, 2, 4, 7, 8, 16, 20, 40};
    
    std::cout << std::fixed << std::setprecision(6);
    
    for (size_t n : matrix_sizes) {
        std::cout << "\n========================================\n";
        std::cout << "Matrix size: " << n << "x" << n << "\n";
        std::cout << "Memory required: " << (n * n * sizeof(ValueType) / (1024.0 * 1024.0)) << " MB\n";
        std::cout << "========================================\n\n";
        
        // Создаем матрицу и векторы
        Matrix matrix(n);
        std::vector<ValueType> vec(n);
        std::vector<ValueType> result(n);
        
        // Параллельная инициализация массивов
        auto init_start = std::chrono::high_resolution_clock::now();
        
        size_t num_init_threads = std::thread::hardware_concurrency();
        if (num_init_threads == 0) num_init_threads = 4;
        
        size_t elements_per_thread = (n * n + num_init_threads - 1) / num_init_threads;
        std::vector<std::thread> init_threads;
        
        // Инициализация матрицы
        for (size_t t = 0; t < num_init_threads; ++t) {
            size_t start_elem = t * elements_per_thread;
            size_t end_elem = std::min(start_elem + elements_per_thread, n * n);
            
            if (start_elem >= n * n) break;
            
            init_threads.emplace_back([&, start_elem, end_elem, t]() {
                std::mt19937 gen(t + 1);
                std::uniform_real_distribution<ValueType> dist(0.0, 1.0);
                
                for (size_t elem = start_elem; elem < end_elem; ++elem) {
                    size_t row = elem / n;
                    size_t col = elem % n;
                    matrix(row, col) = dist(gen);
                }
            });
        }
        
        // Инициализация вектора
        init_array_parallel(vec, 0, n, 100);
        
        for (auto& t : init_threads) {
            t.join();
        }
        
        auto init_end = std::chrono::high_resolution_clock::now();
        double init_time = std::chrono::duration<double>(init_end - init_start).count();
        
        std::cout << "Initialization time (parallel, " << num_init_threads << " threads): " 
                  << init_time << " seconds\n\n";
        
        // Замер времени для однопоточной версии
        std::fill(result.begin(), result.end(), 0);
        auto serial_start = std::chrono::high_resolution_clock::now();
        mat_vec_mult_serial(matrix, vec, result);
        auto serial_end = std::chrono::high_resolution_clock::now();
        double serial_time = std::chrono::duration<double>(serial_end - serial_start).count();
        
        std::cout << "Serial time: " << serial_time << " seconds\n\n";
        
        // Таблица результатов
        std::cout << "Threads\tTime (s)\tSpeedup\n";
        std::cout << "-------\t--------\t-------\n";
        
        std::cout << "1\t" << serial_time << "\t1.00\n";
        
        // Тестирование с разным количеством потоков
        for (size_t num_threads : thread_counts) {
            if (num_threads == 1) continue; // Уже измерили
            
            std::fill(result.begin(), result.end(), 0);
            auto parallel_start = std::chrono::high_resolution_clock::now();
            mat_vec_mult_parallel(matrix, vec, result, num_threads);
            auto parallel_end = std::chrono::high_resolution_clock::now();
            double parallel_time = std::chrono::duration<double>(parallel_end - parallel_start).count();
            
            double speedup = serial_time / parallel_time;
            
            std::cout << num_threads << "\t" << parallel_time << "\t" << speedup << "\n";
        }
        
        // Тестирование async версии
        std::cout << "\nAsync version:\n";
        std::cout << "Threads\tTime (s)\tSpeedup\n";
        std::cout << "-------\t--------\t-------\n";
        
        for (size_t num_threads : thread_counts) {
            std::fill(result.begin(), result.end(), 0);
            auto async_start = std::chrono::high_resolution_clock::now();
            mat_vec_mult_async(matrix, vec, result, num_threads);
            auto async_end = std::chrono::high_resolution_clock::now();
            double async_time = std::chrono::duration<double>(async_end - async_start).count();
            
            double speedup = serial_time / async_time;
            
            std::cout << num_threads << "\t" << async_time << "\t" << speedup << "\n";
        }
    }
    
    return 0;
}
