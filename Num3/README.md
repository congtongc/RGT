# ParallelProcessor 클래스 구현 보고서

## 1. 개요

본 과제는 멀티스레딩과 함수형 프로그래밍을 활용한 병렬 처리를 위한 `ParallelProcessor<T>` 클래스를 구현하는 것입니다. 이 클래스는 대용량 이미지 데이터와 같은 데이터셋을 효율적으로 처리하기 위해 설계되었으며, 함수형 프로그래밍 패러다임의 map, filter, reduce 연산을 병렬로 수행할 수 있습니다.

## 2. 클래스 설계

### 2.1 클래스 구조

`ParallelProcessor<T>` 클래스는 다음과 같은 구조로 설계되었습니다:

```cpp
template <typename T>
class ParallelProcessor {
private:
    std::vector<T> data;
    unsigned int num_threads;
    std::mutex output_mutex;

public:
    ParallelProcessor(const std::vector<T>& input_data, unsigned int threads = std::thread::hardware_concurrency());
    
    std::vector<T> process(std::function<T(const T&)> func);
    std::vector<T> process_with_progress(std::function<T(const T&)> func);
    
    // 함수형 프로그래밍 메서드
    std::vector<T> map(std::function<T(const T&)> func);
    std::vector<T> filter(std::function<bool(const T&)> predicate);
    T reduce(std::function<T(const T&, const T&)> func, T initial_value);
    
    // 병렬 정렬
    void parallel_sort();
    
private:
    void merge_sort_parallel(size_t start, size_t end, std::vector<T>& temp);
};
```

### 2.2 멀티스레딩 활용

본 구현에서는 C++의 `<thread>` 라이브러리를 사용하여 멀티스레딩을 구현했습니다. 주요 특징은 다음과 같습니다:

1. **작업 분할**: 데이터를 스레드 수에 맞게 균등하게 분할하여 각 스레드에 할당합니다.
2. **스레드 풀**: 사용자가 지정한 수의 스레드를 생성하여 작업을 병렬로 처리합니다.
3. **동기화**: `std::mutex`를 사용하여 스레드 간 공유 자원 접근을 동기화합니다.
4. **진행 상황 모니터링**: 별도의 모니터링 스레드를 통해 작업 진행 상황을 실시간으로 출력합니다.

### 2.3 함수형 프로그래밍 패러다임

클래스는 함수형 프로그래밍의 주요 연산을 병렬로 구현합니다:

1. **Map**: 입력 데이터의 각 요소에 함수를 적용하여 새로운 컬렉션을 생성합니다.
2. **Filter**: 주어진 조건을 만족하는 요소만 선택하여 새로운 컬렉션을 생성합니다.
3. **Reduce**: 컬렉션의 요소들을 하나의 결과값으로 축소합니다.

## 3. 주요 기능 구현

### 3.1 병렬 처리 (process)

```cpp
std::vector<T> process(std::function<T(const T&)> func) {
    std::vector<T> result(data.size());
    std::vector<std::thread> threads;
    
    size_t chunk_size = data.size() / num_threads;
    
    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
        
        threads.emplace_back([this, &func, &result, start, end]() {
            for (size_t j = start; j < end; ++j) {
                result[j] = func(data[j]);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    return result;
}
```

- 데이터를 균등하게 분할하여 각 스레드에 할당
- 람다 함수를 사용하여 스레드 작업 정의
- 모든 스레드가 완료될 때까지 대기 후 결과 반환

### 3.2 Map 연산

```cpp
std::vector<T> map(std::function<T(const T&)> func) {
    return process(func);
}
```

- `process` 메서드를 활용하여 map 연산 구현
- 각 요소에 함수를 적용하여 새로운 컬렉션 생성

### 3.3 Filter 연산

```cpp
std::vector<T> filter(std::function<bool(const T&)> predicate) {
    std::vector<T> result;
    std::mutex result_mutex;
    std::vector<std::thread> threads;
    
    size_t chunk_size = data.size() / num_threads;
    
    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
        
        threads.emplace_back([this, &predicate, &result, &result_mutex, start, end]() {
            std::vector<T> local_result;
            
            for (size_t j = start; j < end; ++j) {
                if (predicate(data[j])) {
                    local_result.push_back(data[j]);
                }
            }
            
            std::lock_guard<std::mutex> lock(result_mutex);
            result.insert(result.end(), local_result.begin(), local_result.end());
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    return result;
}
```

- 각 스레드에서 로컬 결과를 생성하여 경합 최소화
- 뮤텍스를 사용하여 최종 결과 병합 시 동기화

### 3.4 Reduce 연산

```cpp
T reduce(std::function<T(const T&, const T&)> func, T initial_value) {
    std::vector<T> partial_results(num_threads, initial_value);
    std::vector<std::thread> threads;
    
    size_t chunk_size = data.size() / num_threads;
    
    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
        
        threads.emplace_back([this, &func, &partial_results, i, start, end, initial_value]() {
            T local_result = initial_value;
            
            for (size_t j = start; j < end; ++j) {
                local_result = func(local_result, data[j]);
            }
            
            partial_results[i] = local_result;
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    T final_result = initial_value;
    for (const auto& partial : partial_results) {
        final_result = func(final_result, partial);
    }
    
    return final_result;
}
```

- 각 스레드에서 부분 결과 계산
- 모든 스레드 완료 후 부분 결과를 병합하여 최종 결과 생성

### 3.5 병렬 정렬

```cpp
void parallel_sort() {
    size_t chunk_size = data.size() / num_threads;
    std::vector<std::thread> threads;
    
    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
        
        threads.emplace_back([this, start, end]() {
            std::sort(data.begin() + start, data.begin() + end);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::vector<T> temp(data.size());
    merge_sort_parallel(0, data.size(), temp);
}
```

- 데이터를 분할하여 각 부분을 병렬로 정렬
- 병합 정렬 알고리즘을 사용하여 부분 정렬된 결과를 병합

## 4. 성능 최적화

본 구현에서는 다음과 같은 성능 최적화 기법을 적용했습니다:

1. **로컬 결과 활용**: 각 스레드에서 로컬 결과를 생성하여 뮤텍스 경합 최소화
2. **작업 분할 최적화**: 데이터를 균등하게 분할하여 작업 부하 균형 유지
3. **비동기 실행**: `std::async`를 사용하여 재귀적 병렬 처리 구현
4. **하드웨어 인식**: 시스템의 하드웨어 스레드 수를 자동으로 감지하여 최적의 스레드 수 결정

## 5. 예외 처리

본 구현에서는 다음과 같은 예외 상황을 처리합니다:

1. **스레드 수 검증**: 스레드 수가 0으로 지정된 경우 기본값 사용
2. **경계 조건 처리**: 마지막 스레드가 나머지 데이터를 모두 처리하도록 설계
3. **동기화 보장**: 뮤텍스를 사용하여 공유 자원 접근 시 데이터 무결성 보장

## 6. 테스트 코드

테스트 코드는 다음과 같은 작업을 수행합니다:

1. 1000x1000 크기의 이미지 데이터 생성
2. 밝기 조정 필터 적용 (process_with_progress 메서드 사용)
3. 그레이스케일 변환 (map 메서드 사용)
4. 밝은 픽셀 필터링 (filter 메서드 사용)
5. 평균 색상 계산 (reduce 메서드 사용)
6. 픽셀 데이터 병렬 정렬 (parallel_sort 메서드 사용)

## 7. 성능 분석

테스트 결과, 병렬 처리를 통해 다음과 같은 성능 향상을 확인할 수 있었습니다:

- 4개의 스레드를 사용하여 1,000,000개의 픽셀 데이터 처리 시 단일 스레드 대비 약 3.5배 성능 향상
- 병렬 정렬의 경우 데이터 크기가 클수록 성능 향상이 두드러짐
- 스레드 간 작업 분배가 균등할 때 최적의 성능 발휘

## 8. 결론

본 구현은 멀티스레딩과 함수형 프로그래밍을 결합하여 대용량 데이터 처리를 효율적으로 수행하는 방법을 보여줍니다. `ParallelProcessor<T>` 클래스는 이미지 처리뿐만 아니라 다양한 데이터 처리 작업에 활용할 수 있는 범용적인 병렬 처리 프레임워크를 제공합니다.

향후 개선 방향으로는 작업 스케줄링 최적화, 캐시 효율성 향상, 더 다양한 함수형 연산 지원 등을 고려할 수 있습니다.

## 터미널 출력
<img width="1036" height="512" alt="Num3" src="https://github.com/user-attachments/assets/7f94b179-1d1d-409f-8a2b-92f599505b09" />

## 실행 영상
https://github.com/user-attachments/assets/f3cd7650-a5ae-4f1b-b0cb-fd1d133300fd
