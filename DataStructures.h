#include <string>
#include <vector>

struct Submission {
    int id;
    std::string title;
    std::string authors;
    std::string email;
    int primaryDomain;
    int secondaryDomain = -1; // We use -1 to indicate that no secondary domain was provided
};

struct Reviewer {
    int id;
    std::string name;
    std::string email;
    int primaryExpertise;
    int secondaryExpertise = -1; // We use -1 to indicate that no secondary expertise was provided
};

struct AppConfig {
    // Parameters
    int minReviewsPerSubmission = 0;
    int maxReviewsPerReviewer = 0;
    int primaryReviewerExpertise = 0;
    int secondaryReviewerExpertise = 0;
    int primarySubmissionDomain = 0;
    int secondarySubmissionDomain = 0;

    // Control
    int generateAssignments = 0;
    int riskAnalysis = 0;
    std::string outputFileName = "output.csv"; // Default file name if not specified
};