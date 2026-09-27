#pragma once
#include <vector>
#include <cmath>
#include <numeric>

class Statistics {
public:
    void add(double value) {
        m_sum += value;
        m_sumSq += value * value;
        m_count++;
    }

    double mean() const {
        if (m_count == 0) return 0.0;
        return m_sum / m_count;
    }

    double variance() const {
        if (m_count < 2) return 0.0;
        double m = mean();
        // E[X^2] - (E[X])^2
        return (m_sumSq / m_count) - (m * m);
    }

    double stdDev() const {
        return std::sqrt(variance());
    }

    // Standard Error of the Mean (SEM) = StdDev / sqrt(N)
    // Crucial for Monte Carlo convergence checking
    double stdError() const {
        if (m_count == 0) return 0.0;
        return stdDev() / std::sqrt(m_count);
    }

    size_t count() const { return m_count; }

    void clear() {
        m_sum = 0.0;
        m_sumSq = 0.0;
        m_count = 0;
    }

    void merge(const Statistics& other) {
        m_sum += other.m_sum;
        m_sumSq += other.m_sumSq;
        m_count += other.m_count;
    }

private:
    double m_sum = 0.0;
    double m_sumSq = 0.0;
    size_t m_count = 0;
};
