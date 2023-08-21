#include <array>

#include "Buffer/BlockBuffer.h"
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include "define/constants.h"
#include <cstdlib>
#include <iostream>
#include <cstring>

int main(int argc, char *argv[]) {
	/* Initialize the Run Copy of Disk */
	std::array<unsigned char, BLOCK_SIZE> buffer;

//	  unsigned char buffer[BLOCK_SIZE];
	Disk disk_run;

	RecBuffer relCatBuffer(RELCAT_BLOCK);
	RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

	HeadInfo relCatHeader;
	HeadInfo attrCatHeader;

	relCatBuffer.getHeader(&relCatHeader);
	attrCatBuffer.getHeader(&attrCatHeader);

	std::cout << relCatHeader.numEntries << " RELCAT entries detected.\n";
	std::cout << attrCatHeader.numEntries << " ATTRCAT entries detected.\n";
	for(int rel = 0 ; rel < relCatHeader.numEntries; rel++) {
		// Record, ie relation in relcat
		std::array<Attribute,RELCAT_NO_ATTRS> relCatRecord;
		// Get record data
		relCatBuffer.getRecord(relCatRecord.data(), rel);

		std::cout << "Relation: " << relCatRecord[RELCAT_REL_NAME_INDEX].sVal << '\n';

		for(int attr =0 ; attr < attrCatHeader.numEntries; attr++) {
			//declare attribute catalog entry
			std::array<Attribute,RELCAT_NO_ATTRS> attrCatRecord;
			attrCatBuffer.getRecord(attrCatRecord.data(), attr);

			if(std::strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal ,relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
				const char* atype = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
				std::cout << "  " << attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal << ':' <<  atype << '\n';
			}
		}
	}
	// StaticBuffer buffer;
	std::cout << '\n';


	return 0;

	return FrontendInterface::handleFrontend(argc, argv);
}
