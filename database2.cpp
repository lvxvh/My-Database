
#include "database.h"
#include "math.h"
#include <algorithm>
#include "string.h"
#include <ctime>
#include <random>



/*
 * Method : getNode
 * --------------------------------------
 * This function is used to read a node block in the index file,
 * it will analyse the information and build a Node pointer;
 *
 *
 */

Node* BplusTree::getNode(int pos) {
	if (pos == none) return NULL;
	else {
		idxfile.seekg(pos, ios::beg);
		Node* cur = new Node();
		char buffer[nodesize];
		idxfile.read(buffer, nodesize);
		char c = buffer[0];
		if (c == '0') cur->isLeaf = 0;
		else cur->isLeaf = 1;
		char _parent[4];
		for (int i = 0; i < 4; i++) {
			_parent[i] = buffer[i + 1];
		}
		cur->parent = (int&)_parent;
		char _next[4];
		for (int i = 0; i < 4; i++) {
			_next[i] = buffer[i + 5];
		}
		cur->next = (int&)_next;
		char _pre[4];
		for (int i = 0; i < 4; i++) {
			_pre[i] = buffer[i + 9];
		}
		cur->pre = (int&)_pre;
		for (int i = 0; i < (ORDER + 1); i++) {
			int key, pos;
			char _key[4];
			for (int j = 0; j < 4; j++) {
				_key[j] = buffer[j + 8 * i + 13];
			}
			key = (int&)_key;
			char _pos[4];
			for (int j = 0; j < 4; j++) {
				_pos[j] = buffer[j + 8 * i + 17];
			}
			pos = (int&)_pos;
			if (key != -1) {
				pair<int, int>child(key, pos);
				cur->children.push_back(child);
			}
			else break;
		}
		return cur;
	}
}

/*
 * Method : putNode
 * ------------------------------------------
 * After making or modifying a node,use this function to put the
 * node as a block of information back to the index file;
 *
 *
 */

void BplusTree::putNode(Node* cur) {
	char buffer[nodesize];
	if (cur->isLeaf == 0) buffer[0] = '0';
	else buffer[0] = '1';
	char* _parent = (char*)&(cur->parent);
	for (int i = 0; i < 4; i++) {
		buffer[i + 1] = _parent[i];
	}
	char* _next = (char*)&(cur->next);
	for (int i = 0; i < 4; i++) {
		buffer[i + 5] = _next[i];
	}
	char* _pre = (char*)&(cur->pre);
	for (int i = 0; i < 4; i++) {
		buffer[i + 9] = _pre[i];
	}
	int len = cur->children.size();
	for (int i = 0; i < len; i++) {
		char* _key = (char*)&(cur->children[i].first);
		for (int j = 0; j < 4; j++) {
			buffer[j + 8 * i + 13] = _key[j];
		}
		char* _pos = (char*)&(cur->children[i].second);
		for (int j = 0; j < 4; j++) {
			buffer[j + 8 * i + 17] = _pos[j];
		}
	}
	int gap = ORDER + 1 - len;
	char* _none = (char*)&none;
	if (gap > 0) {
		for (int i = 0; i < gap; i++) {
			for (int j = 0; j < 4; j++) {
				buffer[13 + 8 * (len + i) + j] = _none[j];
				buffer[13 + 8 * (len + i) + j + 4] = _none[j];
			}
		}
	}
	idxfile.write(buffer, nodesize);
}

/*
 * Method : searchNode
 * ----------------------------------------------
 * Acorrding to the key,this function will find the leaf node which contains
 * the key in the B+ tree,and then you can operate on this node;
 *
 *
 */

Node* BplusTree::searchNode(int key) {
	Node* cur = getNode(root);
	while (!cur->isLeaf) {
		int sizeofC = cur->children.size();
		/*the key is smaller than the first key in current node*/
		if (cur->children[0].first > key) {
			delete cur;
			cur = NULL;
			return NULL;
		}
		/*the key is bigger than the last key*/
		else if (key >= cur->children[sizeofC - 1].first) {
			int newpos = cur->children[sizeofC - 1].second;
			delete cur;
			cur = getNode(newpos);
		}
		/*else*/
		else {
			for (int i = 1; i < sizeofC; i++) {
				if (key < cur->children[i].first) {
					int newpos = cur->children[i - 1].second;
					delete cur;
					cur = getNode(newpos);
					break;
				}
			}
		}
	}
	/*reach to the leaf node and search*/
	int sizeofC = cur->children.size();
	for (int i = 0; i < sizeofC; i++) {
		if (key == cur->children[i].first) {
			return cur;
		}
	}
	delete cur;
	cur = NULL;
	return NULL;
}

/*
 * Method : solveOverflow
 * -------------------------------------------------
 * When the number of the key is bigger than order,use this function to crack
 * current node,this action will affect the parent node of the current node,
 * so it's necessary to slove overflow of its parent node;
 *
 */

void BplusTree::solveOverflow(Node* cur) {
	int len = cur->children.size();
	if (len > ORDER) {
		/*overflow happened*/
		idxfile.seekg(-nodesize, ios::cur);
		int posOfL = idxfile.tellg();
		idxfile.seekg(0, ios::end);
		int posOfR = idxfile.tellg();
		/*if the current node is root node:*/
		if (cur->parent == none) {
			Node* newRoot = new Node;
			Node* rchild = new Node;
			pair<int, int>leftP(cur->children[0].first, posOfL);
			pair<int, int>rightP(cur->children[len / 2].first, posOfR);
			newRoot->isLeaf = 0;
			newRoot->children.push_back(leftP);
			newRoot->children.push_back(rightP);
			if (!cur->isLeaf) rchild->isLeaf = 0;
			else rchild->isLeaf = 1;
			for (int i = len / 2; i < len; i++) {
				rchild->children.push_back(cur->children[i]);
				if (!cur->isLeaf) {
					Node* child = getNode(cur->children[i].second);
					child->parent = posOfR;
					idxfile.seekg(-nodesize, ios::cur);
					putNode(child);
					delete child;
					child = NULL;
				}
			}
			root = posOfR + nodesize;
			cur->children.erase(cur->children.begin() + len / 2, cur->children.end());
			cur->next = posOfR;
			rchild->pre = posOfL;
			cur->parent = posOfR + nodesize;
			rchild->parent = posOfR + nodesize;
			idxfile.seekg(posOfL, ios::beg);
			putNode(cur);
			idxfile.seekg(0, ios::end);
			putNode(rchild);
			delete rchild;
			rchild = NULL;
			putNode(newRoot);
			delete newRoot;
			newRoot = NULL;
		}
		/*the current node isn't root node*/
		else {
			Node* rchild = new Node;
			Node* parent = getNode(cur->parent);
			idxfile.seekg(-nodesize, ios::cur);
			int posOfP = idxfile.tellg();
			int lenofp = parent->children.size();
			if (!cur->isLeaf) rchild->isLeaf = 0;
			else rchild->isLeaf = 1;
			for (int i = len / 2; i < len; i++) {
				rchild->children.push_back(cur->children[i]);
				if (!cur->isLeaf) {
					Node* child = getNode(cur->children[i].second);
					child->parent = posOfR;
					idxfile.seekg(-nodesize, ios::cur);
					putNode(child);
					delete child;
					child = NULL;
				}
			}
			cur->children.erase(cur->children.begin() + len / 2, cur->children.end());

			if (cur->next != none) {
				Node* next = getNode(cur->next);
				rchild->next = cur->next;
				next->pre = posOfR;
				cur->next = posOfR;
				rchild->pre = posOfL;
				idxfile.seekg(-nodesize, ios::cur);
				putNode(next);
				delete next;
				next = NULL;
			}

			else {
				rchild->pre = posOfL;
				cur->next = posOfR;
			}
			/*insert new key in its parent node*/
			int newKey = rchild->children[0].first;
			/*the new key is bigger than the last node*/
			if (newKey > parent->children[lenofp - 1].first) {
				pair<int, int>newP(newKey, posOfR);
				parent->children.insert(parent->children.end(), newP);
				rchild->parent = posOfP;
			}
			/*the new key should be insert in the middle of parent node*/
			else {
				for (int i = 1; i < lenofp; i++) {
					if (newKey < parent->children[i].first) {
						pair<int, int>newP(newKey, posOfR);
						parent->children.insert(parent->children.begin() + i, newP);
						rchild->parent = posOfP;
						break;
					}
				}
			}
			idxfile.seekg(posOfL, ios::beg);
			putNode(cur);
			idxfile.seekg(0, ios::end);
			putNode(rchild);
			delete rchild;
			rchild = NULL;
			idxfile.seekg(cur->parent, ios::beg);
			delete cur;
			cur = NULL;
			putNode(parent);
			/*check whether the parent of current node is overflow*/
			solveOverflow(parent);
		}
	}
	delete cur;
	cur = NULL;
}

/*
 * Method : insert
 * ----------------------------------------------------
 * Use this function to insert a new key int the B+ tree.
 * It will find the suitable place to insert the key,
 * and if the key is already exist,it will return false;
 *
 */

bool BplusTree::insert(int key) {
	Node* cur = getNode(root);
	/*find the place first*/
	/*internal node*/
	while (!cur->isLeaf) {
		int sizeofC = cur->children.size();
		/*key is smaller than the first key of the current node*/
		if (key < cur->children[0].first) {
			cur->children[0].first = key;
			idxfile.seekg(- nodesize, ios::cur);	
			putNode(cur);
			int newpos = cur->children[0].second;
			delete cur;
			cur = getNode(newpos);
		}
		/*is bigger than the last key*/
		else if (key > cur->children[sizeofC - 1].first) {
			int newpos = cur->children[sizeofC - 1].second;
			delete cur;
			cur = getNode(newpos);
		}
		/*else in the middle*/
		else {
			for (int i = 1; i < sizeofC; i++) {
				if (key <= cur->children[i].first) {
					if (key == cur->children[i - 1].first || key == cur->children[i].first) {
						delete cur;
						cur = NULL;
						return 0;
					}
					else {
						int newpos = cur->children[i - 1].second;
						delete cur;
						cur = getNode(newpos);
						break;
					}
				}
			}
		}
	}
	/*current node is leaf node now,insert first and consider overflow later*/
	int sizeofC = cur->children.size();
	/*if the key is the first key of the whole data*/
	if (sizeofC == 0) {  
		idxfile.seekg(-nodesize, ios::cur);
		pair<int, int>newP(key, 4 + count*datasize);
		cur->children.push_back(newP);
		putNode(cur);
		count++;
	}
	/*the tree contains only one key and it's the same of the new key*/
	else if ((sizeofC == 1) && (key == cur->children[0].first)) {
		delete cur;
		cur = NULL;
		return 0;
	}
	/*if the key is smaller than the first key of the node*/
	else if (key < cur->children[0].first) {
		pair<int, int>newP(key, count*datasize + 4);
		cur->children.insert(cur->children.begin(), newP);
		idxfile.seekg(-nodesize, ios::cur);
		putNode(cur);
		count++;
	}
	/*bigger than the last key*/
	else if (key > cur->children[sizeofC - 1].first) {
		pair<int, int>newP(key, count*datasize + 4);
		cur->children.insert(cur->children.end(), newP);
		idxfile.seekg(-nodesize, ios::cur);
		putNode(cur);
		count++;
	}
	/*else in the middle*/
	else {
		for (int i = 1; i < sizeofC; i++) {
			if (key <= cur->children[i].first) {
				if (key == cur->children[i - 1].first || key == cur->children[i].first) {
					delete cur;
					cur = NULL;
					return 0;
				}
				else {
					pair<int, int>newP(key, count*datasize + 4);
					cur->children.insert(cur->children.begin() + i, newP);
					idxfile.seekg(-nodesize, ios::cur);
					putNode(cur);
					count++;
					break;
				}
			}
		}
	}
	solveOverflow(cur);
	return 1;
}

/*
 * Method : remove
 * ---------------------------------------
 * Use this function to remove a key from the B+ tree, 
 * After do this ,check the node size and solve underflow.
 *
 *
 */

bool BplusTree::remove(int key) {
	Node* cur = searchNode(key);
	/*the key exist*/
	if (cur != NULL) {
		idxfile.seekg(-nodesize, ios::cur);
		int posOfC = idxfile.tellg();
		int len = cur->children.size();
		if (cur->children[0].first == key) {
			cur->children.erase(cur->children.begin());
			int newKey;
			if (cur->children.size() > 0) {
				newKey = cur->children[0].first;
			}
			Node* father = getNode(cur->parent);
			while (father != NULL) {
				len = father->children.size();
				for (int i = 0; i < len; i++) {
					if (father->children[i].first == key) {
						father->children[i].first = newKey;
						idxfile.seekg(-nodesize, ios::cur);
						putNode(father);
						break;
					}
				}
				int newpos = father->parent;
				delete father;
				father = getNode(newpos);
			}
			delete father;
			father = NULL;
		}
		else {
			for (int i = 1; i < len; i++) {
				if (cur->children[i].first == key) {
					cur->children.erase(cur->children.begin() + i);
					break;
				}
			}
		}
		idxfile.seekg(posOfC, ios::beg);
		putNode(cur);
		solveUnderflow(cur);
		return 1;
	}
	else {
		delete cur;
		cur = NULL;
		return 0;
	}
}

/*
 * Method : replace
 * -----------------------------------------
 * Use this function to modify a existed key,
 * if the key do not exist,it will return false;
 *
 *
 */

bool BplusTree::replace(int key, int newKey) {
	Node* cur = searchNode(key);
	if (cur != NULL) {
		int len = cur->children.size();
		for (int i = 0; i < len; i++) {
			if (cur->children[i].first == key) {
				cur->children[i].first = newKey;
				idxfile.seekg(-nodesize, ios::cur);
				putNode(cur);
				while (cur->parent != none) {
					if (i == 0) {
						int newpos = cur->parent;
						delete cur;
						cur = getNode(newpos);
						len = cur->children.size();
						for (int j = 0; j < len; j++) {
							if (cur->children[j].first == key) {
								cur->children[j].first = newKey;
								i = j;
								break;
							}
						}
						idxfile.seekg(-nodesize, ios::cur);
						putNode(cur);
					}
					else break;
				}
				delete cur;
				cur = NULL;
				break;
			}
		}
		return 1;
	}
	else {
		delete cur;
		cur = NULL;
		return 0;
	}
}

/*
 * Method : solveUnderflow
 * ---------------------------------------------------
 * If the size of the node is lower then than the min size,underflow happens,
 * in this case ,this node will borrow a key from its neighborhood or it will
 * be merged into its neighborhood;
 *
 */

void BplusTree::solveUnderflow(Node* cur) {
	idxfile.seekg(-nodesize, ios::cur);
	int posOfC = idxfile.tellg();
	/*current node is root node*/
	if (cur->parent == none) {
		if (cur->children.size() == 1 && cur->isLeaf == 0) {
			root = cur->children[0].second;
			Node* newRoot = getNode(cur->children[0].second);
			newRoot->parent = none;
			idxfile.seekg(-nodesize, ios::cur);
			putNode(newRoot);
			delete newRoot;
			newRoot = NULL;
			delete cur;
			cur = NULL;
		}
	}
	/*is not root node*/
	else if (cur->children.size() < minsize) {
		Node* left = getNode(cur->pre);
		int posOfL, leftLen = 0;
		if (left != NULL) {
			idxfile.seekg(-nodesize, ios::cur);
			posOfL = idxfile.tellg();
			leftLen = left->children.size();
		}
		Node* right = getNode(cur->next);
		int posOfR, rightLen = 0;
		if (right != NULL) {
			idxfile.seekg(-nodesize, ios::cur);
			posOfR = idxfile.tellg();
			rightLen = right->children.size();
		}
		/*borrow from left neighborhood*/
		if (leftLen > minsize) {
			pair<int, int>newP(left->children[leftLen - 1]);
			int newKey = newP.first;
			int key = cur->children[0].first;
			cur->children.insert(cur->children.begin(), newP);
			if (!cur->isLeaf) {
				Node* newChild = getNode(newP.second);
				newChild->parent = posOfC;
				idxfile.seekg(-nodesize, ios::cur);
				putNode(newChild);
				delete newChild;
				newChild = NULL;
			}
			left->children.erase(left->children.end() - 1);
			idxfile.seekg(posOfL, ios::beg);
			putNode(left);
			delete left;
			left = NULL;
			idxfile.seekg(posOfC, ios::beg);
			putNode(cur);
			while (cur->parent != none) {
				int newpos = cur->parent;
				delete cur;
				cur = getNode(newpos);
				int len = cur->children.size();
				bool jumpout = 0;
				for (int i = 0; i < len; i++) {
					if (cur->children[i].first == key) {
						cur->children[i].first = newKey;
						if (i != 0) jumpout = 1;
						break;
					}
				}
				idxfile.seekg(-nodesize, ios::cur);
				putNode(cur);
				if (jumpout) break;
			}
			delete cur;
			cur = NULL;
			delete right;
			right = NULL;
		}
		/*borrow from right*/
		else if (rightLen > minsize) {
			int key = right->children[0].first;
			int newKey = right->children[1].first;
			pair<int, int>newP(right->children[0]);
			cur->children.insert(cur->children.end(), newP);
			if (!cur->isLeaf) {
				Node* newChild = getNode(newP.second);
				newChild->parent = posOfC;
				idxfile.seekg(-nodesize, ios::cur);
				putNode(newChild);
				delete newChild;
				newChild = NULL;
			}
			right->children.erase(right->children.begin());
			idxfile.seekg(posOfR, ios::beg);
			putNode(right);
			idxfile.seekg(posOfC, ios::beg);
			putNode(cur);
			delete cur;
			cur = right;
			while (cur->parent != none) {
				int newpos = cur->parent;
				delete cur;
				cur = getNode(newpos);
				int len = cur->children.size();
				bool jumpout = 0;
				for (int i = 0; i < len; i++) {
					if (cur->children[i].first == key) {
						cur->children[i].first = newKey;
						if (i != 0) jumpout = 1;
						break;
					}
				}
				idxfile.seekg(-nodesize, ios::cur);
				putNode(cur);
				if (jumpout) break;
			}
			delete cur;
			cur = NULL;
			delete left;
			left = NULL;
		}
		/*merge*/
		else {
			/*merge to left*/
			if (leftLen > 0) {
				int len = cur->children.size();
				Node* parent = getNode(cur->parent);
				idxfile.seekg(-nodesize, ios::cur);
				int posOfP = idxfile.tellg();
				int lenofp = parent->children.size();
				int key = cur->children[0].first;
				for (int i = 0; i < len; i++) {
					left->children.push_back(cur->children[i]);
					if (!cur->isLeaf) {
						Node* newChild = getNode(cur->children[i].second);
						newChild->parent = posOfL;
						idxfile.seekg(-nodesize, ios::cur);
						putNode(newChild);
						delete newChild;
						newChild = NULL;
					}
				}
				if (right != NULL) {
					left->next = posOfR;
					right->pre = posOfL;
					idxfile.seekg(posOfR, ios::beg);
					putNode(right);
					delete right;
					right = NULL;
				}
				else {
					left->next = -1;
				}
				idxfile.seekg(posOfL, ios::beg);
				putNode(left);
				delete left;
				left = NULL;
				for (int i = 0; i < lenofp; i++) {
					if (parent->children[i].first == key) {
						parent->children.erase(parent->children.begin() + i);
						idxfile.seekg(posOfP, ios::beg);
						putNode(parent);
						if (i == 0) {
							int newKey = parent->children[0].first;
							Node* father = getNode(parent->parent);
							while (father != NULL) {
								bool jumpout = 0;
								len = father->children.size();
								for (int i = 0; i < len; i++) {
									if (father->children[i].first == key) {
										father->children[i].first = newKey;
										idxfile.seekg(-nodesize, ios::cur);
										putNode(father);
										if (i != 0) jumpout = 1;
										break;
									}
								}
								if (jumpout) break;
								int newpos = father->parent;
								delete father;
								father = getNode(newpos);
							}
							delete father;
							father = NULL;
						}
						idxfile.seekg(posOfP + nodesize, ios::beg);
						solveUnderflow(parent);
						delete cur;
						cur = NULL;
						break;
					}
				}
			}
			/*merge to right*/
			else {
				int len = cur->children.size();
				int key = cur->children[0].first;
				int oldKey = right->children[0].first;
				for (int i = 0; i < len; i++) {
					right->children.insert(right->children.begin(), cur->children[len - 1 - i]);
					if (!cur->isLeaf) {
						Node* newChild = getNode(cur->children[i].second);
						newChild->parent = posOfR;
						idxfile.seekg(-nodesize, ios::cur);
						putNode(newChild);
						delete newChild;
						newChild = NULL;
					}
				}
				if (left != NULL) {
					left->next = posOfR;
					right->pre = posOfL;
					idxfile.seekg(posOfL, ios::beg);
					putNode(left);
					delete left;
					left = NULL;
				}
				else {
					right->pre = -1;
				}
				idxfile.seekg(posOfR, ios::beg);
				putNode(right);
				int newpos = right->parent;
				delete right;
				right = NULL;
				Node* newFather = getNode(newpos);
				while (newFather != NULL) {
					bool jumpout = 0;
					len = newFather->children.size();
					for (int i = 0; i < len; i++) {
						if (newFather->children[i].first == oldKey) {
							newFather->children[i].first = key;
							idxfile.seekg(-nodesize, ios::cur);
							putNode(newFather);
							if (i != 0) jumpout = 1;
							break;
						}
					}
					if (jumpout) break;
					int newpos = newFather->parent;
					delete newFather;
					newFather = getNode(newpos);
				}
				delete newFather;
				newFather = NULL;
				Node* parent = getNode(cur->parent);
				idxfile.seekg(-nodesize, ios::cur);
				int posOfP = idxfile.tellg();
				int lenofp = parent->children.size();
				for (int i = 0; i < lenofp; i++) {
					if (parent->children[i].first == key) {
						parent->children.erase(parent->children.begin() + i);
						idxfile.seekg(posOfP, ios::beg);
						putNode(parent);
						if (i == 0) {
							int newKey = parent->children[0].first;
							Node* father = getNode(parent->parent);
							while (father != NULL) {
								bool jumpout = 0;
								len = father->children.size();
								for (int i = 0; i < len; i++) {
									if (father->children[i].first == key) {
										father->children[i].first = newKey;
										idxfile.seekg(-nodesize, ios::cur);
										putNode(father);
										if (i != 0) jumpout = 1;
										break;
									}
								}
								if (jumpout) break;
								int newpos = father->parent;
								delete father;
								father = getNode(newpos);
							}
							delete father;
							father = NULL;
						}
						idxfile.seekg(posOfP + nodesize, ios::beg);
						solveUnderflow(parent);
						delete cur;
						cur = NULL;
						break;
					}
				}
			}
		}
	}
	else {
		delete cur;
		cur = NULL;
	}
}

/*
 * Method : save
 * ---------------------------------
 * Save the infomation about the position of the root node and 
 * the count of the tree then close the index file;
 *
 */

void BplusTree::save() {
	idxfile.seekg(0, ios::beg);
	idxfile.write((char*)&count, INTSIZE);
	idxfile.write((char*)&root, INTSIZE);
	idxfile.close();
}

/*
 * Method : load
 * ------------------------------------
 * Open the index file and get the position of root node and the count,
 * if the file do not exist,it will create a new file and insert a root node in it.
 *
 */

void BplusTree::load() {
	idxfile.open("index.idx",ios::in|ios::out|ios::binary);
	if (!idxfile) {
		idxfile.open("index.idx", ios::app);
		idxfile.close();
		idxfile.open("index.idx", ios::in | ios::out | ios::binary);
		count = 0;
		idxfile.write((char*)&count, INTSIZE);
		root = 8;
		idxfile.write((char*)&root, INTSIZE);
		Node* root = new Node();
		root->isLeaf = 1;
		root->parent = none;
		root->next = none;
		root->pre = none;
		putNode(root);
		delete root;
		root = NULL;
	}
	else {
		char countInfo[4];
		idxfile.read(countInfo, INTSIZE);
		count = (int&)countInfo;
		char rootpos[4];
		idxfile.read(rootpos, INTSIZE);
		root = (int&)rootpos;
	}
}

/*
 * Method : clear
 * ----------------------------------------
 * clear all information in the index file,
 * and then insert a new root node into it.
 */

void BplusTree::clear() {
	idxfile.close();
	idxfile.open("index.idx",  ios::out | ios::binary);
	idxfile.close();
	idxfile.open("index.idx", ios::in | ios::out | ios::binary);
	count = 0;
	idxfile.write((char*)&count, INTSIZE);
	root = 8;
	idxfile.write((char*)&root, INTSIZE);
	Node* root = new Node();
	root->isLeaf = 1;
	root->parent = none;
	root->next = none;
	root->pre = none;         
	putNode(root);
	delete root;
	root = NULL;
}

/*
 * Method : open
 * -----------------------------------------------
 * Open the data file,and then load the index,
 * if the file do not exist,it will create one.
 *
 *
 */

void Database::open() {
	datafile.open("datafile.dat", ios::in | ios::out | ios::binary);
	if (!datafile) {
		datafile.open("datafile.dat", ios::app);
		datafile.close();
		datafile.open("datafile.dat", ios::in | ios::out | ios::binary);
		size = 0;
		datafile.write((char*)&size, INTSIZE);
	}
	index.load();
}

/*
 * Method : close
 * -------------------------------------------------
 * Write down the size of the data,
 * and then save the index and close the data file
 *
 */

void Database::close() {
	datafile.seekg(0, ios::beg);
	datafile.write((char*)&size, INTSIZE);
	index.save();
	datafile.close();
}

/*
 * Method : store
 * --------------------------------------------------------
 * Basic function of database,store the key of the data in
 * the index file,and then store the data in the data file.
 *
 */

bool Database::store(int key,int data) {
	if (index.insert(key)) {
		datafile.seekg(0, ios::end);
		datafile.write((char*)&data, INTSIZE);
		add();
		return 1;
	}
	else return 0;
}

/*
 * Method : fetch
 * --------------------------------------------------------
 * Basic function of database,get a data according to its key,
 * it will search the key in the index file first and then get the
 * data according to the positiong information.
 */

int Database::fetch(int key) {
	Node* cur = index.searchNode(key);
	if (cur != NULL) {
		int len = cur->children.size();
		int pos;
		for (int i = 0; i < len; i++) {
			if (cur->children[i].first == key) {
				pos = cur->children[i].second;
				break;
			}
		}
		datafile.seekg(pos, ios::beg);
		char d[4];
		datafile.read(d, INTSIZE);
		int data = (int&)d;
		delete cur;
		cur = NULL;
		return data;
	}
	else {
		delete cur;
		cur = NULL;
		return -1;
	}
}

/*
 * Method : modify
 * ------------------------------------------------------------
 * Basic function of database,according to the old key,it will find
 * out the node contains the key in the index file and it will modify
 * it with new key and new data,if the key do not exist,it will return
 * false.
 *
 */

bool Database::modify(int key, int newKey, int data) {
	if (index.replace(key, newKey)) {
		Node* cur = index.searchNode(newKey);
		int len = cur->children.size();
		int pos;
		for (int i = 0; i < len; i++) {
			if (cur->children[i].first == newKey) {
				pos = cur->children[i].second;
				break;
			}
		}
		datafile.seekg(pos, ios::beg);
		datafile.write((char*)&data, INTSIZE);
		delete cur;
		cur = NULL;
		return 1;
	}
	else {
		return 0;
	}
}


/*
 * Method : remove
 * ------------------------------------------------------------------
 * Basic function of database,find a key first and remove it from the B+ tree,
 * notably,the data of the removed key is still exist in the data file,but the 
 * pointer to it has been erased,so it won't be got again.
 *
 */

bool Database::remove(int key) {
	if (index.remove(key)) {
		reduce();
		return 1;
	}
	else return 0;
}

/*
 * Method : clear
 * ----------------------------------------------------------
 * Clear all data in the data file and clear the index file also.
 *
 */

void Database::clear() {
	datafile.close();
	datafile.open("datafile.dat", ios::out | ios::binary);
	datafile.close();
	datafile.open("datafile.dat", ios::in | ios::out | ios::binary);
	size = 0;
	datafile.write((char*)&size, INTSIZE);
	index.clear();
}

/*
 * Method : test
 * ----------------------------------------------------------------
 * Use this function to check the correctness of the database,
 * it contains four steps.
 *
 */

void Database::test(int nerc) {
	clear();
	clock_t start, finish;
	start = clock();
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<> dis(0, nerc-1);
	srand(time(NULL));
	map<int, int> test;
	vector<int>keys;
	cout << "write test..." << endl;
	int key, data;
	map<int, int>temp;
	for (int i = 0; i < nerc; i++) {
		data = rand() % 1000;
		temp[i] = data;
	}
	for (int i = 0; i < nerc; i++) {
		key = dis(gen);
		store(key, temp[key]);
		test[key] = temp[key];
	}
	cout << "pass: put in " << size << " datas." << endl;
	cout << "read test..." << endl;
	int count = 0;
	map<int, int>::iterator i;
	for (i = test.begin(); i != test.end(); i++) {
		if (i->second != fetch(i->first)) cout << "error1" << endl;
		else count++;
	}
	cout << "pass:" << count << " datas get." << endl;
	cout << "compound test..." << endl;
	int pos;
	int newKey = nerc;
	int sum = 5 * nerc;
	int mapSize = test.size();
	for (i = test.begin(); i != test.end(); i++) {
		keys.push_back(i->first);
	}
	for (int i = 0; i < sum; i++) {
		uniform_int_distribution<> position(0, mapSize-1);
		pos = position(gen);
		if (fetch(keys[pos]) != test[keys[pos]]) cout << "error2" << endl;
		if ((i + 1) % 37 == 0) {
			pos = position(gen);
			test.erase(keys[pos]);
			remove(keys[pos]);
			keys.erase(keys.begin() + pos);
			mapSize--;
		}
		if ((i + 1) % 11 == 0) {
			int data = rand() % 1000;
			store(newKey, data);
			test[newKey] = data;
			keys.push_back(newKey);
			mapSize++;
			if (fetch(newKey) != test[newKey]) {
				cout << "error3" << endl;
			}
			newKey++;
		}
		if ((i + 1) % 17 == 0) {
			uniform_int_distribution<> position(0, mapSize-1);
			pos = position(gen);
			int newData = rand() % 1000;
			test[keys[pos]] = newData;
			modify(keys[pos], keys[pos], newData);
		}
	}
	cout << "pass." << endl;
	cout << "delete test..." << endl;
	int len;
	mapSize = test.size();
	for (int i = 0; i < mapSize; i++) {
		test.erase(keys[0]);
		remove(keys[0]);
		keys.erase(keys.begin());
		len = keys.size();
		if (len != 0) {
			for (int j = 0; j < 10; j++) {
				uniform_int_distribution<> position(0, len - 1);
				pos = position(gen);
				if (fetch(keys[pos]) != test[keys[pos]])  cout << "error4" << endl;
			}
		}
	}
	cout << "pass." << endl;
	finish = clock();
	cout << "Total time: " << (finish - start) / CLOCKS_PER_SEC << endl;
}

/*
 * Method : performanceTest
 * ---------------------------------------------------------------------------
 * Simplified version of the test,without consideration of the correctness.
 *
 */

void Database::performanceTest(int nerc) {
	clear();
	clock_t start, finish;
	start = clock();
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<> dis(1, nerc);
	srand(time(NULL));
	vector<int>keys;
	int key, data;
	int newKey = nerc;
	for (int i = 0; i < nerc; i++) {
		key = dis(gen);
		data = rand() % 1000;
		if (store(key, data)) {
			keys.push_back(key);
		}
	}
	int len = keys.size();
	for (int i = 0; i < len; i++) {
		fetch(keys[i]);
	}
	int sum = 5 * nerc;
	int pos;
	for (int i = 0; i < sum; i++) {
		uniform_int_distribution<> position(0, len - 1);
		pos = position(gen);
		fetch(keys[pos]);
		if ((i + 1) % 37 == 0) {
			pos = position(gen);
			remove(keys[pos]);
			keys.erase(keys.begin() + pos);
			len--;
		}
		if ((i + 1) % 11 == 0) {
			store(newKey, 1);
			keys.push_back(newKey);
			fetch(newKey);
			len++;
			newKey++;
		}
		if ((i + 1) % 17 == 0) {
			uniform_int_distribution<> position(0, len - 1);
			pos = position(gen);
			modify(keys[pos], keys[pos], 1);
		}
	}
	for (int i = 0; i < len; i++) {
		remove(keys[0]);
		keys.erase(keys.begin());
		len--;
		if (len != 0) {
			for (int j = 0; j < 10; j++) {
				uniform_int_distribution<> position(0, len - 1);
				pos = position(gen);
				fetch(keys[pos]);
			}
		}
	}
	finish = clock();
	cout << "Total time: " << (finish - start) / CLOCKS_PER_SEC << endl;
}

int main() {
	Database db;
	db.open();
	cout << "* * * * * *" << endl;
	cout << "* Menu:   *" << endl;
	cout << "* fetch   *" << endl;
	cout << "* store   *" << endl;
	cout << "* delete  *" << endl;
	cout << "* modify  *" << endl;
	cout << "* close   *" << endl;
	cout << "* clear   *" << endl;
	cout << "* size    *" << endl;
	cout << "* test    *" << endl;
	cout << "* * * * * *" << endl;
	cout << "Both key and data are int." << endl;
	cout << "DO NOT FORGET TO CLOSE!" << endl;
	while (true) {
		string choice;
		cout << endl;
		cout << "Input the the command:";
		cin >> choice;
		if (choice == "fetch") {
			int key;
			cout << "Input the key to fetch:";
			cin >> key;
			int data = db.fetch(key);
			if (data == -1) cout << "Not found" << endl;
			else cout << key << " : " << data << endl;
		}
		else if (choice == "store") {
			int key, data;
			cout << "Input the key:";
			cin >> key;
			cout << "Input data:";
			cin >> data;
			if (!db.store(key, data)) cout << "already exist" << endl;
		}
		else if (choice == "delete") {
			int key;
			cout << "Input the key to delete:";
			cin >> key;
			if (!db.remove(key)) cout << "Not found." << endl;
		}
		else if (choice == "modify") {
			int key, newKey, data;
			cout << "Input the key to replace:";
			cin >> key;
			cout << "Input the new key of it:";
			cin >> newKey;
			cout << "Input the new data:";
			cin >> data;
			if (!db.modify(key, newKey, data)) cout << "Not found" << endl;
		}
		else if (choice == "close") {
			db.close();
			break;
		}
		else if (choice == "clear") {
			db.clear();
		}
		else if (choice == "size") {
			cout << db.getSize() << endl;
		}
		else if (choice == "test") {
			int nerc, mode;
			cout << "Input the data size:";
			cin >> nerc;
			cout << "choose the mode:1.correctness  2.performance" << endl;
			cin >> mode;
			if(mode ==1) db.test(nerc);
			if (mode == 2) db.performanceTest(nerc);
		}
		else cout << "No such command.." << endl;
	}
	
	return 0;
}

