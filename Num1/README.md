# LogFileManager 클래스 구현 보고서

## 1. 개요

본 과제는 스마트 포인터를 활용한 리소스 관리를 위한 `LogFileManager` 클래스를 구현하는 것입니다. 이 클래스는 여러 로그 파일을 동시에 관리하며, 각 파일에 타임스탬프와 함께 로그 메시지를 기록하고 읽을 수 있는 기능을 제공합니다.

## 2. 클래스 설계

### 2.1 클래스 구조

`LogFileManager` 클래스는 다음과 같은 구조로 설계되었습니다:

```cpp
class LogFileManager {
private:
    // 로그 파일들을 관리하기 위한 맵 (파일명, 파일 스트림)
    std::unordered_map<std::string, std::unique_ptr<std::ofstream>> logFiles;
    
    // 현재 시간 문자열 반환 함수
    std::string getCurrentTimestamp();

public:
    // 기본 생성자
    LogFileManager() = default;
    
    // 소멸자
    ~LogFileManager();

    // 로그 파일 열기
    bool openLogFile(const std::string& filename);
    
    // 로그 쓰기
    bool writeLog(const std::string& filename, const std::string& message);
    
    // 로그 파일 내용 읽기
    std::vector<std::string> readLogs(const std::string& filename);
    
    // 로그 파일 닫기
    bool closeLogFile(const std::string& filename);
};
```

### 2.2 스마트 포인터 활용

본 구현에서는 `std::unique_ptr`를 사용하여 파일 스트림 객체를 관리합니다. 이를 통해 다음과 같은 이점을 얻을 수 있습니다:

1. **자동 메모리 관리**: 파일 스트림 객체가 더 이상 필요하지 않을 때 자동으로 메모리가 해제됩니다.
2. **자원 누수 방지**: 예외 발생 시에도 스마트 포인터가 자원을 정리합니다.
3. **소유권 명확화**: `unique_ptr`는 단일 소유권 의미를 명확히 표현합니다.

## 3. 주요 기능 구현

### 3.1 로그 파일 열기 (openLogFile)

```cpp
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
```

- `std::make_unique`를 사용하여 파일 스트림 객체 생성
- `std::ios::trunc` 모드로 파일을 열어 기존 내용을 지우고 새로 시작
- 이미 열려있는 파일인지 확인하여 중복 열기 방지
- 예외 처리를 통한 안전성 확보

### 3.2 로그 쓰기 (writeLog)

```cpp
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
```

- 파일이 열려있는지 확인 후 쓰기 작업 수행
- 현재 시간을 타임스탬프로 추가하여 로그 기록
- 쓰기 작업 성공 여부 반환
- 예외 처리를 통한 안전성 확보

### 3.3 로그 읽기 (readLogs)

```cpp
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
```

- 파일을 열어 모든 라인을 읽어 벡터에 저장
- 파일이 없거나 열 수 없는 경우 빈 벡터 반환
- 읽기 작업 완료 후 파일 닫기

### 3.4 로그 파일 닫기 (closeLogFile)

```cpp
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
```

- 파일이 맵에 있는지 확인
- 파일이 열려있으면 닫기
- 맵에서 파일 항목 제거
- 작업 성공 여부 반환

### 3.5 소멸자

```cpp
~LogFileManager() {
    // 소멸자에서 모든 열린 파일을 닫음
    for (auto& file : logFiles) {
        if (file.second && file.second->is_open()) {
            file.second->close();
        }
    }
}
```

- 소멸자에서 모든 열린 파일을 자동으로 닫음
- 이를 통해 자원 누수 방지

## 4. 예외 처리

본 구현에서는 다음과 같은 예외 상황을 처리합니다:

1. **파일 열기 실패**: 파일을 열 수 없는 경우 false 반환
2. **이미 열린 파일**: 이미 열려있는 파일을 다시 열려고 할 때 처리
3. **닫힌 파일에 쓰기**: 열려있지 않은 파일에 쓰려고 할 때 false 반환
4. **존재하지 않는 파일 읽기**: 존재하지 않는 파일을 읽으려 할 때 빈 벡터 반환
5. **예상치 못한 예외**: try-catch 블록으로 모든 예외 상황 처리

## 5. 테스트 코드

```cpp
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
```

테스트 코드는 다음과 같은 작업을 수행합니다:
1. 세 개의 로그 파일 열기
2. 각 파일에 다른 메시지 기록
3. 각 파일의 내용 읽고 출력
4. readLogs 함수의 반환값 확인
5. 모든 파일 명시적으로 닫기

## 6. 결론

본 구현은 스마트 포인터를 활용하여 안전하고 효율적인 로그 파일 관리 시스템을 구현했습니다. `std::unique_ptr`를 사용하여 자원 누수를 방지하고, 예외 처리를 통해 안정성을 확보했습니다. 또한 여러 로그 파일을 동시에 관리할 수 있는 유연한 구조를 제공합니다.

이 구현은 실제 운영 환경에서 로그 시스템으로 활용할 수 있으며, 필요에 따라 기능을 확장할 수 있습니다.


## 터미널 출력 값
<img width="1036" height="246" alt="Num1" src="https://github.com/user-attachments/assets/182cfa8c-8780-48cf-b047-7d225c4f4143" />

## 실행 영상
https://github.com/user-attachments/assets/96416bd0-c1ef-4868-8bcc-67ed29509048
