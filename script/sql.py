import os
import json
import platform
import re

VERSION = '0.1'

# sql类型
sql_type_map = {
    'BIGINT': "int64",
    'bigint': "int64",
    
    'INT': "int32",
    'int': "int32",

    'SMALLINT': "int16",
    'smallint': "int16",

    'TINYINT': "int8",
    'tinyint': "int8",

    'DOUBLE': "double",
    'double': "double",

    'FLOAT': "float",
    'float': "float",

    'BOOLEAN': "bool",
    'boolean': "bool",

    'VARCHAR(255)': "string",
    'varchar(255)': "string",

    'TEXT': "text",
    'text': "text",

    'BLOB': "blob",
    'blob': "blob"
}

# cpp类型
cpp_type_map = {
    "int64": "int64_t",
    "uint64": "uint64_t",
    "int32": "int32_t",
    "uint32": "uint32_t",
    "int16": "int16_t",
    "uint16": "uint16_t",
    "int8": "int8_t",
    "uint8": "uint8_t",
    "double": "double",
    "float": "float",
    "bool": "bool",
    "string": "std::string",
    "text": "std::string",
    "blob": "FByteArray"
}

def to_upper_camel_case(x):
    """转大驼峰法命名"""
    s = re.sub('_([a-zA-Z])', lambda m: (m.group(1).upper()), x)
    return s[0].upper() + s[1:]

def parse_table_field(line: str) -> dict:
    """解析单行table字段"""
    if line.endswith(','):
        line = line[:-1]

    field_info = {
        "null": True,
        "default": ""
    }
    temp = line.split(' ')

    # 类型解析
    field_info["name"] = temp[0][1:-1]
    field_info["type"] = sql_type_map[temp[1].strip()]

    if temp[2] and (temp[2] == "UNSIGNED" or temp[2] == "unsigned"):
        field_info["type"] = 'u' + field_info["type"]

    for idx, str in enumerate(temp):
        # 默认值
        if str.startswith('DEFAULT'):
            field_info['default'] = temp[idx + 1]

        # 注释
        if str.startswith('COMMENT'):
            field_info['comment'] = temp[idx + 1][1:-1]

        if str.startswith('NOT'):
            if temp[idx + 1] == "NULL":
                field_info['null'] = False

    return field_info


def generate_orm_clazz(src: str, dist: str, desc: str):
    """生成ORM CPP类"""

    assert src, 'sql input path error'
    assert dist, 'orm output path error'
    assert desc, 'desc output path error'

    print(f'-- SQL input path: {os.getcwd()}\\{src}')
    print(f'-- ORM output path: {os.getcwd()}\\{dist}')
    print(f'-- Describe output path: {os.getcwd()}\\{desc}')

    if not os.path.exists(dist):
        os.makedirs(dist)
        print("-- Creating ORM folder")

    sql_list = {}
    table_name_set = set()

    for root, dirs, files in os.walk(src):
        for file in files:
            if not file.endswith('.sql'):
                continue

            table_info = {}
            table_info['field'] = {}
            table_info['origin'] = ""
            table_info['key'] = []

            next = False

            with open(os.path.join(root, file), 'r', encoding='utf-8') as file:
                # sql文件名（无.sql后缀）
                file_name = os.path.basename(file.name)
                file_name, file_extension = os.path.splitext(file_name)
                sql_list[file_name] = []
            
                line = file.readline()
                while line:
                    line = line.strip()

                    # 表名
                    if line.startswith("CREATE TABLE"):
                        if next:
                            if table_info['name'] in table_name_set:
                                raise NameError(f"{table_info['name']} redefined.") 
                    
                            table_name_set.add(table_info['name'])
                            
                            sql_list[file_name].append(table_info)
                            table_info = {}
                            table_info['field'] = {}
                            table_info['origin'] = ""
                            table_info['key'] = []
                        
                        next = True
                    
                        temp = line.split(' ')
                        table_info['name'] = temp[2][1:-1]
                        table_info['origin'] += line
                        if not line.endswith("("):
                            table_info['origin'] += " ("

                    # 字段
                    if line.startswith('`'):
                        table_info['origin'] += line
                        field_info = parse_table_field(line)
                        table_info['field'][field_info['name']] = field_info

                    # 键
                    if line.startswith("PRIMARY KEY"):
                        table_info['origin'] += line
                        position = line.index('(')
                        line = line[position + 1:-1]

                        temp = line.split(',')

                        for key in temp:
                            table_info['field'][key.strip()[1:-1]]['key'] = True
                            table_info['key'].append(key.strip()[1:-1])

                    # 表定义结束
                    if line.startswith(')'):
                        table_info['origin'] += line
                        line = line[1:]
                        if line.endswith(';'):
                            line = line[:-1]

                        temp = line.split(' ')
                        for idx, str in enumerate(temp):
                            if str == 'COMMENT':
                                table_info['comment'] = temp[idx+1][1:-1]

                    line = file.readline()   

                if next:
                    if table_info['name'] in table_name_set:
                        raise NameError(f"{table_info['name']} redefined.") 
                    
                    table_name_set.add(table_info['name'])
                    sql_list[file_name].append(table_info)

                print(f"-- \t{file.name} loaded")    

    # 生成JSON数据文件
    with open(desc, 'w', encoding='utf-8') as file:
        file.write(json.dumps(sql_list, indent=4, ensure_ascii=False))

    
    file_count = 0
    for file_name, table_list in sql_list.items():
    
        # 定义头文件
        with open(os.path.join(dist, to_upper_camel_case(file_name) + '.orm.h'), 'w', encoding='utf-8') as file:

            file.write(f'''/**
 * Object Relational Mapping CPP Class Define
 * This file is generated by Python script. Do not edit!!!
 * Python version: v{platform.python_version()}
 * Script version: v{VERSION}
 * Source file: /sql/{file_name}
 */\n\n''')

            file.write('#pragma once\n\n')
            file.write('#include <database\entity.h>\n\n')

            file.write('namespace orm {\n\n')

            for table in table_list:
                file.write(f'\t// entity: {table['name']}\n\n')

                # 类定义开始
                file.write('\tclass FEntity_%s final : public IEntity {\n' % to_upper_camel_case(table['name']))
                file.write('\tpublic:\n')

                # 含参构造函数参数列表
                cons_args = '\t\t\t'

                count = 0
                # 根据字段拼接所需字符串
                for field in table['field'].values():
                    # 表字段映射为类成员变量
                    file.write(f"\t\t{cpp_type_map[field['type']]} {field['name']}")
            
                    if field['type'] != "string" and field['type'] != "text" and field['type'] != "blob" and field['type'] != "bool":
                        if 'default' in field.keys() and field['default'] != "":
                            file.write(f' = {field['default']}')
                        else:
                            file.write(' = 0')

                    if field['type'] == 'bool' and 'default' in field.keys() and field['default'] != "":
                        if field['default'] == "TRUE":
                            file.write(' = true')
                        else:
                            file.write(' = false')

                    file.write(";\n")

                    # 含参构造列表
                    if field['type'] == "string" or field['type'] == "text" or field['type'] == "blob":
                        cons_args = f"{cons_args}{cpp_type_map[field['type']]} {field['name']},\n\t\t\t"
                    elif 'int' in cpp_type_map[field['type']]:
                        cons_args = f"{cons_args}{cpp_type_map[field['type']]} {field['name']},\n\t\t\t"
                    else:
                        cons_args = f"{cons_args}{cpp_type_map[field['type']]} {field['name']},\n\t\t\t"

                    count += 1

                # 字符串去尾
                if len(cons_args) > 5:
                    cons_args = cons_args[:-5]

                file.write(f"\n\t\tFEntity_{to_upper_camel_case(table['name'])}() = default;\n\n")

                # 含参构造函数
                file.write('\t\tFEntity_%s(\n%s\n\t\t);\n\n' % (to_upper_camel_case(table['name']), cons_args))

                # 纯虚函数重写
                file.write('\t\t[[nodiscard]] constexpr const char* GetTableName() const override {\n')
                file.write(f"\t\t\treturn \"{table['name']}\";\n")
                file.write('\t\t}\n\n')

                file.write('\t\t[[nodiscard]] bool ComparePrimaryKey(mysqlx::Row &row) const override;\n\n')
                
                file.write('\t\tmysqlx::RowResult Query(mysqlx::Table &table) override;\n')
                file.write('\t\tvoid Read(mysqlx::Row &row) override;\n')
                file.write('\t\tvoid Write(mysqlx::Table &table) override;\n')
                file.write('\t\tvoid Remove(mysqlx::Table &table) override;\n\n')

                file.write('\t}; // FEntity_%s\n\n' % to_upper_camel_case(table['name']))
        
            file.write('} // orm')

        with open(os.path.join(dist, to_upper_camel_case(file_name) + '.orm.cpp'), 'w', encoding='utf-8') as file:
            file.write(f'#include \"{to_upper_camel_case(file_name)}.orm.h\"\n\n')

            file.write('namespace orm {\n\n')


            for table in table_list:
                file.write(f'\t// entity: {table['name']}\n')

                # 含参构造函数参数列表
                cons_args = '\t\t'
            
                # 含参构造函数成员变量初始化
                cons_init = ''

                # 条件字符串
                where_str = ''
                bind_expr = ''

                # 条件表达式
                where_expr = ''

                equal_str = ''

                insert_field_str = ''
                insert_value_str = ''

                count = 0
                # 根据字段拼接所需字符串
                for field in table['field'].values():
                    # 含参构造列表
                    if field['type'] == "string" or field['type'] == "text" or field['type'] == "blob":
                        cons_args = f"{cons_args}{cpp_type_map[field['type']]} {field['name']},\n\t\t"
                    elif 'int' in cpp_type_map[field['type']]:
                        cons_args = f"{cons_args}const {cpp_type_map[field['type']]} {field['name']},\n\t\t"
                    else:
                        cons_args = f"{cons_args}const {cpp_type_map[field['type']]} {field['name']},\n\t\t"
                
                    if field['type'] == "string" or field['type'] == "text" or field['type'] == "blob":
                        cons_init = f"{cons_init}{field['name']} (std::move({field['name']})), \n\t\t"
                    else:
                        cons_init = f"{cons_init}{field['name']} ({field['name']}), \n\t\t"
                    
                    insert_field_str = f"{insert_field_str}\"{field['name']}\", "
                    if field['type'] == "blob":
                        insert_value_str = f"{insert_value_str}DB_CAST_TO_BLOB({field['name']}), "
                    else:
                        insert_value_str = f"{insert_value_str}{field['name']}, "

                    if "key" in field and field["key"]:
                        where_str = f"{where_str}{field['name']} = : {field['name']} AND "
                        bind_expr = f"{bind_expr}.bind(\"{field['name']}\", {field['name']})"
                        
                        type = cpp_type_map[field['type']]
                        if type == "int16_t":
                            type = "int32_t"
                            equal_str = f"{equal_str}{field['name']} == static_cast<int16_t>(row[{count}].get<{type}>()) && "
                        elif type == "uint16_t":
                            type = "uint32_t"
                            equal_str = f"{equal_str}{field['name']} == static_cast<uint16_t>(row[{count}].get<{type}>()) && "
                        elif type == "int8_t":
                            type = "int32_t"
                            equal_str = f"{equal_str}{field['name']} == static_cast<int8_t>(row[{count}].get<{type}>()) && "
                        elif type == "uint8_t":
                            type = "uint32_t"
                            equal_str = f"{equal_str}{field['name']} == static_cast<uint8_t>(row[{count}].get<{type}>()) && "
                        else:
                            equal_str = f"{equal_str}{field['name']} == row[{count}].get<{type}>() && "

                    count += 1

                # 字符串去尾
                if len(cons_args) > 4:
                    cons_args = cons_args[:-4]

                if len(cons_init) > 1:
                    cons_init = cons_init[:-5]

                if len(where_str) > 5:
                    where_str = where_str[:-5]

                if len(insert_field_str) > 2:
                    insert_field_str = insert_field_str[:-2]

                if len(insert_value_str) > 2:
                    insert_value_str = insert_value_str[:-2]

                if len(equal_str) > 4:
                    equal_str = equal_str[:-4]

                where_expr = f".where(\"{where_str}\"){bind_expr}"

                file.write('\tFEntity_%s::FEntity_%s(\n%s\n\t) : %s {\n\t}\n\n' % (to_upper_camel_case(table['name']), to_upper_camel_case(table['name']), cons_args, cons_init))

                file.write('\t[[nodiscard]] bool FEntity_%s::ComparePrimaryKey(mysqlx::Row &row) const {\n' % to_upper_camel_case(table['name']))
                file.write(f'\t\treturn {equal_str};\n')
                file.write('\t}\n\n')

                file.write('\tmysqlx::RowResult FEntity_%s::Query(mysqlx::Table &table) {\n' % to_upper_camel_case(table['name']))
                file.write('\t\treturn table.select()\n')
                file.write(f'\t\t\t{where_expr}\n')
                file.write('\t\t\t.execute();\n')
                file.write('\t}\n\n')

                file.write('\tvoid FEntity_%s::Read(mysqlx::Row &row) {\n' % to_upper_camel_case(table['name']))
                file.write('\t\tif (row.isNull())\n \t\t\treturn;\n\n')

                count = 0
                for field in table['field'].values():
                    type = cpp_type_map[field['type']]
                    if type == "int16_t":
                        type = "int32_t"
                        file.write(f"\t\t{field['name']} = static_cast<int16_t>(row[{count}].get<{type}>());\n")
                    elif type == "uint16_t":
                        type = "uint32_t"
                        file.write(f"\t\t{field['name']} = static_cast<uint16_t>(row[{count}].get<{type}>());\n")
                    elif type == "int8_t":
                        type = "int32_t"
                        file.write(f"\t\t{field['name']} = static_cast<int8_t>(row[{count}].get<{type}>());\n")
                    elif type == "uint8_t":
                        type = "uint32_t"
                        file.write(f"\t\t{field['name']} = static_cast<uint8_t>(row[{count}].get<{type}>());\n")
                    elif type == "FByteArray":
                        file.write(f"\t\tDB_CAST_FROM_BLOB({field['name']}, row[{count}])\n")
                    else:
                        file.write(f"\t\t{field['name']} = row[{count}].get<{type}>();\n")
                    count += 1

                file.write('\t}\n\n')

                file.write('\tvoid FEntity_%s::Write(mysqlx::Table &table) {\n' % to_upper_camel_case(table['name']))
                file.write('\t\tmysqlx::RowResult result = Query(table);\n\n')

                # 如果已存在相同键 则调用update()
                file.write("\t\tif (const mysqlx::Row row = result.fetchOne(); !row.isNull()) {\n")

                file.write("\t\t\ttable.update()\n")
                for field in table['field'].values():
                    if "key" not in field or not field["key"]:
                        if field['type'] == "blob":
                            file.write(f"\t\t\t\t.set(\"{field["name"]}\", DB_CAST_TO_BLOB({field["name"]}))\n")
                        else:
                            file.write(f"\t\t\t\t.set(\"{field["name"]}\", {field["name"]})\n")
            
                file.write(f"\t\t\t\t{where_expr}\n")
                file.write('\t\t\t\t.execute();\n') 

                # 否则调用insert()
                file.write("\t\t} else {\n")
                file.write(f"\t\t\ttable.insert({insert_field_str})\n")
                file.write(f"\t\t\t\t.values({insert_value_str})\n")
                file.write('\t\t\t\t.execute();\n') 

                file.write("\t\t}\n")
                file.write('\t}\n\n')

                file.write('\tvoid FEntity_%s::Remove(mysqlx::Table &table) {\n' % to_upper_camel_case(table['name']))
                file.write('\t\ttable.remove()\n')
                file.write(f"\t\t\t{where_expr}\n")
                file.write('\t\t\t.execute();\n')
                file.write('\t}\n\n')

            file.write('} // orm\n')

        file_count += 1

    print(f'-- {file_count} files has done')

if __name__ == "__main__":
    generate_orm_clazz("struct/sql", "struct/orm", "struct/desc.json")