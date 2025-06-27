#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;



//helper function to convert string to lowercase
std::string convertToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return str;
}

//helper function to split strings by delimiter
std::vector<std::string> tokenize(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

//func to create a table (file)
void buildTable(const std::string& sql_cmd) {
    //parse the create table command using regex
    std::regex createRegex("CREATE\\s+TABLE\\s+(\\w+)\\s*\\(\\s*(.+?)\\s*\\)", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(sql_cmd, match, createRegex)) {
        std::string tableName = convertToLower(match[1]);
        std::string columns = match[2];
        
        //clean up column string by removing spaces after commas
        std::stringstream cleanedColumns;
        bool afterComma = false;
        for (char c : columns) {
            if (c == ',') {
                cleanedColumns << c;
                afterComma = true;
            } else if (afterComma && std::isspace(c)) {
                //skip spaces after comma
                continue;
            } else {
                cleanedColumns << c;
                afterComma = false;
            }
        }

        //create database directory if it doesn't exist
        if (!fs::exists("database")) {
            fs::create_directory("database");
                    fs::permissions("database", 
                        fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all,
                        fs::perm_options::replace);
        }

        //create table file and write column headers
        std::ofstream outFile("database/" + tableName + ".txt");
        outFile << cleanedColumns.str() << "\n"; 
        outFile.close();

        // Set file permissions to 777
        fs::permissions("database/"+ tableName + ".txt",
                        fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all,
                        fs::perm_options::replace);

        std::cout << "Table '" << tableName << "' created successfully.\n";
    } else {
        std::cout << "Error: Invalid CREATE TABLE syntax.\n";
    }
}

//func to insert values into a table (file)
void addToTable(const std::string& sql_cmd) {
    //parse the insert command using regex
    std::regex insertRegex("INSERT\\s+INTO\\s+(\\w+)\\s+VALUES\\s*\\(\\s*(.+?)\\s*\\)", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(sql_cmd, match, insertRegex)) {
        std::string tableName = convertToLower(match[1]);
        std::string values = match[2];
        
        //clean up the values string, preserving spaces inside quotes
        std::stringstream cleanedValues;
        bool insideQuotes = false;
        bool afterComma = false;
        
        for (char c : values) {
            if (c == '\'') {
                //toggle quote state
                insideQuotes = !insideQuotes;
                cleanedValues << c;
            } else if (c == ',' && !insideQuotes) {
                cleanedValues << c;
                afterComma = true;
            } else if (afterComma && std::isspace(c) && !insideQuotes) {
                //skip spaces after comma when not in quotes
                continue;
            } else {
                cleanedValues << c;
                afterComma = false;
            }
        }

        //check if table exists before inserting
        std::ifstream checkFile("database/" + tableName + ".txt");
        if (!checkFile) {
            std::cout << "Error: Table '" << tableName << "' does not exist.\n";
            return;
        }
        checkFile.close();

        //add the new values to the table file
        std::ofstream outFile("database/" + tableName + ".txt", std::ios::app);
        outFile << cleanedValues.str() << "\n";
        std::cout << "Values inserted into '" << tableName << "' successfully.\n";
    } else {
        std::cout << "Error: Invalid INSERT INTO syntax.\n";
    }
}

//func to display table content
void showTable(const std::string& tableName) {
    std::ifstream inFile("database/" + tableName + ".txt");
    if (!inFile) {
        std::cout << "Error: Table '" << tableName << "' does not exist.\n";
        return;
    }

    //read and display all lines from the table file
    std::string line;
    while (std::getline(inFile, line)) {
        auto values = tokenize(line, ',');
        for (const auto& value : values) {
            std::cout << std::setw(15) << std::left << value;
        }
        std::cout << "\n";
    }
}

//func to display selected columns with optional filtering

void showTableWithFilters(const std::string& tableName, const std::vector<std::string>& selectedCols,
                          const std::string& whereCol = "", const std::string& whereOp = "=",
                          const std::string& whereValue = "") {
    std::ifstream inFile("database/" + tableName + ".txt");
    if (!inFile) {
        std::cout << "Error: Table '" << tableName << "' does not exist.\n";
        return;
    }

    //read header line and parse column names
    std::string line;
    std::getline(inFile, line);
    auto headers = tokenize(line, ',');

    std::vector<std::string> lowerHeaders;
    for (const auto& header : headers) {
        lowerHeaders.push_back(convertToLower(header));
    }

    //determine which columns to display
    std::vector<int> selectedIndexes;
    if (selectedCols.size() == 1 && selectedCols[0] == "*") {
        for (size_t i = 0; i < headers.size(); ++i) {
            selectedIndexes.push_back(i);
        }
    } else {
        for (const auto& col : selectedCols) {
            std::string lowerCol = convertToLower(col);
            auto it = std::find(lowerHeaders.begin(), lowerHeaders.end(), lowerCol);
            if (it != lowerHeaders.end()) {
                selectedIndexes.push_back(std::distance(lowerHeaders.begin(), it));
            } else {
                std::cout << "Error: Column '" << col << "' not found in table.\n";
                return;
            }
        }
    }

    //display header row
    for (size_t i = 0; i < selectedIndexes.size(); ++i) {
        std::cout << headers[selectedIndexes[i]];
        if (i < selectedIndexes.size() - 1) std::cout << ",";
    }
    std::cout << "\n";

    //process and display data rows
    while (std::getline(inFile, line)) {
        auto values = tokenize(line, ',');

        //apply WHERE id present
        bool matchesCondition = true;
        if (!whereCol.empty()) {
            std::string lowerWhereCol = convertToLower(whereCol);
            auto it = std::find(lowerHeaders.begin(), lowerHeaders.end(), lowerWhereCol);
            if (it == lowerHeaders.end()) continue; //invalid column
            int whereIndex = std::distance(lowerHeaders.begin(), it);

            if (whereIndex >= values.size()) continue;

            std::string cellValue = values[whereIndex];

            //handle the comparison
            if (whereOp == "=") {
                matchesCondition = (cellValue == whereValue);
            } else if (whereOp == "!=") {
                matchesCondition = (cellValue != whereValue);
            } else {
                //try numeric comparison
                try {
                    double cellNum = std::stod(cellValue);
                    double whereNum = std::stod(whereValue);

                    if (whereOp == ">") {
                        matchesCondition = (cellNum > whereNum);
                    } else if (whereOp == "<") {
                        matchesCondition = (cellNum < whereNum);
                    } else if (whereOp == ">=") {
                        matchesCondition = (cellNum >= whereNum);
                    } else if (whereOp == "<=") {
                        matchesCondition = (cellNum <= whereNum);
                    } else {
                        matchesCondition = false;
                    }
                } catch (...) {
                    matchesCondition = false; //skip row if non-numeric comparison failed
                }
            }
        }

        if (!matchesCondition) continue;

        //display selected columns
        for (size_t i = 0; i < selectedIndexes.size(); ++i) {
            int idx = selectedIndexes[i];
            if (idx < values.size()) {
                std::cout << values[idx];
            }
            if (i < selectedIndexes.size() - 1) std::cout << ",";
        }
        std::cout << "\n";
    }
}


//handle DESC tables 

void describeTable(const std::string& sql_cmd) {
    std::regex descRegex(R"(DESC\s+(\w+))", std::regex::icase);
    std::smatch match;

    if (std::regex_match(sql_cmd, match, descRegex)) {
        std::string tableName = convertToLower(match[1]);
        std::ifstream inFile("database/" + tableName + ".txt");

        if (!inFile) {
            std::cout << "Error: Table '" << tableName << "' does not exist.\n";
            return;
        }

        std::string headerLine;
        if (std::getline(inFile, headerLine)) {
            auto headers = tokenize(headerLine, ',');
            std::cout << "Columns in table '" << tableName << "':\n";
            for (const auto& col : headers) {
                std::cout << "- " << col << "\n";
            }
        } else {
            std::cout << "Error: Table '" << tableName << "' is empty.\n";
        }
    } else {
        std::cout << "Error: Invalid DESC syntax. Use: DESC tablename\n";
    }
}

//handle JOIN operations between two tables

void processJoin(const std::string& sql_cmd) {
    //updated regex: supports table.col OP value in WHERE
    std::regex joinRegex(
        R"(SELECT\s+(.+?)\s+FROM\s+(\w+)\s+JOIN\s+(\w+)\s+ON\s+(\w+)\.(\w+)\s*=\s*(\w+)\.(\w+)(?:\s+WHERE\s+(\w+)\.(\w+)\s*(=|!=|<=|>=|<|>)\s*'?(.*?)'?)?\s*$)",
        std::regex::icase
    );
    std::smatch match;

    if (std::regex_search(sql_cmd, match, joinRegex)) {
        std::string columns     = match[1];
        std::string table1      = convertToLower(match[2]);
        std::string table2      = convertToLower(match[3]);
        std::string joinTable1  = convertToLower(match[4]);
        std::string joinCol1    = convertToLower(match[5]);
        std::string joinTable2  = convertToLower(match[6]);
        std::string joinCol2    = convertToLower(match[7]);

        std::string whereTable  = match[8].matched ? convertToLower(match[8].str()) : "";
        std::string whereCol    = match[9].matched ? convertToLower(match[9].str()) : "";
        std::string whereOp     = match[10].matched ? match[10].str() : "";
        std::string whereVal    = match[11].matched ? match[11].str() : "";

       // std::cout << "whereTable=" << whereTable << " whereCol=" << whereCol 
         //         << " whereOp=" << whereOp << " whereVal=" << whereVal << "\n";

        //validate files
        std::ifstream table1File("database/" + table1 + ".txt");
        std::ifstream table2File("database/" + table2 + ".txt");
        if (!table1File || !table2File) {
            std::cout << "Error: One or both tables do not exist.\n";
            return;
        }

        //headers
        std::string line, header1, header2;
        std::getline(table1File, header1);
        std::getline(table2File, header2);
        auto headers1 = tokenize(header1, ',');
        auto headers2 = tokenize(header2, ',');

        int joinIdx1 = -1, joinIdx2 = -1;
        for (size_t i = 0; i < headers1.size(); ++i)
            if (convertToLower(headers1[i]) == joinCol1) joinIdx1 = i;
        for (size_t i = 0; i < headers2.size(); ++i)
            if (convertToLower(headers2[i]) == joinCol2) joinIdx2 = i;
        if (joinIdx1 == -1 || joinIdx2 == -1) {
            std::cout << "Error: Join columns not found.\n";
            return;
        }

        //column selection
        std::vector<std::string> selectedCols = columns == "*" ? std::vector<std::string>{"*"} : tokenize(columns, ',');
        for (auto& col : selectedCols) {
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
        }

        //combined headers
        std::vector<std::string> combinedHeaders;
        for (const auto& h : headers1) combinedHeaders.push_back(table1 + "." + h);
        for (const auto& h : headers2) combinedHeaders.push_back(table2 + "." + h);

        std::vector<int> selectedIndices;
        if (selectedCols[0] == "*") {
            for (size_t i = 0; i < combinedHeaders.size(); ++i) selectedIndices.push_back(i);
        } else {
            for (const auto& col : selectedCols) {
                bool found = false;
                for (size_t i = 0; i < combinedHeaders.size(); ++i) {
                    if (convertToLower(combinedHeaders[i]) == convertToLower(col)) {
                        selectedIndices.push_back(i);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cout << "Error: Column '" << col << "' not found in joined tables.\n";
                    return;
                }
            }
        }

        //print header
        for (size_t i = 0; i < selectedIndices.size(); ++i) {
            std::cout << combinedHeaders[selectedIndices[i]];
            if (i < selectedIndices.size() - 1) std::cout << ",";
        }
        std::cout << "\n";

        //load data
        std::vector<std::vector<std::string>> table1Data, table2Data;
        table1File.clear(); table1File.seekg(0); std::getline(table1File, line);
        while (std::getline(table1File, line)) table1Data.push_back(tokenize(line, ','));
        table2File.clear(); table2File.seekg(0); std::getline(table2File, line);
        while (std::getline(table2File, line)) table2Data.push_back(tokenize(line, ','));

        //join and display
        for (const auto& row1 : table1Data) {
            for (const auto& row2 : table2Data) {
                if (row1[joinIdx1] == row2[joinIdx2]) {
                    bool pass = true;

                    if (!whereCol.empty()) {
                        std::string val;
                        bool found = false;

                        if (whereTable == table1) {
                            for (size_t i = 0; i < headers1.size(); ++i) {
                                if (convertToLower(headers1[i]) == whereCol) {
                                    val = row1[i]; found = true; break;
                                }
                            }
                        } else if (whereTable == table2) {
                            for (size_t i = 0; i < headers2.size(); ++i) {
                                if (convertToLower(headers2[i]) == whereCol) {
                                    val = row2[i]; found = true; break;
                                }
                            }
                        }

                        if (found) {
                            //comparison
                            if (whereOp == "=") pass = val == whereVal;
                            else if (whereOp == "!=") pass = val != whereVal;
                            else {
                                double dVal = std::stod(val), dTarget = std::stod(whereVal);
                                if (whereOp == ">") pass = dVal > dTarget;
                                else if (whereOp == "<") pass = dVal < dTarget;
                                else if (whereOp == ">=") pass = dVal >= dTarget;
                                else if (whereOp == "<=") pass = dVal <= dTarget;
                            }
                        } else pass = false;
                    }

                    if (!pass) continue;

                    std::vector<std::string> combinedRow = row1;
                    combinedRow.insert(combinedRow.end(), row2.begin(), row2.end());

                    for (size_t i = 0; i < selectedIndices.size(); ++i) {
                        std::cout << combinedRow[selectedIndices[i]];
                        if (i < selectedIndices.size() - 1) std::cout << ",";
                    }
                    std::cout << "\n";
                }
            }
        }
    } else {
        std::cout << "Error: Invalid JOIN syntax.\n";
    }
}



//process the SQL command and route to appropriate handler function
void executeSqlCommand(const std::string& sql_cmd) {
    //convert to lowercase for case-insensitive command matching
    std::string lowerCmd = convertToLower(sql_cmd);
    
    //route to appropriate function based on command type
    if (lowerCmd.find("create table") == 0) {
        buildTable(sql_cmd);
    } else if (lowerCmd.find("insert into") == 0) {
        addToTable(sql_cmd);
    } else if (lowerCmd.find("desc") == 0) {
        describeTable(sql_cmd);  
    } else if (lowerCmd.find("select") == 0) {
        //determine if it's a JOIN query or a simple SELECT
        if (lowerCmd.find("join") != std::string::npos) {
            processJoin(sql_cmd);
        } else {
            //handle regular SELECT statements with or without WHERE clause
            std::regex selectRegex;
            std::smatch match;
            
            //handle SELECT with WHERE clause
            if (lowerCmd.find("where") != std::string::npos) {
                  selectRegex = std::regex( R"(SELECT\s+(.+?)\s+FROM\s+(\w+)\s+WHERE\s+(\w+)\s*(=|!=|>=|<=|<|>)\s*'?(.*?)'?(?:\s|$))",std::regex::icase  );

                if (std::regex_search(sql_cmd, match, selectRegex)) {
                    //extract components from the regex match
                    std::string columns = match[1];
                    std::string tableName = convertToLower(match[2]);
                    std::string whereCol = convertToLower(match[3]);
                   // std::string whereValue = match[4];
		    std::string whereOp = match[4];
                    std::string whereValue = match[5];
                    
                    //parse the column list
                    std::vector<std::string> selectedCols;
                    if (columns == "*") {
                        selectedCols.push_back("*");
                    } else {
                        //split comma-separated column list and trim spaces
                        std::stringstream ss(columns);
                        std::string col;
                        while (std::getline(ss, col, ',')) {
                            col.erase(0, col.find_first_not_of(" \t"));
                            col.erase(col.find_last_not_of(" \t") + 1);
                            selectedCols.push_back(col);
                        }
                    }
                    
                    //display the table with column selection and filtering
                    showTableWithFilters(tableName, selectedCols, whereCol,whereOp,  whereValue);
                } else {
                    std::cout << "Error: Invalid SELECT WITH WHERE syntax.\n";
                }
            } 
            //handle simple SELECT without WHERE
            else {
                selectRegex = std::regex("SELECT\\s+(.+?)\\s+FROM\\s+(\\w+)", std::regex::icase);
                
                if (std::regex_search(sql_cmd, match, selectRegex)) {
                    //extract components from regex match
                    std::string columns = match[1];
                    std::string tableName = convertToLower(match[2]);
                    
                    //parse column list
                    std::vector<std::string> selectedCols;
                    if (columns == "*") {
                        selectedCols.push_back("*");
                    } else {
                        //split comma-separated column list and trim spaces
                        std::stringstream ss(columns);
                        std::string col;
                        while (std::getline(ss, col, ',')) {
                            col.erase(0, col.find_first_not_of(" \t"));
                            col.erase(col.find_last_not_of(" \t") + 1);
                            selectedCols.push_back(col);
                        }
                    }
                    
                    //display table with column selection
                    showTableWithFilters(tableName, selectedCols);
                } else {
                    std::cout << "Error: Invalid SELECT syntax.\n";
                }
            }
        }
    } else {
        std::cout << "Error: Unrecognized SQL command.\n";
    }
}

//main function, entry point of the program
int main(int argc, char* argv[]) {
    //check if a command line argument is provided
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " 'SQL command'\n";
        return 1;
    }
    
    //get SQL command from command line argument
    std::string sql_cmd = argv[1];
    
    //process SQL command


    executeSqlCommand(sql_cmd);
    
    return 0;
}




