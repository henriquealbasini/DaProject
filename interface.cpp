#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>
#include <unistd.h>
#include "DataStructures.h"
#include "Graph.h"

// --- Function Prototypes ---
void runBatchMode(const std::string& inputFile, const std::string& outputFile);
bool parseInputData(const std::string& filename);
void generateAssignments(std::ofstream* outFile = nullptr);
void runRiskAnalysis(std::ofstream* outFile = nullptr);
void runInteractiveMenu() ;

// --- Global Data ---
std::vector<Submission> submissionList;
std::vector<Reviewer> reviewerList;
AppConfig currentConfig;

struct Assignment {
    int submissionId;
    int reviewerId;
    int matchDomain;
};

/**
 * Determines compatibility between a Reviewer and a Submission 
 * based on the 4 toggles in the #Parameters section.
 * Returns the matching Domain ID, or -1 if no match exists.
 */
int getMatchDomain(const Reviewer& rev, const Submission& sub) {
    // 1. Check combinations using Reviewer's Primary Expertise
    if (currentConfig.primaryReviewerExpertise == 1) {
        // Primary Reviewer vs Primary Submission
        if (currentConfig.primarySubmissionDomain == 1 && rev.primaryExpertise == sub.primaryDomain)
            return rev.primaryExpertise;
        // Primary Reviewer vs Secondary Submission
        if (currentConfig.secondarySubmissionDomain == 1 && sub.secondaryDomain != -1 && rev.primaryExpertise == sub.secondaryDomain)
            return rev.primaryExpertise;
    }

    // 2. Check combinations using Reviewer's Secondary Expertise
    if (currentConfig.secondaryReviewerExpertise == 1 && rev.secondaryExpertise != -1) {
        // Secondary Reviewer vs Primary Submission
        if (currentConfig.primarySubmissionDomain == 1 && rev.secondaryExpertise == sub.primaryDomain)
            return rev.secondaryExpertise;
        // Secondary Reviewer vs Secondary Submission
        if (currentConfig.secondarySubmissionDomain == 1 && sub.secondaryDomain != -1 && rev.secondaryExpertise == sub.secondaryDomain)
            return rev.secondaryExpertise;
    }
    return -1;
}

/**
 * Builds the Bipartite Graph for Max Flow.
 * SOURCE -> Submissions (capacity = reviews needed)
 * Submissions -> Reviewers (capacity = 1, if compatible)
 * Reviewers -> SINK (capacity = max load per reviewer)
 */
double buildAndSolve(Graph<std::string>& g, int skipReviewerId = -1) {
    g.addVertex("SOURCE");
    g.addVertex("SINK");

    for (const auto& sub : submissionList) {
        std::string sNode = "S_" + std::to_string(sub.id);
        g.addVertex(sNode);
        g.addBidirectionalEdge("SOURCE", sNode, currentConfig.minReviewsPerSubmission);

        for (const auto& rev : reviewerList) {
            if (rev.id == skipReviewerId) continue; // Used for Risk Analysis
            
            std::string rNode = "R_" + std::to_string(rev.id);
            g.addVertex(rNode);
            
            // Connect if the configuration allows this match
            if (getMatchDomain(rev, sub) != -1) {
                g.addBidirectionalEdge(sNode, rNode, 1);
            }
        }
    }

    for (const auto& rev : reviewerList) {
        if (rev.id == skipReviewerId) continue;
        std::string rNode = "R_" + std::to_string(rev.id);
        g.addBidirectionalEdge(rNode, "SINK", currentConfig.maxReviewsPerReviewer);
    }

    return g.edmondsKarp("SOURCE", "SINK");
}

/**
 * Extracts flow from the graph to generate the CSV output tables.
 * This version uses the Edge objects to retrieve flow data since 
 * the Graph class does not have a direct getFlow method.
 */
void generateAssignments(std::ofstream* outFile) {
    Graph<std::string> g;
    // buildAndSolve populates the graph and runs Edmonds-Karp
    double maxFlow = buildAndSolve(g);
    std::vector<Assignment> assignments;

    // 1. Iterate through all submissions to check for assigned reviewers
    for (const auto& sub : submissionList) {
        std::string sNode = "S_" + std::to_string(sub.id);
        Vertex<std::string>* vSub = g.findVertex(sNode);
        
        if (vSub == nullptr) continue;

        // 2. Scan all outgoing edges from this Submission vertex
        // In our graph, these edges point toward Reviewer nodes
        for (auto* edge : vSub->getAdj()) {
            // Check if this edge has positive flow
            if (edge->getFlow() > 0) {
                Vertex<std::string>* vDest = edge->getDest();
                std::string destInfo = vDest->getInfo();

                // 3. Verify the destination is a Reviewer (starts with "R_")
                if (destInfo.substr(0, 2) == "R_") {
                    int revId = std::stoi(destInfo.substr(2));
                    
                    // 4. Find the matching Reviewer object to identify the specific Match Domain
                    for (const auto& rev : reviewerList) {
                        if (rev.id == revId) {
                            // getMatchDomain determines which domain ID linked them
                            assignments.push_back({sub.id, rev.id, getMatchDomain(rev, sub)});
                            break;
                        }
                    }
                }
            }
        }
    }

    // --- Output and Sorting Logic ---
    std::ostream& out = (outFile && outFile->is_open()) ? *outFile : std::cout;

    // Table 1: Primary Sort by SubmissionId, Secondary by ReviewerId
    out << "#SubmissionId,ReviewerId,Match\n";
    std::sort(assignments.begin(), assignments.end(), [](const Assignment& a, const Assignment& b) {
        if (a.submissionId != b.submissionId) return a.submissionId < b.submissionId;
        return a.reviewerId < b.reviewerId;
    });
    for (const auto& a : assignments) {
        out << a.submissionId << ", " << a.reviewerId << ", " << a.matchDomain << "\n";
    }

    // Table 2: Primary Sort by ReviewerId, Secondary by SubmissionId
    out << "#ReviewerId,SubmissionId,Match\n";
    std::vector<Assignment> byRev = assignments;
    std::sort(byRev.begin(), byRev.end(), [](const Assignment& a, const Assignment& b) {
        if (a.reviewerId != b.reviewerId) return a.reviewerId < b.reviewerId;
        return a.submissionId < b.submissionId;
    });
    for (const auto& a : byRev) {
        out << a.reviewerId << ", " << a.submissionId << ", " << a.matchDomain << "\n";
    }

    out << "#Total: " << (int)assignments.size() << "\n";

    // Failure Report: Identify papers that didn't get enough reviews
    int totalRequired = (int)submissionList.size() * currentConfig.minReviewsPerSubmission;
    if ((int)maxFlow < totalRequired) {
        std::map<int, int> counts;
        for (const auto& a : assignments) counts[a.submissionId]++;
        
        out << "#SubmissionId,Domain,MissingReviews\n";
        for (const auto& sub : submissionList) {
            int missing = currentConfig.minReviewsPerSubmission - counts[sub.id];
            if (missing > 0) {
                out << sub.id << ", " << sub.primaryDomain << ", " << missing << "\n";
            }
        }
    }
}
/**
 * Identifies "Critical" reviewers whose absence makes the required flow impossible.
 */
void runRiskAnalysis(std::ofstream* outFile) {
    int required = (int)submissionList.size() * currentConfig.minReviewsPerSubmission;
    std::vector<int> risky;

    for (const auto& rev : reviewerList) {
        Graph<std::string> g;
        // If removing this reviewer drops the flow below the requirement, they are risky
        if ((int)buildAndSolve(g, rev.id) < required) {
            risky.push_back(rev.id);
        }
    }

    std::ostream& out = (outFile && outFile->is_open()) ? *outFile : std::cout;
    out << "#Risk Analysis: " << currentConfig.riskAnalysis << "\n";
    for (size_t i = 0; i < risky.size(); ++i) {
        out << risky[i] << (i == risky.size() - 1 ? "" : ", ");
    }
    out << "\n";
}

// --- Helper: Basic string trimming for C++ ---
std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \r\n\t");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \r\n\t");
    return s.substr(first, (last - first + 1));
}

/**
 * Parses the CSV input and populates global lists and configuration.
 */
bool parseInputData(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "DEBUG: Could not open file '" << filename << "' (cwd=";
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << cwd;
        }
        std::cout << ")\n";
        return false;
    }

    submissionList.clear();
    reviewerList.clear();
    std::string line, section = "";

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            if (line == "#Submissions") section = "SUB";
            else if (line == "#Reviewers") section = "REV";
            else if (line == "#Parameters") section = "PARAM";
            else if (line == "#Control") section = "CTRL";
            continue;
        }

        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> tokens;
        while (std::getline(ss, item, ',')) tokens.push_back(trim(item));

        if (section == "SUB" && tokens.size() >= 5) {
            Submission s;
            s.id = std::stoi(tokens[0]);
            s.primaryDomain = std::stoi(tokens[4]);
            s.secondaryDomain = (tokens.size() > 5 && !tokens[5].empty()) ? std::stoi(tokens[5]) : -1;
            submissionList.push_back(s);
        } else if (section == "REV" && tokens.size() >= 4) {
            Reviewer r;
            r.id = std::stoi(tokens[0]);
            r.primaryExpertise = std::stoi(tokens[3]);
            r.secondaryExpertise = (tokens.size() > 4 && !tokens[4].empty()) ? std::stoi(tokens[4]) : -1;
            reviewerList.push_back(r);
        } else if (section == "PARAM" || section == "CTRL") {
            std::string key = tokens[0];
            if (key == "OutputFileName") continue;
            int val = std::stoi(tokens[1]);
            if (key == "MinReviewsPerSubmission") currentConfig.minReviewsPerSubmission = val;
            else if (key == "MaxReviewsPerReviewer") currentConfig.maxReviewsPerReviewer = val;
            else if (key == "PrimaryReviewerExpertise") currentConfig.primaryReviewerExpertise = val;
            else if (key == "SecondaryReviewerExpertise") currentConfig.secondaryReviewerExpertise = val;
            else if (key == "PrimarySubmissionDomain") currentConfig.primarySubmissionDomain = val;
            else if (key == "SecondarySubmissionDomain") currentConfig.secondarySubmissionDomain = val;
            else if (key == "RiskAnalysis") currentConfig.riskAnalysis = val;
        }
    }
    return true;
}

void runBatchMode(const std::string& inputFile, const std::string& outputFile) {
    if (parseInputData(inputFile)) {
        std::ofstream outFile(outputFile);
        if (outFile.is_open()) {
            generateAssignments(&outFile);
            if (currentConfig.riskAnalysis != 0) {
                runRiskAnalysis(&outFile);
            }
            outFile.close();
        }
    }
}

void runInteractiveMenu() {
    int choice = -1;
    std::string filename;
    
    // This flag prevents the user from crashing the program by running algorithms on empty data
    bool dataLoaded = false; 

    while (choice != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  Scientific Conference Organization Tool   \n";
        std::cout << "============================================\n";
        std::cout << "1. Load Input Data (.csv)\n";
        std::cout << "2. View Loaded Submissions\n";
        std::cout << "3. View Loaded Reviewers\n";
        std::cout << "4. Generate Assignments (Max-Flow)\n";
        std::cout << "5. Run Risk Analysis\n";
        std::cout << "0. Exit\n";
        std::cout << "--------------------------------------------\n";
        std::cout << "Enter your choice: ";

        // --- SAFE INPUT HANDLING ---
        // Prevents an infinite loop if the user accidentally types a letter instead of a number
        if (!(std::cin >> choice)) {
            std::cout << "Error: Invalid input. Please enter a number.\n";
            std::cin.clear(); // Clear the error state
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard bad input
            continue; 
        }

        // --- MENU ROUTING ---
        switch (choice) {
            case 1:
                std::cout << "Enter the name of the input file (e.g., dataset4.csv): ";
                std::cin >> filename;
                
                // If the parser returns true, update our flag to unlock the other options
                if (parseInputData(filename)) {
                    dataLoaded = true;
                    std::cout << "Data loaded successfully.\n";
                } else {
                    std::cout << "Failed to load data (file missing or invalid format).\n";
                }
                break;

            case 2:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\n--- Submissions ---\n";
                    std::cout << "Total submissions loaded: " << submissionList.size() << "\n";
                    // (Optional) You can loop through submissionList here to print them to the console
                }
                break;

            case 3:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\n--- Reviewers ---\n";
                    std::cout << "Total reviewers loaded: " << reviewerList.size() << "\n";
                    // (Optional) You can loop through reviewerList here to print them to the console
                }
                break;

            case 4:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\nRunning Max-Flow Assignment Algorithm...\n";
                    // We pass nullptr because in Interactive Mode, the function 
                    // handles opening and closing its own output file based on currentConfig
                    generateAssignments(nullptr); 
                }
                break;

            case 5:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\nRunning Risk Analysis...\n";
                    // We pass nullptr for the same reason as above
                    runRiskAnalysis(nullptr);
                }
                break;

            case 0:
                std::cout << "Exiting the tool. Goodbye!\n";
                break;

            default:
                std::cout << "Invalid choice. Please select an option between 0 and 5.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    
    // 1. Check for Batch Mode: ./prog -b input.csv output.csv
    if (argc == 4 && std::string(argv[1]) == "-b") {
        
        runBatchMode(argv[2], argv[3]);
        return 0; // Exit immediately after batch processing finishes
    } 
    
    // 2. Default to Interactive Mode
    // If the user runs anything else (like just ./prog), launch the menu
    
    runInteractiveMenu();
    
    return 0;
}