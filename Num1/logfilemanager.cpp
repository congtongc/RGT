#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <sstream>

class LogFileManager {
private:
    // 로그 파일 관리 맵 (파일명, 파일 스트림)
    std::unordered_map<std::string, std::unique_ptr<std::ofstream>> logFiles;
    
    // 현재 시간 반환
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "[";
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << "] ";
        return ss.str();
    }

public:
    LogFileManager() = default;
    ~LogFileManager() {
        // 열린 파일 닫음
        for (auto& file : logFiles) {
            if (file.second && file.second->is_open()) {
                file.second->close();
            }
        }
    }

    // 로그 파일 열기
    bool openLogFile(const std::string& filename) {
        try {
            // 이미 열려있는 파일인지 확인
            if (logFiles.find(filename) != logFiles.end()) {
                return true; // 이미 열려있으면 성공으로 간주
            }
            
            // 새 파일 스트림 생성 및 열기
            auto fileStream = std::make_unique<std::ofstream>(filename, std::ios::trunc);
            
            if (!fileStream->is_open()) {
                return false; // 파일 열기 실패
            }
            
            // 맵에 추가
            logFiles[filename] = std::move(fileStream);
            return true;
        } catch (...) {
            return false; // 예외 발생 시 실패
        }
    }

    // 로그 쓰기
    bool writeLog(const std::string& filename, const std::string& message) {
        try {
            // 파일이 열려있는지 확인
            auto it = logFiles.find(filename);
            if (it == logFiles.end() || !it->second->is_open()) {
                return false; // 파일이 열려있지 않음
            }
            
            // 타임스탬프와 함께 메시지 쓰기
            *(it->second) << getCurrentTimestamp() << message << std::endl;
            
            return !(it->second->fail()); // 쓰기 성공 여부 반환
        } catch (...) {
            return false; // 예외 발생 시 실패
        }
    }

    // 로그 파일 내용 읽기
    std::vector<std::string> readLogs(const std::string& filename) {
        std::vector<std::string> logs;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            return logs; // 빈 벡터 반환
        }
        
        std::string line;
        while (std::getline(file, line)) {
            logs.push_back(line);
        }
        
        file.close();
        return logs;
    }

    // 로그 파일 닫기
    bool closeLogFile(const std::string& filename) {
        auto it = logFiles.find(filename);
        if (it == logFiles.end()) {
            return false; // 파일이 맵에 없음
        }
        
        if (it->second && it->second->is_open()) {
            it->second->close();
        }
        
        logFiles.erase(it); // 맵에서 제거
        return true;
    }
};

// 테스트 코드
int main() {
    LogFileManager manager;
    
    // 로그 파일 열기
    manager.openLogFile("error.log");
    manager.openLogFile("debug.log");
    manager.openLogFile("info.log");
    
    // 로그 쓰기
    manager.writeLog("error.log", "Database connection failed");
    manager.writeLog("debug.log", "User login attempt");
    manager.writeLog("info.log", "Server started successfully");
    
    // 각 로그 파일 내용 출력
    std::cout << "// error.log 파일 내용" << std::endl;
    std::vector<std::string> errorContent = manager.readLogs("error.log");
    for (const auto& line : errorContent) {
        std::cout << line << std::endl;
    }
    
    std::cout << "\n// debug.log 파일 내용" << std::endl;
    std::vector<std::string> debugContent = manager.readLogs("debug.log");
    for (const auto& line : debugContent) {
        std::cout << line << std::endl;
    }
    
    std::cout << "\n// info.log 파일 내용" << std::endl;
    std::vector<std::string> infoContent = manager.readLogs("info.log");
    for (const auto& line : infoContent) {
        std::cout << line << std::endl;
    }
    
    // readLogs 반환값 출력
    std::cout << "\n// readLogs 반환값" << std::endl;
    if (!errorContent.empty()) {
        std::cout << "errorLogs[0] = \"" << errorContent[0] << "\"" << std::endl;
    } else {
        std::cout << "errorLogs is empty" << std::endl;
    }
    
    // 모든 로그 파일 닫기 (소멸자에서도 자동으로 닫힘)
    manager.closeLogFile("error.log");
    manager.closeLogFile("debug.log");
    manager.closeLogFile("info.log");
    
    return 0;
}

