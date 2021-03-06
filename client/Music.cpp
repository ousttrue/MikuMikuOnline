//
// Music.cpp
//

#include <boost/filesystem.hpp>
#include "Music.hpp"
#include "../common/Logger.hpp"
#include "../common/unicode.hpp"

// TODO: どこの関数なのか分からないので将来的に修正の必要あり
extern int LoadFile(const TCHAR *FilePath, void **FileImageAddr, int *FileSize);

Music::Music() :
bgm_handle_(),
se_handle_(),
crossfade_now_(false),
fade_out_(false),
fade_count_(-1),
present_bgm_(-1),
prev_bgm_(-1),
requested_bgm_(-1)
{
}

void Music::Init()
{
    using namespace boost::filesystem;
    using namespace std;

	music_paths_.clear();

	path p("./music");

	try{
		if (exists(p) && is_directory(p)) {
			for (auto it_dir = directory_iterator(p); it_dir != directory_iterator(); ++it_dir) {
				if (!is_directory(*it_dir)) {
					path music_path = it_dir->path();
					auto extension = music_path.extension().string();
					if(	extension == ".wave" || extension == ".wav" ||
						extension == ".ogg"  || extension == ".mp3")
					{
						music_paths_.push_back(music_path);
					}
				}
			}
		}
	} catch (const filesystem_error& ex) {
		Logger::Error(_T("%s"), unicode::ToTString(ex.what()));
	}

	se_paths_.clear();

	p = "./se";

	try{
		if (exists(p) && is_directory(p)) {
			for (auto it_dir = directory_iterator(p); it_dir != directory_iterator(); ++it_dir) {
				if (!is_directory(*it_dir)) {
					path music_path = it_dir->path();
					auto extension = music_path.extension().string();
					if(	extension == ".wave" || extension == ".wav" ||
						extension == ".ogg"  || extension == ".mp3")
					{
						se_paths_.push_back(music_path);
					}
				}
			}
		}
	} catch (const filesystem_error& ex) {
		Logger::Error(_T("%s"), unicode::ToTString(ex.what()));
	}

}

void Music::Play(tstring name,bool crossfade)
{
	if(bgm_handle_.find(name) == bgm_handle_.end())
	{
		SetUseASyncLoadFlag(FALSE);
		auto NameFind = [&](boost::filesystem::path& src)->bool
		{
			return src.stem().string() == unicode::utf82sjis(unicode::ToString(name));
		};
		auto bgm_it_ = std::find_if(music_paths_.begin(),music_paths_.end(),NameFind);
		void *fileaddr;
		int filesize;
		LoadFile(unicode::ToTString(unicode::sjis2utf8(bgm_it_->string())).c_str(),&fileaddr,&filesize);
		auto softhandle_ = LoadSoftSoundFromMemImage(fileaddr,filesize);
		auto handle_ = LoadSoundMemByMemImage(fileaddr,filesize);
		if(handle_ == -1)return;
		bgm_handle_.insert(std::make_pair<tstring,int>(unicode::ToTString(unicode::sjis2utf8(bgm_it_->stem().string())),handle_));
		SetUseASyncLoadFlag(TRUE);
	}
	if(crossfade)
	{
		requested_bgm_ = bgm_handle_[name];
	}else{
		if(present_bgm_ != -1)StopSoundMem(present_bgm_);
		present_bgm_ = bgm_handle_[name];
		PlaySoundMem(present_bgm_,DX_PLAYTYPE_LOOP);
	}
	fade_count_ = 0;
	crossfade_now_ = crossfade;
}

void Music::PlayME(tstring name)
{
	if(present_bgm_ != -1)prev_bgm_ = present_bgm_;
	if(bgm_handle_.find(name) == bgm_handle_.end())
	{
		SetUseASyncLoadFlag(FALSE);
		auto NameFind = [&](boost::filesystem::path& src)->bool
		{
			return src.stem().string() == unicode::utf82sjis(unicode::ToString(name));
		};
		auto bgm_it_ = std::find_if(music_paths_.begin(),music_paths_.end(),NameFind);
		void *fileaddr;
		int filesize;
		LoadFile(unicode::ToTString(unicode::sjis2utf8(bgm_it_->string())).c_str(),&fileaddr,&filesize);
		auto softhandle_ = LoadSoftSoundFromMemImage(fileaddr,filesize);
		auto handle_ = LoadSoundMemByMemImage(fileaddr,filesize);
		if(handle_ == -1)return;
		bgm_handle_.insert(std::make_pair<tstring,int>(unicode::ToTString(unicode::sjis2utf8(bgm_it_->stem().string())),handle_));
		SetUseASyncLoadFlag(TRUE);
	}
	if(present_bgm_ != -1)StopSoundMem(present_bgm_);
	present_bgm_ = bgm_handle_[name];
	PlaySoundMem(present_bgm_,DX_PLAYTYPE_BACK);
	crossfade_now_ = false;
	fade_count_ = 0;
}

void Music::Stop(bool fadeout)
{
	if(!CheckSoundMem(present_bgm_))return;
	if(!fadeout)
	{
		StopSoundMem(present_bgm_);
	}
	if(crossfade_now_)
	{
		StopSoundMem(present_bgm_);
		present_bgm_ = requested_bgm_;
	}
	crossfade_now_ = false;
	fade_out_ = fadeout;
	fadeout_count_ = GetVolumeSoundMem(present_bgm_);
	fade_count_ = 0;
	requested_bgm_ = -1;
}

void Music::PlaySE(tstring name)
{
	if(se_handle_.find(name) == se_handle_.end())
	{
		SetUseASyncLoadFlag(FALSE);
		auto NameFind = [&](boost::filesystem::path& src)->bool
		{
			return src.stem().string() == unicode::utf82sjis(unicode::ToString(name));
		};
		auto se_it_ = std::find_if(se_paths_.begin(),se_paths_.end(),NameFind);
		auto handle_ = LoadSoundMemToBufNumSitei(unicode::ToTString(unicode::sjis2utf8(se_it_->string())).c_str(),64);
		if(handle_ == -1)return;
		se_handle_.insert(std::make_pair<tstring,int>(unicode::ToTString(unicode::sjis2utf8(se_it_->stem().string())),handle_));
	}
	PlaySoundMem(se_handle_[name],DX_PLAYTYPE_BACK);
}

bool Music::CheckLoadedBGM(tstring name)
{
	if(bgm_handle_.find(name) == bgm_handle_.end())return true;
	return CheckHandleASyncLoad(bgm_handle_[name]);
}

void Music::Update()
{
	if(prev_bgm_ != -1)
	{
		if(!CheckSoundMem(present_bgm_))
		{
			present_bgm_ = -1;
			requested_bgm_ = prev_bgm_;
			fade_count_ = 0;
			crossfade_now_ = true;
		}
	}
	if(CheckHandleASyncLoad(requested_bgm_) == false && requested_bgm_ != -1 && crossfade_now_)
	{
		if(fade_count_ == 0)PlaySoundMem(requested_bgm_,DX_PLAYTYPE_LOOP);
		ChangeVolumeSoundMem((int)((90.0-(double)fade_count_)/0.9),present_bgm_);
		ChangeVolumeSoundMem((int)((double)fade_count_/0.9),requested_bgm_);
		++fade_count_;
		if(fade_count_ >= 90)
		{
			ChangeVolumeSoundMem(100,requested_bgm_);
			StopSoundMem(present_bgm_);
			ChangeVolumeSoundMem(100,present_bgm_);
			present_bgm_ = requested_bgm_;
			requested_bgm_ = -1;
			fade_count_ = 0;
			crossfade_now_ = false;
		}
	}
	if(fade_out_ == true)
	{
		if(fadeout_count_ >= 90)
		{
			StopSoundMem(present_bgm_);
			ChangeVolumeSoundMem(100,present_bgm_);
			present_bgm_ = -1;
			fadeout_count_ = 0;
			fade_out_ = false;
			requested_bgm_ = -1;
		}
		ChangeVolumeSoundMem((int)((90.0-(double)fadeout_count_)/0.9),present_bgm_);
		++fadeout_count_;
	}
}

std::vector<boost::filesystem::path>& Music::GetMusicList()
{
	return music_paths_;
}