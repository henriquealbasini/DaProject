#include <iostream>
#include <string>
#include <vector>
#include <limits>

// (If you put your structs in a separate file, include it here)
// #include "DataStructures.h"

// --- Function Declarations (The "Promises" we talked about!) ---
void runInteractiveMenu();
void runBatchMode(const std::string& inputFile, const std::string& outputFile);
bool parseInputData(const std::string& filename); // We will build this later

// --- Global Data (From Step 1) ---
// std::vector<Submission> submissionList;
// std::vector<Reviewer> reviewerList;
// AppConfig currentConfig;

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

bool parseInputData(const std::string& filename) {
    // TODO: We will build the file reading logic here in Steps 4 & 5
    return false; 
}