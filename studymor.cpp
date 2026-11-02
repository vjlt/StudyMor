/*
 * Program Name: studymor.cpp
 * Author:       Vince Tarnecy
 * Date:         11-1-26
 * Purpose:      Runs a program that takes in a particpant's name and general information, and assigns them 
 *               either a real study-enhancing drug called StudyMor, or a placebo. The participants fill out a survey
 *               to help determine the effects of StudyMor. The participant information and the survey answers 
 *               are put into files named participant.dat, and survey.dat, respectively.
 *
 * Runs with: g++ -std=c++11 studymor.cpp -o studymor && ./studymor
 */

#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const string PARTICIPANT_FILE = "participant.dat";
const string SURVEY_FILE = "survey.dat";
const char DELIMITER = '|';

const int MIN_AGE = 1;
const int MAX_AGE = 120;

struct Participant {
    int id;
    string firstName;
    string lastName;
    int age;
    string gender;
    string treatment;
};

// Removes spaces and tabs from both ends of the text.
string trim(const string& text) {
    const string whitespace = " \t\r";
    size_t first = text.find_first_not_of(whitespace);
    if (first == string::npos) {
        return "";
    }
    size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

// Converts text to a whole number. Returns false for input such as "abc" or "12x".
bool parseInt(const string& text, int& value) {
    istringstream input(text);
    char extra;
    return (input >> value) && !(input >> extra);
}

// Splits one line of a data file into its fields.
vector<string> splitFields(const string& line) {
    vector<string> fields;
    string field;
    istringstream input(line);
    while (getline(input, field, DELIMITER)) {
        fields.push_back(field);
    }
    return fields;
}

// Asks for a line of text until a usable answer is given. The delimiter is
// rejected because it would break the record apart when the file is read back.
// Returns false only if the input has ended (for example, Ctrl+D).
bool promptText(const string& prompt, bool allowBlank, string& answer) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) {
            return false;
        }
        answer = trim(line);

        if (answer.find(DELIMITER) != string::npos) {
            cout << "Please do not use the '" << DELIMITER << "' character.\n";
        } else if (answer.empty() && !allowBlank) {
            cout << "This field cannot be blank.\n";
        } else {
            return true;
        }
    }
}

// Asks for a whole number until one inside the given range is entered.
bool promptInt(const string& prompt, int minValue, int maxValue, int& value) {
    while (true) {
        string answer;
        if (!promptText(prompt, false, answer)) {
            return false;
        }
        if (parseInt(answer, value) && value >= minValue && value <= maxValue) {
            return true;
        }
        cout << "Please enter a whole number from " << minValue << " to " << maxValue << ".\n";
    }
}

// Asks a yes/no question and stores the answer as "Yes" or "No" so the file
// stays consistent for calculating statistics later.
bool promptYesNo(const string& question, string& answer) {
    while (true) {
        string input;
        if (!promptText(question + " (y/n): ", false, input)) {
            return false;
        }
        for (size_t i = 0; i < input.length(); i++) {
            input[i] = static_cast<char>(tolower(static_cast<unsigned char>(input[i])));
        }

        if (input == "y" || input == "yes") {
            answer = "Yes";
            return true;
        }
        if (input == "n" || input == "no") {
            answer = "No";
            return true;
        }
        cout << "Please answer y or n.\n";
    }
}

// Reads every participant from the data file. A missing file just means that
// no participants have been added yet.
vector<Participant> loadParticipants() {
    vector<Participant> participants;
    ifstream file(PARTICIPANT_FILE.c_str());
    string line;

    while (getline(file, line)) {
        if (trim(line).empty()) {
            continue;
        }

        vector<string> fields = splitFields(line);
        Participant person;
        if (fields.size() == 6 && parseInt(fields[0], person.id) && parseInt(fields[3], person.age)) {
            person.firstName = fields[1];
            person.lastName = fields[2];
            person.gender = fields[4];
            person.treatment = fields[5];
            participants.push_back(person);
        } else {
            cout << "Warning: skipped a damaged line in " << PARTICIPANT_FILE << ": " << line << "\n";
        }
    }
    return participants;
}

// Finds the next unused id. It is based on the ids already in the file, so
// numbering continues correctly each time the program is restarted.
int getNextId(const vector<Participant>& participants) {
    int highestId = 0;
    for (size_t i = 0; i < participants.size(); i++) {
        if (participants[i].id > highestId) {
            highestId = participants[i].id;
        }
    }
    return highestId + 1;
}

// Returns today's date as YYYY-MM-DD.
string getTodaysDate() {
    time_t now = time(NULL);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", localtime(&now));
    return buffer;
}

// Adds one line to the end of a data file. ios::app keeps the existing contents
// and creates the file if it does not exist yet. Returns false if the write failed.
bool appendLine(const string& fileName, const string& line) {
    ofstream file(fileName.c_str(), ios::app);
    if (!file) {
        return false;
    }
    file << line << "\n";
    file.close();
    return !file.fail();
}

// Prints the participants in a table, without any survey answers.
void printParticipants(const vector<Participant>& participants) {
    cout << "\n" << left
         << setw(6) << "ID"
         << setw(16) << "First Name"
         << setw(16) << "Last Name"
         << setw(6) << "Age"
         << setw(14) << "Gender"
         << "Treatment\n"
         << string(68, '-') << "\n";

    for (size_t i = 0; i < participants.size(); i++) {
        const Participant& person = participants[i];
        cout << setw(6) << person.id
             << setw(16) << person.firstName
             << setw(16) << person.lastName
             << setw(6) << person.age
             << setw(14) << person.gender
             << person.treatment << "\n";
    }
}

// Menu option 1: collects a new participant's details and saves them.
void addParticipant() {
    Participant person;

    cout << "\n--- Add New Participant ---\n";
    if (!promptText("First name: ", false, person.firstName) ||
        !promptText("Last name: ", false, person.lastName) ||
        !promptInt("Age: ", MIN_AGE, MAX_AGE, person.age) ||
        !promptText("Gender: ", false, person.gender)) {
        return;  // Input ended early, so nothing is saved.
    }

    // The id decides the treatment: even ids get StudyMor, odd ids get the placebo.
    person.id = getNextId(loadParticipants());
    person.treatment = (person.id % 2 == 0) ? "StudyMor" : "Placebo";

    ostringstream record;
    record << person.id << DELIMITER << person.firstName << DELIMITER << person.lastName << DELIMITER
           << person.age << DELIMITER << person.gender << DELIMITER << person.treatment;

    if (appendLine(PARTICIPANT_FILE, record.str())) {
        cout << "Added participant " << person.id << " (" << person.treatment << ").\n";
    } else {
        cout << "Error: could not write to " << PARTICIPANT_FILE << ". The participant was not saved.\n";
    }
}

// Menu option 2: lets the user pick a participant, then records their survey.
void collectSurvey() {
    vector<Participant> participants = loadParticipants();
    if (participants.empty()) {
        cout << "\nThere are no participants yet. Please add one first.\n";
        return;
    }

    cout << "\n--- Collect Survey ---";
    printParticipants(participants);
    cout << "\n";

    // Keep asking until the id matches a participant in the list.
    int participantId;
    bool found = false;
    while (!found) {
        string answer;
        if (!promptText("Enter the participant's ID: ", false, answer)) {
            return;
        }
        if (parseInt(answer, participantId)) {
            for (size_t i = 0; i < participants.size(); i++) {
                if (participants[i].id == participantId) {
                    found = true;
                }
            }
        }
        if (!found) {
            cout << "That ID is not in the list.\n";
        }
    }

    string headaches, constipation, difficultySleeping, otherEffects, studyMore;
    if (!promptYesNo("1. Did you have any headaches using StudyMor?", headaches) ||
        !promptYesNo("2. Did you have any constipation using StudyMor?", constipation) ||
        !promptYesNo("3. Did you experience any difficulty sleeping while using StudyMor?", difficultySleeping) ||
        !promptText("4. List any other potential side effects you experienced using StudyMor (Enter for none): ",
                    true, otherEffects) ||
        !promptYesNo("5. Did you feel like you could study more using StudyMor?", studyMore)) {
        return;  // Input ended early, so no partial survey is saved.
    }

    if (otherEffects.empty()) {
        otherEffects = "None";
    }

    // The date and participant id come first so each survey can be matched to its participant.
    ostringstream record;
    record << getTodaysDate() << DELIMITER << participantId << DELIMITER << headaches << DELIMITER
           << constipation << DELIMITER << difficultySleeping << DELIMITER << otherEffects << DELIMITER
           << studyMore;

    if (appendLine(SURVEY_FILE, record.str())) {
        cout << "Survey saved for participant " << participantId << ".\n";
    } else {
        cout << "Error: could not write to " << SURVEY_FILE << ". The survey was not saved.\n";
    }
}

// Menu option 3: lists all of the participants.
void displayParticipants() {
    vector<Participant> participants = loadParticipants();
    if (participants.empty()) {
        cout << "\nThere are no participants yet.\n";
    } else {
        printParticipants(participants);
    }
}

int main() {
    while (true) {
        cout << "\n===== StudyMor Study =====\n"
             << "1. Add new participant\n"
             << "2. Collect survey for participant\n"
             << "3. Display participants\n"
             << "4. Quit\n";

        int choice;
        if (!promptInt("Enter your choice: ", 1, 4, choice) || choice == 4) {
            break;
        }

        if (choice == 1) {
            addParticipant();
        } else if (choice == 2) {
            collectSurvey();
        } else {
            displayParticipants();
        }
    }

    cout << "Goodbye.\n";
    return 0;
}
