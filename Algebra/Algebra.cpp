#include "Algebra.h"
#include <cstring>
#include <functional>
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
		std::strcpy( attr_names.get( )[ i ], attrCatOffsetEntry.attrName );
		attr_types[ i ] = attrCatOffsetEntry.attrType;
	}

	/* Create the relation for target relation by calling Schema::createRel()
	   by providing appropriate arguments */
	// if the createRel returns an error code, then return that value.
	auto res = Schema::createRel( targetRel, src_nAttrs, attr_names.get( ), attr_types.data( ) );
	if ( res != SUCCESS ) {
		return res;
	}

	/* Open the newly created target relation by calling
	   OpenRelTable::openRel() method and store the target relid
	 * If opening fails, delete the target relation by calling
	 */
	int targetRelId = OpenRelTable::openRel( targetRel );

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

	RelCacheTable::resetSearchIndex( srcRelId );
	RelCacheTable::resetSearchIndex( targetRelId );
	AttrCacheTable::resetSearchIndex( srcRelId, attr );

	// read every record that satisfies the condition by repeatedly calling
	// BlockAccess::search() until there are no more records to be read

	res = BlockAccess::search( srcRelId, record.get( ), attr, attrVal, op );
	while ( res == SUCCESS /* BlockAccess::search() returns success */ ) {

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

		res = BlockAccess::search( srcRelId, record.get( ), attr, attrVal, op );
	}

	// Close the targetRel by calling closeRel() method of schema layer
	assert_res( Schema::closeRel( targetRel ), SUCCESS );

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
		} else if ( type == STRING ) {
			std::strcpy( recordValues[ i ].sVal, record[ i ] );
		}
	}

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
		auto res = AttrCacheTable::getAttrCatEntry( srcRelId, tar_Attrs[ i ], &attrCatOffsetEntry );
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
		auto ret = BlockAccess::insert( targetRelId, proj_record.get( ) );

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
		assert_res( AttrCacheTable::getAttrCatEntry( srcRelId, i, &srcAttrCatEntry ), SUCCESS );
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
		res = BlockAccess::project( srcRelId, record.get( ) );
	}

	// Close the targetRel by calling Schema::closeRel()
	assert_res( Schema::closeRel( targetRel ), SUCCESS );
	return SUCCESS;
}

int Algebra::join( char srcRelation1[ ATTR_SIZE ], char srcRelation2[ ATTR_SIZE ], char targetRelation[ ATTR_SIZE ],
	char attribute1[ ATTR_SIZE ], char attribute2[ ATTR_SIZE ] ) {

	// get the srcRelation1's rel-id using OpenRelTable::getRelId() method
	auto srcRelId1 = OpenRelTable::getRelId( srcRelation1 );

	// get the srcRelation2's rel-id using OpenRelTable::getRelId() method
	auto srcRelId2 = OpenRelTable::getRelId( srcRelation2 );

	// if either of the two source relations is not open
	//     return E_RELNOTOPEN
	if ( srcRelId2 == E_RELNOTOPEN || srcRelId1 == E_RELNOTOPEN )
		return E_RELNOTOPEN;

	AttrCatEntry attrCatEntry1, attrCatEntry2;
	// get the attribute catalog entries for the following from the attribute
	// cache (using AttrCacheTable::getAttrCatEntry())
	// - attrCatEntry1 = attribute1 of srcRelation1
	auto res1 = AttrCacheTable::getAttrCatEntry( srcRelId1, attribute1, &attrCatEntry1 );
	// - attrCatEntry2 = attribute2 of srcRelation2
	auto res2 = AttrCacheTable::getAttrCatEntry( srcRelId2, attribute2, &attrCatEntry2 );

	// if attribute1 is not present in srcRelation1 or attribute2 is not
	// present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
	//     return E_ATTRNOTEXIST.
	if ( res1 == E_ATTRNOTEXIST || res2 == E_ATTRNOTEXIST )
		return E_ATTRNOTEXIST;

	// if attribute1 and attribute2 are of different types return
	if ( attrCatEntry1.attrType != attrCatEntry2.attrType )
		return E_ATTRTYPEMISMATCH;
	// E_ATTRTYPEMISMATCH

	RelCatEntry relCatEntry1, relCatEntry2;

	// iterate through all the attributes in both the source relations and check
	// if there are any other pair of attributes other than join attributes (i.e.
	// attribute1 and attribute2) with duplicate names in srcRelation1 and
	// srcRelation2 (use AttrCacheTable::getAttrCatEntry())
	// If yes, return E_DUPLICATEATTR

	// get the relation catalog entries for the relations from the relation cache
	// (use RelCacheTable::getRelCatEntry() function)
	assert_res( RelCacheTable::getRelCatEntry( srcRelId1, &relCatEntry1 ), SUCCESS );
	assert_res( RelCacheTable::getRelCatEntry( srcRelId2, &relCatEntry2 ), SUCCESS );

	int numOfAttributes1 = relCatEntry1.numAttrs /* number of attributes in srcRelation1 */;
	int numOfAttributes2 = relCatEntry2.numAttrs /* number of attributes in srcRelation2 */;
	AttrCatEntry attr1;
	AttrCatEntry attr2;
	for ( int i = 0; i < numOfAttributes1; i++ ) {
		assert_res( AttrCacheTable::getAttrCatEntry( srcRelId1, i, &attr1 ), SUCCESS );
		if ( strcmp( attr1.attrName, attribute1 ) == 0 )
			continue;
		for ( int j = 0; j < numOfAttributes2; j++ ) {
			assert_res( AttrCacheTable::getAttrCatEntry( srcRelId2, j, &attr2 ), SUCCESS );
			if ( strcmp( attr1.attrName, attr2.attrName ) == 0 )
				return E_DUPLICATEATTR;
		}
	}

	// if rel2 does not have an index on attr2
	//     create it using BPlusTree:bPlusCreate()
	//     if call fails, return the appropriate error code
	//     (if your implementation is correct, the only error code that will
	//      be returned here is E_DISKFULL)
	if ( attrCatEntry2.rootBlock == -1 ) {
		auto res = BPlusTree::bPlusCreate( srcRelId2, attrCatEntry2.attrName );
		if ( res != SUCCESS )
			return res;
	}

	int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
	// Note: The target relation has number of attributes one less than
	// nAttrs1+nAttrs2 (Why?)

	// declare the following arrays to store the details of the target relation
	char targetRelAttrNames[ numOfAttributesInTarget ][ ATTR_SIZE ];
	int targetRelAttrTypes[ numOfAttributesInTarget ];

	// iterate through all the attributes in both the source relations and
	// update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding
	// attribute2 in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
	int targetIndex = 0;
	for ( int i = 0; i < numOfAttributes1; i++ ) {
		assert_res( AttrCacheTable::getAttrCatEntry( srcRelId1, i, &attr1 ), SUCCESS );
		std::strcpy( targetRelAttrNames[ targetIndex ], attr1.attrName );
		targetRelAttrTypes[ targetIndex ] = attr1.attrType;
		targetIndex++;
	}
	for ( int i = 0; i < numOfAttributes2; i++ ) {
		assert_res( AttrCacheTable::getAttrCatEntry( srcRelId2, i, &attr2 ), SUCCESS );
		if ( std::strcmp( attr2.attrName, attribute2 ) == 0 ) {
			continue;
		}
		std::strcpy( targetRelAttrNames[ targetIndex ], attr2.attrName );
		targetRelAttrTypes[ targetIndex ] = attr2.attrType;
		targetIndex++;
	}

	// create the target relation using the Schema::createRel() function
	auto res = Schema::createRel( targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes );

	if ( res != SUCCESS )
		return res;

	// if createRel() returns an error, return that error

	// Open the targetRelation using OpenRelTable::openRel()
	auto targetRelId = OpenRelTable::openRel( targetRelation );
	if ( targetRelId < 0 ) {
		Schema::deleteRel( targetRelation );
		return targetRelId;
	}

	Attribute record1[ numOfAttributes1 ];
	Attribute record2[ numOfAttributes2 ];
	Attribute targetRecord[ numOfAttributesInTarget ];
	assert_res( RelCacheTable::resetSearchIndex( srcRelId1 ), SUCCESS );
	assert_res( AttrCacheTable::resetSearchIndex( srcRelId1, attribute1 ), SUCCESS );

	// this loop is to get every record of the srcRelation1 one by one
	while ( BlockAccess::project( srcRelId1, record1 ) == SUCCESS ) {

		// reset the search index of `srcRelation2` in the relation cache
		// using RelCacheTable::resetSearchIndex()
		assert_res( RelCacheTable::resetSearchIndex( srcRelId2 ), SUCCESS );

		// reset the search index of `attribute2` in the attribute cache
		// using AttrCacheTable::resetSearchIndex()
		assert_res( AttrCacheTable::resetSearchIndex( srcRelId2, attribute2 ), SUCCESS );

		// this loop is to get every record of the srcRelation2 which satisfies
		// the following condition:
		// record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
		while (
			BlockAccess::search( srcRelId2, record2, attribute2, record1[ attrCatEntry1.offset ], EQ ) == SUCCESS ) {

			// copy srcRelation1's and srcRelation2's attribute values(except
			// for attribute2 in rel2) from record1 and record2 to targetRecord

			// insert the current record into the target relation by calling
			// BlockAccess::insert()
			//             int targetRelAttrIndex = 0;
			int  targetIndex =0 ;
			for ( int i = 0; i < numOfAttributes1; i++ ) {
				targetRecord[ targetIndex ] = record1[ i ];
				targetIndex++;
			}

			for ( int i = 0; i < numOfAttributes2; i++ ) {
				if ( i == attrCatEntry2.offset ) {
					continue;
				}
				targetRecord[ targetIndex ] = record2[ i ];
				targetIndex++;
			}

			res = BlockAccess::insert( targetRelId, targetRecord );
			if ( res != SUCCESS ) {

				// close the target relation by calling OpenRelTable::closeRel()
				OpenRelTable::closeRel( targetRelId );
				Schema::deleteRel( targetRelation );
				// delete targetRelation (by calling Schema::deleteRel())
				return E_DISKFULL;
			}
		}
	}

	// close the target relation by calling OpenRelTable::closeRel()
	OpenRelTable::closeRel( targetRelId );
	return SUCCESS;
}
