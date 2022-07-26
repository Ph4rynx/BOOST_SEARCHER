#pragma once
#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<boost/algorithm/string.hpp>
#include"cppjieba/Jieba.hpp"
namespace ns_util{
    class FileUtil{
    public:
        static bool ReadFile(const std::string &file_path,std::string *out)
        {
            std::ifstream in(file_path,std::ios::in);
            if(!in.is_open())
            {
                std::cerr<<"open file "<<file_path<<" error"<<std::endl;
                return false;
            }
            std::string line;
            while(std::getline(in,line))//如何理解getline读取到文件结束？getline返回值是一个引用，while(bool)，本质是因为重载了强制类型转换
            {
                *out += line;
            }

            in.close();
            return true;
        }
    };

    class StringUtil{
    public:
        static void Split(const std::string &target,std::vector<std::string> *out,const std::string &sep) //sep是切分符
        {
           //boost::split(vec,test,boost::is_any_of(","),boost::token_compress_on); vec是一个数组，test是一个字符串,is_any_of()里面是分隔符
           //token_compress_on: aaa\3bbb\3\3\3\3\ccc\3 即确定"bbb\3"和"ccc\3"之间的"\3\3\3"需不需要压缩，默认是off，即不压缩
           boost::split(*out,target,boost::is_any_of(sep),boost::token_compress_on);
        }
    };

    const char* const DICT_PATH = "./dict/jieba.dict.utf8";
    const char* const HMM_PATH = "./dict/hmm_model.utf8";
    const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char* const IDF_PATH = "./dict/idf.utf8";
    const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";

    class JiebaUtil{
    private:
        static cppjieba::Jieba jieba;


    public:
        static void CutString(const std::string &src,std::vector<std::string> *out)//src是切分的对象
            {
                jieba.CutForSearch(src,*out);
            }
    };
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH,HMM_PATH,USER_DICT_PATH,IDF_PATH,STOP_WORD_PATH);

}
