# HuffmanCoding
利用哈夫曼编码，实现了对文档的压缩和解压

## 使用方式

### 编译

```shell
gcc -o compressor huffman_compressor.c
```

```shell
gcc -o decompressor huffman_decompressor.c
```

### 压缩

```shell
./compressor ./usconstitution.txt
```

会生成压缩文件：usconstitution.txt.compress

### 解压

```shell
./decompressor ./usconstitution.txt.compress us.txt
```


