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
	/*** Creating and opening the target relation ***/
	// Prepare arguments for createRel() in the following way:
	// get RelcatEntry of srcRel using RelCacheTable::getRelCatEntry()
	RelCatEntry relCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( srcRelId, &relCatEntry ), SUCCESS );
	int src_nAttrs = relCatEntry.numAttrs; /* the no. of attributes present in src relation */

	/* let attr_names[src_nAttrs][ATTR_SIZE] be a 2D array of type char
	 (will store the attribute names of rel).
	 */
	// let attr_types[src_nAttrs] be an array of type int
	std::unique_ptr<char[][ ATTR_SIZE ]> attr_names( new char[ src_nAttrs ][ ATTR_SIZE ] );
	std::vector<int> attr_types( src_nAttrs );

	/*
	 * iterate through 0 to src_nAttrs-1 :
	 * get the i'th attribute's AttrCatEntry using
	 * AttrCacheTable::getAttrCatEntry() fill the attr_names, attr_types
	 * arrays that we declared with the entries of corresponding attributes
	 */
	for ( int i = 0; i < src_nAttrs; i++ ) {
		AttrCatEntry attrCatOffsetEntry;
		assert_res( AttrCacheTable::getAttrCatEntry( srcRelId, i, &attrCatOffsetEntry ), SUCCESS );
		std::strcpy( attr_names.get( )[ i ], attrCatOffsetEntry.relName );
		attr_types[ i ] = attrCatOffsetEntry.attrType;
	}

	/* Create the relation for target relation by calling Schema::createRel()
	   by providing appropriate arguments */
	// if the createRel returns an error code, then return that value.
	int targetRelId = Schema::createRel( targetRel, src_nAttrs, attr_names.get( ), attr_types.data( ) );

	/* Open the newly created target relation by calling
	   OpenRelTable::openRel() method and store the target relid
	 * If opening fails, delete the target relation by calling
	 */

	if ( targetRelId < 0 ) {
		Schema::deleteRel( targetRel );
		return targetRelId;
	}

	/*** Selecting and inserting records into the target relation ***/
	/* Before calling the search function, reset the search to start from the
	   first using RelCacheTable::resetSearchIndex() */
	std::unique_ptr<union Attribute[]> record( new union Attribute[ src_nAttrs ] );

	/*
	 *The
	 *   BlockAccess::search() function can either do a linearSearch or a B+ tree
	 *   search. Hence, reset the search index of the relation in the relation
	 *cache using RelCacheTable::resetSearchIndex(). Also, reset the search index
	 *in the attribute cache for the select condition attribute with name given by
	 *   the argument `attr`. Use AttrCacheTable::resetSearchIndex(). Both these
	 *   calls are necessary to ensure that search begins from the first record.
	 */
	RelCacheTable::resetSearchIndex( targetRelId );
	// AttrCacheTable::resetSearchIndex( targetRelId, targetRel );

	// read every record that satisfies the condition by repeatedly calling
	// BlockAccess::search() until there are no more records to be read

	while ( BlockAccess::search( targetRelId, record.get( ), attr, attrVal, op ) ==
			SUCCESS /* BlockAccess::search() returns success */ ) {

		// ret = BlockAccess::insert(targetRelId, record);
		auto ret = BlockAccess::insert( targetRelId, record.get( ) );
		// if (insert fails) {
		//     close the targetrel(by calling Schema::closeRel(targetrel))
		//     delete targetrel (by calling Schema::deleteRel(targetrel))
		//     return ret;
		// }
		if ( ret != SUCCESS ) {
			Schema::closeRel( targetRel );
			Schema::deleteRel( targetRel );
			return ret;
		}
	}

	// Close the targetRel by calling closeRel() method of schema layer
	Schema::closeRel( targetRel );

	// return SUCCESS.

	return SUCCESS;
}

int Algebra::insert( char relName[ ATTR_SIZE ], int nAttrs, char record[][ ATTR_SIZE ] ) {
	// if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
	// return E_NOTPERMITTED;

	if ( std::strcmp( relName, ATTRCAT_RELNAME ) == 0 || std::strcmp( relName, RELCAT_RELNAME ) == 0 ) {
		return E_NOTPERMITTED;
	}
	// get the relation's rel-id using OpenRelTable::getRelId() method
	int relId = OpenRelTable::getRelId( relName );

	// if relation is not open in open relation table, return E_RELNOTOPEN
	// (check if the value returned from getRelId function call = E_RELNOTOPEN)
	if ( relId == E_RELNOTOPEN )
		return E_RELNOTOPEN;

	// get the relation catalog entry from relation cache
	// (use RelCacheTable::getRelCatEntry() of Cache Layer)
	RelCatEntry relCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( relId, &relCatEntry ), SUCCESS );

	/* if relCatEntry.numAttrs != numberOfAttributes in relation,
	   return E_NATTRMISMATCH */
	if ( relCatEntry.numAttrs != nAttrs ) {
		return E_NATTRMISMATCH;
	}

	// let recordValues[numberOfAttributes] be an array of type union Attribute
	std::vector<union Attribute> recordValues( nAttrs );

	/*
	 */
	// iterate through 0 to nAttrs-1: (let i be the iterator)
	for ( int i = 0; i < nAttrs; i++ ) {
		// get the attr-cat entry for the i'th attribute from the attr-cache
		// (use AttrCacheTable::getAttrCatEntry())
		AttrCatEntry attrCatEntry;
		assert_res( AttrCacheTable::getAttrCatEntry( relId, i, &attrCatEntry ), SUCCESS );

		// let type = attrCatEntry.attrType;
		auto type = attrCatEntry.attrType;

		if ( type == NUMBER ) {
			// if the char array record[i] can be converted to a number
			// (check this using isNumber() function)
			if ( isNumber( record[ i ] ) ) {
				/* convert the char array to numeral and store it
				   at recordValues[i].nVal using atof() */
				recordValues[ i ].nVal = atof( record[ i ] );
			} else {
				return E_ATTRTYPEMISMATCH;
			}
			std::cout << recordValues[ i ].nVal << ' ';
		} else if ( type == STRING ) {
			std::strcpy( recordValues[ i ].sVal, record[ i ] );
			std::cout << recordValues[ i ].sVal << ' ';
		}
	}
	std::cout << '\n';

	// insert the record by calling BlockAccess::insert() function
	// let retVal denote the return value of insert call
	int retVal = BlockAccess::insert( relId, recordValues.data( ) );
	return retVal;
}

int Algebra::project(
	char srcRel[ ATTR_SIZE ], char targetRel[ ATTR_SIZE ], int tar_nAttrs, char tar_Attrs[][ ATTR_SIZE ] ) {

	int srcRelId = OpenRelTable::getRelId( srcRel );
	if ( srcRelId == E_RELNOTOPEN )
		return E_RELNOTOPEN;

	RelCatEntry srcRelCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( srcRelId, &srcRelCatEntry ), SUCCESS );

	int srcNumAttrs = srcRelCatEntry.numAttrs;
	std::unique_ptr<int[]> attrOffsets( new int[ tar_nAttrs ] );
	std::unique_ptr<int[]> attrTypes( new int[ tar_nAttrs ] );

	for ( int i = 0; i < tar_nAttrs; i++ ) {
		AttrCatEntry attrCatOffsetEntry;
		auto res = AttrCacheTable::getAttrCatEntry( srcRelId, i, &attrCatOffsetEntry );
		if ( res != SUCCESS )
			return E_ATTRNOTEXIST;

		attrOffsets.get( )[ i ] = attrCatOffsetEntry.offset;
		attrTypes.get( )[ i ]	= attrCatOffsetEntry.attrType;
	}

	auto res = Schema::createRel( targetRel, tar_nAttrs, tar_Attrs, attrTypes.get( ) );
	if ( res != SUCCESS )
		return res;

	int targetRelId = OpenRelTable::openRel( targetRel );
	if ( targetRelId < 0 ) {
		Schema::deleteRel( targetRel );
		return targetRelId;
	}
	RelCacheTable::resetSearchIndex( srcRelId );
	std::unique_ptr<union Attribute[]> record( new union Attribute[ srcNumAttrs ] );

	res = BlockAccess::project( srcRelId, record.get( ) );
	while ( res == SUCCESS /* BlockAccess::project(srcRelId, record) returns SUCCESS */ ) {
		std::unique_ptr<union Attribute[]> proj_record( new union Attribute[ tar_nAttrs ] );
		for ( int i = 0; i < tar_nAttrs; i++ ) {
			proj_record.get( )[ i ] = record.get( )[ attrOffsets[ i ] ];
		}
		auto ret = BlockAccess::insert( targetRelId, record.get( ) );

		if ( ret != SUCCESS ) {
			assert_res( Schema::closeRel( targetRel ), SUCCESS );
			assert_res( Schema::deleteRel( targetRel ), SUCCESS );
			return ret;
		}
		res = BlockAccess::project( srcRelId, record.get( ) );
	}
	assert_res( Schema::closeRel( targetRel ), SUCCESS );
	return SUCCESS;
}

int Algebra::project( char srcRel[ ATTR_SIZE ], char targetRel[ ATTR_SIZE ] ) {

	// if srcRel is not open in open relation table, return E_RELNOTOPEN

	// get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()

	// get the no. of attributes present in relation from the fetched RelCatEntry.

	// attrNames and attrTypes will be used to store the attribute names
	// and types of the source relation respectively
	int srcRelId = OpenRelTable::getRelId( srcRel );
	if ( srcRelId == E_RELNOTOPEN )
		return E_RELNOTOPEN;

	RelCatEntry srcRelCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( srcRelId, &srcRelCatEntry ), SUCCESS );

	int srcNumAttrs = srcRelCatEntry.numAttrs;
	std::unique_ptr<char[][ ATTR_SIZE ]> attrNames( new char[ srcNumAttrs ][ ATTR_SIZE ] );
	std::unique_ptr<int[]> attrTypes( new int[ srcNumAttrs ] );

	/*iterate through every attribute of the source relation :
	 * - get the AttributeCat entry of the attribute with offset.
	 *   (using AttrCacheTable::getAttrCatEntry())
	 * - fill the arrays `attrNames` and `attrTypes` that we declared earlier
	 *   with the data about each attribute
	 */
	for ( int i = 0; i < srcNumAttrs; i++ ) {
		AttrCatEntry srcAttrCatEntry;
		AttrCacheTable::getAttrCatEntry( srcRelId, i, &srcAttrCatEntry );
		std::strcpy( attrNames.get( )[ i ], srcAttrCatEntry.attrName );
		attrTypes.get( )[ i ] = srcAttrCatEntry.attrType;
	}

	/*** Creating and opening the target relation ***/

	// Create a relation for target relation by calling Schema::createRel()

	// if the createRel returns an error code, then return that value.
	// Open the newly created target relation by calling OpenRelTable::openRel()
	// and get the target relid

	// If opening fails, delete the target relation by calling Schema::deleteRel()
	// of return the error value returned from openRel().

	/*** Inserting projected records into the target relation ***/

	// Take care to reset the searchIndex before calling the project function
	// using RelCacheTable::resetSearchIndex()
	auto res = Schema::createRel( targetRel, srcNumAttrs, attrNames.get( ), attrTypes.get( ) );
	if ( res != SUCCESS )
		return res;

	int targetRelId = OpenRelTable::openRel( targetRel );
	if ( targetRelId < 0 ) {
		Schema::deleteRel( targetRel );
		return targetRelId;
	}
	RelCacheTable::resetSearchIndex( srcRelId );
	std::unique_ptr<union Attribute[]> record( new union Attribute[ srcNumAttrs ] );

	res = BlockAccess::project( srcRelId, record.get( ) );
	while ( res == SUCCESS /* BlockAccess::project(srcRelId, record) returns SUCCESS */ ) {
		// record will contain the next record

		// ret = BlockAccess::insert(targetRelId, proj_record);
		auto ret = BlockAccess::insert( targetRelId, record.get( ) );

		if ( ret != SUCCESS /* insert fails */ ) {
			assert_res( Schema::closeRel( targetRel ), SUCCESS );
			assert_res( Schema::deleteRel( targetRel ), SUCCESS );
			return ret;
			// return ret;
		}
	}

	// Close the targetRel by calling Schema::closeRel()
	assert_res( Schema::closeRel( srcRel ), SUCCESS );
	return SUCCESS;
}
