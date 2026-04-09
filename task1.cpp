#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cmath>

// Размеры матриц (уменьшены для работы с доступной памятью)
const std::vector<size_t> SIZES = {2000, 4000};
// Количество потоков из задания
const std::vector<int> THREAD_COUNTS = {1, 2, 4, 7, 8, 16, 20, 40};

void parallel_init(std::vector<double>& arr, double start_val, int num_threads) {
    size_t chunk_size = arr.size() / num_threads;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? arr.size() : start + chunk_size;
        threads.emplace_back([&arr, start, end, start_val]() {
            for (size_t j = start; j < end; ++j) {
                arr[j] = start_val + static_cast<double>(j);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

void multiply_matrix_vector_chunk(const std::vector<double>& matrix, const std::vector<double>& vector, 
                                  std::vector<double>& result, size_t row_start, size_t row_end, size_t N) {
    for (size_t i = row_start; i < row_end; ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < N; ++j) {
            sum += matrix[i * N + j] * vector[j];
        }
        result[i] = sum;
    }
}

void multiply_matrix_vector_mt(const std::vector<double>& matrix, const std::vector<double>& vector, 
                               std::vector<double>& result, size_t N, int num_threads) {
    std::vector<std::thread> threads;
    size_t chunk_size = N / num_threads;
    
    for (int i = 0; i < num_threads; ++i) {
        size_t row_start = i * chunk_size;
        size_t row_end = (i == num_threads - 1) ? N : row_start + chunk_size;
        
        threads.emplace_back(multiply_matrix_vector_chunk, 
                             std::ref(matrix), std::ref(vector), std::ref(result), 
                             row_start, row_end, N);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

void run_benchmark(size_t N, int num_threads) {
    std::vector<double> matrix(N * N);
    std::vector<double> vector(N);
    std::vector<double> result(N, 0.0);

    // Параллельная инициализация
    auto t_init_start = std::chrono::high_resolution_clock::now();
    parallel_init(matrix, 1.0, num_threads);
    parallel_init(vector, 1.0, num_threads);
    auto t_init_end = std::chrono::high_resolution_clock::now();
    
    double init_time = std::chrono::duration<double, std::milli>(t_init_end - t_init_start).count();

    // Умножение
    auto t_calc_start = std::chrono::high_resolution_clock::now();
    multiply_matrix_vector_mt(matrix, vector, result, N, num_threads);
    auto t_calc_end = std::chrono::high_resolution_clock::now();
    
    double calc_time = std::chrono::duration<double, std::milli>(t_calc_end - t_calc_start).count();
    double total_time = init_time + calc_time;

    // Вывод в формате CSV: Size,Threads,InitTime,CalcTime,TotalTime
    std::cout << N << "," << num_threads << "," << init_time << "," << calc_time << "," << total_time << std::endl;
}

int main() {
    std::cout << "Size,Threads,InitTime_ms,CalcTime_ms,TotalTime_ms" << std::endl;

    for (size_t N : SIZES) {
        std::cerr << "Processing matrix " << N << "x" << N << "..." << std::endl;
        for (int threads : THREAD_COUNTS) {
            run_benchmark(N, threads);
        }
    }

    return 0;
}
