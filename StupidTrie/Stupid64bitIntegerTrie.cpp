#include "Stupid64bitIntegerTrie.h"

namespace StupidUInt64Trie
{

Trie::Trie()
{
	root = new node();
	ReleaseAssert(root != nullptr);
}

Trie::~Trie()
{
	assert(root != nullptr);
	Destroy(root);
	root = nullptr;
}

void Trie::Destroy(node* cur)
{
	assert(cur != nullptr);
	rept(it, cur->child)
	{
		Destroy(it->second);
	}
	delete cur;
}

bool Trie::Insert(uint8_t* input)
{
	node* cur = root;
	int pos = 0;
	while (pos < 8)
	{
		if (!cur->child.count(input[pos]))
		{
			break;
		}
		node* next = cur->child[input[pos]];
		bool match = true;
		int breakPos = -1;
		rep(k, 0, next->len - 1)
		{
			if (next->fullKey[k] != input[k])
			{
				match = false;
				breakPos = k;
				break;
			}
		}
		if (match)
		{
			cur = next;
			pos = cur->len;
			continue;
		}
		node* split = new node();
		memcpy(split->fullKey, next->fullKey, breakPos);
		split->len = breakPos;
		split->child[next->fullKey[breakPos]] = next;
		cur->child[input[pos]] = split;
		pos = breakPos;
		cur = split;
	}
	if (pos == 8)
	{
		return false;
	}
	node* leaf = new node();
	leaf->len = 8;
	memcpy(leaf->fullKey, input, 8);
	cur->child[input[pos]] = leaf;
	return true;
}

bool Trie::Insert(uint64_t value)
{
	uint8_t input[8];
	uint64_t x = value;
	repd(j,7,0)
	{
		input[j] = x % 256;
		x /= 256;
	}
	return Insert(input);
}

void Trie::DumpData(vector<TrieNodeDescriptor>& result)
{
	result.clear();
	std::ignore = Dfs(root, result, 0 /*depth*/, 0 /*curValue*/);
}

uint64_t Trie::Dfs(node* cur, vector<TrieNodeDescriptor>& result, int depth, uint64_t curValue)
{
	uint64_t fullKey = 0;
	rep(i,0,7)
	{
		fullKey <<= 8;
		fullKey += cur->fullKey[i];
	}
	
	uint64_t minv = 0xffffffffffffffffULL;
	rept(it,cur->child)
	{
		minv = min(minv, Dfs(it->second, result, cur->len + 1, fullKey + (((uint64_t)it->first) << (56 - 8 * cur->len))));
	}
	
	if (cur->len == 8)
	{
		minv = fullKey;
	}
	
	if (depth > 0)
	{
		assert((minv >> (64 - depth * 8)) == (curValue >> (64 - depth * 8)));
	}
	
	result.push_back(TrieNodeDescriptor(depth, cur->len, minv, cur->child.size()));
	rept(it, cur->child)
	{
		result.back().AddChild(it->first);
	}
	return minv;
}

}


