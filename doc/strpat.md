## AC自动机（Aho-Corasick Automaton）和暴力匹配的效率比较

随机生成以下数据：

* `std::vector<int> text`：长度为`10000000`的整型数组，其中的元素的值在`0`到`MAX_VALUE`之间。
* `std::vector< std::vector<int> > patterns`：包含`N`个串的数组，平均长度为`PATTERN_LENGTH`。

### 结果

<style type="text/css">
.tg  {border-collapse:collapse;border-spacing:0;}
.tg td{font-family:Arial, sans-serif;font-size:14px;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;}
.tg th{font-family:Arial, sans-serif;font-size:14px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;}
</style>

#### 和关键字数量的关系

<table class="tg">
  <tr>
    <th class="tg-031e">关键字数量</th>
    <th class="tg-031e">平均关键字长度</th>
    <th class="tg-031e">文本长度</th>
    <th class="tg-031e">AC自动机所用时间</th>
    <th class="tg-031e">暴力搜索所用时间(秒)</th>
  </tr>
  <tr>
    <td class="tg-031e">1<br></td>
    <td class="tg-031e">6.0<br></td>
    <td class="tg-031e">10000000<br></td>
    <td class="tg-031e">0.150766s</td>
    <td class="tg-031e">0.021711s</td>
  </tr>
  <tr>
    <td class="tg-031e">10<br></td>
    <td class="tg-031e">5.9<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">0.385894s</td>
    <td class="tg-031e">0.191972s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">5.5<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">0.486919s</td>
    <td class="tg-031e">1.655898s</td>
  </tr>
  <tr>
    <td class="tg-031e">1000<br></td>
    <td class="tg-031e">5.5<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">0.755461s</td>
    <td class="tg-031e">15.750251s</td>
  </tr>
</table>

#### 和关键字长度的关系

<table class="tg">
  <tr>
    <th class="tg-031e">关键字数量</th>
    <th class="tg-031e">平均关键字长度</th>
    <th class="tg-031e">文本长度</th>
    <th class="tg-031e">AC自动机所用时间</th>
    <th class="tg-031e">暴力搜索所用时间(秒)</th>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">1.0<br></td>
    <td class="tg-031e">10000000<br></td>
    <td class="tg-031e">0.454665s</td>
    <td class="tg-031e">0.000144s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">5.6<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">0.356356s</td>
    <td class="tg-031e">1.711832s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">54.9<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">0.514199s</td>
    <td class="tg-031e">2.023048s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">508.8<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">0.492695s</td>
    <td class="tg-031e">2.100005s</td>
  </tr>
</table>

#### 和元素取值范围的关系

<table class="tg">
  <tr>
    <th class="tg-031e">关键字数量</th>
    <th class="tg-031e">平均关键字长度</th>
    <th class="tg-031e">文本长度</th>
    <th class="tg-031e">元素取值范围 </th>
    <th class="tg-031e">AC自动机所用时间</th>
    <th class="tg-031e">暴力搜索所用时间(秒)</th>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">5.0<br></td>
    <td class="tg-031e">10000000<br></td>
    <td class="tg-031e">&lt;2<br></td>
    <td class="tg-031e">0.562211s</td>
    <td class="tg-031e">0.000146s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">5.3<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">&lt;26<br></td>
    <td class="tg-031e">0.561658s</td>
    <td class="tg-031e">1.497082s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">5.3<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">&lt;100<br></td>
    <td class="tg-031e">0.429732s</td>
    <td class="tg-031e">1.637869s</td>
  </tr>
  <tr>
    <td class="tg-031e">100<br></td>
    <td class="tg-031e">5.1<br></td>
    <td class="tg-031e">10000000</td>
    <td class="tg-031e">&lt;1000000<br></td>
    <td class="tg-031e">0.335486s</td>
    <td class="tg-031e">1.756824s</td>
  </tr>
</table>

#### 结论

AC自动机的用时非常稳定，基本等于（文本长度+关键字长度之和）\*一个常数；

暴力搜索受关键字数量的影响很大，关键字数量很多的时候所用的时间会很多，关键字长度和元素取值范围对它的影响在超过一定范围之后就很小了。


## 源代码

### automaton.h


    #include "automaton.h"
    #include "brute_force.h"
    
    #include <cstdlib>
    #include <ctime>
    #include <cmath>
    
    #include <vector>
    #include <cstdio>
    
    int MAX_LENGTH = 10000000;
    int PATTERN_LENGTH = 100;
    int MAX_VALUE = 128;
    
    void test1(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns) {
      printf("find pattern-strings in text using automaton:\n");
      clock_t start_time = clock();
      std::set<int> which = text_matching(text, patterns);
      printf("<%lf> second past\n", double(clock() - start_time) / CLOCKS_PER_SEC);
      printf("[%u] patterns have been found\n", which.size());
    }
    
    void test2(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns) {
      printf("find pattern-strings in text using brute force:\n");
      clock_t start_time = clock();
      std::set<int> which = brute_force_text_matching(text, patterns);
      printf("<<%lf>> second past\n", double(clock() - start_time) / CLOCKS_PER_SEC);
      printf("[[%u]] patterns have been found\n", which.size());
    }
    
    int main() {
      srand(time(0));
    
      for (int i = 0; i < 6; ++i) {
        if (i < 100) MAX_VALUE = 1000;
        else if (i == 0) MAX_VALUE = 2;
        else if (i == 1) MAX_VALUE = 26;
        else if (i == 2) MAX_VALUE = 1000;
        else MAX_VALUE = 1000000;
    
        PATTERN_LENGTH = pow(10, i);
        // PATTERN_LENGTH = 10;
    
        std::vector<int> text(MAX_LENGTH);
        for (size_t i = 0; i < text.size(); ++i) {
          text[i] = rand() % MAX_VALUE;
        }
    
        double average_length = 0;
        std::vector< std::vector<int> > patterns(100);
        for (size_t i = 0; i < patterns.size(); ++i) {
          patterns[i].resize(rand() % PATTERN_LENGTH + 1);
          average_length += patterns[i].size();
          for (size_t j = 0; j < patterns[i].size(); ++j) {
            patterns[i][j] = rand() % MAX_VALUE;
          }
        }
    
        printf("--- Test #%d: ---\n", i);
        printf("MAX_VALUE = %d, text length = %d, "
            " %d patterns, average length = %.1lf\n",
            MAX_VALUE, text.size(), patterns.size(),
            average_length / patterns.size());
        test1(text, patterns);
        test2(text, patterns);
      }
    
      return 0;
    }


### automaton.cpp

    #include "automaton.h"
    
    #include <cstdio>
    
    template<typename Key_t, typename Value_t>
    class HashMap {
    private:
      unsigned long seed;
      int cap;
      int size;
    
    private:
      std::vector< std::list< std::pair<Key_t, Value_t> > > head;
    
    private:
      unsigned long rand() {
        return seed = (seed * 12 + 4) % 1991;
      }
    
      void reallocate() {
        cap = cap * 2 + rand() % 16;
        std::vector< std::list< std::pair<Key_t, Value_t> > > pre_head;
        pre_head.swap(head);
        head.resize(cap);
        for (size_t i = 0; i < head.size(); ++i) {
          std::list< std::pair<Key_t, Value_t> > &list = head[i];
          typename std::list< std::pair<Key_t, Value_t> >::iterator iter;
          for (iter = list.begin(); iter != list.end(); ++iter) {
            this->insert(iter->first, iter->second);
          }
        }
      }
    
    public:
      HashMap(): cap(2), size(0) {
        seed = time(0) % 1991;
        head.resize(cap);
      }
    
      void insert(const Key_t &key, const Value_t &value) {
        if (size <= cap / 2) {
          reallocate();
        }
        ++size;
        std::list< std::pair<Key_t, Value_t> > &list = head[key % cap];
        typename std::list< std::pair<Key_t, Value_t> >::iterator iter;
        for (iter = list.begin(); iter != list.end(); ++iter) {
          if (iter->first == key) {
            iter->second = value;
            return;
          }
        }
        list.insert(list.begin(), std::make_pair(key, value));
      }
    
      bool has(const Key_t &key) {
        std::list< std::pair<Key_t, Value_t> > &list = head[key % cap];
        typename std::list< std::pair<Key_t, Value_t> >::iterator iter;
        for (iter = list.begin(); iter != list.end(); ++iter) {
          if (iter->first == key) {
            return true;
          }
        }
        return false;
      }
    
      bool get(const Key_t &key, Value_t &value) {
        std::list< std::pair<Key_t, Value_t> > &list = head[key % cap];
        typename std::list< std::pair<Key_t, Value_t> >::iterator iter;
        for (iter = list.begin(); iter != list.end(); ++iter) {
          if (iter->first == key) {
            value = iter->first;
            return true;
          }
        }
        return false;
      }
    };
    
    
    #include <unordered_map>
    
    struct Node;
    
    // typedef std::map<int, Node *> MyMap;
    typedef std::unordered_map<int, Node *> MyMap;
    
    struct Node {
      MyMap go;
      Node *fail;
      int id;
    
      Node(): id(-1) {
      }
    };
    
    void insert_string(Node *ptr, int id, const std::vector<int> &str) {
      for (size_t i = 0; i < str.size(); ++i) {
        if (ptr->go.count(str[i]) == 0) {
          ptr->go[str[i]] = new Node;
        }
        ptr = ptr->go[str[i]];
      }
      ptr->id = id;
    }
    
    void init_fail(Node *root) {
      std::queue<Node *> q;
      MyMap::iterator iter;
      root->fail = root;
      for (iter = root->go.begin(); iter != root->go.end(); ++iter) {
        iter->second->fail = root;
        q.push(iter->second);
      }
      while (!q.empty()) {
        Node *cur = q.front();
        q.pop();
        for (iter = cur->go.begin(); iter != cur->go.end(); ++iter) {
          int ch = iter->first;
          Node *next = cur->fail;
          while (next->fail != next && next->go.count(ch) == 0) {
            next = next->fail;
          }
          iter->second->fail = next->go.count(ch) ? next->go[ch] : next;
          q.push(iter->second);
        }
      }
    }
    
    /*
    void init_contain(Node *cur) {
      for (Node *next = cur; next != next->fail; next = next->fail) {
        for (int p: next->id) {
          cur->id.insert(p);
        }
      }
    
      MyMap::iterator iter;
      for (iter = cur->go.begin(); iter != cur->go.end(); ++iter) {
        init_contain(iter->second);
      }
    }
    */
    
    void dispose(Node *cur) {
      MyMap::iterator iter;
      for (iter = cur->go.begin(); iter != cur->go.end(); ++iter) {
        dispose(iter->second);
      }
      delete cur;
    }
    
    Node *trans(Node *cur, int ch) {
      while (cur->go.count(ch) == 0) {
        if (cur->fail == cur) break;
        cur = cur->fail;
      }
      return cur->go.count(ch) ? cur->go[ch] : cur;
    }
    
    bool contains(const std::vector<int> &a, const std::vector<int> &b) {
      if (b.size() > a.size()) {
        return false;
      }
      for (size_t i = 0; i + b.size() <= a.size(); ++i) {
        bool ok = true;
        for (size_t j = 0; j < b.size(); ++j) {
          if (a[i + j] != b[j]) {
            ok = false;
            break;
          }
        }
        if (ok) {
          return true;
        }
      }
      return false;
    }
    
    std::set<int> text_matching(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns) {
      Node *root = new Node;
      for (size_t i = 0; i < patterns.size(); ++i) {
        insert_string(root, i, patterns[i]);
      }
      init_fail(root);
    //  init_contain(root);
    
      std::vector<int> has(patterns.size(), 0);
      Node *cur = root;
      for (size_t i = 0; i < text.size(); ++i) {
        cur = trans(cur, text[i]);
        if (cur->id != -1) {
          has[cur->id] = 1;
    //      printf("found string[%d] at position[%d]\n",
    //          i, i - patterns[i].size() + 1);
        }
      }
    
      dispose(root);
    
      for (size_t i = 0; i < patterns.size(); ++i) {
        if (has[i]) {
          for (size_t j = 0; j < patterns.size(); ++j) {
            if (j != i && !has[j] && contains(patterns[i], patterns[j])) {
              has[j] = 1;
            }
          }
        }
      }
    
      std::set<int> is;
      for (size_t i = 0; i < patterns.size(); ++i) {
        if (has[i]) {
          is.insert(i);
        }
      }
      return is;
    }


### brute\_force.h

    #ifndef BRUTE_FORCE_H
    #define BRUTE_FORCE_H
    
    #include <set>
    #include <vector>
    #include <algorithm>
    
    std::set<int> brute_force_text_matching(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns);
    
    #endif


### brute\_force.cpp


    #include "brute_force.h"
    
    bool search_pattern(
        const std::vector<int> &text,
        const std::vector<int> &pattern) {
      for (size_t i = 0; i < text.size(); ++i) {
        bool ok = true;
        for (size_t j = 0; j < pattern.size(); ++j) {
          if (i + j >= text.size() || text[i + j] != pattern[j]) {
            ok = false;
            break;
          }
        }
        if (ok) {
          return true;
        }
      }
      return false;
    }
    
    std::set<int> brute_force_text_matching(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns) {
      std::set<int> is;
      for (size_t i = 0; i < patterns.size(); ++i) {
        if (search_pattern(text, patterns[i])) {
          is.insert(i);
        }
      }
      return is;
    }


### main.cpp

    #include "automaton.h"
    #include "brute_force.h"
    
    #include <cstdlib>
    #include <ctime>
    #include <cmath>
    
    #include <vector>
    #include <cstdio>
    
    int MAX_LENGTH = 10000000;
    int PATTERN_LENGTH = 100;
    int MAX_VALUE = 128;
    
    void test1(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns) {
      printf("find pattern-strings in text using automaton:\n");
      clock_t start_time = clock();
      std::set<int> which = text_matching(text, patterns);
      printf("<%lf> second past\n", double(clock() - start_time) / CLOCKS_PER_SEC);
      printf("[%u] patterns have been found\n", which.size());
    }
    
    void test2(
        const std::vector<int> &text,
        const std::vector< std::vector<int> > &patterns) {
      printf("find pattern-strings in text using brute force:\n");
      clock_t start_time = clock();
      std::set<int> which = brute_force_text_matching(text, patterns);
      printf("<<%lf>> second past\n", double(clock() - start_time) / CLOCKS_PER_SEC);
      printf("[[%u]] patterns have been found\n", which.size());
    }
    
    int main() {
      srand(time(0));
    
      for (int i = 0; i < 6; ++i) {
        if (i < 100) MAX_VALUE = 1000;
        else if (i == 0) MAX_VALUE = 2;
        else if (i == 1) MAX_VALUE = 26;
        else if (i == 2) MAX_VALUE = 1000;
        else MAX_VALUE = 1000000;
    
        PATTERN_LENGTH = pow(10, i);
        // PATTERN_LENGTH = 10;
    
        std::vector<int> text(MAX_LENGTH);
        for (size_t i = 0; i < text.size(); ++i) {
          text[i] = rand() % MAX_VALUE;
        }
    
        double average_length = 0;
        std::vector< std::vector<int> > patterns(100);
        for (size_t i = 0; i < patterns.size(); ++i) {
          patterns[i].resize(rand() % PATTERN_LENGTH + 1);
          average_length += patterns[i].size();
          for (size_t j = 0; j < patterns[i].size(); ++j) {
            patterns[i][j] = rand() % MAX_VALUE;
          }
        }
    
        printf("--- Test #%d: ---\n", i);
        printf("MAX_VALUE = %d, text length = %d, "
            " %d patterns, average length = %.1lf\n",
            MAX_VALUE, text.size(), patterns.size(),
            average_length / patterns.size());
        test1(text, patterns);
        test2(text, patterns);
      }
    
      return 0;
    }
