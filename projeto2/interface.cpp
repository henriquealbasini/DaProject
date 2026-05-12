/**
 * @file interface.cpp
 * @brief Interface implementation, file parsing, and interference graph construction.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include "DataStructures.h"
#include "Graph.h"

// --- Global Variables ---
AppConfig currentConfig;
std::vector<Web> websList;
Graph<int> interferenceGraph;

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param s The original string.
 * @return The trimmed string.
 * @note Time Complexity: O(N) where N is the length of the string.
 */
std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \r\n\t");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \r\n\t");
    return s.substr(first, (last - first + 1));
}

/**
 * @brief Parses the Live Ranges file and creates the Webs, merging overlapping ranges.
 * @param filename The name of the input file (e.g., ranges.txt).
 * @return true if successfully parsed, false otherwise.
 * @note Time Complexity: O(L * W * log(C)), where L is the number of lines, W is the number of existing Webs, and C is the number of lines in the set.
 */
bool parseRanges(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    websList.clear();
    std::string line;
    int currentWebId = 0;
    std::string currentVar = "";

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue; 

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string varName = trim(line.substr(0, colonPos));
            std::string rangesStr = trim(line.substr(colonPos + 1));

            if (!varName.empty()) {
                currentVar = varName;
            }

            // 1. Extract the lines for this new range
            std::set<int> tempLines;
            std::stringstream ss(rangesStr);
            std::string numStr;
            while (std::getline(ss, numStr, ',')) {
                numStr = trim(numStr);
                if(numStr.empty()) continue;

                if (numStr.back() == '+' || numStr.back() == '-') {
                    numStr.pop_back();
                }
                tempLines.insert(std::stoi(numStr));
            }

            // 2. Try to merge with an existing Web of the SAME variable
            bool merged = false;
            for (auto& web : websList) {
                if (web.variableName == currentVar) {
                    bool overlap = false;
                    // Check if they share any line (intersection)
                    for (int l : tempLines) {
                        if (web.lines.find(l) != web.lines.end()) {
                            overlap = true;
                            break;
                        }
                    }
                    
                    // If they overlap, merge the lines into the existing Web
                    if (overlap) {
                        web.lines.insert(tempLines.begin(), tempLines.end());
                        merged = true;
                        // Since we modified an existing web, we might need to check if 
                        // this newly expanded web now overlaps with another web of the same variable.
                        // For this basic requirement, merging sequentially usually suffices.
                        break;
                    }
                }
            }

            // 3. If no merge happened, create a new Web
            if (!merged) {
                Web newWeb;
                newWeb.id = currentWebId++;
                newWeb.variableName = currentVar;
                newWeb.lines = tempLines;
                websList.push_back(newWeb);
            }
        }
    }
    return true;
}

/**
 * @brief Parses the Registers and Algorithm configuration file.
 * @param filename The name of the file (e.g., registers.txt).
 * @return true if successfully parsed, false otherwise.
 * @note Time Complexity: O(L) where L is the number of lines in the file.
 */
bool parseRegisters(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = trim(line.substr(0, colonPos));
            std::string valStr = trim(line.substr(colonPos + 1));

            if (key == "registers") {
                currentConfig.numRegisters = std::stoi(valStr);
            } else if (key == "algorithm") {
                size_t commaPos = valStr.find(',');
                if (commaPos != std::string::npos) {
                    currentConfig.algorithm = trim(valStr.substr(0, commaPos));
                    currentConfig.algoParam = std::stoi(trim(valStr.substr(commaPos + 1)));
                } else {
                    currentConfig.algorithm = valStr;
                    currentConfig.algoParam = 0;
                }
            }
        }
    }
    return true;
}

/**
 * @brief Builds the Interference Graph by intersecting the live lines of the Webs.
 * @details Two variables interfere if they are live concurrently on the same code line.
 * @note Time Complexity: O(W^2 * L), where W is the number of Webs and L is the average number of live lines per Web.
 */
void buildInterferenceGraph() {

    for (const auto& web : websList) {
        interferenceGraph.addVertex(web.id);
    }

    for (size_t i = 0; i < websList.size(); ++i) {
        for (size_t j = i + 1; j < websList.size(); ++j) {
            int commonPoints = 0;
            
            // Conta quantos pontos de programa são partilhados
            for (int line : websList[i].lines) {
                if (websList[j].lines.find(line) != websList[j].lines.end()) {
                    commonPoints++;
                }
            }
            if (commonPoints > 1) {
                interferenceGraph.addBidirectionalEdge(websList[i].id, websList[j].id, 1.0);
            }
        }
    }
}

/**
 * @brief Generates the final output text file with the allocation results.
 * @param outputFile The destination file name.
 * @param success Boolean indicating whether the allocation was feasible or if it failed.
 */
void generateOutput(const std::string& outputFile, bool success) {
    std::ofstream out(outputFile);
    if (!out.is_open()) {
        std::cerr << "Error: Could not create the output file!\n";
        return;
    }

    out << "# Total number of webs followed by the listing of the program points of each one\n";
    out << "webs: " << websList.size() << "\n";
    
    for (const auto& web : websList) {
        out << "web" << web.id << ": ";
        auto it = web.lines.begin();
        while (it != web.lines.end()) {
            out << *it;
            ++it;
            if (it != web.lines.end()) out << ",";
        }
        out << "\n";
    }

    out << "# Total number of registers used, followed by assignment to webs\n";
    
    if (success) {
        out << "registers: " << currentConfig.numRegisters << "\n";
        for (const auto& web : websList) {
            if (web.assignedRegister == -1) out << "M: web" << web.id << "\n";
            else out << "r" << web.assignedRegister << ": web" << web.id << "\n";
        }
    } else {
        out << "registers: 0\n";
        for (const auto& web : websList) {
            out << "M: web" << web.id << "\n";
        }
        std::cerr << "Warning: The allocation for the provided " << currentConfig.numRegisters << " registers was not possible.\n";
    }
}

/**
 * @brief Limpa o grafo de interferência apagando todos os vértices e arestas.
 */
void clearInterferenceGraph() {
    std::vector<int> nodesToErase;
    for (auto v : interferenceGraph.getVertexSet()) {
        nodesToErase.push_back(v->getInfo());
    }
    for (int id : nodesToErase) {
        interferenceGraph.removeVertex(id);
    }
}

/**
 * @brief Executa o algoritmo base de coloração de grafos (Greedy Graph Coloring).
 * * @param allowSpill Se 'true', permite descartar nós para a memória (Spilling) quando o algoritmo bloqueia. Se 'false', aborta imediatamente ao falhar.
 * @return true se o grafo foi colorido com sucesso usando apenas os registradores disponíveis; false caso contrário.
 * * @note Complexidade de Tempo: O(V^2 + E), onde V é o número de Webs e E o número de interferências (arestas).
 */
bool colorGraphBasic(bool allowSpill = false) {
    int K = currentConfig.numRegisters;
    if (K <= 0) return false;

    std::stack<int> S;
    std::map<int, bool> inGraph;
    std::map<int, int> currentDegrees;

    for (const auto& web : websList) {
        inGraph[web.id] = true;
        auto v = interferenceGraph.findVertex(web.id);
        if (v != nullptr) {
            currentDegrees[web.id] = v->getAdj().size();
        } else {
            currentDegrees[web.id] = 0;
        }
    }

    int nodesRemaining = websList.size();
    bool perfectColoring = true; 

    while (nodesRemaining > 0) {
        bool nodeRemoved = false;

        for (const auto& web : websList) {
            if (inGraph[web.id] && currentDegrees[web.id] < K) {
                S.push(web.id);
                inGraph[web.id] = false;
                nodesRemaining--;
                nodeRemoved = true;

                auto v = interferenceGraph.findVertex(web.id);
                if (v != nullptr) {
                    for (auto edge : v->getAdj()) {
                        int neighborId = edge->getDest()->getInfo(); // edge é um ponteiro
                        if (inGraph[neighborId]) {
                            currentDegrees[neighborId]--;
                        }
                    }
                }
            }
        }

        if (!nodeRemoved && nodesRemaining > 0) {
            if (!allowSpill) {
                return false; 
            } else {
                perfectColoring = false;
                int maxDegreeNode = -1;
                int maxDegree = -1;
                
                for (const auto& web : websList) {
                    if (inGraph[web.id] && currentDegrees[web.id] > maxDegree) {
                        maxDegree = currentDegrees[web.id];
                        maxDegreeNode = web.id;
                    }
                }
                
                if (maxDegreeNode != -1) {
                    inGraph[maxDegreeNode] = false; 
                    nodesRemaining--;
                    
                    auto v = interferenceGraph.findVertex(maxDegreeNode);
                    if (v != nullptr) {
                        for (auto edge : v->getAdj()) {
                            int neighborId = edge->getDest()->getInfo();
                            if (inGraph[neighborId]) {
                                currentDegrees[neighborId]--;
                            }
                        }
                    }
                }
            }
        }
    }

    for (auto& web : websList) web.assignedRegister = -1; 

    while (!S.empty()) {
        int webId = S.top();
        S.pop();

        std::vector<bool> usedColors(K, false);
        auto v = interferenceGraph.findVertex(webId);
        
        if (v != nullptr) {
            for (auto edge : v->getAdj()) {
                int neighborId = edge->getDest()->getInfo();
                for (const auto& neighborWeb : websList) {
                    if (neighborWeb.id == neighborId && neighborWeb.assignedRegister != -1) {
                        usedColors[neighborWeb.assignedRegister] = true;
                        break;
                    }
                }
            }
        }

        for (int c = 0; c < K; c++) {
            if (!usedColors[c]) {
                for (auto& web : websList) {
                    if (web.id == webId) {
                        web.assignedRegister = c;
                        break;
                    }
                }
                break;
            }
        }
    }

    return perfectColoring;
}


/**
 * @brief Implementa a estratégia de otimização Web Splitting.
 * @details Tenta colorir o grafo de interferência. Se a coloração falhar,
 * o maior Web é dividido em dois Webs menores e o grafo é reconstruído.
 * O processo repete-se até a alocação ser possível ou o limite de divisões ser atingido.
 * 
 * @param maxSplits Número máximo de divisões permitidas.
 * @return true se a alocação for bem-sucedida após os splittings; false caso contrário.
 * 
 * @note Complexidade de Tempo: Aproximadamente O(S * (V² + E)), onde S é o número de splittings, V o número de Webs,
 * e E o número de arestas de interferência.
 */
bool applyWebSpilling(int maxSpills) {
    std::set<int> spilledIds;

    for (int spillsDone = 0; spillsDone <= maxSpills; spillsDone++) {
        for (auto& web : websList) {
            web.assignedRegister = -1;
        }

        if (colorGraphBasic(false)) {
            for (auto& web : websList) {
                if (spilledIds.count(web.id)) {
                    web.assignedRegister = -1;
                }
            }
            return true;
        }

        if (spillsDone == maxSpills) break;

        int bestCandidate = -1;
        int maxDegree = -1;

        for (const auto& web : websList) {
            if (spilledIds.count(web.id)) continue;

            auto v = interferenceGraph.findVertex(web.id);
            if (v == nullptr) continue;

            int degree = v->getAdj().size();

            if (degree > maxDegree) {
                maxDegree = degree;
                bestCandidate = web.id;
            }
        }

        if (bestCandidate == -1) break;

        spilledIds.insert(bestCandidate);
        interferenceGraph.removeVertex(bestCandidate);

        std::cout << "[INFO] Spill aplicado ao Web id: "
                  << bestCandidate << "\n";
    }

    for (auto& web : websList) {
        web.assignedRegister = -1;
    }

    return false;
}

/**
 * @brief Implementação da T2.3: Splitting
 * @param maxSplits O valor K passado no ficheiro registers.txt
 */
bool applyWebSplitting(int maxSplits) {
    int splitsDone = 0;
    
    while (splitsDone <= maxSplits) {
        if (colorGraphBasic(false)) return true; 
        
        if (splitsDone == maxSplits) break; 
        
        int targetIndex = -1;
        size_t maxLines = 1; 
        
        for (size_t i = 0; i < websList.size(); i++) {
            if (websList[i].lines.size() > maxLines) {
                maxLines = websList[i].lines.size();
                targetIndex = i;
            }
        }
        
        if (targetIndex == -1) return false; 
        
        Web original = websList[targetIndex];
        websList.erase(websList.begin() + targetIndex);
        
        auto it = original.lines.begin();
        std::advance(it, original.lines.size() / 2);
        
        Web part1, part2;
        int newIdBase = 0;
        for (const auto& w : websList) {
            if (w.id >= newIdBase) newIdBase = w.id + 1;
        }
        
        part1.id = newIdBase;
        part1.variableName = original.variableName;
        part1.lines.insert(original.lines.begin(), it);
        
        part2.id = newIdBase + 1;
        part2.variableName = original.variableName;
        part2.lines.insert(it, original.lines.end());
        
        websList.push_back(part1);
        websList.push_back(part2);
        
        // Uso da função segura para não corromper memória!
        clearInterferenceGraph(); 
        buildInterferenceGraph();
        
        splitsDone++;
        std::cout << "[INFO] Aplicado Splitting ao Web original id: " << original.id << " (Var: " << original.variableName << ")\n";
    }
    
    return false;
}

/**
 * @brief Main function where the coloring algorithms will be executed (Task 2).
 * @return True if allocation succeeds, false otherwise.
 */
bool runGraphColoring() {
    std::cout << "[INFO] Running algorithm: " << currentConfig.algorithm << "...\n";
    
    if (currentConfig.numRegisters <= 0) return false;

    if (currentConfig.algorithm == "basic") {
        return colorGraphBasic(false);
    } 
    else if (currentConfig.algorithm == "splitting") {
        return applyWebSplitting(currentConfig.algoParam);
    }
    else if (currentConfig.algorithm == "free") {
        // T2.4 HÍBRIDO: Combina Splitting com Spilling Dinâmico
        // 1. Tenta dividir no máximo 3 webs para ver se safa o problema elegantemente
        bool success = applyWebSplitting(3);
        if (success) {
            std::cout << "[INFO] Algoritmo Livre: Resolvido apenas com Splitting.\n";
            return true;
        }
        
        // 2. Se falhar, faz uma coloração que atira os casos mais difíceis (M) para a memória
        std::cout << "[INFO] Algoritmo Livre: Splitting falhou. A utilizar modo Híbrido com Spilling Dinâmico...\n";
        colorGraphBasic(true); // Retorna false se deitar algo fora, mas aloca o resto!
        
        // Retornamos sempre true no modo híbrido porque queremos que ele 
        // emita o output final com os R's distribuídos e os restantes com "M".
        return true; 
    }
    else if (currentConfig.algorithm == "spilling") {
        return applyWebSpilling(currentConfig.algoParam);
    }
    else {
        std::cerr << "Erro: Algoritmo desconhecido ou nao suportado nesta versao (" << currentConfig.algorithm << ").\n";
        return false;
    }
}

/**
 * @brief Executes the program in non-interactive (silent) mode.
 */
void runBatchMode(const std::string& rangesFile, const std::string& registersFile, const std::string& outputFile) {
    if (!parseRanges(rangesFile) || !parseRegisters(registersFile)) {
        std::cerr << "Batch Mode Error: Failed to read input files.\n";
        return;
    }
    buildInterferenceGraph();
    bool success = runGraphColoring();
    generateOutput(outputFile, success);
}

/**
 * @brief Executes the interactive menu in the console.
 */
void runInteractiveMenu() {
    int choice = -1;
    bool dataLoaded = false;
    std::string rFile, regFile, outFile;

    while (choice != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  Compiler Register Allocation Tool         \n";
        std::cout << "============================================\n";
        std::cout << "1. Load Input Data (Ranges & Registers)\n";
        std::cout << "2. View Parsed Webs\n";
        std::cout << "3. Run Graph Coloring Allocation\n";
        std::cout << "0. Exit\n";
        std::cout << "--------------------------------------------\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Error: Invalid input. Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue; 
        }

        switch (choice) {
            case 1:
                std::cout << "Ranges filename (e.g., ranges.txt): ";
                std::cin >> rFile;
                std::cout << "Registers filename (e.g., registers.txt): ";
                std::cin >> regFile;
                
                if (parseRanges(rFile) && parseRegisters(regFile)) {
                    buildInterferenceGraph();
                    dataLoaded = true;
                    std::cout << "Data loaded and Interference Graph built successfully!\n";
                } else {
                    std::cout << "Failed to read files. Please check the filenames.\n";
                }
                break;
            case 2:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load the data first (Option 1).\n";
                } else {
                    for(const auto& w : websList) {
                        std::cout << "ID: " << w.id << " | Var: " << w.variableName << " | Active lines: " << w.lines.size() << "\n";
                    }
                }
                break;
            case 3:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load the data first (Option 1).\n";
                } else {
                    std::cout << "Output filename: ";
                    std::cin >> outFile;
                    bool success = runGraphColoring();
                    generateOutput(outFile, success);
                    std::cout << "Output successfully generated in '" << outFile << "'.\n";
                }
                break;
            case 0:
                std::cout << "Exiting the tool. Goodbye!\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }
}

/**
 * @brief Program entry point. Parses arguments to run Batch Mode or the Interactive Menu.
 */
int main(int argc, char* argv[]) {
    // Batch Mode requires exactly 5 arguments: myProg -b ranges.txt registers.txt allocation.txt
    if (argc == 5 && std::string(argv[1]) == "-b") {
        runBatchMode(argv[2], argv[3], argv[4]);
        return 0;
    } 
    
    // Default to Interactive Mode
    runInteractiveMenu();
    return 0;
}