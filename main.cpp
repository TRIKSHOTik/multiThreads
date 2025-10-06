#include <iostream>
#include <vector>
#include <thread>
#include <pthread.h>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <random>
#include <ctime>

#ifdef _WIN32
#define USE_WINDOWS_THREADS
#else
#define USE_PTHREADS
#endif

struct ThreadData {
    std::vector<std::vector<int>>*A;
    std::vector<std::vector<int>>*B;
    std::vector<std::vector<int>>*C;
    int startRow;
    int endRow;
};

std::vector<std::vector<int>> createMatrix(int n) {
    std::vector<std::vector<int>> matrix(n, std::vector<int>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix[i][j] = std::rand() % 100;
        }
    }
    return matrix;
}

void multiplyMatrix(std::vector<std::vector<int>>& A, std::vector<std::vector<int>>& B, std::vector<std::vector<int>>& C) {
    int n = A.size();

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void multiplyMatrixByBlocks(std::vector<std::vector<int>>& A, std::vector<std::vector<int>>& B, std::vector<std::vector<int>>& C,
                   int startRow, int startCol, int blockSize) {
    int n = A.size();
    for (int i = startRow; i < startRow + blockSize && i < n; ++i) {
        for (int j = startCol; j < startCol + blockSize && j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void threadedMultiply_thread(std::vector<std::vector<int>>& A, std::vector<std::vector<int>>& B, std::vector<std::vector<int>>& C,int blockSize) {
    int n = A.size();

    std::vector<std::thread> threads;

    for (int i = 0; i < n; i += blockSize) {
        for (int j = 0; j < n; j += blockSize) {
            threads.emplace_back(std::thread(multiplyMatrixByBlocks, ref(A), ref(B), ref(C), i, j, blockSize));
        }
    }

    for (auto& t : threads) {
        t.join();
    }
}

void * multiplyRows_pthread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int n = data->A->size();
    for (int i = data->startRow; i < data->endRow && i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                (*data->C)[i][j] += (*data->A)[i][k] * (*data->B)[k][j];
            }
        }
    }
    return nullptr;
}

void threadedMultiply_pthread(std::vector<std::vector<int>>& A, std::vector<std::vector<int>>& B, std::vector<std::vector<int>>& C,int blockSize) {
    int n = A.size();
    std::vector<ThreadData> threadData(blockSize);
    std::vector<pthread_t> threads(blockSize);

    for (int i = 0; i < blockSize; ++i) {
        threadData[i].A = &A;
        threadData[i].B = &B;
        threadData[i].C = &C;
        threadData[i].startRow = i * blockSize;
        threadData[i].endRow = std::min((i + 1) * blockSize, n);

        if (pthread_create(&threads[i], nullptr, multiplyRows_pthread, &threadData[i]) != 0) {
            std::cout << "Error in thread creation" << std::endl;\
            return;
        }
    }

    for (int i = 0; i < blockSize; ++i) {
        pthread_join(threads[i], nullptr);
    }

}

std::ostream & printMatrix(std::vector<std::vector<int>>& matrix, std::ostream & output) {
    for (auto& row : matrix) {
        for (auto& elem : row) {
            output << elem << " ";
        }
        output << std::endl;
    }
    return output;
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    int n = 1000;
    std::vector<unsigned int> blockSizes = {40};
    auto file = "output.txt";
    std::ofstream out(file);

    std::vector<std::vector<int>> C(n, std::vector<int>(n, 0));
    std::vector<std::vector<int>> D(n, std::vector<int>(n, 0));
    std::vector<std::vector<int>> E(n, std::vector<int>(n, 0));
    std::vector<std::vector<int>> A = createMatrix(n);
    std::vector<std::vector<int>> B = createMatrix(n);

    printMatrix(A, out);
    printMatrix(B, out);

    auto startSimple = std::chrono::high_resolution_clock::now();
    multiplyMatrix(A, B, C);
    auto endSimple = std::chrono::high_resolution_clock::now();
    auto durationSimple = std::chrono::duration_cast<std::chrono::microseconds>(endSimple - startSimple);

    std::cout << "\nSimple multiply: " << static_cast<double> (durationSimple.count())/1000000 << " s" << std::endl;
    printMatrix(C, out);

    std::cout << "\nstd::thread:" << std::endl;
    for (auto k : blockSizes) {
        auto startThreaded = std::chrono::high_resolution_clock::now();
        threadedMultiply_thread(A, B, D, k);
        auto endThreaded = std::chrono::high_resolution_clock::now();
        auto durationThreaded = std::chrono::duration_cast<std::chrono::microseconds>(endThreaded - startThreaded);

        int blocksPerRow = k;
        int totalBlocks = blocksPerRow * blocksPerRow;

        std::cout << "Block size " << k << " (" << blocksPerRow << "x" << blocksPerRow
            << " Blocks, " << totalBlocks << " threads): "
            << static_cast<double> (durationThreaded.count())/1000000 << " s" << std::endl;
    }

    std::cout << "\npthread:" << std::endl;
    for (auto k : blockSizes) {
        auto startThreaded = std::chrono::high_resolution_clock::now();
        threadedMultiply_pthread(A, B, E, k);
        auto endThreaded = std::chrono::high_resolution_clock::now();
        auto durationThreaded = std::chrono::duration_cast<std::chrono::microseconds>(endThreaded - startThreaded);

        int blocksPerRow = k;
        int totalBlocks = blocksPerRow * blocksPerRow;

        std::cout << "Block size " << k << " (" << blocksPerRow << "x" << blocksPerRow
            << " Blocks, " << totalBlocks << " threads): "
            << static_cast<double> (durationThreaded.count())/1000000 << " s" << std::endl;
    }
}