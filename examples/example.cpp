#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <set>

#include "dtw.h"
#include <omp.h>

// this just generates some random array
vector<double> getrandomwalk(uint size) {
  vector<double> data(size);
  data[0] = 0.0;
  for (uint k = 1; k < size; ++k)
    data[k] = (1.0 * rand() / (RAND_MAX)) - 0.5 + data[k - 1];
  return data;
}

void demo(uint size) {
  std::cout << " I generated a random walk and I will try to match it with "
               "other random walks. "
            << std::endl;

  vector<double> target = getrandomwalk(size); // this is our target
  LB_Improved filter(
      target, size / 10); // we use the DTW with a tolerance of 10% (size/10)
  double bestsofar = filter.getLowestCost();
  uint howmany = 5000;
  for (uint i = 0; i < 5000; i++) {
    vector<double> candidate = getrandomwalk(size);
    double newbest = filter.test(candidate);
    if (newbest < bestsofar) {
      std::cout << " we found a new nearest neighbor, distance (L1 norm) = "
                << newbest << std::endl;
      bestsofar = newbest;
    }
  }
  std::cout << " I compared it with " << howmany
            << " random walks, closest match is at a distance (L1 norm) of "
            << filter.getLowestCost() << std::endl;
}


void storeNeighborsToFile(const std::vector<std::vector<std::pair<int, double>>>& neighbors, const std::string& filename) {
    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    for (size_t i = 0; i < neighbors.size(); ++i) {
        for (const auto& pair : neighbors[i]) {
            int j = pair.first;
            double weight = pair.second;
            outFile << i << ", " << j << ", " << weight << "\n";
        }
    }

    outFile.close();
}


std::vector<std::vector<double>> loadDataset(const std::string& filename) {
    std::vector<std::vector<double>> dataset;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return dataset;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<double> rowData;
        std::istringstream lineStream(line);
        double value;

        while (lineStream >> value) {
            rowData.push_back(value);
        }
        dataset.push_back(rowData);
    }

    file.close();
    return dataset;
}


bool isInSet(const std::set<int>& mySet, int numberToFind) {
    // Use find() to check if the number is in the set
    auto it = mySet.find(numberToFind);
    
    // If the iterator is not equal to mySet.end(), the number is present
    return it != mySet.end();
}

std::vector<std::vector<pair<int, double>>> compute_nearest_neighbor(const std::vector<std::vector<double>>& X, double w, int k) {
    int num_points = X.size();
    int size = X[0].size() * w;
    std::vector<std::vector<pair<int, double>>> neighbors(num_points, std::vector<pair<int, double>>(k));

    #pragma omp parallel for
    for(int i = 0; i < num_points; ++i) {
        std::set<int> found;
        for (int kth=0; kth < k; ++kth){
          LB_Improved filter(X[i], size);
          double bestsofar = filter.getLowestCost();
          int best_neighbor = -1;
          for(int j = 0; j < num_points; ++j) {
            if (i==j) continue;
            if (isInSet(found, j)) continue;
            double newbest = filter.test(X[j]);
            if (newbest < bestsofar) {
              bestsofar = newbest;
              best_neighbor = j;
            }
          }
          neighbors[i][kth] = {best_neighbor, bestsofar};
          found.insert(best_neighbor);
        }
    }
    return neighbors;
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <dataset_filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    double w = atof(argv[2]);
    int k = atof(argv[3]);

    omp_set_num_threads(24);
    #pragma omp parallel
    {
        int num_threads = omp_get_num_threads();
        #pragma omp single
        {
            std::cout << "Total Threads: " << num_threads << std::endl;
        }
    }

    std::vector<std::vector<double>> data = loadDataset(filename);

    std::cout << "data size: " << data.size() << " " << data[0].size() << std::endl;

    auto neighbors = compute_nearest_neighbor(data, w, k);

     storeNeighborsToFile(neighbors, filename + "_neighbors_" + std::to_string(k) + ".txt");

    return 0;
}
 