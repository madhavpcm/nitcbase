#include "Frontend.h"
#include <chrono>
#include <cstring>
#include <iostream>

int Frontend::create_table(
	char relname[ ATTR_SIZE ], int no_attrs, char attributes[][ ATTR_SIZE ], int type_attrs[] ) {
	// Schema::createRel
	return Schema::createRel( relname, no_attrs, attributes, type_attrs );
}

int Frontend::drop_table( char relname[ ATTR_SIZE ] ) {
	// return Schema::deleteRel(relname);
	return Schema::deleteRel( relname );
}

int Frontend::open_table( char relname[ ATTR_SIZE ] ) {
	// Schema::openRel
	return Schema::openRel( relname );
}

int Frontend::close_table( char relname[ ATTR_SIZE ] ) {
	// Schema::closeRel
	return Schema::closeRel( relname );
}

int Frontend::alter_table_rename( char relname_from[ ATTR_SIZE ], char relname_to[ ATTR_SIZE ] ) {
	return Schema::renameRel( relname_from, relname_to );
}

int Frontend::alter_table_rename_column(
	char relname[ ATTR_SIZE ], char attrname_from[ ATTR_SIZE ], char attrname_to[ ATTR_SIZE ] ) {
	return Schema::renameAttr( relname, attrname_from, attrname_to );
}

int Frontend::create_index( char relname[ ATTR_SIZE ], char attrname[ ATTR_SIZE ] ) {
	// Schema::createIndex
	return Schema::createIndex( relname, attrname );
}
int Frontend::drop_index( char relname[ ATTR_SIZE ], char attrname[ ATTR_SIZE ] ) {
	// Schema::dropIndex
	return Schema::dropIndex( relname, attrname );
}

int Frontend::insert_into_table_values( char relname[ ATTR_SIZE ], int attr_count, char attr_values[][ ATTR_SIZE ] ) {
	return Algebra::insert( relname, attr_count, attr_values );
}

int Frontend::select_from_table( char relname_source[ ATTR_SIZE ], char relname_target[ ATTR_SIZE ] ) {
	// Algebra::project

	auto start_time = std::chrono::high_resolution_clock::now( );
	auto res		= Algebra::project( relname_source, relname_target );
	auto end_time	= std::chrono::high_resolution_clock::now( );

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end_time - start_time );
	std::cout << "Time taken to select : " << duration.count( ) << " microsecond(s)" << std::endl;
	return res;
}

int Frontend::select_attrlist_from_table( char relname_source[ ATTR_SIZE ], char relname_target[ ATTR_SIZE ],
	int attr_count, char attr_list[][ ATTR_SIZE ] ) {
	// Algebra::project
	return Algebra::project( relname_source, relname_target, attr_count, attr_list );
}

int Frontend::select_from_table_where( char relname_source[ ATTR_SIZE ], char relname_target[ ATTR_SIZE ],
	char attribute[ ATTR_SIZE ], int op, char value[ ATTR_SIZE ] ) {
	auto start_time = std::chrono::high_resolution_clock::now( );
	auto res		= Algebra::select( relname_source, relname_target, attribute, op, value );
	auto end_time	= std::chrono::high_resolution_clock::now( );
	auto duration	= std::chrono::duration_cast<std::chrono::microseconds>( end_time - start_time );
	std::cout << "Time taken to select : " << duration.count( ) << " microsecond(s)" << std::endl;
	return res;
}

int Frontend::select_attrlist_from_table_where( char relname_source[ ATTR_SIZE ], char relname_target[ ATTR_SIZE ],
	int attr_count, char attr_list[][ ATTR_SIZE ], char attribute[ ATTR_SIZE ], int op, char value[ ATTR_SIZE ] ) {
	// Call select() method of the Algebra Layer with correct arguments to
	// create a temporary target relation with name ".temp" (use constant TEMP)
	auto res = Algebra::select( relname_source, ( char* )TEMP, attribute, op, value );

	// TEMP will contain all the attributes of the source relation as it is the
	// result of a select operation
	// Return Error values, if not successful
	if ( res != SUCCESS )
		return res;

	// Open the TEMP relation using OpenRelTable::openRel()
	// if open fails, delete TEMP relation using Schema::deleteRel() and
	// return the error code
	int tempRelId = OpenRelTable::openRel( ( char* )TEMP );

	// On the TEMP relation, call project() method of the Algebra Layer with
	// correct arguments to create the actual target relation. The final
	// target relation contains only those attributes mentioned in attr_list
	res = Algebra::project( ( char* )TEMP, relname_target, attr_count, attr_list );

	// close the TEMP relation using OpenRelTable::closeRel()
	// delete the TEMP relation using Schema::deleteRel()
	assert_res( OpenRelTable::closeRel( tempRelId ), SUCCESS );
	assert_res( Schema::deleteRel( ( char* )TEMP ), SUCCESS );

	// return any error codes from project() or SUCCESS otherwise
	return SUCCESS;
}

int Frontend::select_from_join_where( char relname_source_one[ ATTR_SIZE ], char relname_source_two[ ATTR_SIZE ],
	char relname_target[ ATTR_SIZE ], char join_attr_one[ ATTR_SIZE ], char join_attr_two[ ATTR_SIZE ] ) {
	// Algebra::join
	return SUCCESS;
}

int Frontend::select_attrlist_from_join_where( char relname_source_one[ ATTR_SIZE ],
	char relname_source_two[ ATTR_SIZE ], char relname_target[ ATTR_SIZE ], char join_attr_one[ ATTR_SIZE ],
	char join_attr_two[ ATTR_SIZE ], int attr_count, char attr_list[][ ATTR_SIZE ] ) {
	// Algebra::join + project
	return SUCCESS;
}

int Frontend::custom_function( int argc, char argv[][ ATTR_SIZE ] ) {
	// argc gives the size of the argv array
	// argv stores every token delimited by space and comma

	// implement whatever you desire

	return SUCCESS;
}
