//
// CardManager.cpp
//

#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include "CardManager.hpp"
#include "../common/unicode.hpp"
#include "ResourceManager.hpp"
#include "ManagerAccessor.hpp"
#include "CommandManager.hpp"
#include "../common/Logger.hpp"

char CardManager::CARDS_DIR[] = "cards";
char CardManager::INFO_FILE[] = "info.json";

CardManager::CardManager(const ManagerAccessorPtr& manager_accessor) :
manager_accessor_(manager_accessor)
{
    v8::V8::LowMemoryNotification();
}

void CardManager::Load()
{
    using namespace boost::filesystem;
    // info.jsonを検索
    if (exists(CARDS_DIR) && is_directory(CARDS_DIR)) {

    } else {
        Logger::Error(_T("Connot find card directory"));
    }

    directory_iterator end_itr;
    for ( directory_iterator itr(CARDS_DIR); itr != end_itr; ++itr) {

        path itr_path(*itr);

        // サブディレクトリを検索
        if ( is_directory( *itr ) ) {
            path info_file_path = itr_path / path(INFO_FILE);

            // info.jsonの存在を確認する
            if (exists(info_file_path)) {
                ParseInfoFile(info_file_path.string());
            }

        }
    }

    // icon_base_handle_ = ResourceManager::LoadCachedGraph("resources/images/gui/card_icon_base.png");
}

void CardManager::ProcessInput(InputManager* input)
{
    std::sort(cards_.begin(), cards_.end(),
            [](const CardPtr& a, const CardPtr& b) {
                return a->focus_index() < b->focus_index();
            });

    for (auto it = cards_.rbegin(); it != cards_.rend(); ++it) {
        (*it)->ProcessInput(input);
    }
}

void CardManager::Update()
{
    for (auto it = cards_.rbegin(); it != cards_.rend(); ++it) {
        (*it)->Update();
    }
}

void CardManager::Draw()
{
    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        const CardPtr& card = *it;
        card->Draw();
    }
}

void CardManager::AddCard(const CardPtr& card)
{
    cards_.push_back(card);
}

bool CardManager::IsGUIActive()
{
    //return !dummy_window->focus_front();
    return true;
}

void CardManager::FocusPlayer()
{
    focus_player_flag = true;
}

void CardManager::SendJSON(const std::string& json)
{
    if (auto command_manager = manager_accessor_->command_manager().lock()) {
        command_manager->SendJSON(json);
    }
}

void CardManager::OnReceiveJSON(const std::string& info_json, const std::string& msg_json)
{
    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        const CardPtr& card = *it;
        card->OnReceiveJSON(info_json, msg_json);
    }
}

void CardManager::OnLogin(const PlayerPtr& player)
{
    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        const CardPtr& card = *it;
        card->OnLogin(player);
    }
}

void CardManager::OnLogout(const PlayerPtr& player)
{
    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        const CardPtr& card = *it;
        card->OnLogout(player);
    }
}

std::vector<CardPtr>& CardManager::cards()
{
    return cards_;
}

void CardManager::ParseInfoFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    std::string source_str = std::string((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());

    std::string name, author, caption, icon;
    std::vector<std::string> scripts;
    bool group = false;
    bool autorun = false;

    using boost::property_tree::ptree;
    ptree pt;
    read_json(filename, pt);

    name = pt.get<std::string>("name", "");
    author = pt.get<std::string>("author", "");
    caption = pt.get<std::string>("caption", "");
    icon = pt.get<std::string>("icon", "");
    group = pt.get<bool>("group", false);
    autorun = pt.get<bool>("autorun", false);

    auto pt_scripts = pt.get_child("scripts", ptree());
    BOOST_FOREACH(auto val, pt_scripts) {
        scripts.push_back(val.second.get_value<std::string>());
    }

    auto source_folder = (boost::filesystem::path(filename)).parent_path().string();

    v8::Isolate::Scope scope(v8::Isolate::New());
    auto card = std::make_shared<Card>(
        manager_accessor_,
        source_folder,
        name,
        author,
        caption,
        icon,
        scripts,
        group,
        autorun);
    if (autorun) {
        card->Run();
    }

    cards_.push_back(card);
}

void CardManager::Error(const v8::Handle<v8::Value>& error)
{
    v8::String::Utf8Value exception_str(error);
    std::cout << "Javascript Error >>>" << std::endl;
    std::cout << unicode::utf82sjis(*exception_str) << std::endl;
    std::cout << "<<<" << std::endl;
}

v8::Handle<v8::Object> CardManager::GetGlobal()
{
    HandleScope handle;
    v8::Isolate::Scope scope(v8::Isolate::New());
    v8::Context::Scope scope2(v8::Context::New());
    v8::Handle<v8::Object> obj = Object::New();
    return obj;
}
