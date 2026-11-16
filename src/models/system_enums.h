#ifndef SYSTEM_ENUMS_H
#define SYSTEM_ENUMS_H

enum class ProductStatus
{
    Pending,   // 待上架
    OnSale,    // 已上架
    OffSale,   // 已下架

    // 兼容管理控制台原有命名
    Draft = Pending,
    Active = OnSale,
    Inactive = OffSale
};

#endif // SYSTEM_ENUMS_H
