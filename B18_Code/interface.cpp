//
// Created by Gokul Sreekumar on 16/08/21.
//
// TODO : rename interface_new.cpp to interface.cpp
// TODO : review if we need to keep ';' at the end of commands
#include <iostream>
#include <fstream>
#include <string>
#include "define/constants.h"
#include "define/errors.h"
#include "interface.h"
#include "schema.h"
#include "Disk.h"
#include "OpenRelTable.h"
#include "block_access.h"
#include "algebra.h"
#include "external_fs_commands.h"

using namespace std;

void display_help();

void printErrorMsg(int ret);

vector<string> extract_tokens(string input_command);

void string_to_char_array(string x, char *a, int size);

int executeCommandsFromFile(string fileName);

bool checkValidCsvFile(string filename);

int getOperator(string op_str);

int getIndexOfFromToken(vector<string> command_tokens);

int getIndexOfWhereToken(vector<string> command_tokens);

string getAttrListStringFromCommand(string input_command, smatch m);

int select_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE]);

int select_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], char attribute[ATTR_SIZE],
                              int op, char value[ATTR_SIZE]);

int select_attr_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                             char attrs[][ATTR_SIZE]);

int select_attr_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                                   char attrs[][ATTR_SIZE], char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]);

void print16(char char_string_thing[ATTR_SIZE]);

/* TODO: RETURN 0 here means Success, return -1 (EXIT or FAILURE) means quit XFS,
 * I have done wherever i saw, check all that you added once again Jezzy
 */
int regexMatchAndExecute(const string input_command) {
    smatch m;
    if (regex_match(input_command, help)) {
        display_help();
    } else if (regex_match(input_command, ex)) {
        return EXIT;
    } else if (regex_match(input_command, run)) {
        regex_search(input_command, m, run);
        string file_name = m[2];
        if (executeCommandsFromFile(file_name) == EXIT) {
            return EXIT;
        }
    } else if (regex_match(input_command, fdisk)) {
        Disk::createDisk();
        Disk::formatDisk();
        add_disk_metainfo();
        cout << "Disk formatted" << endl;
    } else if (regex_match(input_command, dump_rel)) {
        dump_relcat();
        cout << "Dumped relation catalog to $HOME/NITCBase/xfs-interface/relation_catalog" << endl;
    } else if (regex_match(input_command, dump_attr)) {
        dump_attrcat();
        cout << "Dumped attribute catalog to $HOME/NITCBase/xfs-interface/attribute_catalog" << endl;
    } else if (regex_match(input_command, dump_bmap)) {
        dumpBlockAllocationMap();
        cout << "Dumped block allocation map to $HOME/NITCBase/xfs-interface/block_allocation_map" << endl;
    } else if (regex_match(input_command, list_all)) {
        ls();
    } else if (regex_match(input_command, imp)) {
        string Filepath;
        string p = "./Files/";
        regex_search(input_command, m, imp);
        Filepath = m[2];
        p = p + Filepath;
        char f[p.length() + 1];
        string_to_char_array(p, f, p.length() + 1);
        FILE *file = fopen(f, "r");
        if (!file) {
            cout << "Invalid file path or file does not exist" << endl;
            return FAILURE;
        }
        fclose(file);
        string Filename = m[2];
        bool r = checkValidCsvFile(Filename);
        if (!r) {
            cout << "Command Failed" << endl;
            return FAILURE;
        }
        int ret = importRelation(f);
        if (ret == SUCCESS)
            cout << "Imported successfully" << endl;
        else {
            printErrorMsg(ret);
            return FAILURE;
        }
    } else if (regex_match(input_command, exp)) {
        regex_search(input_command, m, exp);
        string tableName = m[2];

        string filePath = m[3];
        filePath = "./Files/" + filePath;

        char relname[ATTR_SIZE];
        string_to_char_array(tableName, relname, ATTR_SIZE - 1);
        char fileName[filePath.length() + 1];
        string_to_char_array(filePath, fileName, filePath.length() + 1);

        int ret = exportRelation(relname, fileName);

        if (ret == SUCCESS)
            cout << "Exported successfully into: " << filePath << endl;
        else {
            cout << "Export Command Failed" << endl;
            return FAILURE;
        }

    } else if (regex_match(input_command, open_table)) {

        regex_search(input_command, m, open_table);
        string tablename = m[3];
        char relname[ATTR_SIZE];
        string_to_char_array(tablename, relname, 15);

        int ret = openRel(relname);
        if (ret >= 0 && ret <= 11) {
            cout << "Relation opened successfully\n";
        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, close_table)) {

        regex_search(input_command, m, close_table);
        string tablename = m[3];
        char relname[ATTR_SIZE];
        string_to_char_array(tablename, relname, 15);

        int ret = closeRel(relname);
        if (ret == SUCCESS) {
            cout << "Relation Closed Successfully\n";
        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, create_table)) {
        regex_search(input_command, m, create_table);

        string tablename = m[3];
        char relname[ATTR_SIZE];
        string_to_char_array(tablename, relname, ATTR_SIZE - 1);

        regex_search(input_command, m, temp);
        string attrs = m[0];
        vector<string> words = extract_tokens(attrs);

        int no_attrs = words.size() / 2;
        char attribute[no_attrs][ATTR_SIZE];
        int type_attr[no_attrs];

        for (int i = 0, k = 0; i < no_attrs; i++, k += 2) {
            string_to_char_array(words[k], attribute[i], ATTR_SIZE - 1);
            if (words[k + 1] == "STR")
                type_attr[i] = STRING;
            else if (words[k + 1] == "NUM")
                type_attr[i] = NUMBER;
        }

        int ret = createRel(relname, no_attrs, attribute, type_attr);
        if (ret == SUCCESS) {
            cout << "Relation created successfully" << endl;
        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, drop_table)) {
        regex_search(input_command, m, drop_table);
        string tablename = m[3];
        char relname[ATTR_SIZE];
        string_to_char_array(tablename, relname, ATTR_SIZE - 1);
        if (strcmp(relname, "RELATIONCAT") == 0 || strcmp(relname, "ATTRIBUTECAT") == 0)
            cout << "Cannot delete Relation Catalog or Attribute Catalog" << endl;
        int ret = deleteRel(relname);
        if (ret == SUCCESS)
            cout << "Relation deleted successfully" << endl;
        else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, rename_table)) {

        regex_search(input_command, m, rename_table);
        string oldTableName = m[4];
        string newTableName = m[6];
        char old_relation_name[ATTR_SIZE];
        char new_relation_name[ATTR_SIZE];
        string_to_char_array(oldTableName, old_relation_name, 15);
        string_to_char_array(newTableName, new_relation_name, 15);

        int ret = renameRel(old_relation_name, new_relation_name);

        if (ret == SUCCESS) {
            cout << "Renamed Relation Successfully" << endl;
        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, rename_column)) {

        regex_search(input_command, m, rename_column);
        string tablename = m[4];
        string oldcolumnname = m[6];
        string newcolumnname = m[8];
        char relname[ATTR_SIZE];
        char old_col[ATTR_SIZE];
        char new_col[ATTR_SIZE];
        string_to_char_array(tablename, relname, 15);
        string_to_char_array(oldcolumnname, old_col, 15);
        string_to_char_array(newcolumnname, new_col, 15);

        int ret = renameAtrribute(relname, old_col, new_col);

        if (ret == SUCCESS) {
            cout << "Renamed Attribute Successfully" << endl;
        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, insert_single)) {

        regex_search(input_command, m, insert_single);
        string table_name = m[3];
        char rel_name[ATTR_SIZE];
        string_to_char_array(table_name, rel_name, 15);
        regex_search(input_command, m, temp);
        string attrs = m[0];
        vector<string> words = extract_tokens(attrs);

        int retValue = insert(words, rel_name);

        if (retValue == SUCCESS) {
            cout << "Inserted successfully" << endl;
        } else {
            printErrorMsg(retValue);
            return FAILURE;
        }
    } else if (regex_match(input_command, insert_multiple)) {
        regex_search(input_command, m, insert_multiple);
        string tablename = m[3];
        char relname[16];
        string p = "./Files/";
        string_to_char_array(tablename, relname, 15);
        string t = m[6];
        p = p + t;
        char Filepath[p.length() + 1];
        string_to_char_array(p, Filepath, p.length() + 1);
        FILE *file = fopen(Filepath, "r");
        cout << Filepath << endl;
        if (!file) {
            cout << "Invalid file path or file does not exist" << endl;
            return FAILURE;
        }
        fclose(file);
        int retValue = insert(relname, Filepath);
        if (retValue == SUCCESS) {
            cout << "Inserted successfully" << endl;
        } else {
            printErrorMsg(retValue);
            return FAILURE;
        }

    } else if (regex_match(input_command, select_from)) {
        regex_search(input_command, m, select_from);
        string sourceRelName_str = m[4];
        string targetRelName_str = m[6];

        char sourceRelName[ATTR_SIZE];
        char targetRelName[ATTR_SIZE];

        string_to_char_array(sourceRelName_str, sourceRelName, ATTR_SIZE - 1);
        string_to_char_array(targetRelName_str, targetRelName, ATTR_SIZE - 1);

        return select_from_handler(sourceRelName, targetRelName);

    } else if (regex_match(input_command, select_from_where)) {
        regex_search(input_command, m, select_from_where);
        string sourceRel_str = m[4];
        string targetRel_str = m[6];
        string attribute_str = m[8];
        string op_str = m[9];
        string value_str = m[10];

        char sourceRelName[ATTR_SIZE];
        char targetRelName[ATTR_SIZE];
        char attribute[ATTR_SIZE];
        char value[ATTR_SIZE];
        string_to_char_array(sourceRel_str, sourceRelName, ATTR_SIZE - 1);
        string_to_char_array(targetRel_str, targetRelName, ATTR_SIZE - 1);
        string_to_char_array(attribute_str, attribute, ATTR_SIZE - 1);
        string_to_char_array(value_str, value, ATTR_SIZE - 1);

        int op = getOperator(op_str);

        // LOGGING DEBUG //
        print16(sourceRelName);
        print16(targetRelName);
        print16(attribute);
        print16(value);
        cout << op_str << endl;
        /**********/

        return select_from_where_handler(sourceRelName, targetRelName, attribute, op, value);

    } else if (regex_match(input_command, select_attr_from)) {
        regex_search(input_command, m, select_attr_from);
        vector<string> command_tokens;
        for (auto token: m)
            command_tokens.push_back(token);
        int index_of_from = getIndexOfFromToken(command_tokens);

        string sourceRel_str = command_tokens[index_of_from + 1];
        string targetRel_str = command_tokens[index_of_from + 3];

        char sourceRelName[ATTR_SIZE];
        char targetRelName[ATTR_SIZE];
        string_to_char_array(sourceRel_str, sourceRelName, ATTR_SIZE - 1);
        string_to_char_array(targetRel_str, targetRelName, ATTR_SIZE - 1);

        /* Get the attribute list string from the input command */
        string attribute_list = getAttrListStringFromCommand(input_command, m);
        vector<string> attr_tokens = extract_tokens(attribute_list);

        int attr_count = attr_tokens.size();
        char attr_list[attr_count][ATTR_SIZE];
        for (int attr_no = 0; attr_no < attr_count; attr_no++) {
            string_to_char_array(attr_tokens[attr_no], attr_list[attr_no], ATTR_SIZE - 1);
        }

        // LOGGING DEBUG //
        print16(sourceRelName);
        print16(targetRelName);
        cout << attr_count << endl;
        cout << "DEBUG | attrlist:" << endl;
        for (auto i = 0; i < attr_count; i++) {
            print16(attr_list[i]);
        }
        /**********/

        return select_attr_from_handler(sourceRelName, targetRelName, attr_count, attr_list);

    } else if ((regex_match(input_command, select_attr_from_where))) {
        regex_search(input_command, m, select_attr_from_where);
        vector<string> command_tokens;
        for (auto token: m)
            command_tokens.push_back(token);

        int index_of_from = getIndexOfFromToken(command_tokens);
        int index_of_where = getIndexOfWhereToken(command_tokens);

        string sourceRel_str = command_tokens[index_of_from + 1];
        string targetRel_str = command_tokens[index_of_from + 3];
        string attribute_str = command_tokens[index_of_where + 1];
        string op_str = command_tokens[index_of_where + 2];
        string value_str = command_tokens[index_of_where + 3];

        char sourceRelName[16];
        char targetRelName[16];
        char attribute[16];
        char value[16];
        int op = getOperator(op_str);

        string_to_char_array(attribute_str, attribute, 15);
        string_to_char_array(value_str, value, 15);
        string_to_char_array(sourceRel_str, sourceRelName, 15);
        string_to_char_array(targetRel_str, targetRelName, 15);

        string attribute_list = getAttrListStringFromCommand(input_command, m);
        vector<string> attr_tokens = extract_tokens(attribute_list);

        int attr_count = attr_tokens.size();
        char attr_list[attr_count][ATTR_SIZE];
        for (int attr_no = 0; attr_no < attr_count; attr_no++) {
            string_to_char_array(attr_tokens[attr_no], attr_list[attr_no], ATTR_SIZE - 1);
        }

        // LOGGING DEBUG //
        print16(sourceRelName);
        print16(targetRelName);
        print16(attribute);
        print16(value);
        cout << op_str << endl;
        cout << attr_count << endl;
        cout << "DEBUG | attrlist:" << endl;
        for (auto i = 0; i < attr_count; i++) {
            print16(attr_list[i]);
        }
        /**********/

        return select_attr_from_where_handler(sourceRelName, targetRelName, attr_count, attr_list, attribute, op,
                                              value);

    } else if (regex_match(input_command, select_from_join)) {

        regex_search(input_command, m, select_from_join);

        // m[4] and m[10] should be equal ( src_rel1)
        // m[6] and m[102 should be equal ( src_rel2)
        if (m[4] != m[10] || m[6] != m[12]) {
            cout << "Syntax Error" << endl;
            return FAILURE;
        }
        char src_rel1[ATTR_SIZE];
        char src_rel2[ATTR_SIZE];
        char tar_rel[ATTR_SIZE];
        char attr1[ATTR_SIZE];
        char attr2[ATTR_SIZE];

        string_to_char_array(m[4], src_rel1, 15);
        string_to_char_array(m[6], src_rel2, 15);
        string_to_char_array(m[8], tar_rel, 15);
        string_to_char_array(m[11], attr1, 15);
        string_to_char_array(m[13], attr2, 15);

        int ret = join(src_rel1, src_rel2, tar_rel, attr1, attr2);
        if (ret == SUCCESS) {
            cout << "Join successful" << endl;
        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else if (regex_match(input_command, select_attr_from_join)) {

        regex_search(input_command, m, select_attr_from_join);

        vector<string> tokens;
        for (auto token: m)
            tokens.push_back(token);

        int refIndex;
        for (refIndex = 0; refIndex < tokens.size(); refIndex++) {
            if (tokens[refIndex] == "from" || tokens[refIndex] == "FROM")
                break;
        }

        char src_rel1[ATTR_SIZE];
        char src_rel2[ATTR_SIZE];
        char tar_rel[ATTR_SIZE];
        char attr1[ATTR_SIZE];
        char attr2[ATTR_SIZE];

        string_to_char_array(tokens[refIndex + 1], src_rel1, 15);
        string_to_char_array(tokens[refIndex + 3], src_rel2, 15);
        string_to_char_array(tokens[refIndex + 5], tar_rel, 15);
        string_to_char_array(tokens[refIndex + 8], attr1, 15);
        string_to_char_array(tokens[refIndex + 10], attr2, 15);

        int ret = join(src_rel1, src_rel2, TEMP, attr1, attr2);

        int relId;
        if (ret == SUCCESS) {

            relId = OpenRelations::openRelation(TEMP);
            if (!(relId >= 0 && relId < MAXOPEN)) {
                cout << "openRel Failed" << endl;
            }

            int attrListPos = 1;
            string attribute_list;
            string inputCommand = input_command;
            while (regex_search(inputCommand, m, attrlist)) {
                if (attrListPos == 2)
                    attribute_list = m.str(0);
                attrListPos++;
                // suffix to find the rest of the string.
                inputCommand = m.suffix().str();
            }

            vector<string> words = extract_tokens(attribute_list);
            int attrCount = words.size();
            char attrs[attrCount][ATTR_SIZE];
            for (int i = 0; i < attrCount; i++) {
                string_to_char_array(words[i], attrs[i], 15);
            }

//			int ret_project = project(TEMP, tar_rel, attrCount, attrs);
//
//			if (ret_project == SUCCESS) {
//				cout << "Join successful" << endl;
//				OpenRelations::closeRelation(relId);
//				deleteRel(TEMP);
//			} else {
//				printErrorMsg(ret_project);
//				return FAILURE;
//			}

        } else {
            printErrorMsg(ret);
            return FAILURE;
        }

    } else {
        cout << "Syntax Error" << endl;
        return FAILURE;
    }
    return SUCCESS;
}

int main() {
    // Initializing Open Relation Table
    OpenRelations::initializeOpenRelationTable();

    while (true) {
        cout << "# ";
        string input_command;
        smatch m;
        getline(cin, input_command);
        int ret = regexMatchAndExecute(input_command);
        if (ret == EXIT) {
            return 0;
        }
    }
}

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

int getIndexOfFromToken(vector<string> command_tokens) {
    int index_of_from;
    for (index_of_from = 0; index_of_from < command_tokens.size(); index_of_from++) {
        if (command_tokens[index_of_from] == "from" || command_tokens[index_of_from] == "FROM")
            break;
    }
    return index_of_from;
}

int getIndexOfWhereToken(vector<string> command_tokens) {
    int index_of_where;
    for (index_of_where = 0; index_of_where < command_tokens.size(); index_of_where++) {
        if (command_tokens[index_of_where] == "where" || command_tokens[index_of_where] == "WHERE")
            break;
    }
    return index_of_where;
}

string getAttrListStringFromCommand(const string input_command, smatch m) {
    int attrListPos = 1;
    string attribute_list;
    string inputCommand = input_command;
    /*
     * At second position of the input attribute list will be obtained
     *      SELECT AttrList FROM  ...
     *      1      2        3     ...
     */
    while (regex_search(inputCommand, m, attrlist)) {
        if (attrListPos == 2)
            attribute_list = m.str(0);
        attrListPos++;
        // suffix to find the rest of the string.
        inputCommand = m.suffix().str();
    }

    return attribute_list;
}

int select_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE]) {
    /* Check if the relation is Open */
    int src_relid = OpenRelations::getRelationId(sourceRelName);
    if (src_relid == E_RELNOTOPEN) {
        cout << "Source relation not open" << endl;
        return FAILURE;
    }

    /* Get the relation catalog entry for the source relation */
    Attribute relcatEntry[6];
    if (getRelCatEntry(src_relid, relcatEntry) != SUCCESS) {
        cout << "Command Failed: Could not get the relation catalogue entry" << endl;
        return FAILURE;
    }
    /*
     * Array for the Attributes of the Target Relation
     *  - target relation has same number of attributes as source relation (nAttrs)
     * Search the Attribute Catalog to find all the attributes belonging to Source Relation
     *  - and store them in this array (for projecting)
     */
    int nAttrs = (int) relcatEntry[1].nval;
    char targetAttrs[nAttrs][ATTR_SIZE];
    Attribute rec_Attrcat[6];
    int recBlock_Attrcat = ATTRCAT_BLOCK; // Block Number
    HeadInfo headInfo;
    while (recBlock_Attrcat != -1) {
        headInfo = getHeader(recBlock_Attrcat);

        unsigned char slotmap[headInfo.numSlots];
        getSlotmap(slotmap, recBlock_Attrcat);

        for (int slotNum = 0; slotNum < SLOTMAP_SIZE_RELCAT_ATTRCAT; slotNum++) {
            if (slotmap[slotNum] != SLOT_UNOCCUPIED) {
                getRecord(rec_Attrcat, recBlock_Attrcat, slotNum);
                if (strcmp(rec_Attrcat[0].sval, sourceRelName) == 0) {
                    // Copy the attribute name to the attribute offset index
                    strcpy(targetAttrs[(int) rec_Attrcat[5].nval], rec_Attrcat[1].sval);
                }
            }
        }
        recBlock_Attrcat = headInfo.rblock;
    }

    int ret = project(sourceRelName, targetRelName, nAttrs, targetAttrs);
    if (ret == SUCCESS) {
        cout << "Selected successfully, result in relation: ";
        print16(targetRelName);
        return SUCCESS;
    } else {
        printErrorMsg(ret);
        return FAILURE;
    }
}

int select_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], char attribute[ATTR_SIZE],
                              int op, char value[ATTR_SIZE]) {
    int ret = select(sourceRelName, targetRelName, attribute, op, value);
    if (ret == SUCCESS) {
        cout << "Selected successfully, result in relation: ";
        print16(targetRelName);
    } else {
        printErrorMsg(ret);
    }

    return SUCCESS;
}

int select_attr_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                             char attrs[][ATTR_SIZE]) {
    int ret = project(sourceRelName, targetRelName, attr_count, attrs);
    if (ret == SUCCESS) {
        cout << "Selected successfully, result in relation: ";
        print16(targetRelName);
    } else {
        printErrorMsg(ret);
    }
    return SUCCESS;
}

int select_attr_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                                   char attrs[][ATTR_SIZE], char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
    int ret = select(sourceRelName, TEMP, attribute, op, value);
    if (ret == SUCCESS) {
        int relid = openRel(TEMP);
        if (relid != E_RELNOTEXIST && relid != E_CACHEFULL) {
            int ret_project = project(TEMP, targetRelName, attr_count, attrs);
            if (ret_project == SUCCESS) {
                cout << "Selected successfully, result in relation: ";
                print16(targetRelName);
                closeRel(relid);
                deleteRel(TEMP);
            } else {
                closeRel(relid);
                deleteRel(TEMP);
                printErrorMsg(ret_project);
                return FAILURE;
            }
        } else {
            printErrorMsg(relid);
            return FAILURE;
        }
        return SUCCESS;
    } else {
        return FAILURE;
    }
}

void print16(char char_string_thing[ATTR_SIZE]) {
    for (int i = 0; i < ATTR_SIZE; i++) {
        if (char_string_thing[i] == '\0') {
            break;
        }
        cout << char_string_thing[i];
    }
    cout << endl;
}

// TODO: What to do when one line Fails - EXIT?
int executeCommandsFromFile(const string fileName) {
    const string filePath = "./Files/";
    fstream commandsFile;
    commandsFile.open(filePath + fileName, ios::in);
    string command;
    vector<string> commands;
    if (commandsFile.is_open()) {
        while (getline(commandsFile, command)) {
            commands.push_back(command);
        }
    } else {
        cout << "The file " << fileName << " does not exist\n";
    }
    int lineNumber = 1;
    for (auto command: commands) {
        int ret = regexMatchAndExecute(command);
        if (ret == EXIT) {
            return EXIT;
        } else if (ret == FAILURE) {
            cout << "At line number " << lineNumber << endl;
            break;
        }
        lineNumber++;
    }
    return SUCCESS;
}

void display_help() {
    printf("fdisk \n\t -Format disk \n\n");
    printf("import <filename> \n\t -loads relations from the UNIX filesystem to the XFS disk. \n\n");
    printf("export <tablename> <filename> \n\t -export a relation from XFS disk to UNIX file system. \n\n");
    printf("ls \n\t  -list the names of all relations in the xfs disk. \n\n");
    printf("dump bmap \n\t-dump the contents of the block allocation map.\n\n");
    printf("dump relcat \n\t-copy the contents of relation catalog to relationcatalog.txt\n \n");
    printf("dump attrcat \n\t-copy the contents of attribute catalog to an attributecatalog.txt. \n\n");
    printf("CREATE TABLE tablename(attr1_name attr1_type ,attr2_name attr2_type....); \n\t -create a relation with given attribute names\n \n");
    printf("DROP TABLE tablename ;\n\t-delete the relation\n  \n");
    printf("OPEN TABLE tablename ;\n\t-open the relation \n\n");
    printf("CLOSE TABLE tablename ;\n\t-close the relation \n \n");
    printf("CREATE INDEX ON tablename.attributename;\n\t-create an index on a given attribute. \n\n");
    printf(" DROP INDEX ON tablename.attributename ; \n\t-delete the index. \n\n");
    printf("ALTER TABLE RENAME tablename TO new_tablename ;\n\t-rename an existing relation to a given new name. \n\n");
    printf("ALTER TABLE RENAME tablename COLUMN column_name TO new_column_name ;\n\t-rename an attribute of an existing relation.\n\n");
    printf("INSERT INTO tablename VALUES ( value1,value2,value3,... );\n\t-insert a single record into the given relation. \n\n");
    printf("INSERT INTO tablename VALUES FROM filepath; \n\t-insert multiple records from a csv file \n\n");
    printf("SELECT * FROM source_relation INTO target_relation ; \n\t-creates a relation with the same attributes and records as of source relation\n\n");
    printf("SELECT Attribute1,Attribute2,....FROM source_relation INTO target_relation ; \n\t-creates a relation with attributes specified and all records\n\n");
    printf("SELECT * FROM source_relation INTO target_relation WHERE attrname OP value; \n\t-retrieve records based on a condition and insert them into a target relation\n\n");
    printf("SELECT Attribute1,Attribute2,....FROM source_relation INTO target_relation ;\n\t-creates a relation with the attributes specified and inserts those records which satisfy the given condition.\n\n");
    printf("SELECT * FROM source_relation1 JOIN source_relation2 INTO target_relation WHERE source_relation1.attribute1 = source_relation2.attribute2 ; \n\t-creates a new relation with by equi-join of both the source relations\n\n");
    printf("SELECT Attribute1,Attribute2,.. FROM source_relation1 JOIN source_relation2 INTO target_relation WHERE source_relation1.attribute1 = source_relation2.attribute2 ; \n\t-creates a new relation by equi-join of both the source relations with the attributes specified \n\n");
    printf("exit \n\t-Exit the interface\n");
    return;
}

void printErrorMsg(int ret) {
    if (ret == FAILURE)
        cout << "Error: Command Failed" << endl;
    else if (ret == E_OUTOFBOUND)
        cout << "Error: Out of bound" << endl;
    else if (ret == E_FREESLOT)
        cout << "Error: Free slot" << endl;
    else if (ret == E_NOINDEX)
        cout << "Error: No index" << endl;
    else if (ret == E_DISKFULL)
        cout << "Error: Insufficient space in Disk" << endl;
    else if (ret == E_INVALIDBLOCK)
        cout << "Error: Invalid block" << endl;
    else if (ret == E_RELNOTEXIST)
        cout << "Error: Relation does not exist" << endl;
    else if (ret == E_RELEXIST)
        cout << "Error: Relation already exists" << endl;
    else if (ret == E_ATTRNOTEXIST)
        cout << "Error: Attribute does not exist" << endl;
    else if (ret == E_ATTREXIST)
        cout << "Error: Attribute already exists" << endl;
    else if (ret == E_CACHEFULL)
        cout << "Error: Cache is full" << endl;
    else if (ret == E_RELNOTOPEN)
        cout << "Error: Relation is not open" << endl;
    else if (ret == E_NOTOPEN)
        cout << "Error: Relation is not open" << endl;
    else if (ret == E_NATTRMISMATCH)
        cout << "Error: Mismatch in number of attributes" << endl;
    else if (ret == E_DUPLICATEATTR)
        cout << "Error: Duplicate attributes found" << endl;
    else if (ret == E_RELOPEN)
        cout << "Error: Relation is open" << endl;
    else if (ret == E_ATTRTYPEMISMATCH)
        cout << "Error: Mismatch in attribute type" << endl;
    else if (ret == E_INVALID)
        cout << "Error: Invalid index or argument" << endl;
}

vector<string> extract_tokens(string input_command) {
    // tokenize with whitespace and brackets as delimiter
    vector<string> tokens;
    string token;
    for (int i = 0; i < input_command.length(); i++) {
        if (input_command[i] == '(' || input_command[i] == ')') {
            if (!token.empty()) {
                tokens.push_back(token);
            }
            token = "";
        } else if (input_command[i] == ',') {
            if (!token.empty()) {
                tokens.push_back(token);
            }
            token = "";
        } else if (input_command[i] == ' ' || input_command[i] == ';') {
            if (!token.empty()) {
                tokens.push_back(token);
            }
            token = "";
        } else {
            token += input_command[i];
        }
    }
    if (!token.empty())
        tokens.push_back(token);

    return tokens;
}

void string_to_char_array(string x, char *a, int size) {
    // Reducing size of string to the size provided
    int i;
    if (size == ATTR_SIZE - 1) {
        for (i = 0; i < x.size() && i < ATTR_SIZE - 1; i++)
            a[i] = x[i];
        a[i] = '\0';
    } else {
        for (i = 0; i < size; i++) {
            a[i] = x[i];
        }
        a[i] = '\0';
    }
}

bool checkValidCsvFile(const string filename) {
    int pos1 = filename.rfind('.');
    if (filename.substr(pos1 + 1, filename.length()) == "csv") {
        string file_name = filename.substr(0, pos1);
        if (file_name.length() > ATTR_SIZE - 1) {
            cout << " File name should have at most 15 characters\n";
            return false;
        } else
            return true;
    } else
        return false;
}