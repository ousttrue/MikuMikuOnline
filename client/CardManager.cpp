//
// CardManager.cpp
//

#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include "CardManager.hpp"
#include "ConfigManager.hpp"
#include "MiniMap.hpp"
#include "ResourceManager.hpp"
#include "ManagerAccessor.hpp"
#include "CommandManager.hpp"
#include "Profiler.hpp"
#include "../common/Logger.hpp"
#include "../common/unicode.hpp"
#include "ui/InputBox.hpp"

char CardManager::START_METADATA[] = "/***MetaData***";
char CardManager::END_METADATA[] = "***MetaData***/";

CardManager::CardManager(const ManagerAccessorPtr& manager_accessor) :
manager_accessor_(manager_accessor)
{
    v8::V8::LowMemoryNotification();
}

void CardManager::Load(const std::string& dir)
{
    using namespace boost::filesystem;
    // info.jsonを検索
    if (exists(dir) && is_directory(dir)) {

    } else {
        Logger::Error(_T("Connot find card directory"));
		return;
    }

    directory_iterator end_itr;
    for ( directory_iterator itr(dir); itr != end_itr; ++itr) {

        path itr_path(*itr);

        // サブディレクトリを検索
        if ( is_directory( *itr ) ) {
            path main_file_path = itr_path / path("main.js");

            // info.jsonの存在を確認する
            if (exists(main_file_path)) {
                ParseScriptFile(main_file_path.string());
            }

        }
    }

    // icon_base_handle_ = ResourceManager::LoadCachedGraph("system/images/gui/card_icon_base.png");
}

void CardManager::ProcessInput(InputManager* input)
{
	MMO_PROFILE_FUNCTION;

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
	MMO_PROFILE_FUNCTION;

    for (auto it = cards_.rbegin(); it != cards_.rend(); ++it) {
        (*it)->Update();
    }
}

void CardManager::Draw()
{
	MMO_PROFILE_FUNCTION;

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
    BOOST_FOREACH(const auto& card, cards_) {
        card->OnReceiveJSON(info_json, msg_json);
    }
}

void CardManager::OnLogin(const PlayerPtr& player)
{
	BOOST_FOREACH(const auto& card, cards_) {
        card->OnLogin(player);
    }
}

void CardManager::OnLogout(const PlayerPtr& player)
{
    BOOST_FOREACH(const auto& card, cards_) {
        card->OnLogout(player);
    }
}

void CardManager::OnModelReload()
{
    BOOST_FOREACH(const auto& card, cards_) {
        card->OnModelReload();
    }
}

void CardManager::OnMusicReload()
{
    BOOST_FOREACH(const auto& card, cards_) {
        card->OnMusicReload();
    }
}

const std::vector<CardPtr>& CardManager::cards()
{
    return cards_;
}

void CardManager::AddNativeCard(const std::string& name, UISuperPtr ptr)
{
	auto it = native_cards_.find(name);
	if (it != native_cards_.end()) {
		ptr->set_icon_image_handle(
				ResourceManager::LoadCachedGraph(
				unicode::ToTString(it->second->source_folder() + "/" + it->second->icon())));
		it->second->set_ui_board(ptr);
	}
}

void CardManager::ParseScriptFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    std::string source_str = std::string((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());

	auto start_metadata = source_str.find(START_METADATA);
	auto end_metadata = source_str.find(END_METADATA);

	if (start_metadata == std::string::npos ||
		end_metadata == std::string::npos || 
		start_metadata >= end_metadata) {
			Logger::Error(_T("Cannot read script metadata : %s"), unicode::ToTString(filename));
			return;
	}

    std::string name, author, caption, icon, native, type;
    std::vector<std::string> scripts;
    bool group = false;
    bool autorun = false;

    using boost::property_tree::ptree;
    ptree pt;
	std::stringstream sourse_stream(
		source_str.substr(start_metadata + sizeof(START_METADATA) - 1,
		end_metadata - (start_metadata + sizeof(START_METADATA) - 1)));

    try {
        read_json(sourse_stream, pt);
    } catch (const std::exception& e) {
        Logger::Error(_T("%s"), unicode::ToTString(e.what()));
        return;
    }

    name = pt.get<std::string>("name", "");
    author = pt.get<std::string>("author", "");
    caption = pt.get<std::string>("caption", "");
    icon = pt.get<std::string>("icon", "");
    group = pt.get<bool>("group", false);
    autorun = pt.get<bool>("autorun", true);
	native = pt.get<std::string>("native", "");
	type = pt.get<std::string>("type","");

    scripts.push_back("main.js");
    auto source_folder = (boost::filesystem::path(filename)).parent_path().string();

    auto card = std::make_shared<Card>(
        manager_accessor_,
        source_folder,
        name,
        icon,
		type,
        scripts,
		(!native.empty()));

	if (!native.empty()) {
		native_cards_[native] = card;
	}

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
