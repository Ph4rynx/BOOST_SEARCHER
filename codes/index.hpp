#pragma once
#include"util.hpp"
#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<fstream>
#include<mutex>
#include"log.hpp"
namespace ns_index{
    struct DocInfo
    {
        std::string title;
        std::string content;
        std::string url;
        uint64_t doc_id;//将来文档的ID
    };

    struct InvertedElem
    {
        int doc_id;//文档ID
        std::string word;
        int weight;//权重
    };

    //倒排拉链
    typedef std::vector<InvertedElem> InvertedList;

    class Index{
        private:
            //正排索引的数据结构:数组，数组的下标天然是文档的ID
            std::vector<DocInfo> forward_index;//正排索引

            //倒排索引一定是一个关键字和一个或一组InvertedElem对应[关键字和倒排拉链的映射关系]
            std::unordered_map<std::string,InvertedList> inverted_index;
        private:
            Index(){}//需要有函数体
            Index(const Index&) = delete;
            Index& operator = (const Index&) = delete;
            static Index* instance;
            static std::mutex mtx;
        public:
            ~Index(){}
        public:
            static Index* GetInstance()
            {
                if(instance == nullptr)
                {
                    mtx.lock();//避免多线程引发错误，需要加锁
                    if(instance == nullptr)
                    {
                        instance = new Index();
                    }
                    mtx.unlock();//解锁
                }
                return instance;//单例
            }

            //根据doc_id找到文档内容
            DocInfo *GetForwardIndex(uint64_t doc_id)
            {
                if(doc_id >= forward_index.size())
                {
                    std::cerr << "doc_id out range" << std::endl;
                    return nullptr;
                }
                return &forward_index[doc_id];
            }

            //根据关键字string获得倒排拉链
            InvertedList *GetInvertedList(const std::string &word)
            {
                auto iter = inverted_index.find(word);
                if(iter == inverted_index.end())
                {
                    std::cerr << word << "habe no InvertedList"<<std::endl;
                    return nullptr;
                }
                return &(iter->second); 
            }

            //根据去标签、格式化之后的文档，构建正排和索引
            bool BuildIndex(const std::string &input)//parse处理完毕的数据需要交付
            {
                std::ifstream in(input,std::ios::in | std::ios::binary);
                if(!in.is_open())
                {
                    std::cerr << "sorry, "<<input<<" open eroor"<<std::endl;
                    return false;
                }
                std::string line;   
                int count = 0;
                while(std::getline(in,line))
                {
                    //建立正排索引
                    DocInfo *doc = BuildForwardIndex(line);
                    if(doc == nullptr)
                    {
                        std::cerr<<"build "<<line<<"error"<<std::endl; //for debug
                        continue;
                    }

                    //建立倒排索引
                    BuildInvertedIndex(*doc);
                    count++;
                    if(count % 50 == 0)
                    {
                        //std::cout<<"当前已经建立的索引文档："<<count<<std::endl;
                        LOG(NORMAL,"当前已经建立的索引文档： " + std::to_string(count));
                    }
                }
                return true;
            }
        private:
            //构建正排
            DocInfo *BuildForwardIndex(const std::string &line)
            {
                //1.解析line，字符串切分，line->3 string,title,content,url
                const std::string sep ="\3";
                std::vector<std::string> results;
                ns_util::StringUtil::Split(line,&results,sep);
                //此时已经把字符串切分完，需要进行判断
                if(results.size() != 3)//我们把字符串切分成title/content/url三部分
                {
                    return nullptr;
                }


                //2.字符串进行填充到DocInfo中
                DocInfo doc;
                doc.title = results[0];//title
                doc.content = results[1];//content
                doc.url = results[2];//url
                doc.doc_id = forward_index.size();      //先保存id再插入，对应的id就是当前doc再vector中的下标，如果先插入再保存，则下标对应size-1


                //3.插入到正排索引的vector
                forward_index.push_back(std::move(doc));//doc里包含了很多html文件内容，会发生拷贝导致效率低下，所以我们使用move
                return &forward_index.back();
            }
            
            //构建倒排
            bool BuildInvertedIndex(const DocInfo &doc)
            {
                //DocInfo[title,content,url,doc_id]
                //word -> 倒排拉链
                struct word_cnt
                {
                    int title_cnt;
                    int content_cnt;
                    word_cnt():title_cnt(0),content_cnt(0){}
                };
                std::unordered_map<std::string,word_cnt> word_map;//用来暂存词频的映射表

                //对标题进行分词
                std::vector<std::string> title_words;
                ns_util::JiebaUtil::CutString(doc.title,&title_words);
                

                //对标题进行词频统计
                for(std::string s : title_words)
                {
                    boost::to_lower(s);     //搜索时不区分大小写，将分词统计转化为小写
                    word_map[s].title_cnt++;//[]:如果存在就获取，如果不存在就新建
                }


                //对文档内容进行分词
                std::vector<std::string> content_words;
                ns_util::JiebaUtil::CutString(doc.content,&content_words);


                //对文档内容进行词频统计
                for(std::string s : content_words)
                {
                    boost::to_lower(s);     //搜索时不区分大小写，将分词统计转化为小写  
                    word_map[s].content_cnt++;
                }
#define X 10
#define Y 2     //倒排索引
                for(auto &word_pair : word_map)
                {
                    InvertedElem item;
                    item.doc_id = doc.doc_id;
                    item.word = word_pair.first;
                    item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt;//相关性
                    InvertedList &inverted_list = inverted_index[word_pair.first];
                    inverted_list.push_back(std::move(item));
                }

                return true;
            }
    };
    Index* Index::instance = nullptr;
    std::mutex Index::mtx;
}
