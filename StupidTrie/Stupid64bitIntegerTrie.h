#pragma once

#include "common.h"

// stupid 256-fanout path-compressed trie tree for debug purpose
//
namespace StupidUInt64Trie
{

// root ======[parent]--child--*---------path-compression-string-------[this]--child-- ....... -- [minimum value in this subtree]
//                            ilen                                      dlen                      minv
//
struct TrieNodeDescriptor
{
	int ilen;
	int dlen;
	uint64_t minv;
	vector<int> children;
	TrieNodeDescriptor(int ilen, int dlen, uint64_t minv, int numChildren)
		: ilen(ilen)
		, dlen(dlen)
		, minv(minv)
		, children()
	{
		assert(0 <= ilen && ilen <= dlen && dlen <= 8);
		chilren.reserve(numChildren);
	}
	
	void AddChild(int c)
	{
		children.push_back(c);
	}
};

struct node
{
	uint8_t fullKey[8];
	int len;
	map<int, node*> child;
	node() { memset(fullKey, 0, 8); len = 0; }
};

class Trie
{
public:
	Trie();
	~Trie();
	
	void Insert(uint64_t value);
	void DumpData(vector<TrieNodeDescriptor>& result);
	
private:
	node* root;
	void Destroy(node* cur);
	void Insert(uint8_t* input);
	uint64_t Dfs(node* cur, vector<TrieNodeDescriptor>& result, int depth, uint64_t curValue);
}

}	// StupidTrie


