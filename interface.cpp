#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include <fstream>
#include "DataStructures.h"

void runInteractiveMenu();
void runBatchMode(const std::string& inputFile, const std::string& outputFile);
bool parseInputData(const std::string& filename); // We will build this later

// --- Global Data (From Step 1) ---
std::vector<Submission> submissionList;
std::vector<Reviewer> reviewerList;

AppConfig currentConfig;

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
    // Attempt to parse the file
    if (parseInputData(inputFile)) {
        std::cout << "Data loaded successfully from " << inputFile << "." << std::endl;
        
        // TODO: Later, we will call your Graph/Max-Flow algorithms here
        
        std::cout << "Process complete. Output generated for: " << outputFile << std::endl;
    } else {
        // The project requires error messages to be directed to standard error 
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

            case 4:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\nRunning Max-Flow Assignment Algorithm...\n";
                    // TODO: Call your graph building and Max-Flow functions here
                }
                break;

            case 5:
                if (!dataLoaded) {
                    std::cout << "Warning: Please load data first (Option 1).\n";
                } else {
                    std::cout << "\nRunning Risk Analysis...\n";
                    // TODO: Call your risk analysis functions here
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

