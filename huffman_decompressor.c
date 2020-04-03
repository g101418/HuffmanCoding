#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//首先预处理生成候选处理列表
//随后处理候选列表，遍历读取两个最小值比较，当数组中只剩一个节点时结束运行
#define MAX_LIST_LEN 256
#define HASH_TABLE_SIZE 1571

typedef struct huffman_tree_node
{
    char code[MAX_LIST_LEN]; //用于记录编码，以字符串形式
    unsigned char letter;    //编码的该字母

    int code_length; //用于记录编码长度
} TreeNode;

// TODO 构建候选列表
void build_list(); //构建候选处理列表
TreeNode *build_huffman_tree();

// 用于字典索引的字母列表
TreeNode *hash_list[MAX_LIST_LEN];
// Hash表
int hash_table[HASH_TABLE_SIZE];

// TODO 字典，该字典用于根据输入文本初始化以及后续变长编码记录
// TODO 哈希表构建：hash, init, insert, search, alter*, add_letter_dict
// ! 哈希表存储的是字典索引数组中的下表

unsigned int hash(char *key)
{
    unsigned int seed = 787;
    unsigned int hash = 0;
    while (*key)
    {
        hash = hash * seed + (*key++);
    }
    return hash % HASH_TABLE_SIZE;
}

// 用于初始化所有全局变量
void init()
{
    for (int i = 0; i < MAX_LIST_LEN; i++)
    {
        hash_list[i] = NULL;
    }
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        hash_table[i] = -1;
    }
}

void insert_hash_table(char *key, int position)
{
    int addr = hash(key);
    while (hash_table[addr] != -1)
    {
        addr = (addr + 1) % HASH_TABLE_SIZE;
    }
    hash_table[addr] = position;
}

int search_hash_table(char *key)
{
    int addr = hash(key);
    while (1)
    {
        if (hash_table[addr] == -1)
            return -1;
        if (!strcmp(key, hash_list[hash_table[addr]]->code))
            return hash_table[addr];
        addr = (addr + 1) % HASH_TABLE_SIZE;
    }
    return -1;
}

// 同时填充list和hash_list，初次填充
void add_letter_dict(int position, const char *code, int code_length, unsigned char letter)
{
    TreeNode *new_node = (TreeNode *)malloc(sizeof(TreeNode));
    new_node->letter = letter;
    for (int i = 0; i < MAX_LIST_LEN; i++)
        new_node->code[i] = '\0';
    strcpy(new_node->code, code);
    new_node->code_length = code_length;

    // 填充hash_list
    hash_list[position] = new_node;
}

// 获得list中存有节点的数量
int len_list(TreeNode *list[MAX_LIST_LEN])
{
    int len = 0;
    for (int i = 0; i < MAX_LIST_LEN; i++)
    {
        if (list[i] != NULL)
        {
            len++;
        }
    }
    return len;
}

void decompress(const char *filename, const char *writefilename)
{
    // ! 文件结构为 {nodes_count, [{节点编码结构}, ...], 文本编码长度, 文本编码}
    // ! 节点编码结构 {letter, code_length, code[0], code[1],...,code[code_length-1]}
    FILE *fin = fopen(filename, "r");

    // 以下读取哈夫曼树结构，构造hash表，表结构为： map[code] = letter
    int nodes_count = 0;
    long codes_count = 0; // 编码bit长度
    // TODO 改为short
    fread(&nodes_count, sizeof(int), 1, fin);

    int letter_num = 0; // 字符种类数
    for (int i = 0; i < nodes_count; i++)
    {
        unsigned char letter;
        int code_length = -1;
        char code[MAX_LIST_LEN] = {0};
        char temp[MAX_LIST_LEN / 7] = {0};

        fread(&letter, sizeof(char), 1, fin);
        // TODO 改为short
        short code_len = -1;
        fread(&code_len, sizeof(short), 1, fin);
        code_length = (int)code_len;
        // TODO 改为按位存储的char
        // 计算占用的char数
        int target_char_num = code_length / 8;
        if (code_length % 8 > 0)
            target_char_num += 1;
        fread(temp, sizeof(char), target_char_num, fin);
        for (int j = 0; j < code_length; j++)
        {
            if ((temp[j / 8] & (1 << (7 - (j % 8)))) > 0)
            {
                strcat(code, "1");
            }
            else
            {
                strcat(code, "0");
            }
        }

        // 写入字典
        int dict_addr = search_hash_table(code);
        if (dict_addr == -1) //哈希表中尚不存在该字符
        {
            insert_hash_table(code, letter_num);
            add_letter_dict(letter_num, code, code_length, letter);
            letter_num++;
        }
        else //哈希表中存在该字符，出错
        {
            return;
        }
    }
    fread(&codes_count, sizeof(long), 1, fin);

    unsigned char *dest_char = (unsigned char *)malloc(sizeof(char) * codes_count);
    for (int i = 0; i < codes_count; i++)
    {
        dest_char[i] = 0;
    }
    int curr_subchar_index = 0;
    unsigned char curr_char;
    char curr_bits[MAX_LIST_LEN] = {0};
    for (int i = 0; i < codes_count; i++)
    {
        if (curr_subchar_index % 8 == 0)
        {
            fread(&curr_char, sizeof(char), 1, fin);
            curr_subchar_index = 0;
        }

        // unsigned char curr_bit_char;
        if ((curr_char & (1 << (7 - curr_subchar_index))) > 0)
        {
            strcat(curr_bits, "1");
        }
        else
        {
            strcat(curr_bits, "0");
        }

        int addr = search_hash_table(curr_bits);
        if (addr != -1)
        {
            // 匹配到
            unsigned char temp[2] = {0};
            temp[0] = hash_list[addr]->letter;
            temp[1] = '\0';
            strcat((char *)dest_char, (char *)temp);
            strcpy(curr_bits, "\0");
        }

        curr_subchar_index += 1;
    }
    // printf("%s", dest_char);
    fclose(fin);

    // 将解码后数据写入文件
    FILE *fout = fopen(writefilename, "wb");
    if (fout == NULL)
    {
        perror("open failed!");
        exit(1);
    }
    fwrite(dest_char, sizeof(char), strlen((char *)dest_char), fout);
    fclose(fout);
}

int main(int argc, char *argv[])
{
    init();
    decompress(argv[1], argv[2]);
    return 0;
}