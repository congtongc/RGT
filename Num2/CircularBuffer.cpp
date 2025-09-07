#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>

template <typename T>
class CircularBuffer {
private:
    std::vector<T> buffer;
    size_t head = 0;  // 첫 번째 요소 위치
    size_t tail = 0;  // 마지막 요소 다음 위치
    size_t max_size;  // 버퍼 최대 크기
    bool is_full = false;  // 버퍼가 가득 찼는지 여부

public:
    // 생성자
    CircularBuffer(size_t capacity) : max_size(capacity) {
        buffer.resize(capacity);
    }

    // 요소 추가
    void push_back(const T& item) {
        buffer[tail] = item;
        
        // tail 위치 업데이트
        tail = (tail + 1) % max_size;
        
        // 버퍼가 가득 찬 경우
        if (is_full) {
            head = (head + 1) % max_size;
        }
        
        // tail이 head를 따라잡으면 버퍼가 가득 찬 것
        if (tail == head) {
            is_full = true;
        }
    }

    // 맨 앞 요소 제거
    void pop_front() {
        if (empty()) {
            throw std::runtime_error("버퍼가 비어 있습니다");
        }
        
        // head 위치 업데이트
        head = (head + 1) % max_size;
        is_full = false;
    }

    // 맨 앞 요소 반환
    T& front() {
        if (empty()) {
            throw std::runtime_error("버퍼가 비어 있습니다");
        }
        return buffer[head];
    }

    // 맨 뒤 요소 반환
    T& back() {
        if (empty()) {
            throw std::runtime_error("버퍼가 비어 있습니다");
        }
        return buffer[(tail + max_size - 1) % max_size];
    }

    // const 버전 front()
    const T& front() const {
        if (empty()) {
            throw std::runtime_error("버퍼가 비어 있습니다");
        }
        return buffer[head];
    }

    // const 버전 back()
    const T& back() const {
        if (empty()) {
            throw std::runtime_error("버퍼가 비어 있습니다");
        }
        return buffer[(tail + max_size - 1) % max_size];
    }

    // 버퍼 크기 반환
    size_t size() const {
        if (is_full) {
            return max_size;
        }
        if (tail >= head) {
            return tail - head;
        }
        return max_size - (head - tail);
    }

    // 버퍼 용량 반환
    size_t capacity() const {
        return max_size;
    }

    // 버퍼가 비어있는지 확인
    bool empty() const {
        return (!is_full && (head == tail));
    }

    // 반복자 구현을 위한 내부 클래스
    class iterator {
    private:
        CircularBuffer<T>* buffer_ptr;
        size_t index;
        size_t count;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(CircularBuffer<T>* buf, size_t idx, size_t cnt) 
            : buffer_ptr(buf), index(idx), count(cnt) {}

        reference operator*() {
            return buffer_ptr->buffer[index];
        }

        pointer operator->() {
            return &(buffer_ptr->buffer[index]);
        }

        iterator& operator++() {
            index = (index + 1) % buffer_ptr->max_size;
            ++count;
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator& other) const {
            return count == other.count;
        }

        bool operator!=(const iterator& other) const {
            return count != other.count;
        }
    };

    // begin 반복자
    iterator begin() {
        return iterator(this, head, 0);
    }

    // end 반복자
    iterator end() {
        return iterator(this, tail, size());
    }
};

// 문제에 주어진 예시 테스트
int main() {
    CircularBuffer<double> tempBuffer(5);
    
    // 예시에서 주어진 값들 추가
    tempBuffer.push_back(26.1);
    tempBuffer.push_back(24.1);
    tempBuffer.push_back(23.8);
    tempBuffer.push_back(25.2);
    tempBuffer.push_back(24.7);
    
    // 결과 확인
    std::cout << "tempBuffer.size() = " << tempBuffer.size() << std::endl;
    std::cout << "tempBuffer.capacity() = " << tempBuffer.capacity() << std::endl;
    std::cout << "tempBuffer.empty() = " << (tempBuffer.empty() ? "true" : "false") << std::endl;
    
    // 최대값 찾기
    double maxTemp = *std::max_element(tempBuffer.begin(), tempBuffer.end());
    std::cout << "maxTemp = " << maxTemp << std::endl;
    
    // 평균값 계산
    double avgTemp = std::accumulate(tempBuffer.begin(), tempBuffer.end(), 0.0) / tempBuffer.size();
    std::cout << "avgTemp = " << avgTemp << std::endl;
    
    // front와 back 확인
    std::cout << "tempBuffer.front() = " << tempBuffer.front() << " // 가장 오래된 데이터" << std::endl;
    std::cout << "tempBuffer.back() = " << tempBuffer.back() << " // 가장 최근 데이터" << std::endl;
    
    return 0;
}