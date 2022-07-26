#include <iostream>
#include<string>
#include<vector>
#include<jsoncpp/json/json.h>
//#include<boost/algorithm/string.hpp>

int main()
{
    //Json:Value Reader Writer
    Json::Value root;
    Json::Value test1;
    test1["key1"] = "value1";
    test1["key2"] = "value2";

    Json::Value test2;
    test2["key1"] = "value3";
    test2["key2"] = "value4";

    root.append(test1);
    root.append(test2);

    Json::StyledWriter writer;//FastWriter直接输出在一行
    std::string s = writer.write(root);
    std::cout << s << std::endl;


    //std::vector<std::string> result;
    //std::string target = "aaa\3\3\3\3bbb\3ccc";
    //boost::split(result,target,boost::is_any_of("\3"),boost::token_compress_on);//boost::token_compress_on默认是off

    //for(auto &s : result)
    //{
    //    std::cout << s << std::endl;
    //} 
    return 0;
}

