#include "PageLibPreprocessor.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include "Configuration.h"
#include "tinyxml2.h"
using std::cout;
using std::endl;
using std::make_pair;
using std::ofstream;
using namespace tinyxml2;

namespace wd {
PageLibPreprocessor::PageLibPreprocessor() { _pageLib.reserve(400); }

void PageLibPreprocessor::doProcess() {
    readPageFromFile();

    auto start = std::chrono::system_clock::now();
    cutRedundantPages();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << ">> cut redundant pages success, use time: "
         << elapsed_seconds.count() << " sec" << endl;

    buildInvertIndex();
    auto end2 = std::chrono::system_clock::now();
    elapsed_seconds = end2 - end;
    cout << ">> build invert index success, use time: "
         << elapsed_seconds.count() << " sec" << endl;

    elapsed_seconds = end2-start;
    cout << ">> total time cost: " << elapsed_seconds.count() << " sec" << endl;
    store();
}

void PageLibPreprocessor::readPageFromFile() {
    XMLDocument doc;
    doc.LoadFile(CONFIG[RIPEPAGE_PATH].c_str());
    XMLElement* page = doc.FirstChildElement("doc");

    do {
        string docid = page->FirstChildElement("docid")->GetText();
        string title = page->FirstChildElement("title")->GetText();
        string link = page->FirstChildElement("link")->GetText();
        string content = page->FirstChildElement("content")->GetText();

        _pageLib.emplace_back(std::stoi(docid), title, link, content);
    } while (page = page->NextSiblingElement());
}

void PageLibPreprocessor::cutRedundantPages() {
    cout << ">> before cut: " << _pageLib.size() << endl;
    for (auto& page : _pageLib) {
        page.generateSimhash();
    }
    std::sort(_pageLib.begin(), _pageLib.end());

    /* ofstream
    ofsSort("/home/whb/project/RssSearchEngine/offline/data/sorted.dat"); for
    (auto& page : _pageLib) { string temp = page.getDoc(); ofsSort <<
    page.getSimhash() << '\n' << temp;
    }
    cout << ">> store sorted ripepage success" << endl;
    ofsSort.close();  */

    auto it = std::unique(_pageLib.begin(), _pageLib.end());

    _pageLib.erase(it, _pageLib.end());
    cout << ">> after cut: " << _pageLib.size() << endl;
}

void PageLibPreprocessor::buildInvertIndex() {
    for (auto& page : _pageLib) {
        page.buildWordsMap();
    }

    for (auto& page : _pageLib) {
        unordered_map<string, int> wordsMap = page.getWordsMap();
        for (auto& wordFreq : wordsMap) {
            _invertIndexTable[wordFreq.first].push_back(
                make_pair(page.getDocId(), wordFreq.second));
        }
    }
    //保存每篇文档中所有词的权重的平方和, int为docid
    unordered_map<int, double> weightSum;  

    int totalPageNum = _pageLib.size();
    for (auto& elem : _invertIndexTable) {
        int df = elem.second.size();  //关键词在所有文章中出现的次数
        double idf = log2(
            static_cast<double>(totalPageNum / (df + 1)));  //关键词的逆文档频率

        for (auto& item : elem.second) {
            double weight = item.second * idf;
            item.second = weight;
            //计算每篇文档中词语的权重
            weightSum[item.first] += pow(weight, 2);
        }
    }

    for (auto& elem : _invertIndexTable) {
        for (auto& item : elem.second) {
            //归一化处理
            item.second = item.second / sqrt(weightSum[item.first]);
        }
    }
}

void PageLibPreprocessor::store() {
    ofstream ofsPage(CONFIG[NEW_RIPEPAGE_PATH]);
    ofstream ofsOffset(CONFIG[NEW_OFFSET_PATH]);

    for (auto& page : _pageLib) {
        int id = page.getDocId();
        string temp = page.getDoc();
        ofstream::pos_type offset = ofsPage.tellp();
        size_t length = temp.size();
        ofsPage << temp;
        ofsOffset << id << '\t' << offset << '\t' << length << '\n';
    }
    cout << ">> store new ripepage and offset success" << endl;
    ofsPage.close();
    ofsOffset.close();

    ofstream ofsIndex(CONFIG[INDEX_PATH]);

    for (auto& elem : _invertIndexTable) {
        ofsIndex << elem.first << '\t';
        for (auto& item : elem.second) {
            ofsIndex << item.first << '\t' << item.second << '\t';
        }
        ofsIndex << '\n';
    }

    _invertIndexTable.clear();//不清空会造成栈溢出
    ofsIndex.close();
    cout << ">> store invert index success" << endl;
}

}  // namespace wd