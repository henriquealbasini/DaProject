#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <string>
#include <vector>
#include <set>

/**
 * @brief Structure that stores the configuration from the registers file.
 */
struct AppConfig {
    int numRegisters = 0;
    std::string algorithm = "basic"; // basic, spilling, splitting, free
    int algoParam = 0;               // Numeric parameter K (e.g., spilling, 2)
};

/**
 * @brief Represents a "Web" (aggregation of live ranges of a single variable).
 */
struct Web {
    int id;                      // Unique Web ID (used as vertex ID in the Graph)
    std::string variableName;    // Name of the variable (e.g., "sum", "i")
    std::set<int> lines;         // Code lines where the variable is live
    int assignedRegister = -1;   // -1 means it will be spilled to Memory (M)
};

#endif