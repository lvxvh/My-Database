#pragma once

#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include<exception>
#include<map>

#define INTSIZE 4
#define ORDER 24
const int minsize = (ORDER % 2 == 0) ? ORDER / 2 : ORDER / 2 + 1;
const int datasize = 4;
const int none = -1;
const int nodesize = 13 + 8 * (ORDER + 1);

using namespace std;

/* 
 * Struct : Node
 * -----------------------------------
 * Use this struct to express a node in B+ Tree.
 * A node contains these information : 
 * its parent node's position in the index file; its neibor node's position in the index file;
 * whether it is a leaf node and some infomation about its children node.
 *
 */

struct Node {
	bool isLeaf;
	int parent;
	int next;
	int pre;
	vector<pair<int,int>> children;
	Node() {
		parent = none;
		next = none;
		pre = none;
	}
	
};

/*
 * Class : BplusTree
 * -----------------------------------------
 * This class is use to operate the index file;the index(B+ Tree). is saved in the index file.
 * With this class,you can do insert,delete,modify and search operation on the B+ Tree.
 *
 *
 */

class BplusTree {
public:
	BplusTree() 
	{
	}
	Node* getNode(int pos);
	void putNode(Node* cur);
	bool insert(int key);
	bool remove(int key);
	Node* searchNode(int key);
	bool replace(int key, int newKey);
	void solveOverflow(Node* cur);
	void solveUnderflow(Node* cur);
	int getRoot() { return root; }
	int getCount() {
		return count;
	}
	void save();
	void load();
	void clear();
	
private:
	int root;
	int count;
	fstream idxfile;
};

/*
 * Class : Database
 * --------------------------------------------------
 * This class is use to operate the date file with the help of class BplusTree.
 * It contains basic operation on the data and it also contains some test function used to
 * check the database;
 *
 */

class Database {
public:
	Database(){}
	void open();
	void close();
	int getSize() { return size; }
	bool store(int key, int data);
	int fetch(int key);
	bool modify(int key, int newKey, int data);
	bool remove(int key);
	void clear();
	void add() { size++; }
	void reduce() { size--; }
	void test(int nerc);
	void performanceTest(int nerc);
private:
	fstream datafile;
	BplusTree index;
	int size;
};