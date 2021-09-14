//
// Created by Jessiya Joy on 12/09/21.
//
#include <string>
#include <vector>
#include "define/constants.h"
#include "define/errors.h"
#include "disk_structures.h"
#include "algebra.h"
#include "block_access.h"
#include "OpenRelTable.h"

using namespace std;

int check_type(char *data);

int insert(vector<string> attributeTokens, char *table_name) {

	// check if relation is open
	int relationId = OpenRelations::getRelationId(table_name);
	if (relationId == E_RELNOTOPEN) {
		return relationId;
	}

	// get #attributes from relation catalog entry
	Attribute relCatEntry[NO_OF_ATTRS_RELCAT_ATTRCAT];
	int retValue;
	retValue = getRelCatEntry(relationId, relCatEntry);
	if (retValue != SUCCESS) {
		return retValue;
	}
	int numAttrs = static_cast<int>(relCatEntry[1].nval);
	if (numAttrs != attributeTokens.size())
		return E_NATTRMISMATCH;

	// get attribute types from attribute catalog entry
	int attrTypes[numAttrs];
	Attribute attrCatEntry[NO_OF_ATTRS_RELCAT_ATTRCAT];

	int relId = OpenRelations::getRelationId(table_name);
	for (int offsetIter = 0; offsetIter < numAttrs; ++offsetIter) {
		getAttrCatEntry(relId, offsetIter, attrCatEntry);
		attrTypes[static_cast<int>(attrCatEntry[5].nval)] = static_cast<int>(attrCatEntry[2].nval);
	}

	// for each attribute, convert string vector to char array
	char recordArray[numAttrs][16];
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
	// Perform type checking for integer types
	Attribute record[numAttrs];
	for (int l = 0; l < numAttrs; l++) {

		if (attrTypes[l] == NUMBER) {
			if (check_type(recordArray[l]) == NUMBER)
				record[l].nval = atof(recordArray[l]);
			else
				return E_ATTRTYPEMISMATCH;
		}

		if (attrTypes[l] == STRING) {
			strcpy(record[l].sval, recordArray[l]);
		}
	}

	retValue = ba_insert(relationId, record);
	if (retValue == SUCCESS) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

// TODO : Find library functions for this?
int check_type(char *data) {
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