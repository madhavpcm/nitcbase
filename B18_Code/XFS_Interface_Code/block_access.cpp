//
// Created by Jessiya Joy on 17/08/21.
//
#include <cstdio>
#include <string>
#include <cstring>
#include "../define/constants.h"
#include "../define/errors.h"
#include "disk_structures.h"
#include "schema.h"
#include "OpenRelTable.h"

int getBlockType(int blocknum);

HeadInfo getHeader(int blockNum);

void setHeader(struct HeadInfo *header, int blockNum);

void getSlotmap(unsigned char *SlotMap, int blockNum);

void setSlotmap(unsigned char *SlotMap, int no_of_slots, int blockNum);

int getFreeRecBlock();

recId getFreeSlot(int block_num);

int getRecord(Attribute *rec, int blockNum, int slotNum);

int setRecord(Attribute *rec, int blockNum, int slotNum);

int getRelCatEntry(int relationId, Attribute *relcat_entry);

int setRelCatEntry(int relationId, Attribute *relcat_entry);

int getAttrCatEntry(int relationId, char attrname[16], union Attribute *attrcat_entry);

int deleteBlock(int blockNum);

int deleteRelCatEntry(recId relcat_recid, Attribute relcat_rec[6]);

int deleteAttrCatEntry(recId attrcat_recid);

recId linear_search(relId relid, char attrName[ATTR_SIZE], union Attribute attrval, int op, recId *prev_recid);

int compareAttributes(union Attribute attr1, union Attribute attr2, int attrType);

/*
 *  Inserts the Record into the given Relation
 */
int ba_insert(int relid, Attribute *rec) {
	Attribute relcat_entry[6];
	getRelCatEntry(relid, relcat_entry);

	int num_attrs = relcat_entry[1].nval;
	int first_block = relcat_entry[3].nval;
	int num_slots = relcat_entry[5].nval;

	HeadInfo header;

	unsigned char slotmap[num_slots];
	int blockNum = first_block;

	if (first_block == -1) {
		blockNum = getFreeRecBlock();
		relcat_entry[3].nval = blockNum;

		// TODO: Take make_headerInfo() function out
		HeadInfo *H = (HeadInfo *) malloc(sizeof(HeadInfo));
		H->blockType = REC;
		H->pblock = -1;
		H->lblock = -1;
		H->rblock = -1;
		H->numEntries = 0;
		H->numAttrs = num_attrs;
		H->numSlots = num_slots;
		setHeader(H, blockNum);
		getSlotmap(slotmap, blockNum);

		// set all slots as free
		memset(slotmap, SLOT_UNOCCUPIED, sizeof(slotmap));
		setSlotmap(slotmap, num_slots, blockNum);
	}

	recId recid = getFreeSlot(blockNum);

	// no free slot found
	if (recid.block == -1 && recid.slot == -1) {
		return FAILURE;
	}
	setRecord(rec, recid.block, recid.slot);

	// increment #entries in header (as record is inserted)
	header = getHeader(recid.block);
	header.numEntries = header.numEntries + 1;
	setHeader(&header, recid.block);

	// increment #entries in relation catalog entry
	relcat_entry[2].nval = relcat_entry[2].nval + 1;
	// update last block in relation catalogue entry
	relcat_entry[4].nval = recid.block;
	setRelCatEntry(relid, relcat_entry);

	return SUCCESS;
}

/*
 *  Searches the relation specified to find the 'next' record starting from the given 'prev' record
 *  that satisfies the op condition on given attrval
 *  Uses the b+ tree if target attribute is indexed, otherwise, linear search
 */
int ba_search(relId relid, Attribute *record, char attrName[ATTR_SIZE], Attribute attrval, int op, recId *prev_recid) {
	// TODO: Cleanup Code
	recId recid;
	if (op == PRJCT) {
		recid = linear_search(relid, attrName, attrval, op, prev_recid);
	} else {
		Attribute attrcat_entry[6];
		getAttrCatEntry(relid, attrName, attrcat_entry);
		// Get the root_block from attribute catalog entry for the given attribute
		int root_block = attrcat_entry[4].nval;
		if (root_block == -1) {
            // No indexing for the attribute
			recid = linear_search(relid, attrName, attrval, op, prev_recid);
		} else {
            // Indexing exists for the attribute
			// TODO: recid = bplus_search(relid, attrName, attrval, op,&prev_recid);
		}
	}

	if ((recid.block == -1) && (recid.slot == -1)) {
		//  Fails to find a record satisfying the given condition
		return FAILURE;
	}

	//  Record Found, recid.block and recid.slot are the block and slot that contains matching record respectively
	getRecord(record, recid.block, recid.slot);
	return SUCCESS;
}


recId linear_search(relId relid, char attrName[ATTR_SIZE], union Attribute attrval, int op, recId *prev_recid) {
	// Get the Relation Catalog Record corresponding to the given relation name
	union Attribute relcat_entry[6];
	getRelCatEntry(relid, relcat_entry);

	int offset, attr_type;
	//get the record itself in relcat_entry array of attributes
	int curr_block, curr_slot, next_block = -1;
	int no_of_attributes = relcat_entry[1].nval;
	int no_of_slots = relcat_entry[5].nval;

	if (op != PRJCT) {
		union Attribute attrcat_entry[6];
		getAttrCatEntry(relid, attrName, attrcat_entry);
		offset = attrcat_entry[5].nval;
		attr_type = attrcat_entry[2].nval;
	}

	recId ret_recid;
	if ((prev_recid->block == -1) && prev_recid->slot == -1) {
		// if linear search is done for first time on this attribute
		curr_block = relcat_entry[3].nval;
		curr_slot = 0;
	} else {
		// if the linear search knows the hit from previous search
		curr_block = prev_recid->block;
		curr_slot = prev_recid->slot + 1;
	}

	unsigned char slotmap[no_of_slots];
	/*
	 * Iterate through all blocks starting from curr_block
	 */
	while (curr_block != -1) {
		struct HeadInfo header;
		header = getHeader(curr_block);
		next_block = header.rblock;
		getSlotmap(slotmap, curr_block);
		/*
		 * Iterate through all the Slots(Records) in the curr_block
		 */
		for (int slotNum = curr_slot; slotNum < no_of_slots; slotNum++) {
			union Attribute record[no_of_attributes];
			if (slotmap[slotNum] == SLOT_UNOCCUPIED) {
				continue;
			}
			// Get the record corresponding to {curr_block, slotNum=slotNum}
			getRecord(record, curr_block, slotNum);
			bool cond = false;
			if (op != PRJCT) {
				int flag = compareAttributes(record[offset], attrval, attr_type);
				switch (op) {
					case NE:
						if (flag != 0)
							cond = true;
						break;
					case LT:
						if (flag < 0)
							cond = true;
						break;
					case LE:
						if (flag <= 0)
							cond = true;
						break;
					case EQ:
						if (flag == 0)
							cond = true;
						break;
					case GT:
						if (flag > 0)
							cond = true;
						break;
					case GE:
						if (flag >= 0)
							cond = true;
				}
			}
			if (cond == true || op == PRJCT) {
				ret_recid = {curr_block, slotNum};
				/*
				 * prev_recid set to denote the previous hit record for the linear search
				 */
				*prev_recid = ret_recid;
				return ret_recid;
			}
		}
		curr_block = next_block;
		curr_slot = 0;
	}
	// No records match the search condition over the relation
	return {-1, -1};
}


// Todo: [AN EXISTING TODO] updating last block of attribute catalog
/*
 * Deletes the relation of the given name
 *      - Clears the Data stored in ALL record blocks corresponding to the relation
 *      - Clears the Attribute Catalog entries for the attributes of the relation
 *      - Clears the Relation Catalog entry for this relation
 */
// I have changed the order in which Relation Catalog and Open Relation table is searched respectively for the given relation
// REMOVE ONCE JEZZY SEES - ITS A NOTE TO CHECK
int ba_delete(char relName[ATTR_SIZE]) {
	// TODO: There was some issue with deletion earlier, we can check it later, I have added and cleaned the code now (A LOOOT of cleaning)
	Attribute relName_Attr;
	strcpy(relName_Attr.sval, relName);

	/* Check if a relation with the given name exists in Relation Catalog and retrieve the relcat_recid */
	recId prev_recid, relcat_recid;
	prev_recid.block = -1;
	prev_recid.slot = -1;

	relcat_recid = linear_search(RELCAT_RELID, "RelName", relName_Attr, EQ, &prev_recid);
	if ((relcat_recid.block == -1) && (relcat_recid.slot == -1)) {
		return E_RELNOTEXIST;
	}

	/* Check if a relation with the given name exists in Open Relation Table */
	if (OpenRelations::checkIfRelationOpen(relName) == SUCCESS) {
		return FAILURE;
	}

	/* Get the Relation Catalog Entry */
	Attribute relcat_rec[6];
	getRecord(relcat_rec, relcat_recid.block, relcat_recid.slot);

	/* Get first record block corresponding to the given relation */
	int curr_block = relcat_rec[3].nval;
	int no_of_attrs = relcat_rec[1].nval;
	int next_block = -1;

	/*
	 * Delete the Relation Block-by-Block starting from the first block
	 * Get the Next Block by using headerInfo of the Current Block
	 * deleteBlock() will erase the block and mark UNUSED_BLK in the Block Allocation Map
	 */
	while (curr_block != -1) {
		struct HeadInfo header = getHeader(curr_block);
		next_block = header.rblock;
		deleteBlock(curr_block);
		curr_block = next_block;
	}

	recId attrcat_recid;
	prev_recid.block = -1;
	prev_recid.slot = -1;
	for (int i = 0; i < no_of_attrs; i++) {
		attrcat_recid = linear_search(ATTRCAT_RELID, "RelName", relName_Attr, EQ, &prev_recid);
		prev_recid.block = -1;
		prev_recid.slot = -1;
		/* Updating the Attribute Catalog for the current Attribute Deletion */
		deleteAttrCatEntry(attrcat_recid);
	}

	/*
	 * Updating the Relation Catalog for the Relation Deletion
	 */
	deleteRelCatEntry(relcat_recid, relcat_rec);

	return SUCCESS;

}

int ba_renamerel(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

	// CHECK IF RELATION WITH NEW NAME ALREADY EXISTS
	recId prev_recid;
	prev_recid.block = -1;
	prev_recid.slot = -1;
	recId relcat_recid, attrcat_recid;
	Attribute temp;
	strcpy(temp.sval, newName);
	relcat_recid = linear_search(RELCAT_RELID, "RelName", temp, EQ, &prev_recid);
	if (!((relcat_recid.block == -1) && (relcat_recid.slot == -1)))
		return E_RELEXIST;

	// CHECK IF RELATION WITH OLD NAME EXISTS
	prev_recid.block = -1;
	prev_recid.slot = -1;
	strcpy(temp.sval, oldName);
	relcat_recid = linear_search(RELCAT_RELID, "RelName", temp, EQ, &prev_recid);
	if ((relcat_recid.block == -1) && (relcat_recid.slot == -1))
		return E_RELNOTEXIST;

	// UPDATE RELATION CATALOG WITH NEW NAME
	Attribute relCatRecord[6];
	getRecord(relCatRecord, relcat_recid.block, relcat_recid.slot);
	strcpy(relCatRecord[0].sval, newName);
	setRecord(relCatRecord, relcat_recid.block, relcat_recid.slot);

	// UPDATE ALL ATTRIBUTE CATALOG ENTRIES WITH NEW NAME
	prev_recid.block = -1;
	prev_recid.slot = -1;
	while (1) {
		attrcat_recid = linear_search(ATTRCAT_RELID, "RelName", temp, EQ, &prev_recid);
		if (!((attrcat_recid.block == -1) && (attrcat_recid.slot == -1))) {
			Attribute attrCatRecord[6];
			getRecord(attrCatRecord, attrcat_recid.block, attrcat_recid.slot);
			if (std::strcmp(attrCatRecord[0].sval, oldName) == 0) {
				strcpy(attrCatRecord[0].sval, newName);
				setRecord(attrCatRecord, attrcat_recid.block, attrcat_recid.slot);
			}
		} else
			break;
	}

	return SUCCESS;
}

int ba_renameattr(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

	// CHECK IF RELATION WITH THE GIVEN NAME EXISTS
	recId prev_recid;
	prev_recid.block = -1;
	prev_recid.slot = -1;
	recId relcat_recid, attrcat_recid;
	Attribute attrcat_record[6];
	Attribute temp;
	std::strcpy(temp.sval, relName);
	relcat_recid = linear_search(RELCAT_RELID, "RelName", temp, EQ, &prev_recid);
	if ((relcat_recid.block == -1) && (relcat_recid.slot == -1)) {
		return E_RELNOTEXIST;
	}


	// CHECK IF ATTRIBUTE WITH THE NEW NAME ALREADY EXISTS
	prev_recid.block = -1;
	prev_recid.slot = -1;
	while (1) {
		attrcat_recid = linear_search(ATTRCAT_RELID, "RelName", temp, EQ, &prev_recid);
		if (!((attrcat_recid.block == -1) && (attrcat_recid.slot == -1))) {
			getRecord(attrcat_record, attrcat_recid.block, attrcat_recid.slot);
			if (std::strcmp(attrcat_record[1].sval, newName) == 0)
				return E_ATTREXIST;
		} else
			break;
	}

	/* RENAME ATTRIBUTE
	 * If attribute with old name does not exist, E_ATTRNOTEXIST
	 */
	prev_recid.block = -1;
	prev_recid.slot = -1;
	while (1) {
		attrcat_recid = linear_search(ATTRCAT_RELID, "RelName", temp, EQ, &prev_recid);
		if (!((attrcat_recid.block == -1) && (attrcat_recid.slot == -1))) {
			getRecord(attrcat_record, attrcat_recid.block, attrcat_recid.slot);
			if (std::strcmp(attrcat_record[1].sval, oldName) == 0) {
				strcpy(attrcat_record[1].sval, newName);
				setRecord(attrcat_record, attrcat_recid.block, attrcat_recid.slot);
				return SUCCESS;
			}
		} else
			return E_ATTRNOTEXIST;
	}
}

/*
 * Retrieves whether the block is occupied or not
 * If occupied returns the type of occupied block (REC: 0, IND_NUMBERERNAL: 1, IND_LEAF: 2)
 * If Not returns UNUSED_BLK: 3
 */
int getBlockType(int blocknum) {
	FILE *disk = fopen(DISK_PATH, "rb");
	fseek(disk, 0, SEEK_SET);
	unsigned char blockAllocationMap[4 * BLOCK_SIZE];
	fread(blockAllocationMap, 4 * BLOCK_SIZE, 1, disk);
	fclose(disk);
	return (int32_t) (blockAllocationMap[blocknum]);
}

/*
 * Reads header for 'blockNum'th block from disk
 */
HeadInfo getHeader(int blockNum) {
	HeadInfo header;
	FILE *disk = fopen(DISK_PATH, "rb");
	fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
	fread(&header, 32, 1, disk);
	fclose(disk);
	return header;
}

/*
 * Writes header for 'blockNum'th block into disk given the header information
 */
void setHeader(struct HeadInfo *header, int blockNum) {
	FILE *disk = fopen(DISK_PATH, "rb+");
	fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
	fwrite(header, 32, 1, disk);
	fclose(disk);
}

/*
 * Reads slotmap for 'blockNum'th block from disk
 */
void getSlotmap(unsigned char *SlotMap, int blockNum) {
	FILE *disk = fopen(DISK_PATH, "rb+");
	fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
	RecBlock R;
	fread(&R, BLOCK_SIZE, 1, disk);
	int numSlots = R.numSlots;
	memcpy(SlotMap, R.slotMap_Records, numSlots);
	fclose(disk);
}

/*
 * Writes slotmap for 'blockNum'th block into disk given the number of blocks occupied
 */
void setSlotmap(unsigned char *SlotMap, int no_of_slots, int blockNum) {
	FILE *disk = fopen(DISK_PATH, "rb+");
	fseek(disk, blockNum * BLOCK_SIZE + 32, SEEK_SET);
	fwrite(SlotMap, no_of_slots, 1, disk);
	fclose(disk);
}

/*
 *
 */
int getFreeRecBlock() {
	// TODO: Title for this
	FILE *disk = fopen(DISK_PATH, "rb+");
	fseek(disk, 0, SEEK_SET);

	unsigned char blockAllocationMap[4 * BLOCK_SIZE];
	fread(blockAllocationMap, 4 * BLOCK_SIZE, 1, disk);

	for (int iter = 0; iter < 4 * BLOCK_SIZE; iter++) {
		if ((int32_t) (blockAllocationMap[iter]) == UNUSED_BLK) {
			blockAllocationMap[iter] = (unsigned char) REC;
			fseek(disk, 0, SEEK_SET);
			fwrite(blockAllocationMap, BLOCK_SIZE * 4, 1, disk);
			fclose(disk);
			return iter;
		}
	}

	return FAILURE;
}

/* Finds a free slot either from :
 *      - the block numbered 'block_num' or
 *      - next blocks in the linked list of blocks for the relation or
 *      - a newly allotted block for the relation
 */
// TODO: is the relation checked for being relcat or attrcat and if so, is it being allocated a second block
recId getFreeSlot(int block_num) {
	recId recid = {-1, -1};
	int prev_block_num, next_block_num;
	int num_slots;
	int num_attrs;
	struct HeadInfo header;

	// finding free slot
	while (block_num != -1) {
		header = getHeader(block_num);
		num_slots = header.numSlots;
		next_block_num = header.rblock;
		num_attrs = header.numAttrs;

		// getting slotmap for the current block
		unsigned char slotmap[num_slots];
		getSlotmap(slotmap, block_num);

		// searching for free slot in block (block_num)
		int iter;
		for (iter = 0; iter < num_slots; iter++) {
			if (slotmap[iter] == SLOT_UNOCCUPIED) {
				break;
			}
		}

		// if free slot found, return it
		if (iter < num_slots) {
			slotmap[iter] = SLOT_OCCUPIED;
			setSlotmap(slotmap, num_slots, block_num);
			recid = {block_num, iter};
			return recid;
		}

		// free slot not found in the current block, check the next block
		prev_block_num = block_num;
		block_num = next_block_num;
	}

	/*
	 * no free slots in current record blocks
	 * get new record block
	 */
	block_num = getFreeRecBlock();

	// no free blocks available in disk
	if (block_num == -1) {
		// no free slot can be found, return {-1, -1}
		return recid;
	}

	// TODO: make_headerInfo() function extract
	// TODO: make_slotMap() which returns an empty slotMap extract
	// TODO: getFreeRecBlock() function may be expanded to also set the header information
	//          OR make a function that gets a free block and sets both its header and slotMap
	//setting header for new record block
	header = getHeader(block_num);
	header.numSlots = num_slots;
	header.lblock = prev_block_num;
	header.rblock = -1;
	header.numAttrs = num_attrs;
	setHeader(&header, block_num);

	//setting slotmap
	unsigned char slotmap[num_slots];
	getSlotmap(slotmap, block_num);
	memset(slotmap, SLOT_UNOCCUPIED, sizeof(slotmap)); //all slots are free
	slotmap[0] = SLOT_OCCUPIED;
	setSlotmap(slotmap, num_slots, block_num);

	// recid of free slot
	recid = {block_num, 0};

	//setting prev_block_num rblock to new block
	header = getHeader(prev_block_num);
	header.rblock = block_num;
	setHeader(&header, prev_block_num);

	return recid;
}

/*
 * Reads record from disk given blockNum and slotNum
 */
int getRecord(Attribute *rec, int blockNum, int slotNum) {
	HeadInfo Header;
	Header = getHeader(blockNum);
	int numOfSlots = Header.numSlots;

	if (slotNum < 0 || slotNum > (numOfSlots - 1))
		return E_OUTOFBOUND;

	FILE *disk = fopen(DISK_PATH, "rb+");
	int BlockType = getBlockType(blockNum);

	if (BlockType == REC) {
		RecBlock R;
		fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
		fread(&R, BLOCK_SIZE, 1, disk);
		int numSlots = R.numSlots;

		if (*((int32_t *) (R.slotMap_Records + slotNum)) == 0)
			return E_FREESLOT;
		int numAttrs = R.numAttrs;

		/* offset :
		 *         slotmap size ( = numSlots ) +
		 *         size of records coming before current record ( = slotNum * numAttrs * ATTR_SIZE )
		 */
		memcpy(rec, (R.slotMap_Records + numSlots + (slotNum * numAttrs * ATTR_SIZE)), numAttrs * ATTR_SIZE);
		fclose(disk);
		return SUCCESS;
	} else if (BlockType == IND_INTERNAL) {
		//TODO
	} else if (BlockType == IND_LEAF) {
		//TODO
	} else {
		fclose(disk);
		return FAILURE;
	}
}


/*
 * Writes record into disk
 */
int setRecord(Attribute *rec, int blockNum, int slotNum) {
	struct HeadInfo header = getHeader(blockNum);
	int numOfSlots = header.numSlots;
	int numAttrs = header.numAttrs;

	if (slotNum < 0 || slotNum > numOfSlots - 1)
		return E_OUTOFBOUND;

	int BlockType = getBlockType(blockNum);
	FILE *disk = fopen(DISK_PATH, "rb+");

	if (BlockType == REC) {
		/* offset :
		 *          size of blocks coming before current block ( = blockNum * BLOCK_SIZE ) +
		 *          header size ( = 32 ) +
		 *          slot_map size ( = numSlots ) +
		 *          size of records coming before current record ( = slotNum * numAttrs * ATTR_SIZE )
		 */
		fseek(disk, blockNum * BLOCK_SIZE + 32 + numOfSlots + slotNum * numAttrs * ATTR_SIZE, SEEK_SET);
		fwrite(rec, numAttrs * ATTR_SIZE, 1, disk);
		fclose(disk);
		return SUCCESS;
	} else if (BlockType == IND_INTERNAL) {
		//TODO
	} else if (BlockType == IND_LEAF) {
		//TODO
	} else {
		fclose(disk);
		return FAILURE;
	}
}

/*
 * Reads relation catalogue entry from disk
 */
int getRelCatEntry(int relationId, Attribute *relcat_entry) {
	if (relationId < 0 || relationId >= MAX_OPEN)
		return E_OUTOFBOUND;

	if (OpenRelations::checkIfRelationOpen(relationId) == FAILURE)
		return E_NOTOPEN;

	char relName[16];
	OpenRelations::getRelationName(relationId, relName);

	for (int i = 0; i < 20; i++) {
		getRecord(relcat_entry, 4, i);
		if (strcmp(relcat_entry[0].sval, relName) == 0)
			return SUCCESS;
	}
	return FAILURE;
}

/*
 * Writes relation catalogue entry into disk
 */
int setRelCatEntry(int relationId, Attribute *relcat_entry) {
	if (relationId < 0 || relationId >= MAX_OPEN)
		return E_OUTOFBOUND;

	if (OpenRelations::checkIfRelationOpen(relationId) == FAILURE)
		return E_NOTOPEN;

	char relName[16];
	OpenRelations::getRelationName(relationId, relName);

	Attribute relcat_entry1[6];
	for (int i = 0; i < 20; i++) {
		getRecord(relcat_entry1, 4, i);
		if (strcmp(relcat_entry1[0].sval, relName) == 0) {
			setRecord(relcat_entry, 4, i);
			return SUCCESS;
		}
	}
}

/*
 * Reads attribute catalogue entry from disk for the given attribute name of a given relation
 */
int getAttrCatEntry(int relationId, char attrname[16], Attribute *attrcat_entry) {
	if (relationId < 0 || relationId >= MAX_OPEN)
		return E_OUTOFBOUND;

	if (OpenRelations::checkIfRelationOpen(relationId) == FAILURE)
		return E_NOTOPEN;

	char relName[16];
	OpenRelations::getRelationName(relationId, relName);

	int curr_block = 5;
	int next_block = -1;
	while (curr_block != -1) {
		struct HeadInfo header;
		header = getHeader(curr_block);
		next_block = header.rblock;
		for (int i = 0; i < 20; i++) {
			getRecord(attrcat_entry, curr_block, i);
			if (strcmp(attrcat_entry[0].sval, relName) == 0) {
				if (strcmp(attrcat_entry[1].sval, attrname) == 0)
					return SUCCESS;
			}
		}
		curr_block = next_block;
	}
	return E_ATTRNOTEXIST;
}

/*
 * Reads attribute catalogue entry from disk for the given attribute name of a given relation
 */
int getAttrCatEntry(int relationId, int offset, Attribute *attrcat_entry) {
	if (relationId < 0 || relationId >= MAX_OPEN)
		return E_OUTOFBOUND;

	if (OpenRelations::checkIfRelationOpen(relationId) == FAILURE)
		return E_NOTOPEN;

	char relName[16];
	OpenRelations::getRelationName(relationId, relName);

	int curr_block = 5;
	int next_block = -1;
	while (curr_block != -1) {
		struct HeadInfo header;
		header = getHeader(curr_block);
		next_block = header.rblock;
		for (int i = 0; i < 20; i++) {
			getRecord(attrcat_entry, curr_block, i);
			if (strcmp(attrcat_entry[0].sval, relName) == 0) {
				if (static_cast<int>(attrcat_entry[5].nval) == offset)
					return SUCCESS;
			}
		}
		curr_block = next_block;
	}
	return E_ATTRNOTEXIST;
}


/*
 * Deletes the given block from the disk
 *      - Clears the Block Data
 *      - Marks the Block UNUSED_BLK in Block Allocation Map
 */
int deleteBlock(int blockNum) {
	FILE *disk;
	disk = fopen(DISK_PATH, "rb+");

	/* Clear the data present in the block */
	fseek(disk, BLOCK_SIZE * blockNum, SEEK_SET);
	for (int i = 0; i < BLOCK_SIZE; i++)
		fputc(0, disk);

	/* Mark this block as UNUSED in the Block Allocation Map */
	fseek(disk, blockNum, SEEK_SET);
	fputc((unsigned char) UNUSED_BLK, disk);
	fclose(disk);

	return SUCCESS;
}


/*
 * Delete a Relation from the Relation Catalog
 *      - update the header & slot map of Relation Catalog
 *      - update the relation catalog record present in the Relation Catalog Block
 *          - decrease the number of records of RELCAT by one
 *      - update the attribute catalog record also present in the Relation Catalog Block
 *          - decrease the number of records of ATTRCAT by number of attributes in the deleted relation
 *      - delete entry of relation being deleted in the relation catalog from the disk
 */
int deleteRelCatEntry(recId relcat_recid, Attribute relcat_rec[6]) {
	struct HeadInfo relcat_header = getHeader(4);
	relcat_header.numEntries = relcat_header.numEntries - 1;
	setHeader(&relcat_header, 4);
	unsigned char relcat_slotmap[20];
	getSlotmap(relcat_slotmap, 4);
	relcat_slotmap[relcat_recid.slot] = SLOT_UNOCCUPIED;
	setSlotmap(relcat_slotmap, 20, relcat_recid.block);

	getRecord(relcat_rec, 4, 0);
	relcat_rec[2].nval = relcat_rec[2].nval - 1;
	setRecord(relcat_rec, 4, 0);

	int no_of_attrs = relcat_rec[1].nval;
	getRecord(relcat_rec, 4, 1);
	relcat_rec[2].nval = relcat_rec[2].nval - no_of_attrs;
	setRecord(relcat_rec, 4, 1);

	FILE *disk = fopen(DISK_PATH, "rb+");
	fseek(disk, (relcat_recid.block) * BLOCK_SIZE + HEADER_SIZE + SLOTMAP_SIZE_RELCAT_ATTRCAT +
	            relcat_recid.slot * NO_OF_ATTRS_RELCAT_ATTRCAT * ATTR_SIZE, SEEK_SET);
	for (int i = 0; i < 16 * 6; i++)
		fputc(0, disk);
	fclose(disk);

	return SUCCESS;
}

/*
 * Deletes a Single Attribute Catalog Entry for a given Attribute of a Relation
 *      - Clears the Disk Entry
 *      - Updates Header and SlotMap of Attribute Catalog
 *      - Removes Indexing on the Attribute (TODO: TBD)
 *      - Deletes Attrbute Catalog's Record Block if multiple blocks had been allocated and current block becomes empty
 */
int deleteAttrCatEntry(recId attrcat_recid) {
	/* Clear the Attribute Catalog Record present in the given (Slot & Block) of the Disk */
	FILE *disk = fopen(DISK_PATH, "rb+");
	fseek(disk, (attrcat_recid.block) * BLOCK_SIZE + HEADER_SIZE + SLOTMAP_SIZE_RELCAT_ATTRCAT +
	            (attrcat_recid.slot) * NO_OF_ATTRS_RELCAT_ATTRCAT * ATTR_SIZE, SEEK_SET);
	for (int i = 0; i < ATTR_SIZE * NO_OF_ATTRS_RELCAT_ATTRCAT; i++)
		fputc(0, disk);
	fclose(disk);

	/* Update the Header and SlotMap for Attribute Catalog */
	struct HeadInfo header = getHeader(attrcat_recid.block);
	header.numEntries = header.numEntries - 1;
	setHeader(&header, attrcat_recid.block);
	unsigned char slotmap[20];
	getSlotmap(slotmap, attrcat_recid.block);
	slotmap[attrcat_recid.slot] = SLOT_UNOCCUPIED;
	setSlotmap(slotmap, 20, attrcat_recid.block);

	/* Removing Indexing on the Attribute */
	// TODO: When indexing use this root_block enty to determine if its present and if yes, remove the indexing on the Attribute
	Attribute attrcat_rec[6];
	getRecord(attrcat_rec, attrcat_recid.block, attrcat_recid.slot);
	int root_block = attrcat_rec[4].nval;

	/*
	 * NOTE: Multiple blocks have been allocated to Attribute Catalog Relation
	 * Delete a Block allocated to an attribute in case it becomes empty
	 */
	if (header.numEntries == 0) {
		/* Standard Linked List Delete for a Block */
		HeadInfo prev_header = getHeader(header.lblock);
		prev_header.rblock = header.rblock;
		setHeader(&prev_header, header.lblock);

		if (header.rblock != -1) {
			HeadInfo next_header = getHeader(header.rblock);
			next_header.lblock = header.lblock;
			setHeader(&next_header, header.rblock);
		}
		deleteBlock(attrcat_recid.block);
	}
	return SUCCESS;
}

// TODO : review in which file this function should be
void add_disk_metainfo() {
	union Attribute rec[6];
	struct HeadInfo *H = (struct HeadInfo *) malloc(sizeof(struct HeadInfo));

	// TODO: use the set_headerInfo, make_relcatrec and make_attrcatrec function in schema.cpp
	/*
	 * Set the header for Block 4 - First Block of Relation Catalog
	 */
	H->blockType = REC;
	H->pblock = -1;
	H->lblock = -1;
	H->rblock = -1;
	H->numEntries = 2;
	H->numAttrs = 6;
	H->numSlots = 20;
	setHeader(H, 4);

	/*
	 * Set the slot allocation map for Block 4
	 */
	unsigned char slot_map[20];
	for (int i = 0; i < 20; i++) {
		if (i == 0 || i == 1)
			slot_map[i] = SLOT_OCCUPIED;
		else
			slot_map[i] = SLOT_UNOCCUPIED;
	}
	setSlotmap(slot_map, 20, 4);

	/*
	 * Create and Add 2 Records into Block 4 (Relation Catalog)
	 *  - First for Relation Catalog Relation (Block 4 itself is used for this relation)
	 *  - Second for Attribute Catalog Relation (Block 5 is used for this relation)
	 */
	strcpy(rec[0].sval, "RELATIONCAT");
	rec[1].nval = 6;
	rec[2].nval = 2;
	rec[3].nval = 4;
	rec[4].nval = 4;
	rec[5].nval = 20;
	setRecord(rec, 4, 0);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	rec[1].nval = 6;
	rec[2].nval = 12;
	rec[3].nval = 5;
	rec[4].nval = 5;
	rec[5].nval = 20;
	setRecord(rec, 4, 1);

	/*
	 * Set the header for Block 5 - First Block of Attribute Catalog
	 */
	H->blockType = REC;
	H->pblock = -1;
	H->lblock = -1;
	H->rblock = -1;
	H->numEntries = 12;
	H->numAttrs = 6;
	H->numSlots = 20;
	setHeader(H, 5);

	/*
	 * Set the slot allocation map for Block 5
	 */
	for (int i = 0; i < 20; i++) {
		if (i >= 0 && i <= 11)
			slot_map[i] = SLOT_OCCUPIED;
		else
			slot_map[i] = SLOT_UNOCCUPIED;
	}
	setSlotmap(slot_map, 20, 5);

	/*
	 *  Create Entries for every attribute for Relation Catalog and Attribute Catalog
	 */
	strcpy(rec[0].sval, "RELATIONCAT");
	strcpy(rec[1].sval, "RelName");
	rec[2].nval = STRING;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 0;
	setRecord(rec, 5, 0);

	strcpy(rec[0].sval, "RELATIONCAT");
	strcpy(rec[1].sval, "#Attributes");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 1;
	setRecord(rec, 5, 1);

	strcpy(rec[0].sval, "RELATIONCAT");
	strcpy(rec[1].sval, "#Records");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 2;
	setRecord(rec, 5, 2);

	strcpy(rec[0].sval, "RELATIONCAT");
	strcpy(rec[1].sval, "FirstBlock");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 3;
	setRecord(rec, 5, 3);

	strcpy(rec[0].sval, "RELATIONCAT");
	strcpy(rec[1].sval, "LastBlock");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 4;
	setRecord(rec, 5, 4);

	strcpy(rec[0].sval, "RELATIONCAT");
	strcpy(rec[1].sval, "#Slots");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 5;
	setRecord(rec, 5, 5);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	strcpy(rec[1].sval, "RelName");
	rec[2].nval = STRING;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 0;
	setRecord(rec, 5, 6);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	strcpy(rec[1].sval, "AttributeName");
	rec[2].nval = STRING;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 1;
	setRecord(rec, 5, 7);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	strcpy(rec[1].sval, "AttributeType");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 2;
	setRecord(rec, 5, 8);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	strcpy(rec[1].sval, "PrimaryFlag");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 3;
	setRecord(rec, 5, 9);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	strcpy(rec[1].sval, "RootBlock");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 4;
	setRecord(rec, 5, 10);

	strcpy(rec[0].sval, "ATTRIBUTECAT");
	strcpy(rec[1].sval, "Offset");
	rec[2].nval = NUMBER;
	rec[3].nval = -1;
	rec[4].nval = -1;
	rec[5].nval = 5;
	setRecord(rec, 5, 11);

}

/*
 * Compare two attributes based on their type
 */
int compareAttributes(union Attribute attr1, union Attribute attr2, int attrType) {
	if (attrType == STRING) {
		return strcmp(attr1.sval, attr2.sval);
	}

	if (attrType == NUMBER) {
		if (attr1.nval < attr2.nval)
			return -1;
		else if (attr1.nval == attr2.nval)
			return 0;
		else
			return 1;
	}
}