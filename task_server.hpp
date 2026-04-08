#ifndef TASK_SERVER_HPP
#define TASK_SERVER_HPP

#include <queue>
#include <unordered_map>
#include <vector>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <cstddef>
#include <stdexcept>

template<typename T>
class TaskServer {
public:
    using TaskType = std::packaged_task<T()>;
    using ResultMap = std::unordered_map<size_t, std::shared_future<T>>;

    TaskServer() : running_(false), task_id_counter_(0) {}

    ~TaskServer() {
        stop();
    }

    // Запустить сервер
    void start() {
        if (running_) return;
        running_ = true;
        server_thread_ = std::thread(&TaskServer::server_loop, this);
    }

    // Остановить сервер
    void stop() {
        if (!running_) return;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            running_ = false;
        }
        queue_cond_var_.notify_all();
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    // Добавить задачу, возвращает id задачи
    size_t add_task(TaskType task) {
        std::future<T> fut = task.get_future();
        size_t task_id = ++task_id_counter_;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.push({task_id, std::move(task)});
            // Сохраняем shared_future для возможности многократного доступа
            results_.emplace(task_id, std::shared_future<T>(std::move(fut)));
        }
        queue_cond_var_.notify_one();
        
        return task_id;
    }

    // Получить результат (блокирующий)
    T request_result(size_t task_id) {
        std::shared_future<T> fut;
        {
            std::unique_lock<std::mutex> lock(results_mutex_);
            auto it = results_.find(task_id);
            if (it == results_.end()) {
                throw std::runtime_error("Task ID not found: " + std::to_string(task_id));
            }
            fut = it->second;
        }
        // get() можно вызывать только один раз для обычного future, 
        // но shared_future позволяет многократный доступ
        return fut.get();
    }

    // Проверить готовность результата (неблокирующий)
    bool is_result_ready(size_t task_id) const {
        std::lock_guard<std::mutex> lock(results_mutex_);
        auto it = results_.find(task_id);
        if (it == results_.end()) {
            return false;
        }
        return it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

private:
    void server_loop() {
        while (true) {
            TaskType task;
            size_t task_id;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cond_var_.wait(lock, [this] {
                    return !tasks_.empty() || !running_;
                });
                
                if (!running_ && tasks_.empty()) {
                    break;
                }
                
                if (!tasks_.empty()) {
                    task_id = tasks_.front().first;
                    task = std::move(tasks_.front().second);
                    tasks_.pop();
                } else {
                    continue;
                }
            }
            
            // Выполняем задачу
            try {
                task();
            } catch (...) {
                // Исключение будет сохранено в future
            }
            
            // После выполнения задачи shared_future уже содержит результат
            // Удаляем из результатов после выполнения (опционально)
            {
                std::lock_guard<std::mutex> lock(results_mutex_);
                // Не удаляем сразу, чтобы клиент мог получить результат
                // Клиент сам решит когда удалить
            }
        }
    }

    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<size_t> task_id_counter_;
    
    // Очередь задач: pair<id, task>
    std::queue<std::pair<size_t, TaskType>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cond_var_;
    
    // Контейнер результатов: используем unordered_map для быстрого поиска
    ResultMap results_;
    mutable std::mutex results_mutex_;
};

#endif // TASK_SERVER_HPP
