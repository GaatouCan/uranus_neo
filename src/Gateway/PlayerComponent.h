#pragma once

#include "Common.h"

#include <cstdint>
#include <concepts>
#include <bsoncxx/document/value.hpp>


class UComponentModule;
class UPlayerBase;


class BASE_API IPlayerComponent {


public:
    IPlayerComponent();
    virtual ~IPlayerComponent();

    DISABLE_COPY_MOVE(IPlayerComponent)

    void SetUpModule(UComponentModule* module);

    virtual bsoncxx::document::value Serialize() const;
    virtual void Deserialize(const bsoncxx::document::value& doc);

    [[nodiscard]] UPlayerBase* GetPlayer() const;
    [[nodiscard]] int64_t GetPlayerID() const;

    virtual void OnLogin();
    virtual void OnLogout();

private:
    UComponentModule *mModule;
};

template<class T>
concept CComponentType = std::derived_from<T, IPlayerComponent>;


#define DECLARE_COMPONENT_SERIALIZATION \
bsoncxx::document::value Serialize() const override; \
void Deserialize(const bsoncxx::document::value& doc) override;

#define DB_SERIALIZE_BEGIN(XX) \
bsoncxx::document::value XX##::Serialize() const { \
    bsoncxx::builder::basic::document doc;

#define DB_SERIALIZE_END \
    return doc.extract(); \
}

#define DB_MAKE_KVP(KEY, VALUE)                 bsoncxx::builder::basic::kvp(KEY, VALUE)
#define DB_WRITE_FIELD(NAME, FIELD)             doc.append(DB_MAKE_KVP(NAME, FIELD))
#define DB_WRITE_ARRAY_ELEMENT(KEY, VALUE)      bsoncxx::builder::basic::kvp(KEY, val.##VALUE)

#define DB_WRITE_ARRAY(NAME, DATA, ...) \
{ \
    bsoncxx::builder::basic::array array_builder; \
    for (const auto &val: DATA) { \
        array_builder.append( \
            bsoncxx::builder::basic::make_document(__VA_ARGS__) \
        ); \
    } \
    doc.append(bsoncxx::builder::basic::kvp(NAME, array_builder.extract()));\
} \

#define DB_DESERIALIZE_BEGIN(XX) \
void XX##::Deserialize(const bsoncxx::document::value &doc) {

#define DB_DESERIALIZE_END }

#define DB_READ_FIELD(NAME, TYPE, FIELD) \
if (const auto elem = doc[NAME]; elem && elem.type() == bsoncxx::type::k_##TYPE) { \
    FIELD = elem.get_##TYPE##().value; \
}

#define DB_READ_ARRAY_ELEMENT(KEY, TYPE, VALUE) \
if (auto elem = element_view[KEY]; elem && elem.type() == bsoncxx::type::k_##TYPE) { \
    data.##VALUE = elem.get_##TYPE##().value; \
}

#define DB_READ_ARRAY(NAME, DATA, ...) \
if (const auto array_elem = doc[NAME]; array_elem && array_elem.type() == bsoncxx::type::k_array) { \
    for (auto &&val: array_elem.get_array().value) { \
        auto element_view = val.get_document().value; \
        decltype(DATA)::value_type data; \
        __VA_ARGS__ \
        DATA.push_back(data); \
    } \
}
