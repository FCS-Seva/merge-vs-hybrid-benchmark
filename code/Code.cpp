#include <iostream>
#include <random>
#include <vector>



class ArrayGenerator {
public:
    ArrayGenerator(const size_t max_n, const int min_val, const int max_val, const uint64_t seed = 9238417)
        : max_n_(max_n), min_val_(min_val), max_val_(max_val), rng_(seed)
    {
        generate_base_arrays();
    }

    std::vector<int> get_random(const size_t n) const {
        return std::vector<int>(random_base_.begin(), random_base_.begin() + n);
    }

    std::vector<int> get_reversed(const size_t n) const {
        return std::vector<int>(reversed_base_.begin(), reversed_base_.begin() + n);
    }

    std::vector<int> get_almost_sorted(const size_t n) const {
        return std::vector<int>(almost_sorted_base_.begin(), almost_sorted_base_.begin() + n);
    }

private:
    size_t max_n_;
    int min_val_, max_val_;
    mutable std::mt19937_64 rng_;

    std::vector<int> random_base_;
    std::vector<int> reversed_base_;
    std::vector<int> almost_sorted_base_;

    void generate_base_arrays() {
        std::uniform_int_distribution<int> dist(min_val_, max_val_);

        random_base_.resize(max_n_);
        for (size_t i = 0; i < max_n_; ++i) {
            random_base_[i] = dist(rng_);
        }

        reversed_base_ = random_base_;
        std::sort(reversed_base_.begin(), reversed_base_.end());
        std::reverse(reversed_base_.begin(), reversed_base_.end());

        almost_sorted_base_ = random_base_;
        std::sort(almost_sorted_base_.begin(), almost_sorted_base_.end());

        size_t num_swaps = max_n_ / 100;
        std::uniform_int_distribution<size_t> pos_dist(0, max_n_ - 1);
        for (size_t k = 0; k < num_swaps; ++k) {
            size_t i = pos_dist(rng_);
            size_t j = pos_dist(rng_);
            std::swap(almost_sorted_base_[i], almost_sorted_base_[j]);
        }
    }
};


void insertion_sort(std::vector<int>& a, int l, int r) {
    for (int i = l + 1; i < r; ++i) {
        const int key = a[i];
        int j = i - 1;
        while (j >= l && a[j] > key) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = key;
    }
}

void merge_range(std::vector<int>& a, std::vector<int>& buf, const int l, const int m, const int r) {
    int i = l, j = m, k = l;
    while (i < m && j < r) {
        if (a[i] <= a[j]) {
            buf[k++] = a[i++];
        } else {
            buf[k++] = a[j++];
        }
    }
    while (i < m) buf[k++] = a[i++];
    while (j < r) buf[k++] = a[j++];
    for (int t = l; t < r; ++t) {
        a[t] = buf[t];
    }
}

void merge_sort_rec(std::vector<int>& a, std::vector<int>& buf, int l, int r) {
    if (r - l <= 1) return;
    const int m = l + (r - l) / 2;
    merge_sort_rec(a, buf, l, m);
    merge_sort_rec(a, buf, m, r);
    merge_range(a, buf, l, m, r);
}

void merge_sort(std::vector<int>& a) {
    std::vector<int> buf(a.size());
    merge_sort_rec(a, buf, 0, static_cast<int>(a.size()));
}

void hybrid_merge_sort_rec(std::vector<int>& a, std::vector<int>& buf, const int l, const int r, const int threshold) {
    const int len = r - l;
    if (len <= 1) return;
    if (len <= threshold) {
        insertion_sort(a, l, r);
        return;
    }
    const int m = l + len / 2;
    hybrid_merge_sort_rec(a, buf, l, m, threshold);
    hybrid_merge_sort_rec(a, buf, m, r, threshold);
    merge_range(a, buf, l, m, r);
}

void hybrid_merge_sort(std::vector<int>& a, const int threshold) {
    std::vector<int> buf(a.size());
    hybrid_merge_sort_rec(a, buf, 0, static_cast<int>(a.size()), threshold);
}

class SortTester {
public:
    SortTester(ArrayGenerator& gen, int repeats)
        : gen_(gen), repeats_(repeats) {}

    void run_all() {
        const std::vector<int> thresholds = {5, 10, 20, 30, 50};

        std::cout << "type;n;algo;threshold;time_ms\n";

        for (const std::string& type : {"random", "reversed", "almost"}) {
            for (int n = 500; n <= 100000; n += 100) {
                long long avg_ms = measure_one(type, n, "merge", 0);
                std::cout << type << ";" << n << ";merge;0;" << avg_ms << "\n";
            }
            for (int thr : thresholds) {
                for (int n = 500; n <= 100000; n += 100) {
                    long long avg_ms = measure_one(type, n, "hybrid", thr);
                    std::cout << type << ";" << n << ";hybrid;" << thr << ";" << avg_ms << "\n";
                }
            }
        }
    }

private:
    ArrayGenerator& gen_;
    int repeats_;

    std::vector<int> make_array(const std::string& type, const int n) const {
        if (type == "random") {
            return gen_.get_random(static_cast<size_t>(n));
        }

        if (type == "reversed") {
            return gen_.get_reversed(static_cast<size_t>(n));
        }

        return gen_.get_almost_sorted(static_cast<size_t>(n));

    }

    long long measure_one(const std::string& type, const int n, const std::string& algo, const int threshold) const {
        using namespace std::chrono;
        long long total_ms = 0;

        for (int rep = 0; rep < repeats_; ++rep) {
            std::vector<int> a = make_array(type, n);

            auto start = high_resolution_clock::now();
            if (algo == "merge") {
                merge_sort(a);
            } else {
                hybrid_merge_sort(a, threshold);
            }
            auto elapsed = high_resolution_clock::now() - start;
            long long ms = duration_cast<milliseconds>(elapsed).count();
            total_ms += ms;
        }
        return total_ms / std::max(1, repeats_);
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    constexpr size_t MAX_N = 100000;
    constexpr int MIN_VAL = 0;
    constexpr int MAX_VAL = 6000;

    ArrayGenerator gen(MAX_N, MIN_VAL, MAX_VAL);
    SortTester tester(gen, 5);

    tester.run_all();

    return 0;
}
