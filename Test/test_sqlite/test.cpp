#include <sqlite3.h>
#include <string>
#include <iostream>

struct studentInfo
{
    std::string _name;
    std::string _gender;
    int _age;
    double _gap;

    studentInfo(const std::string& name, const std::string& gender, int age, double gap)
    :_name(name),_gender(gender),_age(age),_gap(gap)
    {}
};

class studentDB
{
public:
    studentDB()
    {
        // 打开数据库文件，没有就创建
        if(SQLITE_OK != sqlite3_open("studentDB.db",&_db))
        {
            std::cout << "sqlite3_open fail" << std::endl;
            exit(1);
        }
        
        // 初始化数据库文件
        if(!initSqlite())
        {
            sqlite3_close(_db);
            _db = nullptr;
        }
    }

    bool insertSqlite(const studentInfo& stu)
    {
        // 创建sql语句
        const std::string insert = R"(
            insert into student(name,gender,age,gap) values (?,?,?,?);
        )";

        // 检查语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,insert.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            std::cerr<<"准备语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            return false;
        }

        // 绑定参数，填充？占位符
        sqlite3_bind_text(stmt,1,stu._name.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,stu._gender.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,3,stu._age);
        sqlite3_bind_double(stmt,4,stu._gap);

        // 执行语句
        if(SQLITE_DONE != sqlite3_step(stmt))
        {
            std::cerr<<"执行语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 清除
        sqlite3_finalize(stmt);
        return true;
    }

    bool queryAllInfo()
    {
        // 准备sql语句
        const std::string select = R"(
            select * from student;
        )";

        // 检验
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,select.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            std::cerr<<"准备语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            return false;
        }

        // 执行语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW && rc != SQLITE_DONE)
        {
            std::cerr<<"执行语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 提取结果
        std::cout<<"-------------所有学生信息-----------------"<<std::endl;
        std::cout<<"查询到学生信息："<<std::endl;
        while(rc == SQLITE_ROW)
        {
            int stuid = sqlite3_column_int(stmt,0);
            std::string queryName = (const char*)sqlite3_column_text(stmt,1);
            std::string queryGender = (const char*)sqlite3_column_text(stmt,2);
            int queryAge = sqlite3_column_int(stmt,3);
            double queryGap = sqlite3_column_double(stmt,4);

            // 打印结果
            std::cout<<"stuid: "<<stuid<<" "
            <<"name: "<<queryName<<" "
            <<"gender: "<<queryGender<<" "
            <<"age: "<<queryAge<<" "
            <<"gap: "<<queryGap<<std::endl;

            // 执行下一行
            rc = sqlite3_step(stmt);
        }

        // 清理
        sqlite3_finalize(stmt);
        return true;
    }

    // 查询某个学生信息
    bool queryInfo(const std::string& name)
    {
        // 准备sql语句
        const std::string select = R"(
            select * from student where name = ?;
        )";

        // 检查
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,select.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            std::cerr<<"准备语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            return false;
        }

        // 绑定参数，填充？占位符
        sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);

        // 执行语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW && rc != SQLITE_DONE)
        {
            std::cerr<<"执行语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 提取结果
        if(rc == SQLITE_ROW)
        {
            int stuid = sqlite3_column_int(stmt,0);
            std::string name = (const char*)sqlite3_column_text(stmt,1);
            std::string gender = (const char*)sqlite3_column_text(stmt,2);
            int age = sqlite3_column_int(stmt,3);
            double gap = sqlite3_column_double(stmt,4);
            
            // 打印信息
            std::cout<<"查询到学生信息："<<std::endl;
            std::cout<<"stuid: "<<stuid<<" "
            <<"name: "<<name<<" "
            <<"gender: "<<gender<<" "
            <<"age: "<<age<<" "
            <<"gap: "<<gap<<std::endl;
        }
        else
        {
            std::cerr<<"查询到的学生信息为空"<<std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    // 更新学生信息
    bool updateInfo(const std::string& name, const studentInfo& stu)
    {
        // 准备sql语句
        const std::string update = R"(
            update student set name = ?, gender = ?, age = ?, gap = ? where name = ?;
        )";

        // 检查
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,update.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            std::cerr<<"准备语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            return false;
        }

        // 绑定参数，填充？占位符
        sqlite3_bind_text(stmt,1,stu._name.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,stu._gender.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,3,stu._age);
        sqlite3_bind_double(stmt,4,stu._gap);
        sqlite3_bind_text(stmt,5,name.c_str(),-1,SQLITE_TRANSIENT);

        // 执行语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            std::cerr<<"执行语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    // 删除学生信息
    bool deleteInfo(const std::string& name)
    {
        // 准备sql语句
        const std::string deleteInfo = R"(
            delete from student where name = ?;
        )";

        // 检查
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,deleteInfo.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            std::cerr<<"准备语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            return false;
        }

        // 绑定参数，填充？占位符
        sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);

        // 执行语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            std::cerr<<"执行语句失败："<<sqlite3_errmsg(_db)<<std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    ~studentDB()
    {
        if(_db)
            sqlite3_close(_db);
    }
private:
    bool initSqlite()
    {
        // 创建一个字符串用来表示创建表结构
        const std::string studentTable = R"(
            create table if not exists student(
                stuid integer primary key AUTOINCREMENT,
                name text,
                gender text,
                age integer,
                gap real
            );
        )";

        int rc = sqlite3_exec(_db,studentTable.c_str(),nullptr,nullptr,nullptr);
        if(rc != SQLITE_OK)
        {
            std::cerr<<"创建表失败："<<sqlite3_errmsg(_db)<<std::endl;
            return false;
        }
        return true;
    }
private:
    sqlite3* _db = nullptr;
};

int main()
{
    studentInfo stu1("张三","男",18,3.5);
    studentInfo stu2("李四","男",19,3.6);
    studentInfo stu3("王五","男",20,3.7);
    studentInfo stu4("赵六","男",21,3.8);

    studentDB stuDB;
    stuDB.insertSqlite(stu1);
    stuDB.insertSqlite(stu2);
    stuDB.insertSqlite(stu3);
    stuDB.insertSqlite(stu4);
    std::cout << "insertDB success" << std::endl;

    stuDB.queryAllInfo();

    stuDB.queryInfo("张三");

    stu1._gap = 5.0;
    stuDB.updateInfo("张三",stu1);

    std::cout << "----------------------" << std::endl;
    stuDB.queryInfo("张三");


    stuDB.deleteInfo("张三");
    stuDB.queryAllInfo();

    return 0;
}