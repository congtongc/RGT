#include <iostream>
#include <vector>
#include <thread>
#include <functional>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <future>
#include <mutex>

// 이미지 처리를 위한 병렬 프로세서 템플릿 클래스
template <typename T>
class ParallelProcessor {
private:
    // 처리할 데이터
    std::vector<T> data;
    // 스레드 개수
    unsigned int num_threads;
    // 뮤텍스 (스레드 안전한 출력을 위함)
    std::mutex output_mutex;

public:
    // 생성자: 데이터와 스레드 개수 초기화
    ParallelProcessor(const std::vector<T>& input_data, unsigned int threads = std::thread::hardware_concurrency())
        : data(input_data), num_threads(threads == 0 ? 1 : threads) {
        // 하드웨어 스레드가 감지되지 않으면 기본값 4 사용
        if (num_threads == 0) {
            num_threads = 4;
        }
    }

    // 병렬 처리 메서드 - 함수형 프로그래밍 스타일
    std::vector<T> process(std::function<T(const T&)> func) {
        // 결과를 저장할 벡터
        std::vector<T> result(data.size());
        
        // 스레드 벡터
        std::vector<std::thread> threads;
        
        // 각 스레드가 처리할 데이터 범위 계산
        size_t chunk_size = data.size() / num_threads;
        
        // 스레드 생성 및 작업 할당
        for (unsigned int i = 0; i < num_threads; ++i) {
            size_t start = i * chunk_size;
            size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
            
            threads.emplace_back([this, &func, &result, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    result[j] = func(data[j]);
                }
            });
        }
        
        // 모든 스레드 완료 대기
        for (auto& thread : threads) {
            thread.join();
        }
        
        return result;
    }
    
    // 병렬 처리 메서드 (진행 상황 출력)
    std::vector<T> process_with_progress(std::function<T(const T&)> func) {
        // 결과를 저장할 벡터
        std::vector<T> result(data.size());
        
        // 진행 상황 카운터
        std::atomic<size_t> progress(0);
        
        // 스레드 벡터
        std::vector<std::thread> threads;
        
        // 시작 시간 기록
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 각 스레드가 처리할 데이터 범위 계산
        size_t chunk_size = data.size() / num_threads;
        
        // 스레드 생성 및 작업 할당
        for (unsigned int i = 0; i < num_threads; ++i) {
            size_t start = i * chunk_size;
            size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
            
            threads.emplace_back([this, &func, &result, start, end, &progress]() {
                for (size_t j = start; j < end; ++j) {
                    result[j] = func(data[j]);
                    progress++;
                }
            });
        }
        
        // 진행 상황 모니터링 스레드
        std::thread monitor([this, &progress, start_time, total_size = data.size()]() {
            while (progress < total_size) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                auto current_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
                
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "\r진행 상황: " << progress << "/" << total_size 
                          << " (" << (progress * 100 / total_size) << "%) - "
                          << "경과 시간: " << elapsed << "ms" << std::flush;
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "\r완료: " << progress << "/" << total_size 
                      << " (100%) - 총 처리 시간: " << total_time << "ms" << std::endl;
        });
        
        // 모든 작업 스레드 완료 대기
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 모니터링 스레드 완료 대기
        monitor.join();
        
        return result;
    }
    
    // 맵 함수 - 함수형 프로그래밍 스타일
    std::vector<T> map(std::function<T(const T&)> func) {
        return process(func);
    }
    
    // 필터 함수 - 함수형 프로그래밍 스타일
    std::vector<T> filter(std::function<bool(const T&)> predicate) {
        std::vector<T> result;
        std::mutex result_mutex;
        
        std::vector<std::thread> threads;
        
        size_t chunk_size = data.size() / num_threads;
        
        for (unsigned int i = 0; i < num_threads; ++i) {
            size_t start = i * chunk_size;
            size_t end = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
            
            threads.emplace_back([this, &predicate, &result, &result_mutex, start, end]() {
                // 각 스레드별 로컬 결과 저장
                std::vector<T> local_result;
                
                for (size_t j = start; j < end; ++j) {
                    if (predicate(data[j])) {
                        local_result.push_back(data[j]);
                    }
                }
                
                // 로컬 결과를 글로벌 결과에 병합
                std::lock_guard<std::mutex> lock(result_mutex);
                result.insert(result.end(), local_result.begin(), local_result.end());
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        return result;
    }
    
    // 리듀스 함수 - 함수형 프로그래밍 스타일
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
        
        // 부분 결과 병합
        T final_result = initial_value;
        for (const auto& partial : partial_results) {
            final_result = func(final_result, partial);
        }
        
        return final_result;
    }
    
    // 병렬 정렬 구현
    void parallel_sort() {
        size_t chunk_size = data.size() / num_threads;
        std::vector<std::thread> threads;
        
        // 각 스레드에서 부분 정렬 수행
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
        
        // 병합 정렬로 부분 정렬된 결과 병합
        std::vector<T> temp(data.size());
        merge_sort_parallel(0, data.size(), temp);
    }
    
private:
    // 병합 정렬 구현 (병렬 정렬 보조 함수)
    void merge_sort_parallel(size_t start, size_t end, std::vector<T>& temp) {
        if (end - start <= 1) {
            return;
        }
        
        size_t middle = start + (end - start) / 2;
        
        // 충분히 큰 작업만 병렬 처리
        if (end - start > 10000 && num_threads > 1) {
            auto future = std::async(std::launch::async, [this, start, middle, &temp]() {
                merge_sort_parallel(start, middle, temp);
            });
            
            merge_sort_parallel(middle, end, temp);
            future.wait();
        } else {
            merge_sort_parallel(start, middle, temp);
            merge_sort_parallel(middle, end, temp);
        }
        
        // 병합
        size_t i = start;
        size_t j = middle;
        size_t k = start;
        
        while (i < middle && j < end) {
            if (data[i] <= data[j]) {
                temp[k++] = data[i++];
            } else {
                temp[k++] = data[j++];
            }
        }
        
        while (i < middle) {
            temp[k++] = data[i++];
        }
        
        while (j < end) {
            temp[k++] = data[j++];
        }
        
        // 결과 복사
        for (size_t i = start; i < end; ++i) {
            data[i] = temp[i];
        }
    }
};

// 이미지 처리 예제를 위한 픽셀 클래스
struct Pixel {
    int r, g, b;
    
    Pixel() : r(0), g(0), b(0) {}
    Pixel(int r, int g, int b) : r(r), g(g), b(b) {}
    
    // 픽셀 값 더하기 (리듀스 연산에 사용)
    Pixel operator+(const Pixel& other) const {
        return Pixel(r + other.r, g + other.g, b + other.b);
    }
    
    // 픽셀 값 비교 (정렬에 사용)
    bool operator<=(const Pixel& other) const {
        // 밝기 기준 비교
        int brightness = r + g + b;
        int other_brightness = other.r + other.g + other.b;
        return brightness <= other_brightness;
    }
    
    // 픽셀 값 비교 (정렬에 사용)
    bool operator<(const Pixel& other) const {
        // 밝기 기준 비교
        int brightness = r + g + b;
        int other_brightness = other.r + other.g + other.b;
        return brightness < other_brightness;
    }
};

// 테스트 코드
int main() {
    // 테스트 이미지 데이터 생성 (1000x1000 픽셀)
    std::vector<Pixel> image_data;
    const int width = 1000;
    const int height = 1000;
    image_data.reserve(width * height);
    
    std::cout << "이미지 데이터 생성 중..." << std::endl;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 예시 이미지 패턴 생성
            int r = (x * 255) / width;
            int g = (y * 255) / height;
            int b = ((x + y) * 255) / (width + height);
            image_data.emplace_back(r, g, b);
        }
    }
    
    // 병렬 프로세서 생성 (4개 스레드 사용)
    ParallelProcessor<Pixel> processor(image_data, 4);
    
    std::cout << "1. 밝기 조정 필터 적용 (병렬 처리)" << std::endl;
    auto brightened_image = processor.process_with_progress([](const Pixel& p) {
        // 밝기 증가 필터
        return Pixel(
            std::min(p.r + 50, 255),
            std::min(p.g + 50, 255),
            std::min(p.b + 50, 255)
        );
    });
    
    std::cout << "\n2. 그레이스케일 변환 (병렬 map 함수 사용)" << std::endl;
    auto grayscale_image = processor.map([](const Pixel& p) {
        // 그레이스케일 변환
        int gray = (p.r + p.g + p.b) / 3;
        return Pixel(gray, gray, gray);
    });
    
    std::cout << "3. 밝은 픽셀 필터링 (병렬 filter 함수 사용)" << std::endl;
    auto bright_pixels = processor.filter([](const Pixel& p) {
        // 밝기가 임계값 이상인 픽셀만 선택
        return (p.r + p.g + p.b) > 500;
    });
    
    std::cout << "밝은 픽셀 수: " << bright_pixels.size() << std::endl;
    
    std::cout << "4. 평균 색상 계산 (병렬 reduce 함수 사용)" << std::endl;
    Pixel sum_pixel = processor.reduce(
        [](const Pixel& a, const Pixel& b) {
            return Pixel(a.r + b.r, a.g + b.g, a.b + b.b);
        },
        Pixel(0, 0, 0)
    );
    
    // 평균 계산
    Pixel avg_pixel(
        sum_pixel.r / image_data.size(),
        sum_pixel.g / image_data.size(),
        sum_pixel.b / image_data.size()
    );
    
    std::cout << "평균 색상: R=" << avg_pixel.r << ", G=" << avg_pixel.g << ", B=" << avg_pixel.b << std::endl;
    
    std::cout << "5. 이미지 픽셀 병렬 정렬 (밝기 기준)" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    processor.parallel_sort();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto sort_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    std::cout << "정렬 완료: " << sort_time << "ms 소요" << std::endl;
    
    // 처리된 이미지의 첫 10개 픽셀 출력
    std::cout << "\n처리된 이미지의 첫 10개 픽셀 (정렬 후):" << std::endl;
    for (int i = 0; i < 10 && i < image_data.size(); ++i) {
        std::cout << "Pixel " << i << ": R=" << image_data[i].r 
                  << ", G=" << image_data[i].g 
                  << ", B=" << image_data[i].b 
                  << " (밝기: " << image_data[i].r + image_data[i].g + image_data[i].b << ")" 
                  << std::endl;
    }
    
    return 0;
}
