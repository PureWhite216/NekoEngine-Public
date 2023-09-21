//
// Created by 80529 on 2023/7/19.
//

#pragma once

#include "Core.h"
#include "UUID.h"

#define SET_ASSET_TYPE(type)                        \
    static AssetType GetStaticType()                \
    {                                               \
        return type;                                \
    }                                               \
    virtual AssetType GetAssetType() const override \
    {                                               \
        return GetStaticType();                     \
    }
namespace NekoEngine
{


    enum class AssetFlag : uint8_t
    {
        None = 0,
        Missing = BIT(0),
        Invalid = BIT(1),
        Loaded = BIT(2),
        UnLoaded = BIT(3)
    };

    enum class AssetType : uint16_t
    {
        None = 0,
        Texture = 1,
        Mesh = 2,
        Scene = 3,
        Audio = 4,
        Font = 5,
        Shader = 6,
        Material = 7,
        PhysicsMaterial = 8,
        Model = 9,
    };

    class Asset
    {
    protected:
        String name = "NULL";
    public:
        uint16_t Flags = (uint16_t) AssetFlag::None;
        UUID Handle;

        virtual ~Asset()
        {}

        static AssetType GetStaticType()
        { return AssetType::None; }

        virtual AssetType GetAssetType() const
        { return AssetType::None; }

        bool IsValid() const
        { return ((Flags & (uint16_t) AssetFlag::Missing) | (Flags & (uint16_t) AssetFlag::Invalid)) == 0; }

        virtual bool operator==(const Asset &other) const
        {
            return Handle == other.Handle;
        }

        virtual bool operator!=(const Asset &other) const
        {
            return !(*this == other);
        }

        bool IsFlagSet(AssetFlag flag) const
        { return (uint16_t) flag & Flags; }

        void SetFlag(AssetFlag flag, bool value = true)
        {
            if(value)
                Flags |= (uint16_t) flag;
            else
                Flags &= ~(uint16_t) flag;
        }

        void SetName(const String& name)
        {
            this->name = name;
        }
    };

} // NekoEngine


