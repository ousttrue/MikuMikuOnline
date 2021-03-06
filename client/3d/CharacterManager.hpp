#pragma once

#include <memory>
#include <map>
#include "Character.hpp"

// キャラクタデータを一括管理する
class CharacterManager
{
public:
    typedef std::shared_ptr<Character> CharacterPtrType;
    typedef std::map<unsigned int, CharacterPtrType> CharacterMapType;

    CharacterManager();
    ~CharacterManager();

    void Add(unsigned int character_id, CharacterPtrType character);
    CharacterPtrType Get(unsigned int character_id) const;
    const CharacterMapType& GetAll() const;
    CharacterPtrType Remove(unsigned int character_id);

    void set_my_character_id(unsigned int character_id);
    unsigned int my_character_id() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

