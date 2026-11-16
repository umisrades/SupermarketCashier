# SupermarketCashier 3.0

基于 Qt 6 Widgets 和 SQLite 的超市收银与库存管理系统。

## 已实现功能

- 用户登录与角色权限控制
- 管理员库存管理入口
- 商品新增、删除、入库、报损、盘点和库存预警
- 条形码搜索与商品快捷按钮
- 购物车合并、删除、挂起和恢复
- 事务性销售扣库与库存不足校验
- 付款、找零和小票生成
- 销售统计记录
- 默认商品数据库及商品图片导入
- 命令行管理员工具
- 数据库迁移、资源导入和权限冒烟测试

## 技术环境

- C++17
- Qt 6.5 或更高版本
- Qt Core、Sql、Widgets
- CMake 3.19 或更高版本
- SQLite（通过 Qt SQL 驱动）

## 构建

### 使用 Qt Creator（推荐）

打开根目录的 `CMakeLists.txt` 即可。

### 命令行构建

```powershell
cd <项目目录>
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=<Qt安装目录>
cmake --build build --parallel
```

### 运行测试

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 运行数据

- `resources/data/default_goods.db` 是随程序分发的默认商品数据库。
- `resources/images/products` 保存与商品编号匹配的商品图片。
- 程序运行时使用可执行文件所在目录中的 `goods.db` 保存实际商品和库存数据。
- 构建时 CMake 会自动将资源复制到构建输出目录。
- `build/`、`out/`、`.vs/` 和 `.qtcreator/` 均为本地生成内容，不属于项目源码。

详细目录说明见 [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)。
