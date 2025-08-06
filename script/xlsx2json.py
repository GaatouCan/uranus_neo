import pandas
import os
import json

def generate_config_json(src: str, dist: str):
    """将xlsx配置表转换为json文件"""

    assert src, 'XLSX 输入路径错误'
    assert dist, 'JSON 输出路径错误'

    print(f'-- XLSX 输入路径: {os.getcwd()}\\{src}')
    print(f'-- JSON 输出路径: {os.getcwd()}\\{dist}')

    if not os.path.exists(dist):
        os.makedirs(dist)
        print('-- 创建JSON输出路径')

    count = 0

    # 遍历所有xlsx文件
    for root, dirs, files in os.walk(src):

        for file in files:
            if not file.endswith('.xlsx'):
                continue

            # 临时文件跳过
            if file.startswith('~'):
                continue

            path = os.path.join(root, file)
            dfs = pandas.read_excel(path, sheet_name=None)
            print(f'-- \tLoaded {path}')

            # 对应JSON文件夹
            json_path = ''

            # 工作表索引
            sheet_index = 0
            sheet_name_list = []

            # 顺序读取所有工作表
            for name, df in dfs.items():

                # 第一张表是后面工作表对应命名的说明
                if sheet_index == 0:
                    df_T = df.T
                    
                    for idx, row in df_T.iterrows():
                        if idx == 'Directory':
                            json_path = row[0]
                            continue

                        if idx == 'Desc':
                            continue

                        # 映射到列表里
                        sheet_name_list.append({"name": row[0], "desc": idx})

                    sheet_index = sheet_index + 1
                    continue

                assert name == sheet_name_list[sheet_index - 1]['desc'], "工作表定义顺序与声明顺序不一致"

                # 每一列的列名使用第一行数据
                df.columns = df.iloc[0]

                # 去掉中文表头
                df = df.iloc[1:].reset_index(drop=True)

                # 表头数据
                columns = []
                
                # 具体数据
                sheet_data = []

                # 类型说明行
                type_row = df.iloc[0]
                
                # 生成目标说明行
                target_row = df.iloc[1]
            
                # 读取每一列元信息
                for col in df.columns:
                    if 's' in target_row[col]:
                        columns.append({"name": col, "type": type_row[col]})

                for idx, row in df.iterrows():
                    # 从实际第4行开始读数据(去中文减一行及idx从0开始)
                    if idx <= 1:
                        continue

                    # 读到首列数据为空的行停止读取当前工作表
                    if pandas.isna(row.iloc[0]):
                        break

                    # 行数据
                    item = {}
                    for col in columns:
                        value = row[col["name"]]

                        if pandas.isna(value):
                            continue

                        # 解析表格中的JSON数据
                        if col["type"] == "object" or col["type"] == "list":
                            if isinstance(value, str):
                                try:
                                    parsed = json.loads(value)
                                    if isinstance(parsed, (dict, list)):
                                        item[col["name"]] = parsed
                                    else:
                                        raise ValueError(f'file[{path}] - sheet[{name}] - row[{idx}], column[{col['name']}] is not an object or array')
                                except json.JSONDecodeError:
                                    raise ValueError(f'file[{path}] - sheet[{name}] - row[{idx}], column[{col['name']}] failed to parse to json')
                            else:
                                raise TypeError(f'file[{path}] - sheet[{name}] - row[{idx}], column[{col['name']}] expect string')
                        else:
                            item[col["name"]] = value

                    sheet_data.append(item)

                if json_path == '':
                    raise RuntimeError(f'file{path} 无法读取对应的生成JSON目录名')

                json_dir = os.path.join(root.replace(src, dist), json_path)
                if not os.path.exists(json_dir):
                    os.makedirs(json_dir)

                json_file = os.path.join(json_dir, sheet_name_list[sheet_index - 1]['name'] + '.json')
                with open(json_file, 'w', encoding='utf-8') as file:
                    file.write(json.dumps(sheet_data, indent=4, ensure_ascii=False))

                print(f'-- \t已生成 {json_file}')
                sheet_index = sheet_index + 1
            
            count += 1

    print(f'-- {count} files has done')

if __name__ == "__main__":
    generate_config_json("config/xlsx", "config/json")
