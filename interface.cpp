#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>
#include "DataStructures.h"
#include "Graph.h"


void runInteractiveMenu();
void runBatchMode(const std::string& inputFile, const std::string& outputFile);
bool parseInputData(const std::string& filename); // We will build this later
void generateAssignments(std::ofstream* outFile = nullptr);
void runRiskAnalysis(std::ofstream* outFile = nullptr);

// --- Global Data (From Step 1) ---
std::vector<Submission> submissionList;
std::vector<Reviewer> reviewerList;
AppConfig currentConfig;

struct Assignment {
    int submissionId;
    int reviewerId;
    int matchDomain;
};


// --- Graph Building and Max-Flow Function ---

double buildAndSolve(Graph<std::string>& g, int skipReviewerId = -1) {
    g.addVertex("SOURCE");
    g.addVertex("SINK");
 
    // 1. SOURCE -> Submissions
    for (const auto& sub : submissionList) {
        std::string sNode = "S_" + std::to_string(sub.id);
        g.addVertex(sNode);
        // The capacity is the exact number of reviews this paper DEMANDS
        g.addBidirectionalEdge("SOURCE", sNode, currentConfig.minReviewsPerSubmission); 
    }
 
    // 2. Reviewers -> SINK
    for (const auto& rev : reviewerList) {
        if (rev.id == skipReviewerId) continue;
        std::string rNode = "R_" + std::to_string(rev.id);
        g.addVertex(rNode);
        // The capacity is the MAX SUPPLY this reviewer can provide
        g.addBidirectionalEdge(rNode, "SINK", currentConfig.maxReviewsPerReviewer);
    }
 
    // 3. Submissions -> Reviewers (The Match)
    for (const auto& sub : submissionList) {
        std::string sNode = "S_" + std::to_string(sub.id);
 
        for (const auto& rev : reviewerList) {
            if (rev.id == skipReviewerId) continue;
            std::string rNode = "R_" + std::to_string(rev.id);
 
            if (rev.primaryExpertise == sub.primaryDomain) {
                // A reviewer can only review a specific paper ONCE
                g.addBidirectionalEdge(sNode, rNode, 1);
            }
        }
    }
 
    // 4. Run the algorithm
    // If the return value == (Total Submissions * minReviewsPerSubmission), 
    // then the assignment was 100% successful!
    return g.edmondsKarp("SOURCE", "SINK"); 
}


// This function will extract the assignments from the graph after running Max-Flow

std::vector<Assignment> getAssignments(Graph<std::string>& g) {
    std::vector<Assignment> assignments;
 
    for (auto e : g.getUsedEdges()) {
        std::string orig = e->getOrig()->getInfo();
        std::string dest = e->getDest()->getInfo();
 
        if (orig.size() > 2 && orig.substr(0, 2) == "S_" &&
            dest.size() > 2 && dest.substr(0, 2) == "R_") {
 
            int subId = std::stoi(orig.substr(2)); // orig is now S_
            int revId = std::stoi(dest.substr(2)); // dest is now R_
        }
    }
 
    std::sort(assignments.begin(), assignments.end(),
        [](const Assignment& a, const Assignment& b) {
            return a.submissionId < b.submissionId;
        });
 
    return assignments;
}

// This function will write the output to a file or standard output based on the success of the assignment

void writeAssignmentOutput(const std::vector<Assignment>& assignments, std::ostream& out, double maxFlow) {
    int required = (int)submissionList.size() * currentConfig.minReviewsPerSubmission;
    bool success = ((int)maxFlow >= required);
 
    if (success) {
        // Submission -> Reviewer view
        out << "#SubmissionId,ReviewerId,Match\n";
        for (const auto& a : assignments) {
            out << a.submissionId << ", " << a.reviewerId << ", " << a.matchDomain << "\n";
        }
 
        // Reviewer -> Submission view (sorted by reviewerId)
        out << "#ReviewerId,SubmissionId,Match\n";
        std::vector<Assignment> byReviewer = assignments;
        std::sort(byReviewer.begin(), byReviewer.end(),
            [](const Assignment& a, const Assignment& b) {
                return a.reviewerId < b.reviewerId;
            });
        for (const auto& a : byReviewer) {
            out << a.reviewerId << ", " << a.submissionId << ", " << a.matchDomain << "\n";
        }
 
        out << "#Total: " << (int)assignments.size() << "\n";
 
    } else {
        // Count how many reviews each submission actually got
        std::map<int, int> reviewCount;
        for (const auto& sub : submissionList) reviewCount[sub.id] = 0;
        for (const auto& a : assignments)       reviewCount[a.submissionId]++;
 
        out << "#SubmissionId,Domain,MissingReviews\n";
        for (const auto& sub : submissionList) {
            int missing = currentConfig.minReviewsPerSubmission - reviewCount[sub.id];
            if (missing > 0) {
                out << sub.id << ", " << sub.primaryDomain << ", " << missing << "\n";
            }
        }
    }
}


// --- Main Logic for Generating Assignments and Running Risk Analysis ---

void generateAssignments(std::ofstream* outFile) {
    if (submissionList.empty() || reviewerList.empty()) {
        std::cerr << "Error: No data loaded.\n";
        return;
    }
 
    Graph<std::string> g;
    double maxFlow = buildAndSolve(g);
    int required = (int)submissionList.size() * currentConfig.minReviewsPerSubmission;
 
    if ((int)maxFlow >= required) {
        std::cout << "Assignment successful! Total reviews assigned: " << (int)maxFlow << "\n";
    } else {
        std::cout << "Warning: Could not meet minimum reviews for all submissions.\n";
        std::cout << "Achieved " << (int)maxFlow << " of " << required << " required reviews.\n";
    }
 
    // Only write to file if GenerateAssignments != 0
    if (currentConfig.generateAssignments != 0) {
        auto assignments = getAssignments(g);
 
        if (outFile && outFile->is_open()) {
            // Batch mode: write to the provided file stream
            writeAssignmentOutput(assignments, *outFile, maxFlow);
        } else {
            // Interactive mode: write to the configured output file
            std::ofstream f(currentConfig.outputFileName);
            if (!f.is_open()) {
                std::cerr << "Error: Could not open output file " << currentConfig.outputFileName << "\n";
                return;
            }
            writeAssignmentOutput(assignments, f, maxFlow);
            f.close();
            std::cout << "Output written to: " << currentConfig.outputFileName << "\n";
        }
    }
}

void runRiskAnalysis(std::ofstream* outFile) {
    if (submissionList.empty() || reviewerList.empty()) {
        std::cerr << "Error: No data loaded.\n";
        return;
    }
 
    int required = (int)submissionList.size() * currentConfig.minReviewsPerSubmission;
    std::vector<int> riskyReviewers;
 
    std::cout << "Running risk analysis (testing " << reviewerList.size()
              << " reviewers)...\n";
 
    for (const auto& rev : reviewerList) {
        Graph<std::string> g;
        double flow = buildAndSolve(g, rev.id); // exclude this reviewer
        if ((int)flow < required) {
            riskyReviewers.push_back(rev.id);
        }
    }
 
    std::sort(riskyReviewers.begin(), riskyReviewers.end());
 
    // Build the output text
    std::ostringstream result;
    result << "#Risk Analysis: 1\n";
    for (int i = 0; i < (int)riskyReviewers.size(); i++) {
        if (i > 0) result << ", ";
        result << riskyReviewers[i];
    }
    if (!riskyReviewers.empty()) result << "\n";
 
    // Print to console
    if (riskyReviewers.empty()) {
        std::cout << "Result: No single reviewer is critical. Assignment holds even if any one drops out.\n";
    } else {
        std::cout << "Result: The following reviewers are CRITICAL (removing any one breaks the assignment):\n";
        for (int id : riskyReviewers) {
            for (const auto& rev : reviewerList) {
                if (rev.id == id) {
                    std::cout << "  Reviewer " << id << " (" << rev.name << ")\n";
                    break;
                }
            }
        }
    }
 
    // Write to file
    if (outFile && outFile->is_open()) {
        *outFile << result.str();
    } else {
        // Append to the output file (assignment may already be there)
        std::ofstream f(currentConfig.outputFileName, std::ios::app);
        if (f.is_open()) {
            f << result.str();
            f.close();
            std::cout << "Risk analysis appended to: " << currentConfig.outputFileName << "\n";
        } else {
            std::cerr << "Error: Could not open output file for risk analysis.\n";
        }
    }
}




int main(int argc, char* argv[]) {
    // 1. Check for Batch Mode
    // The command should be: myProg -b input.csv output.csv 
    // argc will be 4: [0]=myProg, [1]="-b", [2]="input.csv", [3]="output.csv"
    if (argc == 4 && std::string(argv[1]) == "-b") {
        std::string inputFile = argv[2];
        std::string outputFile = argv[3];
        
        std::cout << "--- Starting Batch Mode ---" << std::endl;
        runBatchMode(inputFile, outputFile);
        
        return 0; // Exit the program after batch processing finishes
    }

    // 2. Default to Interactive Mode
    // If the arguments don't match the batch pattern, load the menu 
    runInteractiveMenu();

    return 0;
}

// --- Function Definitions (Skeletons for now) ---

void runBatchMode(const std::string& inputFile, const std::string& outputFile) {
    if (parseInputData(inputFile)) {
        std::cout << "Data loaded. Processing assignment..." << std::endl;
        
        std::ofstream outFile(outputFile);
        if (!outFile.is_open()) {
            std::cerr << "Batch Mode Error: Could not open " << outputFile << " for writing." << std::endl;
            return;
        }

        // Trigger the assignment logic you wrote
        generateAssignments(&outFile);

        // Task 3.2: If the input file's #Control section enabled RiskAnalysis
        if (currentConfig.riskAnalysis != 0) {
            runRiskAnalysis(&outFile);
        }

        outFile.close();
        std::cout << "Batch processing complete. Results saved to " << outputFile << std::endl;
    } else {
        std::cerr << "Batch Mode Error: Failed to parse the input file." << std::endl;
    }
}

void runInteractiveMenu() {
    int choice = -1;
    std::string filename;
    bool dataLoaded = false; // Flag to prevent running algorithms on empty data

    // The menu loop will continue until the user enters 0
    while (choice != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  Scientific Conference Organization Tool   \n";
        std::cout << "============================================\n";
        std::cout << "1. Load Input Data (.csv)\n";
        std::cout << "2. View Loaded Submissions\n";
        std::cout << "3. View Loaded Reviewers\n";
        std::cout << "4. Generate Assignments (Base Max-Flow)\n";
        std::cout << "5. Run Risk Analysis\n";
        std::cout << "0. Exit\n";
        std::cout << "--------------------------------------------\n";
        std::cout << "Enter your choice: ";

        // --- SAFE INPUT HANDLING ---
        if (!(std::cin >> choice)) {
            // If the input wasn't an integer, std::cin goes into a "fail state"
            std::cout << "Error: Invalid input. Please enter a number.\n";
            
            std::cin.clear(); // 1. Clear the fail state
            // 2. Ignore all leftover characters in the buffer until the next new line
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            
            continue; // Skip the switch statement and restart the loop
        }

        // --- MENU ROUTING ---
        switch (choice) {
            case 1:
                std::cout << "Enter the name of the input file (e.g., input.csv): ";
                std::cin >> filename;
                
                // Call the parser. If it returns true, update our flag.
                if (parseInputData(filename)) {
                    dataLoaded = true;
                }
                break;

            case 2:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\n--- Submissions ---\n";
                    // TODO: Loop through submissionList and print them
                    // std::cout << "Total submissions loaded: " << submissionList.size() << "\n";
                    std::cout << "(Display logic to be implemented)\n";
                }
                break;

            case 3:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\n--- Reviewers ---\n";
                    // TODO: Loop through reviewerList and print them
                    // std::cout << "Total reviewers loaded: " << reviewerList.size() << "\n";
                    std::cout << "(Display logic to be implemented)\n";
                }
                break;

            case 4: // Generate Assignments
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\nRunning Max-Flow Assignment Algorithm...\n";
                    // This uses the output filename defined in the #Control section of the CSV
                    generateAssignments(); 
                }
                break;

            case 5: // Run Risk Analysis
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\nRunning Risk Analysis (N-1)...\n";
                    runRiskAnalysis(); 
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

// Helper function to remove leading and trailing whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return ""; // String is all whitespace
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool parseInputData(const std::string& filename) {
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return false;
    }

    // Clear existing data in case the user loads a file twice from the menu
    submissionList.clear();
    reviewerList.clear();

    std::string line;
    // Our state machine to track which section we are reading
    enum ParseState { NONE, SUBMISSIONS, REVIEWERS, PARAMETERS, CONTROL };
    ParseState currentState = NONE;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;

        // 1. Check for exact section headers
        if (line == "#Submissions") { currentState = SUBMISSIONS; continue; }
        if (line == "#Reviewers")   { currentState = REVIEWERS; continue; }
        if (line == "#Parameters")  { currentState = PARAMETERS; continue; }
        if (line == "#Control")     { currentState = CONTROL; continue; }

        // 2. Handle Comments
        // The '#' character ignores everything after it, unless it's a section header (handled above)
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos); // Cut off the comment part
            line = trim(line);
            if (line.empty()) continue; // If the whole line was a comment, skip it
        }

        // 3. Tokenize the line by commas
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(ss, token, ',')) {
            tokens.push_back(trim(token));
        }

        // 4. Extract data based on the current state
        try {
            if (currentState == SUBMISSIONS && tokens.size() >= 5) {
                Submission sub;
                sub.id = std::stoi(tokens[0]);
                sub.title = tokens[1];
                sub.authors = tokens[2];
                sub.email = tokens[3];
                sub.primaryDomain = std::stoi(tokens[4]);
                
                // Secondary domain is optional
                if (tokens.size() >= 6 && !tokens[5].empty()) {
                    sub.secondaryDomain = std::stoi(tokens[5]);
                }
                submissionList.push_back(sub);
            } 
            else if (currentState == REVIEWERS && tokens.size() >= 4) {
                Reviewer rev;
                rev.id = std::stoi(tokens[0]);
                rev.name = tokens[1];
                rev.email = tokens[2];
                rev.primaryExpertise = std::stoi(tokens[3]);
                
                // Secondary expertise is optional
                if (tokens.size() >= 5 && !tokens[4].empty()) {
                    rev.secondaryExpertise = std::stoi(tokens[4]);
                }
                reviewerList.push_back(rev);
            }
            else if ((currentState == PARAMETERS || currentState == CONTROL) && tokens.size() >= 2) {
                std::string key = tokens[0];
                
                if (key == "OutputFileName") {
                    // Remove quotes if present, e.g., "assign.csv"
                    std::string val = tokens[1];
                    if (val.front() == '"' && val.back() == '"') {
                        val = val.substr(1, val.length() - 2);
                    }
                    currentConfig.outputFileName = val;
                } else {
                    int val = std::stoi(tokens[1]);
                    if (key == "MinReviewsPerSubmission") currentConfig.minReviewsPerSubmission = val;
                    else if (key == "MaxReviewsPerReviewer") currentConfig.maxReviewsPerReviewer = val;
                    else if (key == "PrimaryReviewerExpertise") currentConfig.primaryReviewerExpertise = val;
                    else if (key == "SecondaryReviewerExpertise") currentConfig.secondaryReviewerExpertise = val;
                    else if (key == "PrimarySubmissionDomain") currentConfig.primarySubmissionDomain = val;
                    else if (key == "SecondarySubmissionDomain") currentConfig.secondarySubmissionDomain = val;
                    else if (key == "GenerateAssignments") currentConfig.generateAssignments = val;
                    else if (key == "RiskAnalysis") currentConfig.riskAnalysis = val;
                }
            }
        } catch (const std::exception& e) {
            // Files with inconsistent values should be rejected 
            std::cerr << "Summary Error: Inconsistent data format encountered in line: " << line << "\n";
            return false; 
        }
    }

    std::cout << "Successfully loaded " << submissionList.size() << " submissions and " 
              << reviewerList.size() << " reviewers.\n";
    return true;
}

