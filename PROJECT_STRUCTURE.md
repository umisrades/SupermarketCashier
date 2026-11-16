# SupermarketCashier 3.0 项目结构

当前项目采用一个 Qt Widgets 图形程序、一个管理员命令行程序、共享数据访问层和独立冒烟测试的结构。

```text
SupermarketCashier3.0/
├─ CMakeLists.txt
├─ main.cpp
├─ README.md
├─ PROJECT_STRUCTURE.md
├─ 项目功能构想.pdf
├─ resources/
│  ├─ data/
│  │  └─ default_goods.db
│  └─ images/
│     └─ products/
│        ├─ 1001.png
│        ├─ ...
│        └─ 1150.png
├─ src/
│  ├─ console/
│  │  └─ admin_console_main.cpp
│  ├─ data/
│  │  ├─ databasemanager.cpp
│  │  ├─ databasemanager.h
│  │  ├─ product_repository.cpp
│  │  └─ product_repository.h
│  ├─ models/
│  │  ├─ product.cpp
│  │  ├─ product.h
│  │  ├─ system_enums.h
│  │  └─ user.h
│  └─ ui/
│     ├─ cashierwindow.cpp
│     ├─ cashierwindow.h
│     ├─ cashierwindow.ui
│     ├─ inventory_widget.cpp
│     ├─ inventory_widget.h
│     ├─ inventory_widget.ui
│     ├─ logindialog.cpp
│     ├─ logindialog.h
│     ├─ logindialog.ui
│     ├─ statistics_dialog.cpp
│     ├─ statistics_dialog.h
│     └─ statistics_dialog.ui
└─ tests/
   ├─ auth_permission_smoke.cpp
   ├─ database_manager_smoke.cpp
   └─ product_repository_smoke.cpp
```

## 根目录

- `CMakeLists.txt`：配置 Qt 依赖、程序目标、测试目标、资源复制和部署规则。
- `main.cpp`：初始化 QApplication、共享数据库、默认商品资源和登录流程，然后显示收银窗口。
- `README.md`：项目功能、环境和构建说明。
- `项目功能构想.pdf`：项目需求与功能设计参考文档。

## `src/console`

- `admin_console_main.cpp`：管理员命令行工具入口，复用数据访问层管理用户和商品数据。

## `src/data`

- `databasemanager.*`：数据库管理接口，包括用户验证、商品管理和默认数据导入。
- `product_repository.*`：收银台和库存页面共享的商品持久化层，负责数据库初始化、迁移、库存更新以及事务性销售扣库。

## `src/models`

- `product.*`：统一商品模型。
- `user.h`：用户、角色和启用状态模型。
- `system_enums.h`：商品状态等共享枚举。

## `src/ui`

- `cashierwindow.*`：条形码搜索、商品展示、购物车、挂起/恢复、付款、小票和管理员库存入口。
- `inventory_widget.*`：商品与库存管理页面。
- `logindialog.*`：用户登录页面。
- `statistics_dialog.*`：销售统计页面。

每个 Qt 页面由 `.h`、`.cpp` 和 `.ui` 三个文件组成。

## `resources`

- `data/default_goods.db`：首次运行或空数据库导入时使用的默认商品数据。
- `images/products/*.png`：按商品编号命名的商品图片，目前为 `1001.png` 至 `1150.png`。

构建时，CMake 会把数据库和图片复制到构建目录；安装时会复制到程序部署目录。

## `tests`

- `product_repository_smoke.cpp`：空数据库初始化、旧数据库迁移和资源数据库导入测试。
- `database_manager_smoke.cpp`：用户与商品数据库管理测试。
- `auth_permission_smoke.cpp`：登录、角色权限、条形码收银和库存窗口权限测试。

测试通过 CMake 的 `-DBUILD_TESTING=ON` 选项启用。

## 不纳入项目结构的生成内容

以下目录由构建工具或 IDE 自动生成，需要时可重新创建，不应作为源码保存：

```text
build/
out/
.vs/
.qtcreator/
```

程序运行后生成的 `goods.db`、SQLite 临时日志、编译中间文件以及 Qt Creator 用户配置同样不纳入源码。
