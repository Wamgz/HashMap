#include "bplus_tree.h"
#include "FileOption.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <ctime>

using namespace std;


unsigned int RSHash(char *str){
	//映射字符串为整数，整数作为key索引
	unsigned int b = 378551;
	unsigned int a = 63689;
	unsigned int hash = 0;

	while (*str){
		hash = hash * a + (*str++);
		a *= b;
	}

	return (hash & 0x7FFFFFFF);
}

// LeafListNode::LeafListNode(char* val){
// 	//叶子链表节点初始化
// 	this -> val = val;
// 	this -> next = NULL;
// }

// IndexListNode::IndexListNode(unsigned int val){
// 	//索引链表节点初始化
// 	this -> val = val;
// 	this -> next = NULL;
// }

IndexNode::IndexNode(int order){
	//索引节点初始化
	this -> order = order;
	this -> keynum = 0;
	this -> isLeaf = false;
	this -> keys = new char*[order + 1];//+1是因为超出时进行分裂
	this -> parent = NULL;
	this -> children = new Node*[order + 1];
}

LeafNode::LeafNode(int order){
	//叶子节点初始化
	this -> order = order;
	this -> keynum = 0;
	this -> isLeaf = true;
	this -> keys = new char*[order + 1];
	this -> parent = NULL;
	this -> values = new char*[order + 1];
}

BPlusTree::BPlusTree(int order){
	//初始化树
	this -> order = order;
	this -> root = new LeafNode(order);
}

BPlusTree::~BPlusTree(){
	//释放树的内存
	if(this -> root != NULL){
		delete this -> root;
	}
}


Node* BPlusTree::search(char* s, char* key){//查找需要插入到的叶子节点
	Node* t = this -> root;
	while(!t -> isLeaf){//如果t就是叶子节点直接返回，否则继续向下
		int flag = 0;
		for(int i = 0;i < t -> keynum;i++){
			if(key < t -> keys[i]){
				t = ((IndexNode*)t) -> children[i];
				flag = 1;
				break;
			}
		}
		if(flag == 0){//key大于所有关键字，则修改最后一个关键字为当前关键字并进入最后一个关键字
			t -> keys[t -> keynum - 1] = key;
			t = ((IndexNode*)t) -> children[t -> keynum - 1];
		}
	}
	return t;
}

void BPlusTree::insert(char* s, char* key){//插入到节点
	Node* t = search(s, key);
	int flag = 0;
	//叶子节点进行插入
	for(int i = 0;i < t -> keynum;i++){
		if(key < t -> keys[i]){
			flag = 1;
			for(int j = t -> keynum - 1;j >= i;j--){
				t -> keys[j + 1] = t -> keys[j];
			}
			t -> keys[i] = key;
			((LeafNode*)t) -> values[i] = s;
			t -> keynum ++;
			break;
		}
	}
	if(flag == 0){
		t -> keys[t -> keynum] = key;
		((LeafNode*)t) -> values[t -> keynum] = s;
		t -> keynum ++;
	}

	//如果叶子节点个数超过阶数
	if(t -> keynum > order){
		//叶子节点分裂
		Node* tnew = new LeafNode(this -> order);
		int idx = t -> keynum / 2;
		
		for(int i = idx;i < t -> keynum;i++){
			tnew -> keys[i - idx] = t -> keys[i];
			((LeafNode*)tnew) -> values[i - idx] = ((LeafNode*)t) -> values[i];

		}
		//t[0, keynum / 2 - 1];
		//tnew[keynum / 2, keynum]
		tnew -> keynum = (t -> keynum + 1) / 2;
		t -> keynum = t -> keynum / 2;
		((LeafNode*)tnew) -> next = ((LeafNode*)t) -> next;
		((LeafNode*)t) -> next = tnew;
		key = t -> keys[t-> keynum - 1];
		
		//将当前的左边节点最后一个节点放入父节点
		while(t -> parent != NULL){
			Node* left = t;
			Node* right = tnew;
			t = t -> parent;

			for(int i = 0;i < t -> keynum;i++){
				if(key < t -> keys[i]){
					((IndexNode*)t) -> children[i] = tnew;
					tnew -> parent = t;
					for(int j = t -> keynum - 1;j >= i;j--){
						t -> keys[j + 1] = t -> keys[j];
						((IndexNode*)t) -> children[j + 1] = ((IndexNode*)t) -> children[j];
					}
					t -> keys[i] = key;
					((IndexNode*)t) -> children[i] = left;
					t -> keynum ++;
					break;
				}
			}

			//父节点的分裂
			if(t -> keynum > this -> order){
				IndexNode* innew = new IndexNode(this -> order);
				int idxnew = t -> keynum / 2;

				for(int i = idxnew;i < t -> keynum;i++){
					innew -> keys[i - idxnew] = t -> keys[i];
					innew -> children[i - idxnew] = ((IndexNode*)t) -> children[i]; 
				}
				
				//t[0, keynum / 2]
				//innew[(keynum + 1) / 2, keynum]
				innew -> keynum = (t -> keynum  + 1)/ 2;
				t -> keynum = t -> keynum / 2;
				
				key = t -> keys[t -> keynum - 1];
				left -> parent = t;
				right -> parent = innew;
				tnew = innew;
			}
			else{
				//父节点不用分裂
				tnew -> parent = t;
				
				return ;
			}
		}
		if(t -> parent == NULL){
			t -> parent = new IndexNode(this -> order);
			
			((IndexNode*)t -> parent) -> children[0] = t;
			((IndexNode*)t -> parent) -> children[1] = tnew;
			t -> parent -> keys[0] = t -> keys[t -> keynum - 1];
			t -> parent -> keys[1] = tnew -> keys[tnew -> keynum - 1];
            
			t -> parent -> keynum += 2;

			tnew -> parent = t -> parent;
			this -> root = t -> parent;
		}
	}
}

bool BPlusTree::find(char* s, char* key){
	Node* t = this -> root;
	while(!t -> isLeaf){//如果t就是叶子节点直接返回，否则继续向下
		int flag = 0;
		for(int i = 0;i < t -> keynum;i++){
			if(key <= t -> keys[i]){
				t = ((IndexNode*)t) -> children[i];
				flag = 1;
				break;
			}
		}
		if(flag == 0){//key大于所有关键字，则修改最后一个关键字为当前关键字并进入最后一个关键字
			return false;
		}
	}

	return true;
}

void BPlusTree::printKeys(){
	if(this -> root-> keynum == 0){
		cout<<"No nodes now"<<endl;
		return ;
	}
	vector<Node*> vec;
	Node* p = this -> root;
	vec.push_back(p);
	while(vec.size() != 0){
		int size = vec.size();
		// cout<<"size  "<<size<<endl;
		for(int i = 0;i < size;i++){
			if(!vec[i]->isLeaf){
				for(int j = 0;j < vec[i] -> keynum;j++){
					vec.push_back(((IndexNode*)vec[i]) -> children[j]);
				}
			}
			// cout<<"size  "<<vec[i] -> keynum<<endl;
			for(int j = 0;j < vec[i] -> keynum;j++){
				cout<<vec[i] -> keys[j]<<"  "; 
			}
			cout<<"||";
		}
		vec.erase(vec.begin(), vec.begin() + size);
		cout<<endl;
	}
}

void BPlusTree::printValues(){
	Node* p = this -> root;
	while(!p->isLeaf){
		p = ((IndexNode*)p) -> children[0];
	}
	while(p != NULL && p -> keynum != 0){
		
		for(int i = 0;i < p -> keynum;i++){
			// printf("%s ", ((LeafNode*)p) -> values[i]);
			cout<<((LeafNode*)p) -> keys[i]<<"   "<<((LeafNode*)p) -> values[i]<<endl;
		}
		p = ((LeafNode*)p) -> next;
	}
}

int main(){
	BPlusTree* tree = new BPlusTree(3);
	tree -> insert("a", "a");
	tree -> insert("b", "b");
	tree -> insert("c", "c");
	tree -> insert("d", "d");
	tree -> insert("h", "h");

	tree -> insert("e", "e");
	tree -> insert("f", "f");
	tree -> insert("g", "g");
	tree -> insert("i", "i");
	tree -> insert("j", "j");
	tree -> insert("k", "k");
	tree -> insert("m", "m");

	tree -> insert("l", "l");
	tree -> insert("n", "n");
	tree -> insert("o", "o");
	tree -> insert("s", "s");

	tree -> insert("p", "p");
	tree -> insert("q", "q");
	tree -> insert("r", "r");
	
	tree -> printKeys();
	tree -> printValues();

	// clock_t start, finish;
	// start = clock();

	// BPlusTree* tree = new BPlusTree(3);
	// FileReader* fr = new FileReader();
	// char* fileLoc = { "./dict.txt" };
	// bool openRe = fr->openFile(fileLoc,"r");
	// if (openRe == false) {
	// 	cout << "文件打开错误" << endl;
	// 	system("pause");
	// 	return 0;
	// }
	// char* data = new char[BUFFER_SIZE];
	// memset(data, 0x00, BUFFER_SIZE);
	// while (fr->getline(data))
	// {
	// 	int length = strlen(data);
	// 	if (length == 0) continue;
	// 	char* m = new char[length+1];
	// 	memcpy(m, data, length+1);
	// 	tree->insert(m, m);
	// }
	// delete fr;

	// FileReader* fr_target = new FileReader();
	// fileLoc = { "./string.txt" };
	// openRe = fr_target->openFile(fileLoc,"r");
	// if (!openRe) {
	// 	cout << "error happen when open target file" << endl;
	// 	system("pause");
	// 	return 0;
	// }
	
	// FileWriter* fw = new FileWriter();
	// fileLoc = { "./test _result.txt" };
	// openRe = fw->openFile(fileLoc, "w");
	// if (!openRe) {
	// 	cout << "error happen when open result file" << endl;
	// 	system("pause");
	// 	return 0;
	// }

	
	// memset(data, 0x00, BUFFER_SIZE);
	// int i = 1;
	// while (fr_target->getline(data))
	// {
	// 	int length = strlen(data);
	// 	if (length == 0) continue;
	// 	if (tree -> find(data, data)) {
	// 		//cout<<i<<":" << data << endl;
	// 		fw->putline(data);
	// 		i++;
	// 	}
	// }
	// delete fr_target;
	// delete fw;
	// finish = clock();
	// cout<<i<<" lines matched"<<endl;
	// cout << "use time  is " << finish - start << endl;
	// system("pause");
	
	// return 0;
}