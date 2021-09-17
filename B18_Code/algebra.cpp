//
// Created by Jessiya Joy on 12/09/21.
//
#include <iostream>
#include <vector>
#include "define/constants.h"
#include "define/errors.h"
#include "disk_structures.h"
#include "algebra.h"
#include "block_access.h"
#include "OpenRelTable.h"
#include "schema.h"

using namespace std;

int checkAttrTypeOfValue(char *data);

int getNumberOfAttrsForRelation(int relationId);

void getAttrTypesForRelation(int relId, int numAttrs, int attrTypes[numAttrs]);

int constructRecordFromAttrsArray(int numAttrs, Attribute record[numAttrs], char recordArray[numAttrs][ATTR_SIZE],
                                  int attrTypes[numAttrs]);


int project(char srcrel[ATTR_SIZE], char targetrel[ATTR_SIZE], int tar_nAttrs, char tar_attrs[][ATTR_SIZE]) {
    int ret;
    /* Check source relation is open */
    int srcrelid = OpenRelations::getRelationId(srcrel);
    if (srcrelid == E_RELNOTOPEN) {
        // src relation not open
        return E_RELNOTOPEN;
    }

    Attribute srcrelcat[NO_OF_ATTRS_RELCAT_ATTRCAT];
    ret = getRelCatEntry(srcrelid, srcrelcat);
    int nAttrs = (int) srcrelcat[1].nval;

    int attr_offset[tar_nAttrs];
    int attr_type[tar_nAttrs];
    int attr_no;
    /* Check if attributes of target rel are present in source rel. */
    for (attr_no = 0; attr_no < tar_nAttrs; attr_no++) {
        Attribute attrcat[NO_OF_ATTRS_RELCAT_ATTRCAT];
        ret = getAttrCatEntry(srcrelid, tar_attrs[attr_no], attrcat);
        if (ret != SUCCESS) {
            return ret;
        }
        attr_offset[attr_no] = (int) attrcat[5].nval;
        attr_type[attr_no] = (int) attrcat[2].nval;
    }

    ret = createRel(targetrel, tar_nAttrs, tar_attrs, attr_type);
    if (ret != SUCCESS) {
        /* Unsuccessful creation of target relation */
        return ret;
    }

    /* Open the target relation */
    // TODO: MOVE THIS UP - DESIGN CHANGE
    int targetrelid = openRel(targetrel);
    if (targetrelid == E_CACHEFULL) {
        ba_delete(targetrel);
        return E_CACHEFULL;
    }

    /*
     * Get record by record from the source relation
     *  and take the projected attributes alone for the record
     */
    recId prev_recid;
    prev_recid.block = -1;
    prev_recid.slot = -1;
    while (true) {
        Attribute rec[nAttrs];
        char attr[ATTR_SIZE];
        Attribute val;
        strcpy(val.sval, "PRJCT");
        strcpy(attr, "PRJCT");

        ret = ba_search(srcrelid, rec, attr, val, PRJCT, &prev_recid);
        if (ret == SUCCESS) {
            Attribute proj_rec[tar_nAttrs];
            for (attr_no = 0; attr_no < tar_nAttrs; attr_no++) {
                proj_rec[attr_no] = rec[attr_offset[attr_no]];
            }
            ret = ba_insert(targetrelid, proj_rec);
            if (ret != SUCCESS) {
                // unable to insert into target relation
                closeRel(targetrelid);
                ba_delete(targetrel);
                return ret;
            }
        } else
            break;

    }

    closeRel(targetrelid);
    return SUCCESS;
}



int insert(vector<string> attributeTokens, char *table_name) {

    // check if relation is open
    int relId = OpenRelations::getRelationId(table_name);
    if (relId == E_RELNOTOPEN) {
        return relId;
    }

    // get #attributes from relation catalog entry
    int numAttrs = getNumberOfAttrsForRelation(relId);
    if (numAttrs != attributeTokens.size())
        return E_NATTRMISMATCH;

    // get attribute types from attribute catalog entry
    int attrTypes[numAttrs];
    getAttrTypesForRelation(relId, numAttrs, attrTypes);

    // for each attribute, convert string vector to char array
    char recordArray[numAttrs][ATTR_SIZE];
    for (int i = 0; i < numAttrs; i++) {
        string attrValue = attributeTokens[i];
        char tempAttribute[ATTR_SIZE];
        int j;
        for (j = 0; j < 15 && j < attrValue.size(); j++) {
            tempAttribute[j] = attrValue[j];
        }
        tempAttribute[j] = '\0';
        strcpy(recordArray[i], tempAttribute);
    }

    // Construct a record ( array of type Attribute ) from previous character array
    // Perform type checking for number types
    Attribute record[numAttrs];
    int retValue = constructRecordFromAttrsArray(numAttrs, record, recordArray, attrTypes);
    if (retValue == E_ATTRTYPEMISMATCH)
        return E_ATTRTYPEMISMATCH;

    retValue = ba_insert(relId, record);
    if (retValue == SUCCESS) {
        return SUCCESS;
    } else {
        return FAILURE;
    }
}

int insert(char relName[16], char *fileName) {

	char ch;

	// check if relation is open
	int relId = OpenRelations::getRelationId(relName);
	if (relId == E_RELNOTOPEN) {
		return relId;
	}

	// get #attributes from relation catalog entry
	int numAttrs = getNumberOfAttrsForRelation(relId);

	// get attribute types from attribute catalog entry
	int attrTypes[numAttrs];
	getAttrTypesForRelation(relId, numAttrs, attrTypes);


	char *record = (char *) malloc(sizeof(char));
	int len = 1;

	FILE *file = fopen(fileName, "r");
	while (1) {
		ch = fgetc(file);
		if (ch == EOF)
			break;
		while (ch == ' ' || ch == '\t' || ch == '\n') {
			ch = fgetc(file);
		}
		if (ch == EOF)
			break;
		len = 1;
		int fieldsCount = 0;
		char oldch = ',';
		while ((ch != '\n') && (ch != EOF)) {
			if (ch == ',')
				fieldsCount++;

			if (oldch == ch && ch == ',') {
				cout << "Null values not allowed\n";
				return FAILURE;
			}

			record[len - 1] = ch;
			len++;
			record = (char *) realloc(record, (len) * sizeof(char));
			oldch = ch;
			ch = fgetc(file);

		}

		if (oldch == ',' && ch != '\n') {
			cout << "Null values not allowed in attribute values\n";
			return FAILURE;
		}

		if (numAttrs != fieldsCount + 1 && ch != '\n') {
			std::cout << "Mismatch in number of attributes\n";
			return FAILURE;
		}
		record[len - 1] = '\0';
		int i = 0;
		//record contains each record in the file (seperated by commas)
		char recordArray[numAttrs][ATTR_SIZE];
		int j = 0;

		while (j < numAttrs) {
			int k = 0;

			while (((record[i] != ',') && (record[i] != '\0')) && (k < 15)) {
				recordArray[j][k++] = record[i++];
			}
			if (k == 15) {
				while (record[i] != ',')
					i++;
			}
			i++;
			recordArray[j][k] = '\0';
			j++;
		}

		// Construct a record ( array of type Attribute ) from previous character array
		// Perform type checking for number types
		Attribute record[numAttrs];
		int retValue = constructRecordFromAttrsArray(numAttrs, record, recordArray, attrTypes);
		if (retValue == E_ATTRTYPEMISMATCH)
			return E_ATTRTYPEMISMATCH;

		retValue = ba_insert(relId, record);
		if (retValue != SUCCESS) {
			return FAILURE;
		}

		if (ch == EOF)
			break;
	}

	fclose(file);
	return SUCCESS;
}

int join(char srcrel1[ATTR_SIZE], char srcrel2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attr1[ATTR_SIZE], char attr2[ATTR_SIZE]) {

	// IF ANY SOURCE RELATION IS NOT OPEN, return E_RELNOTOPEN
	int srcRelId1 = OpenRelations::getRelationId(srcrel1);
	if (srcRelId1 == E_RELNOTOPEN)
		return srcRelId1;
	int srcRelId2 = OpenRelations::getRelationId(srcrel2);
	if (srcRelId2 == E_RELNOTOPEN)
		return srcRelId2;

	// GET RELATION CATALOG ENTRIES OF JOIN ATTRIBUTES OF SRC RELATIONS
	Attribute attrcat_entry1[6];
	int flag1 = getAttrCatEntry(srcRelId1, attr1, attrcat_entry1);
	Attribute attrcat_entry2[6];
	int flag2 = getAttrCatEntry(srcRelId2, attr2, attrcat_entry2);

	// if attr1 is not present in rel1 or attr2 not present in rel2 (failure of call to Openreltable) return E_ATTRNOTEXIST.
	if (flag1 != SUCCESS || flag2 != SUCCESS)
		return E_ATTRNOTEXIST;

	// if attr1 and attr2 are of different types return E_ATTRTYPEMISMATCH
	if (attrcat_entry1[2].nval != attrcat_entry2[2].nval)
		return E_ATTRTYPEMISMATCH;

	// GET RELATION CATALOG ENTRIES OF SRC RELATIONS
	union Attribute relcat_entry1[NO_OF_ATTRS_RELCAT_ATTRCAT];
	getRelCatEntry(srcRelId1, relcat_entry1);
	int nAttrs1 = static_cast<int>(relcat_entry1[1].nval);

	union Attribute relcat_entry2[NO_OF_ATTRS_RELCAT_ATTRCAT];
	getRelCatEntry(srcRelId2, relcat_entry2);
	int nAttrs2 = static_cast<int>(relcat_entry2[1].nval);

	/*
	 * TODO :
	 * Once B+ tree layer is implemented, ensure index exists for at least one attribute
	 */

	/* TARGET RELATION -
	 * targetRelAttrNames : array of attribute names
	 * targetRelAttrTypes : array of attribute types
	 */

	char targetRelAttrNames[nAttrs1 + nAttrs2 - 1][ATTR_SIZE];
	int targetRelAttrTypes[nAttrs1 + nAttrs2 - 1];

	char srcRelation1[ATTR_SIZE];
	OpenRelations::getRelationName(srcRelId1, srcRelation1 );
	for (int iter = 0; iter < nAttrs1; iter++) {
		Attribute attrCatalogEntry[NO_OF_ATTRS_RELCAT_ATTRCAT];
		getAttrCatEntry(srcRelId1, iter, attrCatalogEntry);
		strncpy(targetRelAttrNames[iter], attrCatalogEntry[1].sval, ATTR_SIZE);
		targetRelAttrTypes[iter] = attrCatalogEntry[2].nval;
	}
	int attrIndex = nAttrs1;

	char srcRelation2[ATTR_SIZE];
	OpenRelations::getRelationName(srcRelId2, srcRelation2);
	for (int iter = 0; iter < nAttrs2; iter++) {
		Attribute attrCatalogEntry[NO_OF_ATTRS_RELCAT_ATTRCAT];
		getAttrCatEntry(srcRelId2, iter, attrCatalogEntry);

		if (strcmp(attr2, attrCatalogEntry[1].sval) != 0) {
			targetRelAttrTypes[attrIndex] = attrCatalogEntry[2].nval;
			strncpy(targetRelAttrNames[attrIndex++], attrCatalogEntry[1].sval, ATTR_SIZE);
		}
	}

	int flag = createRel(targetRelation, nAttrs1 + nAttrs2 - 1, targetRelAttrNames, targetRelAttrTypes);
	if (flag != SUCCESS) {
		return flag; // target rel may already exist or attrs more than limit or ...
	}

	int targetRelId = OpenRelations::openRelation(targetRelation);

	if (targetRelId == E_CACHEFULL) {
		deleteRel(targetRelation);
		return E_CACHEFULL;
	}

	Attribute record1[nAttrs1];
	recId prev_recid1;
	prev_recid1.block = -1;
	prev_recid1.slot = -1;
	Attribute dummy;
	strcpy(dummy.sval, " ");

	// FORMING TARGET RELATION
	/*
	 *
	 */
	while (ba_search(srcRelId1, record1, "PROJECT", dummy, PRJCT, &prev_recid1) == SUCCESS) {

		Attribute record2[nAttrs2];
		Attribute targetRecord[nAttrs1 + nAttrs2 - 1];
		recId prev_recid2;
		prev_recid2.block = -1;
		prev_recid2.slot = -1;

		while (ba_search(srcRelId2, record2, attr2, record1[static_cast<int>(attrcat_entry1[5].nval)], EQ, &prev_recid2) ==SUCCESS) {

			for (int iter = 0; iter < nAttrs1; iter++)
				targetRecord[iter] = record1[iter];
			int targetIndex = nAttrs1;
			for (int iter = 0; iter < nAttrs2; iter++) {
				if (iter != static_cast<int>(attrcat_entry2[5].nval))
					targetRecord[targetIndex++] = record2[iter];
			}

			flag = ba_insert(targetRelId, targetRecord);

			if (flag != SUCCESS) {
				OpenRelations::closeRelation(targetRelId);
				deleteRel(targetRelation);
				return flag;
			}

		}
	}

	OpenRelations::closeRelation(targetRelId);
	return SUCCESS;
}


// TODO : Find library functions for this?
int checkAttrTypeOfValue(char *data) {
	int count_int = 0, count_dot = 0, count_string = 0, i;
	for (i = 0; data[i] != '\0'; i++) {

		if (data[i] >= '0' && data[i] <= '9')
			count_int++;
		if (data[i] == '.')
			count_dot++;
		else
			count_string++;
	}

	if (count_dot == 1 && count_int == (strlen(data) - 1))
		return NUMBER;
	if (count_int == strlen(data)) {
		return NUMBER;
	} else
		return STRING;
}

/*
 * Gets #ttribute of relation from Relation Catalog Entry
 */
int getNumberOfAttrsForRelation(int relationId) {
	Attribute relCatEntry[NO_OF_ATTRS_RELCAT_ATTRCAT];
	int retValue;
	retValue = getRelCatEntry(relationId, relCatEntry);
	if (retValue != SUCCESS) {
		return retValue;
	}
	int numAttrs = static_cast<int>(relCatEntry[1].nval);
	return numAttrs;
}

/*
 * Gets attribute types of relation from Attribute Catalog Entry
 */
void getAttrTypesForRelation(int relId, int numAttrs, int attrTypes[numAttrs]) {

	Attribute attrCatEntry[NO_OF_ATTRS_RELCAT_ATTRCAT];

	for (int offsetIter = 0; offsetIter < numAttrs; ++offsetIter) {
		getAttrCatEntry(relId, offsetIter, attrCatEntry);
		attrTypes[static_cast<int>(attrCatEntry[5].nval)] = static_cast<int>(attrCatEntry[2].nval);
	}
}


/* Construct a record ( array of type Attribute ) from char array of attributes
 * Also performs type checking
 * @param numAttrs : #attributes in the relation
 * @param record : 'Attribute' array, new record is stored here
 * @param recordArray : 2D char array, a single row stores an attribute as a char array
 * @param attrTypes : attribute types of the relation, used for type checking
 * @return :
 *      SUCCESS
 *      E_ATTRTYPEMISMATCH : types dont match
 */
int constructRecordFromAttrsArray(int numAttrs, Attribute record[numAttrs], char recordArray[numAttrs][ATTR_SIZE], int attrTypes[numAttrs]) {
	for (int l = 0; l < numAttrs; l++) {

		if (attrTypes[l] == NUMBER) {
			if (checkAttrTypeOfValue(recordArray[l]) == NUMBER)
				record[l].nval = atof(recordArray[l]);
			else
				return E_ATTRTYPEMISMATCH;
		}

		if (attrTypes[l] == STRING) {
			strcpy(record[l].sval, recordArray[l]);
		}
	}
	return SUCCESS;
}