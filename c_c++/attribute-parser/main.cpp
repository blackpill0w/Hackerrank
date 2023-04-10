#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <vector>

using std::cout;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

inline string ltrim(const string& str)
{
   return str.substr(str.find_first_not_of(" \t\n"));
}

inline string rtrim(const string& str)
{
   return str.substr(0, str.find_last_not_of(" \t\n") + 1);
}

inline string trim(const string& str)
{
   return ltrim(rtrim(str));
}

void err_and_exit(const string& err_msg, const int line_num = -1)
{
   if (line_num >= 1)
      std::cerr << "Line " << line_num << "\n\t";
   std::cerr << err_msg << '\n';
   exit(1);
}

struct Tag
{
   Tag*                    parent{ nullptr };
   string                  name{};
   vector<shared_ptr<Tag>> children{};
   map<string, string>     attrs{};

   Tag()
   {
   }
   Tag(const string& name) : name(name)
   {
   }

   string get_name() const
   {
      return name;
   }

   void add_attr(const string& attr, const string& val)
   {
      attrs[attr] = val;
   }

   Tag& new_child()
   {
      children.emplace_back(make_shared<Tag>(Tag{}));
      children.back()->parent = this;
      return *children.back();
   }
};

std::ostream& operator<<(std::ostream& o, const Tag& t)
{
   o << "Tag name: " << t.name << '\n';
   o << "Parent: " << (t.parent == nullptr ? "nullptr" : t.parent->get_name()) << '\n';
   cout << "         ----------------Attributes-----------------        \n";
   for (auto& [attr, val] : t.attrs)
      cout << "  --  `" << attr << "` = `" << val << "`\n";
   cout << "         -----------------Children------------------        \n";
   for (auto& child : t.children)
      cout << *child;
   cout << "         -------------------------------------------        \n";
   return o;
}

Tag build_tag_tree(const string& input)
{
   std::stringstream ss{ input };
   Tag               curr_origin{};
   Tag*              curr = &curr_origin;
   string            line{};
   // For err messages
   int line_num = 0;
   while (std::getline(ss, line))
   {
      ++line_num;

      std::smatch sm;
      // Match closing or opening tag
      const std::regex tag_regex{ "<((\\S+(( )+[\\S=]+( )*=( )*\"[^\"]+\")*( )*)|(/\\S+))>" };
      if (!std::regex_match(line, sm, tag_regex))
         err_and_exit("Syntax error", line_num);
      // Remove <>
      line = line.substr(1, line.size() - 2);

      if (line[0] == '/')
      {
         auto closing_tag_name = line.substr(1, line.size() - 1);
         if (closing_tag_name != curr->get_name())
            err_and_exit("Closing inexisting tag: " + closing_tag_name, line_num);
         else if (curr->parent) // Keep root tag to return it
            curr = curr->parent;
      }
      else
      {
         if (curr == nullptr)
         {
            curr_origin = Tag{};
            curr        = &curr_origin;
         }
         else
            curr = &curr->new_child();

         line = trim(line);
         // Get tag's name
         curr->name = line.substr(0, line.find(' '));
         line.erase(0, line.find(' '));

         string           attr{}, val{};
         const std::regex attr_val_regex{ "( )*[\\S=]+( )*=( )*\"[^\"]+\"( )*" };
         while (std::regex_search(line, sm, attr_val_regex))
         {
            auto line2 = line;
            line2      = trim(line2);
            auto eqi   = line2.find('=');
            attr       = trim(line2.substr(0, eqi));
            line2      = trim(line2.erase(0, line2.find('=') + 1));
            line2.erase(0, line2.find('\"') + 1);
            val = trim(line2.substr(0, line2.find('"')));

            curr->add_attr(attr, val);
            line = sm.suffix();
         }
      }
   }
   if (curr && curr->parent != nullptr)
      err_and_exit("Missing closing tag");
   return curr_origin;
}

std::optional<string> get_attr_val(Tag& tag_tree, const string& input)
{
   std::optional<string> res{};
   Tag*                  tag = &tag_tree;
   string                token{};

   for (auto c : input)
   {
      if (c == '~')
      {
         if (tag == nullptr)
         {
            if (token != tag_tree.get_name())
               break;
            tag = &tag_tree;
         }
         else
         {
            auto has_name = [&](const shared_ptr<Tag>& t) { return t->name == token; };
            auto child    = find_if(tag->children.begin(), tag->children.end(), has_name);
            if (child == tag->children.end())
               break;
            tag = child->get();
         }
         token = input.substr(input.find('~') + 1);
         if (tag->attrs.find(token) != tag->attrs.end())
            res = tag->attrs.at(token);
      }
      else if (c == '.')
      {
         if (tag == nullptr)
         {
            if (token != tag_tree.get_name())
            {
               break;
            }
            tag = &tag_tree;
         }
         else
         {
            auto has_name = [&](const shared_ptr<Tag>& t) { return t->name == token; };
            auto child    = find_if(tag->children.begin(), tag->children.end(), has_name);
            if (child == tag->children.end())
               break;
            tag = child->get();
         }
         token.clear();
      }
      else
         token += c;
   }
   return res;
}

int main()
{
   int n, q;
   std::cin >> n >> q;
   string input{};
   getline(std::cin, input);
   input.clear();
   for (int i = 0; i < n; ++i)
   {
      string tmp{};
      std::getline(std::cin, tmp);
      input += tmp + "\n";
   }
   Tag tag = build_tag_tree(input);

   vector<string> queries{};
   for (int i = 0; i < q; ++i)
   {
      string tmp{};
      std::getline(std::cin, tmp);
      queries.emplace_back(tmp);
   }
   for (auto& query : queries)
   {
      auto v = get_attr_val(tag, query);
      if (v.has_value())
         cout << *v;
      else
         cout << "Not Found!";
      cout << "\n";
   }
}
