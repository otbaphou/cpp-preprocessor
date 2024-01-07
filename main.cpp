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

vector<string> UnpackFile(path& in_file, const vector<path>& include_directories, size_t lineNum)
{
    static regex include_reg_f(R"(\s*#\s*include\s*"([^"]*)\"\s*)");
    static regex include_reg_s(R"(\s*#\s*include\s*<([^>]*)>\s*)");
    
    vector<string> rows;
    
    ifstream inFile(in_file);
    
    if(!inFile.is_open())
        return {};
    
    string line;
    while(getline(inFile, line))
    {
        smatch match;
        if(regex_match(line, match, include_reg_f))
        {
            path p = in_file.parent_path() / string(match[1]);
            
            vector<string> lines = UnpackFile(p, include_directories, lineNum);
            
            if(!lines.empty())
            {

            for(string& line_ : lines)
                rows.push_back(line_);
            }else
            {
                bool found = false;
                
                for(auto& dir : include_directories)
                {
                    path tmp = dir / string(match[1]);
                    if(filesystem::exists(tmp))
                    {
                        ifstream includeFile(tmp);

                        for(string& line_ : UnpackFile(tmp, include_directories, lineNum))
                    rows.push_back(line_);
                    
                        found=true;
                        break;
                    }
                }
                
                    if (!found) 
                    {
                    cout << "unknown include file " << p.filename().string() << " at file " << in_file.string() << " at line " << lineNum << endl;
                    return rows;
                    }
            }
        }else if(regex_match(line, match, include_reg_s))
        {
            bool found = false;
            path p = string(match[1]);
            
            for(auto& dir : include_directories)
            {
                path tmp = dir / p;
                if(filesystem::exists(tmp))
                {
                    ifstream includeFile(tmp);

                    for(string& line_ : UnpackFile(tmp, include_directories, lineNum))
                rows.push_back(line_);
                    
                    found=true;
                    break;
                }
                if (!found) 
                {
                cout << "unknown include file " << p.filename().string() << " at file " << in_file.string() << " at line " << lineNum << endl;
                return rows;
                }
            }
        }else
           rows.push_back(line); 
    }
    return rows;
}
                 

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    static regex include_reg_f(R"(\s*#\s*include\s*"([^"]*)\"\s*)");
    static regex include_reg_s(R"(\s*#\s*include\s*<([^>]*)>\s*)");
    const path pPath = in_file.parent_path();
    
    ifstream inFile(in_file);
    if(!inFile.is_open())
        return false;
    ofstream outFile(out_file);
    
    string line;
    size_t lineNum = 1;
    while(getline(inFile, line))
    {
        smatch match;
        if(regex_match(line, match, include_reg_f))
        {        
            path p = string(match[1]);
            path includePath = in_file.parent_path() / p;
            
            bool found = false;
            
            if(filesystem::exists(includePath))
                found = true;
            else
            {
                for(auto& dir : include_directories)
                {
                    path tmp = dir / p;
                    if(filesystem::exists(tmp))
                    {
                        includePath = tmp;
                        found=true;
                        break;
                    }
                }
            }
            
            if(!found)
            { 
                cout << "unknown include file " << p.filename().string() << " at file " << in_file.string() << " at line " << lineNum << endl;
                return false;
            }

            ifstream includeFile(includePath);
            if(!includeFile.is_open())
            { 
                cout << "unknown include file " << p.filename().string() << " at file " << in_file.string() << " at line " << lineNum << endl;
                return false;
            }
            
            for(string& line_ : UnpackFile(includePath, include_directories, lineNum))
            {
                outFile << line_ << '\n';
            }
            
        }else if(regex_match(line, match, include_reg_s))
        {

            path p = string(match[1]);
            bool found = false;
            
            for(auto& dir : include_directories)
            {
                path tmp = dir / p;
                if(filesystem::exists(tmp))
                {
                    p = tmp;
                    found=true;
                    break;
                }

            }
                if (!found) 
                {
                cout << "unknown include file " << p.filename().string() << " at file " << in_file.string() << " at line " << lineNum << endl;
                return false;
                }
            vector<string> lines = UnpackFile(p, include_directories, lineNum);
            for(string& line_ : lines)
            {
                outFile << line_ << '\n';
            }
            if (lines.empty()) 
            {
            cout << "unknown include file " << p.filename().string() << " at file " << in_file.string() << " at line " << lineNum << endl;
            return false;
            }
            
        }else
        {
            outFile << line << endl;
        }
        lineNum++;
    }
    
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

    //assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));
    
Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, {"sources"_p / "include1"_p,"sources"_p / "include2"_p});
    
    cout << GetFileContents("a.in");
    
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

    //assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
