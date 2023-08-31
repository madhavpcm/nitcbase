#include "Algebra.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

bool isNumber( char* str ) {
	int len;
	float ignore;
	/*
	  sscanf returns the number of elements read, so if there is no float matching
	  the first %f, ret will be 0, else it'll be 1

	  %n gets the number of characters read. this scanf sequence will read the
	  first float ignoring all the whitespace before and after. and the number of
	  characters read that far will be stored in len. if len == strlen(str), then
	  the string only contains a float with/without whitespace. else, there's
	  other characters.
	*/
	int ret = sscanf( str, "%f %n", &ignore, &len );
	return ret == 1 && len == strlen( str );
}
/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::select(
	char srcRel[ ATTR_SIZE ], char targetRel[ ATTR_SIZE ], char attr[ ATTR_SIZE ], int op, char strVal[ ATTR_SIZE ] ) {
	int srcRelId = OpenRelTable::getRelId( srcRel ); // we'll implement this later
	if ( srcRelId == E_RELNOTOPEN ) {
		return E_RELNOTOPEN;
	}

	AttrCatEntry attrCatEntry;
	// get the attribute catalog entry for attr, using
	// AttrCacheTable::getAttrcatEntry()
	//    return E_ATTRNOTEXIST if it returns the error
	if ( AttrCacheTable::getAttrCatEntry( srcRelId, attr, &attrCatEntry ) != SUCCESS ) {
		return E_ATTRNOTEXIST;
	}

	/*** Convert strVal (string) to an attribute of data type NUMBER or STRING
	 * ***/
	int type = attrCatEntry.attrType;
	Attribute attrVal;
	if ( type == NUMBER ) {
		if ( isNumber( strVal ) ) { // the isNumber() function is implemented below
			attrVal.nVal = atof( strVal );
		} else {
			return E_ATTRTYPEMISMATCH;
		}
	} else if ( type == STRING ) {
		strcpy( attrVal.sVal, strVal );
	}

	/*** Selecting records from the source relation ***/

	// Before calling the search function, reset the search to start from the
	// first hit using RelCacheTable::resetSearchIndex()
	RelCacheTable::resetSearchIndex( srcRelId );
	RelCatEntry relCatEntry;
	// get relCatEntry using RelCacheTable::getRelCatEntry()
	RelCacheTable::getRelCatEntry( srcRelId, &relCatEntry );

	/************************
	The following code prints the contents of a relation directly to the output
	console. Direct console output is not permitted by the actual the NITCbase
	specification and the output can only be inserted into a new relation. We will
	be modifying it in the later stages to match the specification.
	************************/

	std::vector<int> searchAttrType;
	std::cout << '|';
	for ( int i = 0; i < relCatEntry.numAttrs; ++i ) {
		AttrCatEntry attrCatEntry;
		// get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
		AttrCacheTable::getAttrCatEntry( srcRelId, i, &attrCatEntry );
		searchAttrType.push_back( attrCatEntry.attrType );

		std::cout << std::setw( 16 ) << std::left << attrCatEntry.attrName << "|";
		// printf( " %s |", attrCatEntry.attrName );
	}
	std::cout << '\n';

	while ( true ) {
		RecId searchRes = BlockAccess::linearSearch( srcRelId, attr, attrVal, op );
		if ( searchRes.block == -1 || searchRes.slot == -1 )
			break;

		// get the record at searchRes using BlockBuffer.getRecord
		RecBuffer searchBlockBuffer( searchRes.block );
		HeadInfo searchBlockHeader;

		searchBlockBuffer.getHeader( &searchBlockHeader );
		std::vector<union Attribute> searchData( searchBlockHeader.numAttrs );
		searchBlockBuffer.getRecord( searchData.data( ), searchRes.slot );

		std::cout << '|';
		for ( int i = 0; i < searchData.size( ); i++ ) {
			if ( searchAttrType[ i ] == NUMBER ) {
				std::cout << std::setw( 16 ) << std::left << searchData[ i ].nVal << "|";
			} else {
				std::cout << std::setw( 16 ) << std::left << searchData[ i ].sVal << "|";
			}
		}
		std::cout << '\n';
	}

	return SUCCESS;
}

// will return if a string can be parsed as a floating point number
