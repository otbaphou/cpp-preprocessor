#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

pair<vector<string>, bool> UnpackFile(const path& in_file, const vector<path>& include_directories, size_t line_num)
{
    static regex include_reg_f(R"(\s*#\s*include\s*"([^"]*)\"\s*)");
    static regex include_reg_s(R"(\s*#\s*include\s*<([^>]*)>\s*)");
    
    vector<string> rows;
    
    ifstream in_file_unpack(in_file);
    
    if(!in_file_unpack.is_open())
        return {{}, false};
    
    string line;
    while(getline(in_file_unpack, line))
    {
        smatch match;
        if(regex_match(line, match, include_reg_f) || regex_match(line, match, include_reg_s))
        {
            path file_path = in_file.parent_path() / string(match[1]);
            
            pair<vector<string>, bool> lines = UnpackFile(file_path, include_directories, line_num);
            
            if(!lines.first.empty())
            {
                for(string& line_ : lines.first)
                {
                    rows.push_back(line_);
                }
                line_num++;
                continue;
            }

                bool found = false;
                
                for(auto& dir : include_directories)
                {
                    path tmp = dir / string(match[1]);
                    if(filesystem::exists(tmp))
                    {
                        ifstream include_file(tmp);
                        
                        for(string& line_ : UnpackFile(tmp, include_directories, line_num).first)
                        rows.push_back(line_);
                    
                        found = true;
                        break;
                    }
                }
                
                if (!found) 
                {
                cout << "unknown include file " << file_path.filename().string()
                << " at file " << in_file.string() << " at line " << line_num << endl;
                return {rows, false};
                }
        }
        else
        {
           rows.push_back(line); 
        }
        line_num++;
    }
    return {rows, true};
}
                 

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    const path pPath = in_file.parent_path();
    
    ifstream in_file_preprocess_(in_file);
    if(!in_file_preprocess_.is_open())
    {
        return false;
    }
    ofstream out_file_preprocess_(out_file);
    
    string line;
    pair<vector<string>, bool> lines = UnpackFile(in_file, include_directories, 1);
    
    for(string& line_ : lines.first)
    {
        out_file_preprocess_ << line_ << '\n';
    }
    
    if(!lines.second)
        return false;
    
    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "// text between b.h and c.h\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));
    
    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;
    //cout << GetFileContents("a.in");
    
    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
