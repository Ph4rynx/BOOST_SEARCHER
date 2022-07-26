#pragma once
#include"log.hpp"
#include"index.hpp"
#include"util.hpp"
#include<algorithm>
#include<jsoncpp/json/json.h>
namespace ns_searcher{
    struct InvertedElemPrint
    {
        uint64_t doc_id;
        int weight;
        std::vector<std::string> words;
        InvertedElemPrint():doc_id(0),weight(0){}
    };

    class Searcher{
    private:
        ns_index::Index *index;//供系统进行查找的索引
    public:
        Searcher(){}
        ~Searcher(){}
    public:
        void InitSearcher(const std::string &input)
        {
            //1.获取或者创建index对象
            index = ns_index::Index::GetInstance();
            //std::cout<<"获取index单例成功..." <<std::endl;
            LOG(NORMAL,"获取index单例成功...");

            //2.根据index对象建立对应的索引结构
            index->BuildIndex(input);
            //std::cout<<"建立正排和倒排索引成功..."<<std::endl;
            LOG(NORMAL,"建立正排和倒排索引成功...");
        }

        //query:搜索关键字
        //json_string:返回给用户浏览器的搜索结果
        void Search(const std::string &query,std::string *json_string)
        {    
            //1.分词：对query进行按照searcher的要求的分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query,&words);

            //2.根据分词后的各个“词”，进行index查找，建立index时是忽略大小写的，所以搜索的关键字也需要忽略大小写，全部转化为小写
            //ns_index::InvertedList inverted_list_all;//存放的是倒排拉链的结点InvertedElem
            std::vector<InvertedElemPrint> inverted_list_all;
            std::unordered_map<uint64_t,InvertedElemPrint> tokens_map;//去重

            for(std::string word : words)
            {
                boost::to_lower(word);
                ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                if(inverted_list == nullptr)//没有倒排结点
                {
                    continue;
                }
                //此处需要处理一个词被分词后形成了许多常见的简单的小词，这些词在同一个文档中出现，导致这些词doc_id相同，显示的搜索结果重复
                //inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());//倒排拉链合并在一起
                for(const auto &elem : *inverted_list)
                {
                    auto &item = tokens_map[elem.doc_id];//[]:如果存在直接获取，如果不存在则新建
                    //item一定是doc_id相同的print结点
                    item.doc_id = elem.doc_id;
                    item.weight += elem.weight;
                    item.words.push_back(elem.word);
                }
            }
            for(const auto &item : tokens_map)
            {
                inverted_list_all.push_back(std::move(item.second));
            }
                
            //3.合并与排序：汇总查找结果，按照相关性即权重weight降序排序
            //std::sort(inverted_list_all.begin(),inverted_list_all.end(),
            //        [](const ns_index::InvertedElem &e1,const ns_index::InvertedElem &e2){
            //         return e1.weight > e2.weight;
            //          });
            std::sort(inverted_list_all.begin(),inverted_list_all.end(),
                        [](const InvertedElemPrint &e1,const InvertedElemPrint &e2){
                        return e1.weight > e2.weight;
                        });


            //4.根据查找出的结果，构建json串，返回给用户，需要使用第三方库jsoncpp，一般通过jsoncpp完成序列化和反序列化的功能
            Json::Value root;
            for(auto &item : inverted_list_all)
            {
                ns_index::DocInfo *doc = index->GetForwardIndex(item.doc_id);
                if(doc == nullptr)
                {
                    continue;
                }
                Json::Value elem;
                elem["title"] = doc->title;
                elem["desc"] = GetDesc(doc->content,item.words[0]);//content是文档去标签后的结果，但不是我们想要的，我们想要的是一部分->获取摘要
                elem["url"] = doc->url;
                
                //for debug
                //elem["id"] = (int)item.doc_id;
                //elem["weight"] = item.weight;
                root.append(elem);
            }

            //Json::StyledWriter writer;
            Json::FastWriter writer;
            *json_string = writer.write(root);
        }
        //摘要函数的编写
        std::string GetDesc(const std::string &html_content,const std::string &word)
        {
            //找到word在html_content中的首次出现，然后往前找些许字节，往后找些许字节，截取这部分字节片段;若没有这些字节，就从begin开始，或到end结束也可以
            //截取内容片段
            const int prev_step = 50;
            const int next_step = 100; 
            //1.找到首次出现
            auto iter = std::search(html_content.begin(),html_content.end(),word.begin(),word.end(),[](int x,int y){
                                    return (std::tolower(x) == std::tolower(y));
                                    });
            if(iter == html_content.end())
            {
                return "None1";
            }
            int pos = std::distance(html_content.begin(),iter);


            //2.获取start,end,std::size_t是一个无符号整数
            int start = 0;
            int end = html_content.size() - 1;
            //如果之前有50个字符，就更新位置
            if(pos > start + prev_step)//使用pos - prev_step > start 相减可能会出现负数并溢出 
            {
                start = pos - prev_step;
            }
            //如果当前位置后不足100字符
            if(pos < end - next_step)//如果是size_t类型则强转成int，防止溢出
            {
                end = pos + next_step;
            }
            

            //3.截取子串，返回
            if(start >= end)
            {
                return "None";
            }
            std::string desc = html_content.substr(start,end - start);
            desc +="...";
            return desc;
        }
   };
}
