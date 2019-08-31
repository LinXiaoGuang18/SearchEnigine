#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "WebPage.h"
#include "WordSegmentation.h"
using std::pair;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

namespace wd {
class WordQuery {
   public:
    WordQuery();

    void loadLibrary();
    string doQuery(const string& str);
    private:
    string returnNoAnswer();

   private:
    WordSegmentation _jieba;
    unordered_map<int, WebPage> _pageLib;
    unordered_map<string, set<pair<int, double>>> _invertIndex;
};

}  // namespace wd
