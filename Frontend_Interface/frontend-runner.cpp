// clang-format off
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
// clang-format on

#include "../Disk_Class/Disk.h"
#include "../Frontend/Frontend.h"
#include "../define/constants.h"
#include "commands.h"

using namespace std;

int getOperator(string op_str);

void nameToTruncatedArray(string nameString, char *nameArray);

void printErrorMsg(int error);

void printHelp();

class RegexHandler {
  typedef int (RegexHandler::*handlerFunction)(void);  // function pointer type

 private:
  // extract tokens delimited by whitespace and comma
  vector<string> extractTokens(string input) {
    regex re("\\s*,\\s*|\\s+");
    sregex_token_iterator first(input.begin(), input.end(), re, -1), last;
    vector<string> tokens(first, last);
    return tokens;
  }

  // handler functions
  smatch m;
  int helpHandler() {
    printHelp();
    return SUCCESS;
  };

  int exitHandler() {
    return EXIT;
  };

  int echoHandler() {
    string message = m[2];
    cout << message << endl;
    return SUCCESS;
  }

  int runHandler() {
    string fileName = m[2];
    const string filePath = BATCH_FILES_PATH;
    fstream commandsFile;
    commandsFile.open(filePath + fileName, ios::in);
    string command;
    if (!commandsFile.is_open()) {
      cout << "The file " << fileName << " does not exist\n";
      return FAILURE;
    }

    int lineNumber = 1;
    while (getline(commandsFile, command)) {
      int ret = this->handle(command);
      if (ret == EXIT) {
        break;
      } else if (ret != SUCCESS) {
        cout << "Executed up till line " << lineNumber - 1 << ".\n";
        cout << "Error at line number " << lineNumber << ". Subsequent lines will be skipped.\n";
        break;
      }
      lineNumber++;
    }

    commandsFile.close();

    return SUCCESS;  // error messages if any will be printed in recursive call to handle
  }

  int openHandler() {
    string tablename = m[3];
    char relname[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);

    int ret = Frontend::open_table(relname);
    if (ret >= 0) {
      cout << "Relation " << relname << " opened successfully\n";
      return SUCCESS;
    }
    return ret;
  }

  int closeHandler() {
    string tablename = m[3];
    char relname[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);

    int ret = Frontend::close_table(relname);
    if (ret == SUCCESS) {
      cout << "Relation " << relname << " closed successfully\n";
    }

    return ret;
  }

  int createTableHandler() {
    string tablename = m[1];
    char relname[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);

    vector<string> words = extractTokens(m[2]);

    int no_attrs = words.size() / 2;

    if (no_attrs > 125) {
      return E_MAXATTRS;
    }

    char attribute[no_attrs][ATTR_SIZE];
    int type_attr[no_attrs];

    for (int i = 0, k = 0; i < no_attrs; i++, k += 2) {
      nameToTruncatedArray(words[k], attribute[i]);
      if (words[k + 1] == "STR")
        type_attr[i] = STRING;
      else if (words[k + 1] == "NUM")
        type_attr[i] = NUMBER;
    }

    int ret = Frontend::create_table(relname, no_attrs, attribute, type_attr);
    if (ret == SUCCESS) {
      cout << "Relation " << relname << " created successfully" << endl;
    }

    return ret;
  }

  int dropTableHandler() {
    string tablename = m[3];
    char relname[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);

    if (strcmp(relname, "RELATIONCAT") == 0 || strcmp(relname, "ATTRIBUTECAT") == 0) {
      cout << "Error: Cannot Delete Relation Catalog or Attribute Catalog" << endl;
      return FAILURE;
    }

    int ret = Frontend::drop_table(relname);
    if (ret == SUCCESS) {
      cout << "Relation " << relname << " deleted successfully" << endl;
    }
    return ret;
  }

  int createIndexHandler() {
    string tablename = m[4];
    string attrname = m[5];
    char relname[ATTR_SIZE], attr_name[ATTR_SIZE];

    nameToTruncatedArray(tablename, relname);
    nameToTruncatedArray(attrname, attr_name);

    int ret = Frontend::create_index(relname, attr_name);
    if (ret == SUCCESS) {
      cout << "Index created successfully\n";
    }

    return ret;
  }

  int dropIndexHandler() {
    string tablename = m[4];
    string attrname = m[5];
    char relname[ATTR_SIZE], attr_name[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);
    nameToTruncatedArray(attrname, attr_name);

    int ret = Frontend::drop_index(relname, attr_name);
    if (ret == SUCCESS) {
      cout << "Index deleted successfully\n";
    }

    return ret;
  }

  int renameTableHandler() {
    string oldTableName = m[4];
    string newTableName = m[6];

    char old_relation_name[ATTR_SIZE];
    char new_relation_name[ATTR_SIZE];
    nameToTruncatedArray(oldTableName, old_relation_name);
    nameToTruncatedArray(newTableName, new_relation_name);

    int ret = Frontend::alter_table_rename(old_relation_name, new_relation_name);
    if (ret == SUCCESS) {
      cout << "Renamed Relation Successfully" << endl;
    }

    return ret;
  }

  int renameColumnHandler() {
    string tablename = m[4];
    string oldcolumnname = m[6];
    string newcolumnname = m[8];
    char relname[ATTR_SIZE];
    char old_col[ATTR_SIZE];
    char new_col[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);
    nameToTruncatedArray(oldcolumnname, old_col);
    nameToTruncatedArray(newcolumnname, new_col);

    int ret = Frontend::alter_table_rename_column(relname, old_col, new_col);
    if (ret == SUCCESS) {
      cout << "Renamed Attribute Successfully" << endl;
    }

    return ret;
  }

  int insertSingleHandler() {
    string table_name = m[1];
    char rel_name[ATTR_SIZE];
    nameToTruncatedArray(table_name, rel_name);

    vector<string> words = extractTokens(m[2]);

    int attr_count = words.size();
    char attr_values_arr[words.size()][ATTR_SIZE];
    for (int i = 0; i < words.size(); ++i) {
      strcpy(attr_values_arr[i], words[i].c_str());
    }

    int ret = Frontend::insert_into_table_values(rel_name, words.size(), attr_values_arr);
    if (ret == SUCCESS) {
      cout << "Inserted successfully" << endl;
    }

    return ret;
  }

  int insertFromFileHandler() {
    string tablename = m[3];
    char relname[ATTR_SIZE];
    nameToTruncatedArray(tablename, relname);

    string filepath = string(INPUT_FILES_PATH) + string(m[6]);
    std::cout << "File path: " << filepath << endl;

    ifstream file(filepath);
    if (!file.is_open()) {
      cout << "Invalid file path or file does not exist" << endl;
      return FAILURE;
    }

    string errorMsg("");
    string fileLine;

    int retVal = SUCCESS;
    int columnCount = -1, lineNumber = 1;
    while (getline(file, fileLine)) {
      vector<string> row;

      stringstream lineStream(fileLine);

      string item;
      while (getline(lineStream, item, ',')) {
        if (item.size() == 0) {
          errorMsg += "Null values not allowed in attribute values\n";
          retVal = FAILURE;
          break;
        }
        row.push_back(item);
      }
      if (retVal == FAILURE) {
        break;
      }

      if (columnCount == -1) {
        columnCount = row.size();
      } else if (columnCount != row.size()) {
        errorMsg += "Mismatch in number of attributes\n";
        retVal = FAILURE;
        break;
      }

      char rowArray[columnCount][ATTR_SIZE];
      for (int i = 0; i < columnCount; ++i) {
        nameToTruncatedArray(row[i], rowArray[i]);
      }

      retVal = Frontend::insert_into_table_values(relname, columnCount, rowArray);

      if (retVal != SUCCESS) {
        break;
      }

      lineNumber++;
    }

    file.close();

    if (retVal == SUCCESS) {
      cout << lineNumber - 1 << " rows inserted successfully" << endl;
    } else {
      if (lineNumber > 1) {
        std::cout << "Rows till line " << lineNumber - 1 << " successfully inserted\n";
      }
      std::cout << "Insertion error at line " << lineNumber << " in file \n";
      std::cout << "Subsequent lines will be skipped\n";
      if (retVal == FAILURE) {
        std::cout << "Error:" << errorMsg;
      }
    }

    return retVal;
  }

  int selectFromHandler() {
    string sourceRelName_str = m[4];
    string targetRelName_str = m[6];

    char sourceRelName[ATTR_SIZE];
    char targetRelName[ATTR_SIZE];

    nameToTruncatedArray(sourceRelName_str, sourceRelName);
    nameToTruncatedArray(targetRelName_str, targetRelName);

    int ret = Frontend::select_from_table(sourceRelName, targetRelName);
    if (ret == SUCCESS) {
      cout << "Selected successfully into " << targetRelName << endl;
    }

    return ret;
  }

  int selectFromWhereHandler() {
    string sourceRel_str = m[4];
    string targetRel_str = m[6];
    string attribute_str = m[8];
    string op_str = m[9];
    string value_str = m[10];

    char sourceRelName[ATTR_SIZE];
    char targetRelName[ATTR_SIZE];
    char attribute[ATTR_SIZE];
    char value[ATTR_SIZE];
    nameToTruncatedArray(sourceRel_str, sourceRelName);
    nameToTruncatedArray(targetRel_str, targetRelName);
    nameToTruncatedArray(attribute_str, attribute);
    nameToTruncatedArray(value_str, value);

    int op = getOperator(op_str);

    int ret = Frontend::select_from_table_where(sourceRelName, targetRelName, attribute, op, value);
    if (ret == SUCCESS) {
      cout << "Selected successfully into " << targetRelName << endl;
    }

    return ret;
  }

  int selectAttrFromHandler() {
    string attribute_list = m[1];
    string sourceRel_str = m[2];
    string targetRel_str = m[3];

    char sourceRelName[ATTR_SIZE];
    char targetRelName[ATTR_SIZE];
    nameToTruncatedArray(sourceRel_str, sourceRelName);
    nameToTruncatedArray(targetRel_str, targetRelName);

    vector<string> attr_tokens = extractTokens(attribute_list);

    int attr_count = attr_tokens.size();
    char attr_list[attr_count][ATTR_SIZE];
    for (int attr_no = 0; attr_no < attr_count; attr_no++) {
      nameToTruncatedArray(attr_tokens[attr_no], attr_list[attr_no]);
    }

    int ret = Frontend::select_attrlist_from_table(sourceRelName, targetRelName, attr_count, attr_list);
    if (ret == SUCCESS) {
      cout << "Selected successfully into " << targetRelName << endl;
    }

    return ret;
  }

  int selectAttrFromWhereHandler() {
    string attribute_list = m[1];
    string sourceRel_str = m[2];
    string targetRel_str = m[3];
    string attribute_str = m[4];
    string op_str = m[5];
    string value_str = m[6];

    char sourceRelName[ATTR_SIZE];
    char targetRelName[ATTR_SIZE];
    char attribute[ATTR_SIZE];
    char value[ATTR_SIZE];
    int op = getOperator(op_str);

    nameToTruncatedArray(attribute_str, attribute);
    nameToTruncatedArray(value_str, value);
    nameToTruncatedArray(sourceRel_str, sourceRelName);
    nameToTruncatedArray(targetRel_str, targetRelName);

    vector<string> attr_tokens = extractTokens(attribute_list);

    int attr_count = attr_tokens.size();
    char attr_list[attr_count][ATTR_SIZE];
    for (int attr_no = 0; attr_no < attr_count; attr_no++) {
      nameToTruncatedArray(attr_tokens[attr_no], attr_list[attr_no]);
    }

    int ret = Frontend::select_attrlist_from_table_where(sourceRelName, targetRelName, attr_count, attr_list,
                                                         attribute, op, value);
    if (ret == SUCCESS) {
      cout << "Selected successfully into " << targetRelName << endl;
    }

    return ret;
  }

  int selectFromJoinHandler() {
    // m[4] and m[10] should be equal ( = sourceRelOneName)
    // m[6] and m[102 should be equal ( = sourceRelTwoName)
    if (m[4] != m[10] || m[6] != m[12]) {
      cout << "Syntax Error" << endl;
      return FAILURE;
    }
    char sourceRelOneName[ATTR_SIZE];
    char sourceRelTwoName[ATTR_SIZE];
    char targetRelName[ATTR_SIZE];
    char joinAttributeOne[ATTR_SIZE];
    char joinAttributeTwo[ATTR_SIZE];

    nameToTruncatedArray(m[4], sourceRelOneName);
    nameToTruncatedArray(m[6], sourceRelTwoName);
    nameToTruncatedArray(m[8], targetRelName);
    nameToTruncatedArray(m[11], joinAttributeOne);
    nameToTruncatedArray(m[13], joinAttributeTwo);

    int ret = Frontend::select_from_join_where(sourceRelOneName, sourceRelTwoName, targetRelName,
                                               joinAttributeOne, joinAttributeTwo);
    if (ret == SUCCESS) {
      cout << "Selected successfully into " << targetRelName << endl;
    }

    return ret;
  }

  int selectAttrFromJoinHandler() {
    char sourceRelOneName[ATTR_SIZE];
    char sourceRelTwoName[ATTR_SIZE];
    char targetRelName[ATTR_SIZE];
    char joinAttributeOne[ATTR_SIZE];
    char joinAttributeTwo[ATTR_SIZE];

    nameToTruncatedArray(m[2], sourceRelOneName);
    nameToTruncatedArray(m[3], sourceRelTwoName);
    nameToTruncatedArray(m[4], targetRelName);
    nameToTruncatedArray(m[6], joinAttributeOne);
    nameToTruncatedArray(m[8], joinAttributeTwo);

    int attrListPos = 1;
    string attributesList = m[1];

    vector<string> attributesListAsWords = extractTokens(m[1]);
    int attrCount = attributesListAsWords.size();
    char attributeList[attrCount][ATTR_SIZE];
    for (int i = 0; i < attrCount; i++) {
      nameToTruncatedArray(attributesListAsWords[i], attributeList[i]);
    }

    int ret = Frontend::select_attrlist_from_join_where(sourceRelOneName, sourceRelTwoName, targetRelName,
                                                        joinAttributeOne, joinAttributeTwo, attrCount,
                                                        attributeList);
    if (ret == SUCCESS) {
      cout << "Selected successfully into " << targetRelName;
    }

    return ret;
  }

  const vector<pair<regex, handlerFunction>> handlers = {
      {help, &RegexHandler::helpHandler},
      {ex, &RegexHandler::exitHandler},
      {echo, &RegexHandler::echoHandler},
      {run, &RegexHandler::runHandler},
      {open_table, &RegexHandler::openHandler},
      {close_table, &RegexHandler::closeHandler},
      {create_table, &RegexHandler::createTableHandler},
      {drop_table, &RegexHandler::dropTableHandler},
      {create_index, &RegexHandler::createIndexHandler},
      {drop_index, &RegexHandler::dropIndexHandler},
      {rename_table, &RegexHandler::renameTableHandler},
      {rename_column, &RegexHandler::renameColumnHandler},
      {insert_single, &RegexHandler::insertSingleHandler},
      {insert_multiple, &RegexHandler::insertFromFileHandler},
      {select_from, &RegexHandler::selectFromHandler},
      {select_from_where, &RegexHandler::selectFromWhereHandler},
      {select_attr_from, &RegexHandler::selectAttrFromHandler},
      {select_attr_from_where, &RegexHandler::selectAttrFromWhereHandler},
      {select_from_join, &RegexHandler::selectFromJoinHandler},
      {select_attr_from_join, &RegexHandler::selectAttrFromJoinHandler},
  };

 public:
  int handle(const string command) {
    for (auto iter = handlers.begin(); iter != handlers.end(); ++iter) {
      regex testCommand = iter->first;
      handlerFunction handler = iter->second;
      if (regex_match(command, testCommand)) {
        regex_search(command, m, testCommand);
        int status = (this->*handler)();
        if (status == SUCCESS || status == EXIT) {
          return status;
        }
        printErrorMsg(status);
        return FAILURE;
      }
    }
    cout << "Syntax Error" << endl;
    return FAILURE;
  }
} regexHandler;

int handleFrontend(int argc, char *argv[]) {
  // Taking Run Command as Command Line Argument(if provided)
  if (argc == 3 && strcmp(argv[1], "run") == 0) {
    string run_command("run ");
    run_command.append(argv[2]);
    int ret = regexHandler.handle(run_command);
    if (ret == EXIT) {
      return 0;
    }
  }
  char *buf;
  rl_bind_key('\t', rl_insert);
  while ((buf = readline("# ")) != nullptr) {
    if (strlen(buf) > 0) {
      add_history(buf);
    }
    int ret = regexHandler.handle(string(buf));
    free(buf);
    if (ret == EXIT) {
      return 0;
    }
  }
  return 0;
}

// get the operator constant corresponding to the string
int getOperator(string op_str) {
  int op = 0;
  if (op_str == "=")
    op = EQ;
  else if (op_str == "<")
    op = LT;
  else if (op_str == "<=")
    op = LE;
  else if (op_str == ">")
    op = GT;
  else if (op_str == ">=")
    op = GE;
  else if (op_str == "!=")
    op = NE;
  return op;
}

// truncates a given name string to ATTR_NAME sized char array
void nameToTruncatedArray(string nameString, char *nameArray) {
  string truncated = nameString.substr(0, ATTR_SIZE - 1);
  truncated.c_str();
  strcpy(nameArray, truncated.c_str());
  if (nameString.size() >= ATTR_SIZE) {
    printf("(warning: \'%s\' truncated to \'%s\')\n", nameString.c_str(), nameArray);
  }
}

void printErrorMsg(int error) {
  if (error == FAILURE)
    cout << "Error: Command Failed" << endl;
  else if (error == E_OUTOFBOUND)
    cout << "Error: Out of bound" << endl;
  else if (error == E_FREESLOT)
    cout << "Error: Free slot" << endl;
  else if (error == E_NOINDEX)
    cout << "Error: No index" << endl;
  else if (error == E_DISKFULL)
    cout << "Error: Insufficient space in Disk" << endl;
  else if (error == E_INVALIDBLOCK)
    cout << "Error: Invalid block" << endl;
  else if (error == E_RELNOTEXIST)
    cout << "Error: Relation does not exist" << endl;
  else if (error == E_RELEXIST)
    cout << "Error: Relation already exists" << endl;
  else if (error == E_ATTRNOTEXIST)
    cout << "Error: Attribute does not exist" << endl;
  else if (error == E_ATTREXIST)
    cout << "Error: Attribute already exists" << endl;
  else if (error == E_CACHEFULL)
    cout << "Error: Cache is full" << endl;
  else if (error == E_RELNOTOPEN)
    cout << "Error: Relation is not open" << endl;
  else if (error == E_RELNOTOPEN)
    cout << "Error: Relation is not open" << endl;
  else if (error == E_NATTRMISMATCH)
    cout << "Error: Mismatch in number of attributes" << endl;
  else if (error == E_DUPLICATEATTR)
    cout << "Error: Duplicate attributes found" << endl;
  else if (error == E_RELOPEN)
    cout << "Error: Relation is open" << endl;
  else if (error == E_ATTRTYPEMISMATCH)
    cout << "Error: Mismatch in attribute type" << endl;
  else if (error == E_INVALID)
    cout << "Error: Invalid index or argument" << endl;
  else if (error == E_MAXRELATIONS)
    cout << "Error: Maximum number of relations already present" << endl;
  else if (error == E_MAXATTRS)
    cout << "Error: Maximum number of attributes allowed for a relation is 125" << endl;
  else if (error == E_NOTPERMITTED)
    cout << "Error: This operation is not permitted" << endl;
  else if (error == E_INDEX_BLOCKS_RELEASED)
    cout << "Warning: Operation succeeded, but some indexes had to be dropped." << endl;
}

void printHelp() {
  printf("CREATE TABLE tablename(attr1_name attr1_type ,attr2_name attr2_type....); \n\t -create a relation with given attribute names\n \n");
  printf("DROP TABLE tablename;\n\t-delete the relation\n  \n");
  printf("OPEN TABLE tablename;\n\t-open the relation \n\n");
  printf("CLOSE TABLE tablename;\n\t-close the relation \n \n");
  printf("CREATE INDEX ON tablename.attributename;\n\t-create an index on a given attribute. \n\n");
  printf("DROP INDEX ON tablename.attributename; \n\t-delete the index. \n\n");
  printf("ALTER TABLE RENAME tablename TO new_tablename;\n\t-rename an existing relation to a given new name. \n\n");
  printf("ALTER TABLE RENAME tablename COLUMN column_name TO new_column_name;\n\t-rename an attribute of an existing relation.\n\n");
  printf("INSERT INTO tablename VALUES ( value1,value2,value3,... );\n\t-insert a single record into the given relation. \n\n");
  printf("INSERT INTO tablename VALUES FROM filepath; \n\t-insert multiple records from a csv file \n\n");
  printf("SELECT * FROM source_relation INTO target_relation; \n\t-creates a relation with the same attributes and records as of source relation\n\n");
  printf("SELECT Attribute1,Attribute2,....FROM source_relation INTO target_relation; \n\t-creates a relation with attributes specified and all records\n\n");
  printf("SELECT * FROM source_relation INTO target_relation WHERE attrname OP value; \n\t-retrieve records based on a condition and insert them into a target relation\n\n");
  printf("SELECT Attribute1,Attribute2,....FROM source_relation INTO target_relation;\n\t-creates a relation with the attributes specified and inserts those records which satisfy the given condition.\n\n");
  printf("SELECT * FROM source_relation1 JOIN source_relation2 INTO target_relation WHERE source_relation1.attribute1 = source_relation2.attribute2; \n\t-creates a new relation with by equi-join of both the source relations\n\n");
  printf("SELECT Attribute1,Attribute2,.. FROM source_relation1 JOIN source_relation2 INTO target_relation WHERE source_relation1.attribute1 = source_relation2.attribute2; \n\t-creates a new relation by equi-join of both the source relations with the attributes specified \n\n");
  printf("echo <any message> \n\t  -echo back the given string. \n\n");
  printf("run <filename> \n\t  -run commands from an input file in sequence. \n\n");
  printf("exit \n\t-Exit the interface\n");
}
