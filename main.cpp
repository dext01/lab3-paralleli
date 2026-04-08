#include <iostream>
#include <fstream>
#include <cmath>
#include <random>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <future>
#include "task_server.hpp"

// Задача 1: вычисление синуса
struct SinTask {
    double arg;
    double operator()() const {
        return std::sin(arg);
    }
};

// Задача 2: вычисление квадратного корня
struct SqrtTask {
    double arg;
    double operator()() const {
        return std::sqrt(arg);
    }
};

// Задача 3: возведение в степень
struct PowTask {
    double base;
    double exp;
    double operator()() const {
        return std::pow(base, exp);
    }
};

// Клиентский поток
template<typename TaskFunc>
void client_thread(TaskServer<double>& server, int client_id, int num_tasks, 
                   const std::string& output_file, TaskFunc task_generator) {
    std::ofstream out(output_file);
    if (!out.is_open()) {
        std::cerr << "Failed to open file: " << output_file << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::vector<size_t> task_ids;
    
    for (int i = 0; i < num_tasks; ++i) {
        auto task = task_generator(gen);
        size_t task_id = server.add_task(std::move(task));
        task_ids.push_back(task_id);
    }
    
    // Получаем результаты и записываем в файл
    out << std::fixed << std::setprecision(10);
    out << "Client " << client_id << " Results:\n";
    out << "========================\n";
    
    for (size_t i = 0; i < task_ids.size(); ++i) {
        try {
            double result = server.request_result(task_ids[i]);
            out << "Task " << (i + 1) << ": result = " << result << "\n";
        } catch (const std::exception& e) {
            out << "Task " << (i + 1) << ": error = " << e.what() << "\n";
        }
    }
    
    out.close();
    std::cout << "Client " << client_id << " finished, results saved to " << output_file << std::endl;
}

int main() {
    const int NUM_TASKS_PER_CLIENT = 100;  // N = 100 (в диапазоне 5 < N < 10000)
    
    TaskServer<double> server;
    server.start();
    
    std::cout << "Server started\n";
    
    // Генераторы задач для каждого клиента
    auto sin_generator = [](std::mt19937& gen) -> std::packaged_task<double()> {
        std::uniform_real_distribution<> dist(-100.0, 100.0);
        double arg = dist(gen);
        std::packaged_task<double()> task([arg]() { return std::sin(arg); });
        return task;
    };
    
    auto sqrt_generator = [](std::mt19937& gen) -> std::packaged_task<double()> {
        std::uniform_real_distribution<> dist(0.0, 1000.0);
        double arg = dist(gen);
        std::packaged_task<double()> task([arg]() { return std::sqrt(arg); });
        return task;
    };
    
    auto pow_generator = [](std::mt19937& gen) -> std::packaged_task<double()> {
        std::uniform_real_distribution<> base_dist(1.0, 10.0);
        std::uniform_real_distribution<> exp_dist(0.0, 5.0);
        double base = base_dist(gen);
        double exp = exp_dist(gen);
        std::packaged_task<double()> task([base, exp]() { return std::pow(base, exp); });
        return task;
    };
    
    // Создаем три клиентских потока
    std::thread client1(client_thread<decltype(sin_generator)>, 
                        std::ref(server), 1, NUM_TASKS_PER_CLIENT, 
                        "client1_sin.txt", sin_generator);
    
    std::thread client2(client_thread<decltype(sqrt_generator)>, 
                        std::ref(server), 2, NUM_TASKS_PER_CLIENT, 
                        "client2_sqrt.txt", sqrt_generator);
    
    std::thread client3(client_thread<decltype(pow_generator)>, 
                        std::ref(server), 3, NUM_TASKS_PER_CLIENT, 
                        "client3_pow.txt", pow_generator);
    
    // Ждем завершения клиентов
    client1.join();
    client2.join();
    client3.join();
    
    std::cout << "All clients finished\n";
    
    server.stop();
    std::cout << "Server stopped\n";
    
    return 0;
}
