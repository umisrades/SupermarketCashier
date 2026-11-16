cmake_minimum_required(VERSION 3.19)
project(SupermarketCashier LANGUAGES CXX)

include(CTest)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Sql Widgets)

qt_standard_project_setup()

file(GLOB PRODUCT_IMAGE_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/products/*.png"
)

set(PRODUCT_RESOURCE_STAMP
    "${CMAKE_CURRENT_BINARY_DIR}/product_resources.stamp"
)

add_custom_command(
    OUTPUT "${PRODUCT_RESOURCE_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/images/products"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/products"
        "${CMAKE_CURRENT_BINARY_DIR}/images/products"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/data/default_goods.db"
        "${CMAKE_CURRENT_BINARY_DIR}/data/default_goods.db"
    COMMAND ${CMAKE_COMMAND} -E touch
        "${PRODUCT_RESOURCE_STAMP}"
    DEPENDS
        ${PRODUCT_IMAGE_FILES}
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/data/default_goods.db"
    COMMENT "Copying product database and images"
)

add_custom_target(ProductResources ALL
    DEPENDS "${PRODUCT_RESOURCE_STAMP}"
)

qt_add_executable(SupermarketCashier
    WIN32 MACOSX_BUNDLE
    main.cpp
    src/data/product_repository.h
    src/data/product_repository.cpp
    src/data/databasemanager.h
    src/data/databasemanager.cpp
    src/models/user.h
    src/models/product.h
    src/models/product.cpp
    src/models/system_enums.h
    src/ui/cashierwindow.h
    src/ui/cashierwindow.cpp
    src/ui/cashierwindow.ui
    src/ui/logindialog.h
    src/ui/logindialog.cpp
    src/ui/logindialog.ui
    src/ui/inventory_widget.h
    src/ui/inventory_widget.cpp
    src/ui/inventory_widget.ui
    src/ui/statistics_dialog.h
    src/ui/statistics_dialog.cpp
    src/ui/statistics_dialog.ui
)

qt_add_executable(SupermarketAdminConsole
    src/console/admin_console_main.cpp
    src/data/databasemanager.h
    src/data/databasemanager.cpp
    src/data/product_repository.h
    src/data/product_repository.cpp
    src/models/user.h
    src/models/system_enums.h
    src/models/product.h
    src/models/product.cpp
)

add_dependencies(SupermarketCashier ProductResources)
add_dependencies(SupermarketAdminConsole ProductResources)

target_link_libraries(SupermarketCashier
    PRIVATE
        Qt::Core
        Qt::Sql
        Qt::Widgets
)

target_link_libraries(SupermarketAdminConsole
    PRIVATE
        Qt::Core
        Qt::Sql
)

target_include_directories(SupermarketCashier
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS
    SupermarketCashier
    SupermarketAdminConsole
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET SupermarketCashier
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

install(
    DIRECTORY resources/images/products
    DESTINATION ${CMAKE_INSTALL_BINDIR}/images
)

install(
    FILES resources/data/default_goods.db
    DESTINATION ${CMAKE_INSTALL_BINDIR}/data
)

if(BUILD_TESTING)
    qt_add_executable(ProductRepositorySmoke
        tests/product_repository_smoke.cpp
        src/data/product_repository.h
        src/data/product_repository.cpp
        src/models/system_enums.h
        src/models/product.h
        src/models/product.cpp
    )

    target_link_libraries(ProductRepositorySmoke
        PRIVATE
            Qt::Core
            Qt::Sql
    )

    qt_add_executable(DatabaseManagerSmoke
        tests/database_manager_smoke.cpp
        src/data/databasemanager.h
        src/data/databasemanager.cpp
        src/data/product_repository.h
        src/data/product_repository.cpp
        src/models/user.h
        src/models/system_enums.h
        src/models/product.h
        src/models/product.cpp
    )

    target_link_libraries(DatabaseManagerSmoke
        PRIVATE
            Qt::Core
            Qt::Sql
    )

    qt_add_executable(AuthenticationPermissionSmoke
        tests/auth_permission_smoke.cpp
        src/data/databasemanager.h
        src/data/databasemanager.cpp
        src/data/product_repository.h
        src/data/product_repository.cpp
        src/models/user.h
        src/models/system_enums.h
        src/models/product.h
        src/models/product.cpp
        src/ui/cashierwindow.h
        src/ui/cashierwindow.cpp
        src/ui/cashierwindow.ui
        src/ui/logindialog.h
        src/ui/logindialog.cpp
        src/ui/logindialog.ui
        src/ui/inventory_widget.h
        src/ui/inventory_widget.cpp
        src/ui/inventory_widget.ui
        src/ui/statistics_dialog.h
        src/ui/statistics_dialog.cpp
        src/ui/statistics_dialog.ui
    )

    target_link_libraries(AuthenticationPermissionSmoke
        PRIVATE
            Qt::Core
            Qt::Sql
            Qt::Widgets
    )

    add_test(
        NAME ProductRepositoryEmptyDatabase
        COMMAND ProductRepositorySmoke empty
    )

    add_test(
        NAME ProductRepositoryLegacyMigration
        COMMAND ProductRepositorySmoke legacy
    )

    add_test(
        NAME ProductResourceDatabaseImport
        COMMAND ProductRepositorySmoke resource
    )

    add_test(
        NAME DatabaseManagerUsersAndProducts
        COMMAND DatabaseManagerSmoke
    )

    add_test(
        NAME AuthenticationAndInventoryPermissions
        COMMAND AuthenticationPermissionSmoke
    )

    get_filename_component(
        QT_INSTALL_ROOT
        "${Qt6_DIR}/../../.."
        ABSOLUTE
    )

    set_tests_properties(
        ProductRepositoryEmptyDatabase
        ProductRepositoryLegacyMigration
        ProductResourceDatabaseImport
        DatabaseManagerUsersAndProducts
        AuthenticationAndInventoryPermissions
        PROPERTIES
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:${QT_INSTALL_ROOT}/bin;QT_PLUGIN_PATH=set:${QT_INSTALL_ROOT}/plugins;QT_QPA_PLATFORM=set:offscreen"
    )
endif()
