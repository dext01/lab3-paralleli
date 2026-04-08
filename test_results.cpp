#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <string>

// Тест читает результаты из файлов и проверяет корректность
bool test_sin_results(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    int passed = 0, failed = 0;
    
    while (std::getline(file, line)) {
        // Ищем строки с результатами: "Task X: result = Y"
        size_t pos = line.find("result = ");
        if (pos != std::string::npos) {
            double result = std::stod(line.substr(pos + 9));
            // Проверяем, что результат в допустимом диапазоне для sin [-1, 1]
            if (result >= -1.0 && result <= 1.0) {
                passed++;
            } else {
                std::cerr << "FAIL: sin result out of range: " << result << std::endl;
                failed++;
            }
        }
    }
    
    std::cout << "Sin test: " << passed << " passed, " << failed << " failed\n";
    return failed == 0;
}

bool test_sqrt_results(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    int passed = 0, failed = 0;
    
    while (std::getline(file, line)) {
        size_t pos = line.find("result = ");
        if (pos != std::string::npos) {
            double result = std::stod(line.substr(pos + 9));
            // sqrt должен быть >= 0
            if (result >= 0.0) {
                passed++;
            } else {
                std::cerr << "FAIL: sqrt result is negative: " << result << std::endl;
                failed++;
            }
        }
    }
    
    std::cout << "Sqrt test: " << passed << " passed, " << failed << " failed\n";
    return failed == 0;
}

bool test_pow_results(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    int passed = 0, failed = 0;
    
    while (std::getline(file, line)) {
        size_t pos = line.find("result = ");
        if (pos != std::string::npos) {
            double result = std::stod(line.substr(pos + 9));
            // Для base > 0 и exp >= 0, pow должен быть >= 0
            if (result >= 0.0) {
                passed++;
            } else {
                std::cerr << "FAIL: pow result is negative: " << result << std::endl;
                failed++;
            }
        }
    }
    
    std::cout << "Pow test: " << passed << " passed, " << failed << " failed\n";
    return failed == 0;
}

int main() {
    std::cout << "Running tests...\n\n";
    
    bool all_passed = true;
    
    if (!test_sin_results("client1_sin.txt")) {
        all_passed = false;
    }
    
    if (!test_sqrt_results("client2_sqrt.txt")) {
        all_passed = false;
    }
    
    if (!test_pow_results("client3_pow.txt")) {
        all_passed = false;
    }
    
    std::cout << "\n";
    if (all_passed) {
        std::cout << "All tests PASSED!\n";
        return 0;
    } else {
        std::cout << "Some tests FAILED!\n";
        return 1;
    }
}
