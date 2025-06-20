#include <iostream>
#include <iomanip>
#include <string>
#include <mysql.h>
#include "DatabaseConnection.h"
#include <set>
#include <vector>
#include <algorithm>
#include <cstdlib> 
#include <fstream>
#include <sstream>

//for detecting keyboard inputs
#ifdef _WIN32
#include <conio.h> 
#else
#include <termios.h>
#include <unistd.h>

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

#define ARROW_UP 72
#define ARROW_DOWN 80
#define ENTER_KEY 13

bool login(const std::string& table, const std::string& idColumn, const std::string& nameColumn, const std::string& id, const std::string& password) {
    if (!conn) {
        std::cerr << "Database connection is not initialized!" << std::endl;
        return false;
    }

    std::string query = "SELECT " + nameColumn + " FROM " + table + " WHERE " + idColumn + " = '" + id + "' AND Password = '" + password + "'";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query Execution Failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::cout << "Login Successful! Welcome, " << row[0] << "." << std::endl;
        mysql_free_result(res);
        return true;
    }

    std::cerr << "Invalid ID or Password!" << std::endl;
    mysql_free_result(res);
    return false;
}

void printHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "     Student Record Management System           " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

std::string menuSelection() {
    const std::string options[3] = { "Student", "Lecturer", "Admin" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H";  // Clear screen
        printHeader();  // Print the header with a border

        for (int i = 0; i < 3; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <" << std::endl;  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << std::endl;  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 3) % 3;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 3;
        else if (key == ENTER_KEY) {
            return options[selectedIndex];  // Return the selected option
        }
    }
}


// Structure to hold student performance data
struct StudentPerformance {
    std::string studentID;
    std::string studentName;
    int semester;
    float semesterGPA;
    float CGPA;
};

// Function to fetch student performance data from the database
std::vector<StudentPerformance> getAllStudentPerformance(MYSQL* conn) {
    std::vector<StudentPerformance> students;

    const std::string query = R"(
        SELECT s.StudentID, s.StudentName, sg.Semester, sg.GPA, op.CGPA
        FROM student s
        INNER JOIN semestergpa sg ON s.StudentID = sg.StudentID
        INNER JOIN overallperformance op ON s.StudentID = op.StudID
        ORDER BY s.StudentID, sg.Semester
    )";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return students;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn) << std::endl;
        return students;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        StudentPerformance student;
        student.studentID = row[0] ? row[0] : "";
        student.studentName = row[1] ? row[1] : "";
        student.semester = row[2] ? std::stoi(row[2]) : 0;
        student.semesterGPA = row[3] ? std::stof(row[3]) : 0.0f;
        student.CGPA = row[4] ? std::stof(row[4]) : 0.0f;

        students.push_back(student);
    }

    mysql_free_result(result);  // Clean up the result set

    return students;
}


// Function to retrieve and display all student performance, sorted by StudentID
void viewAllStudentPerformance(MYSQL* conn) {
    std::vector<StudentPerformance> performances;

    // SQL query to fetch data from the database
    const char* query = R"(
        SELECT s.StudentID, s.StudentName, sp.Semester, sp.GPA AS SemesterGPA, op.CGPA
        FROM student s
        JOIN semestergpa sp ON s.StudentID = sp.StudentID
        JOIN overallperformance op ON s.StudentID = op.StudID
        ORDER BY s.StudentID, sp.Semester;
    )";

    // Execute the query
    if (mysql_query(conn, query)) {
        std::cerr << "Error executing query: " << mysql_error(conn) << std::endl;
        return;
    }

    // Retrieve the result set
    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        std::cerr << "Error storing result: " << mysql_error(conn) << std::endl;
        return;
    }

    // Fetch each row from the result set
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        StudentPerformance performance;
        performance.studentID = row[0];  // StudentID
        performance.studentName = row[1];  // StudentName
        performance.semester = std::stoi(row[2]);  // Semester
        performance.semesterGPA = std::stof(row[3]);  // SemesterGPA
        performance.CGPA = std::stof(row[4]);  // CGPA

        // Add to the vector of performances
        performances.push_back(performance);
    }

    // Sort the performances by StudentID
    std::sort(performances.begin(), performances.end(), [](const StudentPerformance& a, const StudentPerformance& b) {
        return a.studentID < b.studentID;
        });

    // Display the sorted data
    std::cout << std::setw(10) << "StudentID"
        << std::setw(15) << "StudentName"
        << std::setw(10) << "Semester"
        << std::setw(12) << "SemesterGPA"
        << std::setw(10) << "CGPA" << std::endl;

    std::cout << "------------------------------------------------------------" << std::endl;

    // Print each student's performance
    for (const auto& performance : performances) {
        std::cout << std::setw(10) << performance.studentID
            << std::setw(15) << performance.studentName
            << std::setw(10) << performance.semester
            << std::setw(12) << std::fixed << std::setprecision(2) << performance.semesterGPA
            << std::setw(10) << performance.CGPA << std::endl;
    }

    // Clean up the result set
    mysql_free_result(res);
}

void saveReportToFile(const std::string& studentID, const std::string& report) {
    // Define the path to save the file in Downloads folder
    std::ofstream outFile("C:\\Users\\danis\\Downloads\\" + studentID + "_performance_report.txt");
    if (outFile.is_open()) {
        outFile << report;
        outFile.close();
        std::cout << "\nReport saved successfully!\n";
    }
    else {
        std::cerr << "\nError saving report to file.\n";
    }
}

void displayStudentPerformance(const std::string& studentID) {
    std::cout << "\nDisplaying Performance for Student ID: " << studentID << "\n";

    // Query to get Student Name 
    std::string query = "SELECT StudentName FROM student WHERE StudentID = '" + studentID + "';";
    MYSQL_RES* res;
    MYSQL_ROW row;

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }

    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    std::string studentName = row[0];
    std::cout << "\nStudent ID: " << studentID << "\n";
    std::cout << "Name: " << studentName << "\n\n";

    // Query to get all semesters for the student
    query = "SELECT DISTINCT SemesterID FROM performancedetail WHERE StudentID = '" + studentID + "' ORDER BY SemesterID;";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }
    res = mysql_store_result(conn);

    float totalCGPA = 0.0;
    int totalSemesters = 0;

    // Loop through each semester
    while ((row = mysql_fetch_row(res))) {
        int semesterID = atoi(row[0]);
        std::cout << "\n---------------------------------\n";
        std::cout << "Semester " << semesterID << "\n";
        std::cout << "---------------------------------\n";

        // Query to get performance details for this specific semester
        query = "SELECT pd.SubjectID, s.SubjectName, pd.Quiz1, pd.Quiz2, pd.Quiz3, pd.Midterm, pd.LabTest1, pd.LabTest2, pd.FinalExam, pd.FinalEvaluation, pd.Grade, pd.SubjectGPA "
            "FROM performancedetail pd "
            "JOIN subject s ON pd.SubjectID = s.SubjectID "
            "WHERE pd.StudentID = '" + studentID + "' AND pd.SemesterID = " + std::to_string(semesterID) + ";";

        if (mysql_query(conn, query.c_str())) {
            std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
            return;
        }
        MYSQL_RES* semesterRes = mysql_store_result(conn);

        // Display the performance data for the current semester
        float semesterGPA = 0.0;
        int subjectCount = 0;

        // Print header for columns
        std::cout << std::setw(12) << std::left << "Subject ID"
            << std::setw(30) << std::left << "Subject Name"
            << std::setw(7) << std::left << "Quiz1"
            << std::setw(7) << std::left << "Quiz2"
            << std::setw(7) << std::left << "Quiz3"
            << std::setw(10) << std::left << "Midterm"
            << std::setw(22) << std::left << "LabTest1/Assignment1"
            << std::setw(22) << std::left << "LabTest2/Assignment2"
            << std::setw(10) << std::left << "FinalExam"
            << std::setw(15) << std::left << "FinalEvaluation"
            << std::setw(10) << std::left << "Grade"
            << std::setw(10) << std::left << "SubjectGPA"
            << "\n";
        std::cout << "----------------------------------------------------------------------------------------\n";

        // Loop through subjects in the semester and display their data
        while ((row = mysql_fetch_row(semesterRes))) {
            subjectCount++;

            // Format the subject data
            std::stringstream rowStream;
            rowStream << std::setw(12) << std::left << row[0]       // SubjectID
                << std::setw(30) << std::left << row[1]       // SubjectName
                << std::setw(7) << std::left << (row[2] ? row[2] : "-")   // Quiz1
                << std::setw(7) << std::left << (row[3] ? row[3] : "-")   // Quiz2
                << std::setw(7) << std::left << (row[4] ? row[4] : "-")   // Quiz3
                << std::setw(10) << std::left << (row[5] ? row[5] : "-")   // Midterm
                << std::setw(22) << std::left << (row[6] ? row[6] : "-")   // LabTest1/Assignment1
                << std::setw(22) << std::left << (row[7] ? row[7] : "-")   // LabTest2/Assignment2
                << std::setw(10) << std::left << (row[8] ? row[8] : "-")   // FinalExam
                << std::setw(15) << std::left << (row[9] ? row[9] : "-")   // FinalEvaluation
                << std::setw(10) << std::left << (row[10] ? row[10] : "-") // Grade
                << std::setw(10) << std::left << (row[11] ? row[11] : "-"); // SubjectGPA

            // Output the formatted row to the screen
            std::cout << rowStream.str() << "\n";

            // Accumulate GPA for the semester
            semesterGPA += atof(row[11]);  // Add subject GPA to semester GPA
        }

        // If there were subjects, calculate Semester GPA
        if (subjectCount > 0) {
            semesterGPA /= subjectCount;  // Average the SubjectGPA for the semester
        }

        std::cout << "\nSemester GPA: " << std::fixed << std::setprecision(2) << semesterGPA << "\n";
        std::cout << "---------------------------------\n";

        // Add to CGPA calculation
        totalCGPA += semesterGPA;
        totalSemesters++;

        // Free the result for this semester
        mysql_free_result(semesterRes);
    }

    // Calculate and display the CGPA
    if (totalSemesters > 0) {
        totalCGPA /= totalSemesters;  // Average CGPA across all semesters
    }

    std::cout << "\nCGPA: " << std::fixed << std::setprecision(2) << totalCGPA << "\n";

    // Pause to let the user see the report before prompting
    std::cout << "\nPress any key to continue to save prompt...";
    std::cin.ignore();  // Ignore any leftover input (like from previous queries)
    std::cin.get();     // Wait for the user to press a key

    // Ask the user if they want to save the report
    const std::string saveOptions[2] = { "Yes", "No" };
    int selectedIndex = 0;

    while (true) {
        // Clear the screen and show the menu options
        std::cout << "\033[2J\033[H"; // Clear the screen
        std::cout << "Would you like to save this report?\n";
        for (int i = 0; i < 2; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << saveOptions[i] << " <\n"; // Highlight selected option
            }
            else {
                std::cout << "  " << saveOptions[i] << "\n"; // Regular options
            }
        }

        int key;

#ifdef _WIN32
        key = _getch();
        if (key == 224) key = _getch();  // For handling special keys like arrows
#else
        key = getch(); 
#endif

        // Handle arrow key navigation
        if (key == ARROW_UP) {
            selectedIndex = (selectedIndex - 1 + 2) % 2;  // Wrap around if going past the first option
        }
        else if (key == ARROW_DOWN) {
            selectedIndex = (selectedIndex + 1) % 2;  // Wrap around if going past the last option
        }
        else if (key == ENTER_KEY) {
            if (saveOptions[selectedIndex] == "Yes") {
                // Open the file in the Downloads folder to save the report
                std::ofstream outFile("C:\\Users\\danis\\Downloads\\" + studentID + "_performance_report.txt");

                // Write Student information
                outFile << "Student ID: " << studentID << "\n";
                outFile << "Name: " << studentName << "\n\n";

                // Query to get all semesters for the student
                std::string query = "SELECT DISTINCT SemesterID FROM performancedetail WHERE StudentID = '" + studentID + "' ORDER BY SemesterID;";
                if (mysql_query(conn, query.c_str())) {
                    std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
                    return;
                }
                MYSQL_RES* res = mysql_store_result(conn);
                MYSQL_ROW row;

                float totalCGPA = 0.0;
                int totalSemesters = 0;

                // Loop through each semester
                while ((row = mysql_fetch_row(res))) {
                    int semesterID = atoi(row[0]);
                    outFile << "\n---------------------------------\n";
                    outFile << "Semester " << semesterID << "\n";
                    outFile << "---------------------------------\n";

                    // Query to get performance details for this specific semester
                    query = "SELECT pd.SubjectID, s.SubjectName, pd.SubjectGPA "
                        "FROM performancedetail pd "
                        "JOIN subject s ON pd.SubjectID = s.SubjectID "
                        "WHERE pd.StudentID = '" + studentID + "' AND pd.SemesterID = " + std::to_string(semesterID) + ";";

                    if (mysql_query(conn, query.c_str())) {
                        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
                        return;
                    }
                    MYSQL_RES* semesterRes = mysql_store_result(conn);

                    // Print Subject details for the semester
                    outFile << std::setw(12) << std::left << "Subject ID"
                        << std::setw(30) << std::left << "Subject Name"
                        << std::setw(10) << std::left << "Subject GPA" << "\n";
                    outFile << "----------------------------------------------------\n";

                    float semesterGPA = 0.0;
                    int subjectCount = 0;

                    // Loop through subjects in the semester
                    while ((row = mysql_fetch_row(semesterRes))) {
                        subjectCount++;
                        outFile << std::setw(12) << std::left << row[0]       // SubjectID
                            << std::setw(30) << std::left << row[1]       // SubjectName
                            << std::setw(10) << std::left << (row[2] ? row[2] : "-")   // Subject GPA
                            << "\n";

                        // Accumulate Subject GPA for the semester
                        semesterGPA += atof(row[2]);
                    }

                    // If there were subjects, calculate Semester GPA
                    if (subjectCount > 0) {
                        semesterGPA /= subjectCount;  // Average the Subject GPA for the semester
                    }

                    // Write the Semester GPA to the file
                    outFile << "\nSemester GPA: " << std::fixed << std::setprecision(2) << semesterGPA << "\n";
                    outFile << "---------------------------------\n";

                    // Add to CGPA calculation
                    totalCGPA += semesterGPA;
                    totalSemesters++;

                    // Free the result for this semester
                    mysql_free_result(semesterRes);
                }

                // Calculate and display the CGPA
                if (totalSemesters > 0) {
                    totalCGPA /= totalSemesters;  // Average CGPA across all semesters
                }

                // Write the CGPA at the bottom of the report
                outFile << "\nOverall CGPA: " << std::fixed << std::setprecision(2) << totalCGPA << "\n";
                outFile.close();

                std::cout << "Report saved successfully!\n";
            }

            else {
                std::cout << "Report not saved.\n";
            }
            break; // Exit the loop
        }
    }
}






void viewSpecificStudentPerformance(MYSQL* conn) {
    // Prompt for StudentID
    std::string studentID;
    std::cout << "Enter StudentID: ";
    std::cin >> studentID;

    // Query to get Student Name and ID
    std::string query = "SELECT StudentID, StudentName FROM student WHERE StudentID = '" + studentID + "';";
    MYSQL_RES* res;
    MYSQL_ROW row;

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);

    if (!row) {
        std::cerr << "No student found with the provided StudentID." << std::endl;
        return;
    }

    std::string studentName = row[1];  // Get the StudentName
    std::cout << "\nStudent ID: " << row[0] << "\n";
    std::cout << "Name: " << studentName << "\n\n";

    // Query to get all semesters for the student
    query = "SELECT DISTINCT SemesterID FROM performancedetail WHERE StudentID = '" + studentID + "' ORDER BY SemesterID;";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }
    res = mysql_store_result(conn);

    float totalCGPA = 0.0;
    int totalSemesters = 0;

    // Loop through each semester
    while ((row = mysql_fetch_row(res))) {
        int semesterID = atoi(row[0]);
        std::cout << "\n---------------------------------\n";
        std::cout << "Semester " << semesterID << "\n";
        std::cout << "---------------------------------\n";

        // Query to get performance details for this specific semester
        query = "SELECT pd.SubjectID, s.SubjectName, pd.Quiz1, pd.Quiz2, pd.Quiz3, pd.Midterm, pd.LabTest1, pd.LabTest2, pd.FinalExam, pd.FinalEvaluation, pd.Grade, pd.SubjectGPA "
            "FROM performancedetail pd "
            "JOIN subject s ON pd.SubjectID = s.SubjectID "
            "WHERE pd.StudentID = '" + studentID + "' AND pd.SemesterID = " + std::to_string(semesterID) + ";";

        if (mysql_query(conn, query.c_str())) {
            std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
            return;
        }
        MYSQL_RES* semesterRes = mysql_store_result(conn);

        // Display the performance data for the current semester
        float semesterGPA = 0.0;
        int subjectCount = 0;
        while ((row = mysql_fetch_row(semesterRes))) {
            subjectCount++;
            std::cout << "\nSubject ID: " << row[0] << "\n";
            std::cout << "Subject Name: " << row[1] << "\n";

            // Display quizzes, midterm, lab tests, and final exam if they exist
            if (row[2] != nullptr && row[3] != nullptr && row[4] != nullptr && row[5] != nullptr &&
                row[6] != nullptr && row[7] != nullptr && row[8] != nullptr) {
                // Print headers with proper alignment
                std::cout << std::setw(10) << "Quiz1"
                    << std::setw(10) << "Quiz2"
                    << std::setw(10) << "Quiz3"
                    << std::setw(10) << "Midterm"
                    << std::setw(20) << "LabTest1/Assignment1 "
                    << std::setw(20) << "LabTest2/Assignment2"
                    << std::setw(10) << "FinalExam"
                    << std::setw(15) << " FinalEvaluation" << std::endl;

                // Print data for each subject with proper alignment
                std::cout << std::setw(10) << row[2]
                    << std::setw(10) << row[3]
                    << std::setw(10) << row[4]
                    << std::setw(10) << row[5]
                    << std::setw(20) << row[6]
                    << std::setw(20) << row[7]
                    << std::setw(10) << row[8]
                    << std::setw(15) << row[9] << std::endl;
            }
            else {
                // Print headers for subjects with no quizzes, etc., but include SubjectGPA
                std::cout << std::setw(15) << "FinalEvaluation"
                    << std::setw(10) << " SubjectGPA" << std::endl;
                std::cout << std::setw(15) << row[9]
                    << std::setw(10) << row[11] << std::endl;
            }

            std::cout << "Grade: " << row[10] << "\n";
            std::cout << "---------------------------------\n";

            // Accumulate GPA for the semester
            semesterGPA += atof(row[11]);  // Add subject GPA to semester GPA
        }

        // If there were subjects, calculate Semester GPA
        if (subjectCount > 0) {
            semesterGPA /= subjectCount;  // Average the SubjectGPA for the semester
        }

        std::cout << "\nSemester GPA: " << semesterGPA << "\n";
        std::cout << "---------------------------------\n";

        // Add to CGPA calculation
        totalCGPA += semesterGPA;
        totalSemesters++;

        // Free the result for this semester
        mysql_free_result(semesterRes);
    }

    // Calculate and display the CGPA
    if (totalSemesters > 0) {
        totalCGPA /= totalSemesters;  // Average CGPA across all semesters
    }

    std::cout << "\nCGPA: " << totalCGPA << "\n";
}






void updateCGPA(const std::string& studentID) {
    // Calculate the total GPA of all semesters for the student
    std::string query = "SELECT SUM(GPA) AS TotalGPA, COUNT(*) AS TotalSemesters "
        "FROM semestergpa "
        "WHERE StudentID = '" + studentID + "'";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Error calculating CGPA: " << mysql_error(conn) << "\n";
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        double totalGPA = std::stod(row[0] ? row[0] : "0");
        int totalSemesters = std::stoi(row[1] ? row[1] : "0");

        double cgpa = (totalSemesters > 0) ? totalGPA / totalSemesters : 0.0;

        // Insert or update the CGPA in the overallperformance table
        std::string updateCGPAQuery = "INSERT INTO overallperformance (StudID, CGPA) "
            "VALUES ('" + studentID + "', '" + std::to_string(cgpa) + "') "
            "ON DUPLICATE KEY UPDATE CGPA = '" + std::to_string(cgpa) + "'";

        if (mysql_query(conn, updateCGPAQuery.c_str())) {
            std::cerr << "Error updating CGPA: " << mysql_error(conn) << "\n";
        }
        else {
            std::cout << "CGPA updated successfully!\n";
        }
    }
    else {
        std::cerr << "No semester records found for the given student!\n";
    }

    mysql_free_result(res);
}

void updateSemesterGPA(const std::string& studentID, const std::string& semesterID) {
    // Calculate the total grade weight and total credits for the student in the given semester
    std::string query = "SELECT SUM(pd.SubjectGPA * s.Credit) AS TotalGradeWeight, SUM(s.Credit) AS TotalCredits "
        "FROM performancedetail pd "
        "JOIN subject s ON pd.SubjectID = s.SubjectID "
        "WHERE pd.StudentID = '" + studentID + "' AND pd.SemesterID = '" + semesterID + "'";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Error calculating GPA: " << mysql_error(conn) << "\n";
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        double totalGradeWeight = std::stod(row[0] ? row[0] : "0");
        double totalCredits = std::stod(row[1] ? row[1] : "0");

        double semesterGPA = (totalCredits > 0) ? totalGradeWeight / totalCredits : 0.0;

        // Update the SemesterGPA table
        std::string updateGPAQuery = "INSERT INTO semestergpa (StudentID, Semester, GPA) "
            "VALUES ('" + studentID + "', '" + semesterID + "', '" + std::to_string(semesterGPA) + "') "
            "ON DUPLICATE KEY UPDATE GPA = '" + std::to_string(semesterGPA) + "'";

        if (mysql_query(conn, updateGPAQuery.c_str())) {
            std::cerr << "Error updating SemesterGPA: " << mysql_error(conn) << "\n";
        }
        else {
            std::cout << "Semester GPA updated successfully!\n";
        }

        // Update CGPA in the overallperformance table after updating the semester GPA
        updateCGPA(studentID);
    }
    else {
        std::cerr << "No performance records found for the given student and semester!\n";
    }

    mysql_free_result(res);
}



void SubjectAnalysis(MYSQL* conn) {
    std::string subjectID, grade;

    std::cout << "Enter Subject ID: ";
    std::cin >> subjectID;
    std::cout << "Enter Grade (A, A-, B, B-, C, C-, D, F): ";
    std::cin >> grade;

    // Validate SubjectID exists
    std::string validateSubject = "SELECT SubjectID FROM subject WHERE SubjectID = '" + subjectID + "';";
    if (mysql_query(conn, validateSubject.c_str())) {
        std::cerr << "Subject validation failed: " << mysql_error(conn) << std::endl;
        return;
    }
    MYSQL_RES* validateResult = mysql_store_result(conn);
    if (mysql_num_rows(validateResult) == 0) {
        std::cout << "Invalid SubjectID. Please try again.\n";
        mysql_free_result(validateResult);
        return;
    }
    mysql_free_result(validateResult);

    // Query to count total students in the subject
    std::string totalStudentsQuery = "SELECT COUNT(DISTINCT StudentID) FROM performancedetail WHERE SubjectID = '" + subjectID + "';";
    if (mysql_query(conn, totalStudentsQuery.c_str())) {
        std::cerr << "Query to count total students failed: " << mysql_error(conn) << std::endl;
        return;
    }
    MYSQL_RES* totalStudentsResult = mysql_store_result(conn);
    if (!totalStudentsResult) {
        std::cerr << "Error storing total students result: " << mysql_error(conn) << std::endl;
        return;
    }
    MYSQL_ROW totalRow = mysql_fetch_row(totalStudentsResult);
    int totalStudentsInSubject = totalRow ? std::stoi(totalRow[0]) : 0;
    mysql_free_result(totalStudentsResult);

    // Main query to fetch students with specific grade
    std::string query = "SELECT s.StudentName FROM student s "
        "INNER JOIN performancedetail p ON s.StudentID = p.StudentID "
        "WHERE p.SubjectID = '" + subjectID + "' AND p.Grade = '" + grade + "';";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "Error storing result: " << mysql_error(conn) << std::endl;
        return;
    }

    if (mysql_num_rows(result) == 0) {
        std::cout << "No students found for SubjectID: " << subjectID << " with Grade: " << grade << ".\n";
        mysql_free_result(result);
        std::cout << "\nPress Enter to return to the menu...";
        std::cin.ignore();
        std::cin.get();
        return;
    }

    // Display results
    MYSQL_ROW row;
    int gradeStudentsCount = 0;
    std::cout << "\nSubject: " << subjectID << "\nGrade: " << grade << "\n\n";

    while ((row = mysql_fetch_row(result))) {
        std::cout << row[0] << "\n-----------------------------------------\n";
        ++gradeStudentsCount;
    }

    std::cout << "\nTotal: " << gradeStudentsCount << " Students out of " << totalStudentsInSubject << "\n";

    mysql_free_result(result);

    // Pause for the user to see the output
    std::cout << "\nPress Enter to return to the menu...";
    std::cin.ignore();
    std::cin.get();
}






void editStudentPerformanceMenu() {
    std::string studentID, semesterID;

    // Prompt lecturer to enter the StudentID
    std::cout << "\033[2J\033[H"; // Clear the screen
    std::cout << "-----------------------------------------------\n";
    std::cout << "             Edit Student Performance         \n";
    std::cout << "-----------------------------------------------\n";
    std::cout << "Enter StudentID: ";
    std::cin >> studentID;

    // Prompt lecturer to enter the SemesterID
    std::cout << "Enter SemesterID: ";
    std::cin >> semesterID;

    const std::string options[4] = { "Insert Performance Record", "Update Performance Record", "Delete Performance Record", "Back to Previous Page" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        std::cout << "-----------------------------------------------\n";
        std::cout << "             Edit Student Performance         \n";
        std::cout << "-----------------------------------------------\n";
        std::cout << "Selected StudentID: " << studentID << "\n";
        std::cout << "Selected SemesterID: " << semesterID << "\n\n";

        for (int i = 0; i < 4; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";
            }
            else {
                std::cout << "  " << options[i] << "\n";
            }
        }

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) {
            selectedIndex = (selectedIndex - 1 + 4) % 4;
        }
        else if (key == ARROW_DOWN) {
            selectedIndex = (selectedIndex + 1) % 4;
        }
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Insert Performance Record") {
                // Insert Performance Record Functionality
                std::cout << "Insert Performance Record selected.\n";

                std::string subjectID;
                double quiz1 = 0, quiz2 = 0, quiz3 = 0, labTest1 = 0, labTest2 = 0, midterm = 0, finalExam = 0, finalEvaluation = 0;

                std::cout << "Enter SubjectID: ";
                std::cin >> subjectID;

                // Check if the record already exists for the subject regardless of semester
                std::string checkQuery = "SELECT * FROM performancedetail WHERE StudentID = '" + studentID + "' AND SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking existing record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) > 0) {
                    std::cerr << "Error: A record for this subject already exists for this student!\n";
                    std::cout << "Press any key to return to the menu and try again...\n";
#ifdef _WIN32
                    _getch();
#else
                    getchar();
#endif
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // Check if the subject is a special subject
                const std::set<std::string> specialSubjects = { "BITU2913", "BITU3923", "BITU3926", "BITU3946", "BITU3973", "BITU3983", "BKK1", "BKK2" };
                if (specialSubjects.count(subjectID)) {
                    std::cout << "Enter Final Evaluation Marks (out of 1000): ";
                    std::cin >> finalEvaluation;

                    // Insert into performancedetail table for special subject
                    std::string query = "INSERT INTO performancedetail (StudentID, SemesterID, SubjectID, FinalEvaluation) "
                        "VALUES ('" + studentID + "', '" + semesterID + "', '" + subjectID + "', '" + std::to_string(finalEvaluation) + "')";

                    if (mysql_query(conn, query.c_str())) {
                        std::cerr << "Error inserting record into performancedetail: " << mysql_error(conn) << "\n";
                    }
                    else {
                        std::cout << "Special subject record inserted into performancedetail successfully!\n";
                    }
                }
                else {
                    std::cout << "Enter Quiz 1 Marks (out of 20): ";
                    std::cin >> quiz1;

                    std::cout << "Enter Quiz 2 Marks (out of 20): ";
                    std::cin >> quiz2;

                    std::cout << "Enter Quiz 3 Marks (out of 20): ";
                    std::cin >> quiz3;

                    std::cout << "Enter Lab Test 1 OR Assignment 1 Marks (out of 30): ";
                    std::cin >> labTest1;

                    std::cout << "Enter Lab Test 2 OR Assignment 2 Marks (out of 30): ";
                    std::cin >> labTest2;

                    std::cout << "Enter Midterm Marks (out of 100): ";
                    std::cin >> midterm;

                    std::cout << "Enter Final Exam Marks (out of 100): ";
                    std::cin >> finalExam;

                    // Calculate Final Evaluation
                    finalEvaluation = (quiz1 / 20 * 5) + (quiz2 / 20 * 5) + (quiz3 / 20 * 5) +
                        (labTest1 / 30 * 15) + (labTest2 / 30 * 20) +
                        (midterm / 100 * 20) + (finalExam / 100 * 30);

                    // Insert into performancedetail table for regular subject
                    std::string query = "INSERT INTO performancedetail (StudentID, SemesterID, SubjectID, Quiz1, Quiz2, Quiz3, Midterm, LabTest1, LabTest2, FinalExam, FinalEvaluation) "
                        "VALUES ('" + studentID + "', '" + semesterID + "', '" + subjectID + "', '" + std::to_string(quiz1) + "', '" + std::to_string(quiz2) + "', '" + std::to_string(quiz3) + "', '" + std::to_string(midterm) + "', '" + std::to_string(labTest1) + "', '" + std::to_string(labTest2) + "', '" + std::to_string(finalExam) + "', '" + std::to_string(finalEvaluation) + "')";

                    if (mysql_query(conn, query.c_str())) {
                        std::cerr << "Error inserting record into performancedetail: " << mysql_error(conn) << "\n";
                    }
                    else {
                        std::cout << "Regular subject record inserted into performancedetail successfully!\n";
                    }
                }

                // Now calculate grade and GPA for insertion into performancedetail
                std::string grade;
                double subjectGPA;

                if (finalEvaluation >= 80) {
                    grade = "A";
                    subjectGPA = 4.00;
                }
                else if (finalEvaluation >= 70) {
                    grade = "A-";
                    subjectGPA = 3.67;
                }
                else if (finalEvaluation >= 60) {
                    grade = "B+";
                    subjectGPA = 3.33;
                }
                else if (finalEvaluation >= 55) {
                    grade = "B";
                    subjectGPA = 3.00;
                }
                else if (finalEvaluation >= 50) {
                    grade = "B-";
                    subjectGPA = 2.67;
                }
                else if (finalEvaluation >= 45) {
                    grade = "C";
                    subjectGPA = 2.00;
                }
                else if (finalEvaluation >= 35) {
                    grade = "C-";
                    subjectGPA = 1.67;
                }
                else {
                    grade = "F";
                    subjectGPA = 0.00;
                }

                // Update the performancedetail table with Grade and SubjectGPA
                std::string updateQuery = "UPDATE performancedetail SET Grade = '" + grade + "', SubjectGPA = '" + std::to_string(subjectGPA) + "' "
                    "WHERE StudentID = '" + studentID + "' AND SemesterID = '" + semesterID + "' AND SubjectID = '" + subjectID + "'";

                if (mysql_query(conn, updateQuery.c_str())) {
                    std::cerr << "Error updating record in performancedetail: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Grade and Subject GPA updated in performancedetail successfully!\n";
                }
                updateSemesterGPA(studentID, semesterID);
            }



            else if (options[selectedIndex] == "Update Performance Record") {
                // Update Performance Record Functionality
                std::cout << "Update Performance Record selected.\n";

                std::string subjectID;
                std::cout << "Enter SubjectID to update: ";
                std::cin >> subjectID;

                // Check if the record exists
                std::string checkQuery = "SELECT * FROM performancedetail WHERE StudentID = '" + studentID + "' AND SemesterID = '" + semesterID + "' AND SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this subject!\n";
                    std::cout << "Press any key to return to the menu and try again...\n";
#ifdef _WIN32
                    _getch();
#else
                    getchar();
#endif
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // Allow lecturer to update the fields
                double quiz1, quiz2, quiz3, labTest1, labTest2, midterm, finalExam;
                std::cout << "Enter Quiz 1 Marks (out of 20): ";
                std::cin >> quiz1;
                std::cout << "Enter Quiz 2 Marks (out of 20): ";
                std::cin >> quiz2;
                std::cout << "Enter Quiz 3 Marks (out of 20): ";
                std::cin >> quiz3;
                std::cout << "Enter Lab Test 1 OR Assignment 1 Marks (out of 30): ";
                std::cin >> labTest1;
                std::cout << "Enter Lab Test 2 OR Assignment 2 Marks (out of 30): ";
                std::cin >> labTest2;
                std::cout << "Enter Midterm Marks (out of 100): ";
                std::cin >> midterm;
                std::cout << "Enter Final Exam Marks (out of 100): ";
                std::cin >> finalExam;

                // Calculate Final Evaluation
                double finalEvaluation = (quiz1 / 20 * 5) + (quiz2 / 20 * 5) + (quiz3 / 20 * 5) +
                    (labTest1 / 30 * 15) + (labTest2 / 30 * 20) +
                    (midterm / 100 * 20) + (finalExam / 100 * 30);

                // Update the record in performancedetail
                std::string updateQuery = "UPDATE performancedetail SET Quiz1 = '" + std::to_string(quiz1) + "', Quiz2 = '" + std::to_string(quiz2) +
                    "', Quiz3 = '" + std::to_string(quiz3) + "', LabTest1 = '" + std::to_string(labTest1) + "', LabTest2 = '" + std::to_string(labTest2) +
                    "', Midterm = '" + std::to_string(midterm) + "', FinalExam = '" + std::to_string(finalExam) +
                    "', FinalEvaluation = '" + std::to_string(finalEvaluation) + "' "
                    "WHERE StudentID = '" + studentID + "' AND SemesterID = '" + semesterID + "' AND SubjectID = '" + subjectID + "'";

                if (mysql_query(conn, updateQuery.c_str())) {
                    std::cerr << "Error updating record in performancedetail: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Performance record updated successfully!\n";
                }

                // Recalculate the grade and GPA based on the updated marks
                std::string grade;
                double subjectGPA;

                if (finalEvaluation >= 80) {
                    grade = "A";
                    subjectGPA = 4.00;
                }
                else if (finalEvaluation >= 70) {
                    grade = "A-";
                    subjectGPA = 3.67;
                }
                else if (finalEvaluation >= 60) {
                    grade = "B+";
                    subjectGPA = 3.33;
                }
                else if (finalEvaluation >= 55) {
                    grade = "B";
                    subjectGPA = 3.00;
                }
                else if (finalEvaluation >= 50) {
                    grade = "B-";
                    subjectGPA = 2.67;
                }
                else if (finalEvaluation >= 45) {
                    grade = "C";
                    subjectGPA = 2.00;
                }
                else if (finalEvaluation >= 35) {
                    grade = "C-";
                    subjectGPA = 1.67;
                }
                else {
                    grade = "F";
                    subjectGPA = 0.00;
                }

                // Update the Grade and SubjectGPA in performancedetail
                std::string updateGradeGPAQuery = "UPDATE performancedetail SET Grade = '" + grade + "', SubjectGPA = '" + std::to_string(subjectGPA) + "' "
                    "WHERE StudentID = '" + studentID + "' AND SemesterID = '" + semesterID + "' AND SubjectID = '" + subjectID + "'";

                if (mysql_query(conn, updateGradeGPAQuery.c_str())) {
                    std::cerr << "Error updating Grade and SubjectGPA in performancedetail: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Grade and Subject GPA updated successfully!\n";
                }
                updateSemesterGPA(studentID, semesterID);
            }

            else if (options[selectedIndex] == "Delete Performance Record") {
                // Delete Performance Record Functionality
                std::cout << "Delete Performance Record selected.\n";

                std::string subjectID;
                std::cout << "Enter SubjectID to delete: ";
                std::cin >> subjectID;

                // Check if the record exists
                std::string checkQuery = "SELECT * FROM performancedetail WHERE StudentID = '" + studentID + "' AND SemesterID = '" + semesterID + "' AND SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this subject!\n";
                    std::cout << "Press any key to return to the menu and try again...\n";
#ifdef _WIN32
                    _getch();
#else
                    getchar();
#endif
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // Delete the record from performancedetail
                std::string deleteQuery = "DELETE FROM performancedetail WHERE StudentID = '" + studentID + "' AND SemesterID = '" + semesterID + "' AND SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, deleteQuery.c_str())) {
                    std::cerr << "Error deleting record from performancedetail: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Performance record deleted successfully!\n";
                }
                updateSemesterGPA(studentID, semesterID);
            }

            else if (options[selectedIndex] == "Back to Previous Page") {
                std::cout << "Returning to previous menu...\n";
                break; // Exit the loop to go back to lecturerMenu()
            }

            std::cout << "\nPress any key to continue...\n";
#ifdef _WIN32
            _getch();
#else
            getchar();
#endif
        }
    }
}

void printViewStudentMenuHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            View Student Menu                    " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void viewStudentMenu(MYSQL* conn) {
    const std::string options[3] = { "View All Student Performance", "View Specific Student Performance", "Back to Previous Page" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printViewStudentMenuHeader();  // Print the header with a border

        for (int i = 0; i < 3; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

        int key;

#ifdef _WIN32
        key = _getch();
        if (key == 224) key = _getch();  // For handling special keys like arrows
#else
        key = getch(); 
#endif

        // Handle arrow key and enter key inputs
        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 3) % 3;  // Wrap around if going past the first option
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 3;  // Wrap around if going past the last option
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "View All Student Performance") {
                // Call function to show all student performance data
                viewAllStudentPerformance(conn);  // Assuming this function is implemented
                std::cout << "\nPress any key to go back to the previous menu...";
                std::cin.ignore(); // Clear any residual input (like the newline from arrow key)
                std::cin.get(); // Wait for user to press a key to go back
                break; // Exit the loop and return to the previous menu
            }
            else if (options[selectedIndex] == "View Specific Student Performance") {
                // Call the function to view specific student performance
                viewSpecificStudentPerformance(conn);  // This will prompt for StudentID and display the data
                std::cout << "\nPress any key to go back to the previous menu...";
                std::cin.ignore(); // Clear any residual input
                std::cin.get(); // Wait for user to press a key to go back
                break; // Exit the loop and return to the previous menu
            }
            else if (options[selectedIndex] == "Back to Previous Page") {
                std::cout << "Returning to previous menu...\n";
                break; // Exit the loop and return to the previous menu
            }
        }
    }
}



void printLecturerMenuHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Lecturer Menu                        " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void lecturerMenu() {
    const std::string options[4] = { "View Student Performance", "Edit Student Performance", "Subject Analysis", "Logout" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printLecturerMenuHeader();  // Print the header with a border

        for (int i = 0; i < 4; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        // Handle arrow key and enter key inputs
        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 4) % 4;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 4;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "View Student Performance") {
                // Call the view student menu function
                viewStudentMenu(conn);  // Assuming viewStudentMenu is implemented and connected to `conn`
            }
            else if (options[selectedIndex] == "Edit Student Performance") {
                // Call the edit student performance menu function
                editStudentPerformanceMenu();  // Assuming editStudentPerformanceMenu is implemented
            }
            else if (options[selectedIndex] == "Subject Analysis") {
                // Call the Subject Analysis function
                std::cin.ignore();  // Clear the input buffer before the next input prompt
                SubjectAnalysis(conn);  // Assuming SubjectAnalysis is implemented and connected to `conn`
            }
            else if (options[selectedIndex] == "Logout") {
                std::cout << "Logging out...\n";
                break; // Exit the loop and log out
            }
        }
    }
}

void changePassword(const std::string& studentID) {
    std::string oldPassword, newPassword, confirmPassword;

    // Step 1: Prompt the user to enter their old password
    std::cout << "\033[2J\033[H"; // Clear the screen
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "               Change Password                    " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Enter your current password: ";

    // Hide password input 
#ifdef _WIN32
    oldPassword = "";
    char ch;
    while ((ch = _getch()) != '\r') { // Stop at ENTER
        if (ch == '\b' && !oldPassword.empty()) { // Handle backspace
            oldPassword.pop_back();
            std::cout << "\b \b";
        }
        else if (ch != '\b') {
            oldPassword += ch;
            std::cout << '*';
        }
    }
    std::cout << '\n';
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);  // Disable echo for password input
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::cin >> oldPassword;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    // Step 2: Verify the old password against the database
    std::string query = "SELECT Password FROM Student WHERE StudentID = '" + studentID + "'";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query Execution Failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::string storedPassword = row[0];  // Get the stored password

        if (oldPassword != storedPassword) {
            std::cerr << "Incorrect old password! Returning to menu...\n";
            mysql_free_result(res);
            return;  // Return to the StudentMenu if passwords don't match
        }

        mysql_free_result(res);

        // Step 3: Prompt for a new password
        std::cout << "Enter a new password: ";

#ifdef _WIN32
        newPassword = "";
        while ((ch = _getch()) != '\r') { // Stop at ENTER
            if (ch == '\b' && !newPassword.empty()) { // Handle backspace
                newPassword.pop_back();
                std::cout << "\b \b";
            }
            else if (ch != '\b') {
                newPassword += ch;
                std::cout << '*';
            }
        }
        std::cout << '\n';
#else
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ECHO);  // Disable echo for password input
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        std::cin >> newPassword;
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

        // Step 4: Confirm the new password
        std::cout << "Confirm new password: ";

#ifdef _WIN32
        confirmPassword = "";
        while ((ch = _getch()) != '\r') { // Stop at ENTER
            if (ch == '\b' && !confirmPassword.empty()) { // Handle backspace
                confirmPassword.pop_back();
                std::cout << "\b \b";
            }
            else if (ch != '\b') {
                confirmPassword += ch;
                std::cout << '*';
            }
        }
        std::cout << '\n';
#else
        tcgetattr(STDIN_FILENO, &oldt);
        confirmPassword = "";
        newt = oldt;
        newt.c_lflag &= ~(ECHO);  // Disable echo for password input
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        std::cin >> confirmPassword;
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

        // Step 5: Check if new passwords match
        if (newPassword != confirmPassword) {
            std::cerr << "Passwords do not match! Returning to menu...\n";
            return;  // Return to StudentMenu if passwords don't match
        }

        // Step 6: Update the password in the database
        std::string updateQuery = "UPDATE Student SET Password = '" + newPassword + "' WHERE StudentID = '" + studentID + "'";

        if (mysql_query(conn, updateQuery.c_str())) {
            std::cerr << "Failed to update password: " << mysql_error(conn) << std::endl;
        }
        else {
            std::cout << "Password successfully changed!\n";
        }
    }
    else {
        std::cerr << "Student not found! Returning to menu...\n";
    }
}


void StudentMenu(const std::string& studentID) {
    const std::string options[3] = { "Display Personal Performance", "Change Password", "Logout" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        std::cout << "-------------------------------------------------" << std::endl;
        std::cout << "            Student Menu                         " << std::endl;
        std::cout << "-------------------------------------------------" << std::endl;

        // Display options with the current selection highlighted
        for (int i = 0; i < 3; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight the selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;

#ifdef _WIN32
        int key = _getch();  // Read key press
        if (key == 224) key = _getch();  // Handle extended keys (like arrow keys)
#else
        int key = getch();  
#endif

        // Handle arrow key and enter key inputs
        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 3) % 3;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 3;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Display Personal Performance") {
                // Call the displayStudentPerformance function
                displayStudentPerformance(studentID);
            }
            else if (options[selectedIndex] == "Change Password") {
                // Call the changePassword function (you will implement this later)
                changePassword(studentID);
            }
            else if (options[selectedIndex] == "Logout") {
                std::cout << "Logging out...\n";
                break; // Exit the loop and log out
            }
        }
    }
}



void studentLogin() {
    std::string studentID, password;

    std::cout << "\033[2J\033[H"; // Clear the screen
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Student Login                        " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;

    // Prompt for StudentID and Password
    std::cout << "StudentID: ";
    std::cin >> studentID;

    std::cout << "Password: ";
#ifdef _WIN32
    password = ""; // Hide password input (Windows)
    char ch;
    while ((ch = _getch()) != '\r') { // Stop at ENTER
        if (ch == '\b' && !password.empty()) { // Handle backspace
            password.pop_back();
            std::cout << "\b \b";
        }
        else if (ch != '\b') {
            password += ch;
            std::cout << '*';
        }
    }
    std::cout << '\n';
#else
    termios oldt, newt; 
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::cin >> password;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    // Query to verify login
    std::string query = "SELECT StudentName FROM Student WHERE StudentID = '" + studentID + "' AND Password = '" + password + "'";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query Execution Failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::cout << "\nLogin Successful! Welcome, " << row[0] << ".\n";
        mysql_free_result(res);

        // Call the function to display the Student Menu
        StudentMenu(studentID);

        // Logout option
        std::cout << "\nPress any key to log out...\n";
#ifdef _WIN32
        _getch();
#else
        getchar();
#endif
    }
    else {
        std::cerr << "Invalid StudentID or Password!\n";
        mysql_free_result(res);
    }
}



void lecturerLogin() {
    std::string lecturerID, password;

    std::cout << "\033[2J\033[H"; // Clear the screen
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "              Lecturer Login                     " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    // Prompt for LecturerID and Password
    std::cout << "LecturerID: ";
    std::cin >> lecturerID;

    std::cout << "Password: ";
#ifdef _WIN32
    password = ""; // Hide password input (Windows)
    char ch;
    while ((ch = _getch()) != '\r') { // Stop at ENTER
        if (ch == '\b' && !password.empty()) { // Handle backspace
            password.pop_back();
            std::cout << "\b \b";
        }
        else if (ch != '\b') {
            password += ch;
            std::cout << '*';
        }
    }
    std::cout << '\n';
#else
    termios oldt, newt; // Hide password input (Unix-like systems), if not included, system wont run
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::cin >> password;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    // Query to verify login
    std::string query = "SELECT LecturerName FROM Lecturer WHERE LecturerID = '" + lecturerID + "' AND Password = '" + password + "'";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query Execution Failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::cout << "Login Successful! Welcome, " << row[0] << ".\n";
        mysql_free_result(res);

        // Call the lecturer menu after successful login
        lecturerMenu();
    }
    else {
        std::cerr << "Invalid LecturerID or Password!\n";
        mysql_free_result(res);
    }
}

void printAdminStudentHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Manage Students                      " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void AdminStudent() {
    const std::string options[4] = { "Insert New Student", "Update Existing Student", "Delete Existing Student", "Back to Previous Page" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printAdminStudentHeader();  // Print the header with a border

        for (int i = 0; i < 4; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 4) % 4;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 4;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Insert New Student") {
                // Insert New Student functionality
                std::cout << "Insert New Student selected.\n";

                std::string studentID, studentName, studentEmail, studentCourse, studentPassword;
                std::cout << "Enter StudentID: ";
                std::cin >> studentID;
                std::cout << "Enter Student Name: ";
                std::cin.ignore();  // Clear the buffer before using getline
                std::getline(std::cin, studentName);
                std::cout << "Enter Student Email: ";
                std::cin >> studentEmail;
                std::cout << "Enter Course: ";
                std::cin >> studentCourse;
                std::cout << "Enter Password: ";
                std::cin >> studentPassword;

                // Insert query for new student
                std::string insertQuery = "INSERT INTO student (StudentID, StudentName, StudentEmail, Course, Password) "
                    "VALUES ('" + studentID + "', '" + studentName + "', '" + studentEmail + "', '" + studentCourse + "', '" + studentPassword + "')";

                if (mysql_query(conn, insertQuery.c_str())) {
                    std::cerr << "Error inserting student: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "New student inserted successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Update Existing Student") {
                // Update Existing Student functionality
                std::cout << "Update Existing Student selected.\n";

                std::string studentID;
                std::cout << "Enter StudentID to update: ";
                std::cin >> studentID;

                // Check if the student exists
                std::string checkQuery = "SELECT * FROM student WHERE StudentID = '" + studentID + "'";

                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking student record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No student found with this StudentID!\n";
                    std::cout << "Press any key to return to the menu and try again...\n";
#ifdef _WIN32
                    _getch();
#else
                    getchar();
#endif
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // Allow admin to update student details
                std::string studentName, studentEmail, studentCourse, studentPassword;
                std::cout << "Enter new Student Name: ";
                std::cin.ignore();
                std::getline(std::cin, studentName);
                std::cout << "Enter new Student Email: ";
                std::cin >> studentEmail;
                std::cout << "Enter new Course: ";
                std::cin >> studentCourse;
                std::cout << "Enter new Password: ";
                std::cin >> studentPassword;

                // Update query for existing student
                std::string updateQuery = "UPDATE student SET StudentName = '" + studentName + "', StudentEmail = '" + studentEmail + "', "
                    "Course = '" + studentCourse + "', Password = '" + studentPassword + "' "
                    "WHERE StudentID = '" + studentID + "'";

                if (mysql_query(conn, updateQuery.c_str())) {
                    std::cerr << "Error updating student: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Student record updated successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Delete Existing Student") {
                std::cout << "Delete Existing Student selected.\n";

                std::string studentID;
                std::cout << "Enter StudentID to delete: ";
                std::cin >> studentID;

                // Check if the student exists
                std::string checkQuery = "SELECT * FROM student WHERE StudentID = '" + studentID + "'";

                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking student record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No student found with this StudentID!\n";
                    std::cout << "Press any key to return to the menu and try again...\n";
#ifdef _WIN32
                    _getch();
#else
                    getchar();
#endif
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // Archive relevant data from performancedetail
                std::string archiveQuery =
                    "INSERT INTO studentarchive (StudentName, SubjectName, SubjectGPA) "
                    "SELECT s.StudentName, sub.SubjectName, pd.SubjectGPA "
                    "FROM student s "
                    "JOIN performancedetail pd ON s.StudentID = pd.StudentID "
                    "JOIN subject sub ON pd.SubjectID = sub.SubjectID "
                    "WHERE s.StudentID = '" + studentID + "'";

                if (mysql_query(conn, archiveQuery.c_str())) {
                    std::cerr << "Error archiving student data: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Student data archived successfully!\n";
                }

                // Delete from performancedetail
                std::string deletePerformanceDetailQuery =
                    "DELETE FROM performancedetail WHERE StudentID = '" + studentID + "'";
                if (mysql_query(conn, deletePerformanceDetailQuery.c_str())) {
                    std::cerr << "Error deleting from performancedetail: " << mysql_error(conn) << "\n";
                }

                // Delete from overallperformance
                std::string deleteOverallPerformanceQuery =
                    "DELETE FROM overallperformance WHERE StudID = '" + studentID + "'";
                if (mysql_query(conn, deleteOverallPerformanceQuery.c_str())) {
                    std::cerr << "Error deleting from overallperformance: " << mysql_error(conn) << "\n";
                }

                // Delete from semestergpa
                std::string deleteSemesterGPAQuery =
                    "DELETE FROM semestergpa WHERE StudentID = '" + studentID + "'";
                if (mysql_query(conn, deleteSemesterGPAQuery.c_str())) {
                    std::cerr << "Error deleting from semestergpa: " << mysql_error(conn) << "\n";
                }

                // Finally, delete from student
                std::string deleteStudentQuery =
                    "DELETE FROM student WHERE StudentID = '" + studentID + "'";
                if (mysql_query(conn, deleteStudentQuery.c_str())) {
                    std::cerr << "Error deleting student: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Student record deleted successfully!\n";
                }

                std::cout << "\nPress any key to continue...\n";
#ifdef _WIN32
                _getch();
#else
                getchar();
#endif
            }
            else if (options[selectedIndex] == "Back to Previous Page") {
                std::cout << "Returning to the previous page...\n";
                break; // Exit the loop and go back to the previous page
            }
        }
    }
}


void printAdminLecturerHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Manage Lecturers                     " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void AdminLecturer() {
    std::string lecturerID, lecturerName, lecturerEmail, password;

    const std::string options[4] = { "Add Lecturer", "Update Lecturer", "Delete Lecturer", "Back to Previous Page" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printAdminLecturerHeader();  // Print the header with a border

        for (int i = 0; i < 4; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 4) % 4;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 4;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Add Lecturer") {
                std::cout << "Enter LecturerID: ";
                std::cin >> lecturerID;

                std::cin.ignore();  // To ignore the newline character left in the buffer from previous input

                std::cout << "Enter Lecturer Name: ";
                std::getline(std::cin, lecturerName); // Use getline to read names with spaces

                std::cout << "Enter Lecturer Email: ";
                std::getline(std::cin, lecturerEmail); // Use getline to read emails with spaces

                std::cout << "Enter Lecturer Password: ";
                std::getline(std::cin, password); // Use getline for password

                std::string query = "INSERT INTO lecturer (LecturerID, LecturerName, LecturerEmail, Password) "
                    "VALUES ('" + lecturerID + "', '" + lecturerName + "', '" + lecturerEmail + "', '" + password + "')";
                if (mysql_query(conn, query.c_str())) {
                    std::cerr << "Error inserting lecturer: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Lecturer added successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Update Lecturer") {
                std::cout << "Enter LecturerID to update: ";
                std::cin >> lecturerID;

                std::cin.ignore();  // To ignore the newline character

                std::string checkQuery = "SELECT * FROM lecturer WHERE LecturerID = '" + lecturerID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this Lecturer!\n";
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                std::cout << "Enter New Lecturer Name: ";
                std::getline(std::cin, lecturerName);

                std::cout << "Enter New Lecturer Email: ";
                std::getline(std::cin, lecturerEmail);

                std::cout << "Enter New Lecturer Password: ";
                std::getline(std::cin, password);

                std::string updateQuery = "UPDATE lecturer SET LecturerName = '" + lecturerName + "', LecturerEmail = '" + lecturerEmail +
                    "', Password = '" + password + "' WHERE LecturerID = '" + lecturerID + "'";

                if (mysql_query(conn, updateQuery.c_str())) {
                    std::cerr << "Error updating lecturer: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Lecturer updated successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Delete Lecturer") {
                std::cout << "Enter LecturerID to delete: ";
                std::cin >> lecturerID;

                std::cin.ignore();  // To ignore the newline character

                std::string checkQuery = "SELECT * FROM lecturer WHERE LecturerID = '" + lecturerID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this Lecturer!\n";
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                std::string deleteQuery = "DELETE FROM lecturer WHERE LecturerID = '" + lecturerID + "'";
                if (mysql_query(conn, deleteQuery.c_str())) {
                    std::cerr << "Error deleting lecturer: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Lecturer deleted successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Back to Previous Page") {
                std::cout << "Returning to previous menu...\n";
                break;
            }

            std::cout << "\nPress any key to continue...\n";
#ifdef _WIN32
            _getch();
#else
            getchar();
#endif
        }
    }
}


void printAdminAdminHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Manage Admins                        " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void AdminAdmin() {
    std::string adminID, adminName, adminPassword;

    const std::string options[4] = { "Add Admin", "Update Admin", "Delete Admin", "Back to Previous Page" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printAdminAdminHeader();  // Print the header with a border

        for (int i = 0; i < 4; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 4) % 4;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 4;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Add Admin") {
                std::cout << "Enter AdminID: ";
                std::cin >> adminID;

                std::cin.ignore();  // To ignore the newline character left in the buffer from previous input

                std::cout << "Enter Admin Name: ";
                std::getline(std::cin, adminName);  // Use getline to read names with spaces

                std::cout << "Enter Admin Password: ";
                std::getline(std::cin, adminPassword);  // Use getline for password

                std::string query = "INSERT INTO admin (AdminID, AdminName, AdminPassword) "
                    "VALUES ('" + adminID + "', '" + adminName + "', '" + adminPassword + "')";
                if (mysql_query(conn, query.c_str())) {
                    std::cerr << "Error inserting admin: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Admin added successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Update Admin") {
                std::cout << "Enter AdminID to update: ";
                std::cin >> adminID;

                std::cin.ignore();  // To ignore the newline character

                std::string checkQuery = "SELECT * FROM admin WHERE AdminID = '" + adminID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this Admin!\n";
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                std::cout << "Enter New Admin Name: ";
                std::getline(std::cin, adminName);

                std::cout << "Enter New Admin Password: ";
                std::getline(std::cin, adminPassword);

                std::string updateQuery = "UPDATE admin SET AdminName = '" + adminName + "', AdminPassword = '" + adminPassword +
                    "' WHERE AdminID = '" + adminID + "'";

                if (mysql_query(conn, updateQuery.c_str())) {
                    std::cerr << "Error updating admin: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Admin updated successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Delete Admin") {
                std::cout << "Enter AdminID to delete: ";
                std::cin >> adminID;

                std::cin.ignore();  // To ignore the newline character

                std::string checkQuery = "SELECT * FROM admin WHERE AdminID = '" + adminID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this Admin!\n";
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                std::string deleteQuery = "DELETE FROM admin WHERE AdminID = '" + adminID + "'";
                if (mysql_query(conn, deleteQuery.c_str())) {
                    std::cerr << "Error deleting admin: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Admin deleted successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Back to Previous Page") {
                std::cout << "Returning to previous menu...\n";
                break;
            }

            std::cout << "\nPress any key to continue...\n";
#ifdef _WIN32
            _getch();
#else
            getchar();
#endif
        }
    }
}


void printAdminSubjectHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Manage Subjects                       " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void AdminSubject() {
    std::string subjectID, subjectName;
    int credit;

    const std::string options[4] = { "Add Subject", "Update Subject", "Delete Subject", "Back to Previous Page" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printAdminSubjectHeader();  // Print the header with a border

        for (int i = 0; i < 4; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 4) % 4;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 4;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Add Subject") {
                std::cout << "Enter SubjectID: ";
                std::cin >> subjectID;

                std::cin.ignore();  // To ignore the newline character left in the buffer from previous input

                std::cout << "Enter Subject Name: ";
                std::getline(std::cin, subjectName);  // Use getline to read names with spaces

                std::cout << "Enter Credit: ";
                std::cin >> credit;

                std::string query = "INSERT INTO subject (SubjectID, SubjectName, Credit) "
                    "VALUES ('" + subjectID + "', '" + subjectName + "', " + std::to_string(credit) + ")";
                if (mysql_query(conn, query.c_str())) {
                    std::cerr << "Error inserting subject: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Subject added successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Update Subject") {
                std::cout << "Enter SubjectID to update: ";
                std::cin >> subjectID;

                std::cin.ignore();  // To ignore the newline character

                std::string checkQuery = "SELECT * FROM subject WHERE SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No record found for this Subject!\n";
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                std::cout << "Enter New Subject Name: ";
                std::getline(std::cin, subjectName);

                std::cout << "Enter New Credit: ";
                std::cin >> credit;

                std::string updateQuery = "UPDATE subject SET SubjectName = '" + subjectName + "', Credit = " + std::to_string(credit) +
                    " WHERE SubjectID = '" + subjectID + "'";

                if (mysql_query(conn, updateQuery.c_str())) {
                    std::cerr << "Error updating subject: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Subject updated successfully!\n";
                }
            }
            else if (options[selectedIndex] == "Delete Subject") {
                std::cout << "Enter SubjectID to delete: ";
                std::cin >> subjectID;

                std::cin.ignore();  // To ignore the newline character

                // Check if the subject exists
                std::string checkQuery = "SELECT * FROM subject WHERE SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, checkQuery.c_str())) {
                    std::cerr << "Error checking subject record: " << mysql_error(conn) << "\n";
                    continue;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                if (res && mysql_num_rows(res) == 0) {
                    std::cerr << "Error: No subject found with this SubjectID!\n";
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // Delete related data from performancedetail
                std::string deletePerformanceDetailQuery =
                    "DELETE FROM performancedetail WHERE SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, deletePerformanceDetailQuery.c_str())) {
                    std::cerr << "Error deleting related data in performancedetail: " << mysql_error(conn) << "\n";
                    continue;
                }
                else {
                    std::cout << "Related data in performancedetail deleted successfully.\n";
                }

                // Delete the subject
                std::string deleteSubjectQuery = "DELETE FROM subject WHERE SubjectID = '" + subjectID + "'";
                if (mysql_query(conn, deleteSubjectQuery.c_str())) {
                    std::cerr << "Error deleting subject: " << mysql_error(conn) << "\n";
                }
                else {
                    std::cout << "Subject deleted successfully!\n";
                }

                std::cout << "\nPress any key to continue...\n";

#ifdef _WIN32
                _getch();
#else
                getchar();
#endif
            }
            else if (options[selectedIndex] == "Back to Previous Page") {
                std::cout << "Returning to the previous page...\n";
                break; // Exit the loop and go back to the previous page
            }
        }
    }
}


void StudentArchive(MYSQL* conn) {
    // Query to fetch archived students
    const char* query = "SELECT ArchiveID, StudentName, SubjectName, SubjectGPA FROM studentarchive";

    // Execute the query
    if (mysql_query(conn, query)) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }

    // Store the result set
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        std::cerr << "Failed to retrieve results: " << mysql_error(conn) << std::endl;
        return;
    }

    // Print table headers
    std::cout << std::left << std::setw(10) << "ArchiveID"
        << std::setw(30) << "StudentName"
        << std::setw(40) << "SubjectName"
        << std::setw(10) << "SubjectGPA" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // Fetch and print each row
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << std::left << std::setw(10) << row[0]  // ArchiveID
            << std::setw(30) << row[1]  // StudentName
            << std::setw(40) << row[2]  // SubjectName
            << std::setw(10) << row[3]  // SubjectGPA
            << std::endl;
    }

    // Free the result set
    mysql_free_result(res);

    // Prompt user to continue
    std::cout << "\nPress Enter to return to the menu.";
    std::cin.ignore();
    std::cin.get();
}


void printAdminMenuHeader() {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "            Admin Menu                           " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
}

void adminMenu() {
    const std::string options[6] = { "Manage Students", "Manage Lecturers", "Manage Admins", "Manage Subjects", "Student Archive", "Logout" };
    int selectedIndex = 0;

    while (true) {
        std::cout << "\033[2J\033[H"; // Clear the screen
        printAdminMenuHeader();  // Print the header with a border

        for (int i = 0; i < 6; ++i) {
            if (i == selectedIndex) {
                std::cout << "> " << options[i] << " <\n";  // Highlight selected option
            }
            else {
                std::cout << "  " << options[i] << "\n";  // Display unselected options
            }
        }

        std::cout << "-------------------------------------------------" << std::endl;  // Bottom border

#ifdef _WIN32
        int key = _getch();
        if (key == 224) key = _getch();
#else
        int key = getch();
#endif

        if (key == ARROW_UP) selectedIndex = (selectedIndex - 1 + 6) % 6;  // Wrap around the options
        else if (key == ARROW_DOWN) selectedIndex = (selectedIndex + 1) % 6;
        else if (key == ENTER_KEY) {
            if (options[selectedIndex] == "Manage Students") {
                AdminStudent();  // Function to manage students
            }
            else if (options[selectedIndex] == "Manage Lecturers") {
                AdminLecturer();  // Function to manage lecturers
            }
            else if (options[selectedIndex] == "Manage Admins") {
                AdminAdmin();  // Function to manage admins
            }
            else if (options[selectedIndex] == "Manage Subjects") {
                AdminSubject();  // Function to manage subjects
            }
            else if (options[selectedIndex] == "Student Archive") {
                StudentArchive(conn);  // Display the archived students
            }
            else if (options[selectedIndex] == "Logout") {
                break;  // Exit the loop and log out
            }
        }
    }
}


void adminLogin() {
    std::string adminID, password;

    std::cout << "\033[2J\033[H"; // Clear the screen
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "              Admin Login                        " << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    // Prompt for AdminID and Password
    std::cout << "AdminID: ";
    std::cin >> adminID;

    std::cout << "Password: ";
#ifdef _WIN32
    password = ""; // Hide password input (Windows)
    char ch;
    while ((ch = _getch()) != '\r') { // Stop at ENTER
        if (ch == '\b' && !password.empty()) { // Handle backspace
            password.pop_back();
            std::cout << "\b \b";
        }
        else if (ch != '\b') {
            password += ch;
            std::cout << '*';
        }
    }
    std::cout << '\n';
#else
    termios oldt, newt; // Hide password input (Unix-like systems)
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::cin >> password;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    // Query to verify login
    std::string query = "SELECT AdminName FROM Admin WHERE AdminID = '" + adminID + "' AND AdminPassword = '" + password + "'";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query Execution Failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res && mysql_num_rows(res) > 0) {
        MYSQL_ROW row = mysql_fetch_row(res);
        std::cout << "Login Successful! Welcome, " << row[0] << ".\n";
        mysql_free_result(res);

        // Call the admin menu after successful login
        adminMenu();
    }
    else {
        std::cerr << "Invalid AdminID or Password!\n";
        mysql_free_result(res);
    }
}




int main() {
    db_response::ConnectionFunction();

    std::string userType = menuSelection();
    if (userType == "Lecturer") {
        lecturerLogin();
    }
    else if (userType == "Student") {
        studentLogin();  // Assuming you have this function already implemented
    }
    else if (userType == "Admin") {
        adminLogin();  // Calling admin login function
    }

    db_response::Cleanup();
    return 0;
}



